/**
 * @file dylib.cpp
 *
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2025 Martin Olivier
 *
 * This library is released under MIT license
 */

#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <cstring>
#include <fcntl.h>
#include <system_error>

#include "dylib.hpp"
#include "internal.hpp"

using dylib::library;
using dylib::native_handle_type;
using dylib::native_symbol_type;
using dylib::symbol_info;

static native_handle_type open_lib(const char *path) noexcept {
#ifdef _WIN32
    return LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

static native_symbol_type locate_symbol(native_handle_type lib, const char *name) noexcept {
#ifdef _WIN32
    return GetProcAddress(lib, name);
#else
    return dlsym(lib, name);
#endif
}

static void close_lib(native_handle_type lib) noexcept {
#ifdef _WIN32
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
}

static std::string get_error_description() noexcept {
#ifdef _WIN32
    WORD lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    char description[512];
    DWORD error_code;
    DWORD length;

    error_code = GetLastError();
    if (!error_code)
        return "Unknown error (GetLastError failed)";

    length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_code, lang, description, 512,
                            nullptr);

    return (length == 0) ? "Unknown error (FormatMessage failed)" : description;
#else
    char *description = dlerror();

    return (description == nullptr) ? "Unknown error (dlerror failed)" : description;
#endif
}

#ifndef _WIN32
class scoped_fd {
public:
    /*
     * O_CLOEXEC keeps the descriptor from leaking into a process forked while the
     * library image is being read.
     */
    explicit scoped_fd(const std::string &path) : m_fd(open(path.c_str(), O_RDONLY | O_CLOEXEC)) {
        if (m_fd < 0) {
            int error = errno;

            /*
             * strerror is not thread safe; the generic category formats the same
             * message through a thread safe path.
             */
            throw std::runtime_error("Could not open file '" + path +
                                     "': " + std::generic_category().message(error));
        }
    }

    scoped_fd(const scoped_fd &) = delete;
    scoped_fd &operator=(const scoped_fd &) = delete;

    ~scoped_fd() {
        close(m_fd);
    }

    int get() const noexcept {
        return m_fd;
    }

private:
    int m_fd;
};
#endif

#ifdef _WIN32
library::library(library &&other) noexcept : m_handle(other.m_handle) {
    other.m_handle = nullptr;
}
#else
library::library(library &&other) noexcept
    : m_handle(other.m_handle), m_path(std::move(other.m_path)) {
    other.m_handle = nullptr;
    other.m_path.clear();
}
#endif

library &library::operator=(library &&other) noexcept {
    if (this != &other) {
        /*
         * Release the library currently held rather than handing it over to the
         * moved-from object: the source has to end up empty, which is the state
         * the rest of the class reports as "moved", and the previous library has
         * to be unloaded now rather than whenever the source happens to die.
         */
        if (m_handle)
            close_lib(m_handle);

        m_handle = other.m_handle;
        other.m_handle = nullptr;
#ifndef _WIN32
        m_path = std::move(other.m_path);
        other.m_path.clear();
#endif
    }
    return *this;
}

#ifdef _WIN32
library::library(const char *lib_path, dylib::decorations decorations) {
#else
library::library(const char *lib_path, dylib::decorations decorations) : m_path() {
#endif
    std::string lib_name;
    std::string lib_dir;
    std::string lib;

    if (!lib_path)
        throw std::invalid_argument("The library path to lookup is null");

    lib = lib_path;

#ifdef _WIN32
    while (lib.find('\\') != std::string::npos)
        lib.replace(lib.find('\\'), 1, "/");
#endif

    if (lib.empty())
        throw std::invalid_argument("The library path to lookup is an empty string");
    if (lib.find('/') == std::string::npos)
        throw std::invalid_argument("Could not load library '" + lib + "': invalid path");

    lib_name = lib.substr(lib.find_last_of('/') + 1);
    lib_dir = lib.substr(0, lib.find_last_of('/'));

    if (lib_name.empty())
        throw std::invalid_argument("Could not load library '" + lib +
                                    "': a directory was provided");

    lib = lib_dir + '/' + decorations.prefix + lib_name + decorations.suffix;

    m_handle = open_lib(lib.c_str());
    if (!m_handle) {
        /*
         * On Windows the description comes from GetLastError, which any
         * intervening allocation is free to overwrite, so it is captured before
         * the message is built.
         */
        std::string error = get_error_description();

        throw load_error("Could not load library '" + lib + "':\n" + error);
    }

#ifndef _WIN32
    m_path = lib;
#endif
}

library::library(const std::string &lib_path, decorations decorations)
    : library(lib_path.c_str(), decorations) {}

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
library::library(const std::filesystem::path &lib_path, decorations decorations)
    : library(lib_path.string(), decorations) {}
#endif

library::~library() {
    if (m_handle)
        close_lib(m_handle);
}

native_symbol_type library::get_symbol(const char *symbol_name) const {
    std::vector<std::string> matching_symbols;
    std::string initial_error;
    native_symbol_type symbol;
    size_t symbol_name_len;

    if (!symbol_name)
        throw std::invalid_argument("The symbol name to lookup is null");
    if (symbol_name[0] == '\0')
        throw std::invalid_argument("The symbol name to lookup is an empty string");
    if (!m_handle)
        throw std::logic_error("Attempted to use a moved library object");

    symbol_name_len = strlen(symbol_name);

    symbol = locate_symbol(m_handle, symbol_name);
    if (symbol)
        return symbol;

    initial_error = get_error_description();

    for (const auto &sym : symbols()) {
        /*
         * Only C++ symbols are considered here: a C symbol is not mangled, so it would
         * already have been resolved by the locate_symbol call above.
         */
        if (!sym.loadable || sym.type != symbol_type::CPP)
            continue;

        const std::string &demangled = sym.demangled_name;

        if (demangled.size() >= symbol_name_len &&
            demangled.compare(0, symbol_name_len, symbol_name) == 0 &&
            (demangled.size() == symbol_name_len || demangled[symbol_name_len] == '('))
            matching_symbols.push_back(sym.name);
    }

    switch (matching_symbols.size()) {
    case 0:
        throw symbol_not_found(symbol_name, initial_error);
    case 1: {
        /*
         * The match was reported as loadable while the symbol list was collected,
         * so this normally succeeds. Report the failure rather than handing back a
         * null pointer that get_variable would dereference.
         */
        symbol = locate_symbol(m_handle, matching_symbols.front().c_str());
        if (!symbol)
            throw symbol_not_found(symbol_name, get_error_description());

        return symbol;
    }
    default:
        std::string matching_symbols_display;

        for (auto &sym : matching_symbols)
            matching_symbols_display += "- " + sym + '\n';

        throw symbol_multiple_matches(symbol_name, matching_symbols_display);
    }
}

native_symbol_type library::get_symbol(const std::string &symbol_name) const {
    return get_symbol(symbol_name.c_str());
}

native_handle_type library::native_handle() const noexcept {
    return m_handle;
}

std::vector<symbol_info> library::symbols() const {
    std::vector<dylib_detail::symbol_info> internal_symbols;
    std::vector<symbol_info> symbols;

    if (!m_handle)
        throw std::logic_error("Attempted to use a moved library object");

    try {
#ifdef __APPLE__
        scoped_fd fd(m_path);

        internal_symbols = dylib_detail::get_symbols(m_handle, fd.get());
#else
        internal_symbols = dylib_detail::get_symbols(m_handle, -1);
#endif

        symbols.reserve(internal_symbols.size());

        for (auto &symbol : internal_symbols) {
            symbols.push_back(symbol_info{
                std::move(symbol.name),
                std::move(symbol.demangled_name),
                static_cast<symbol_type>(symbol.type),
                symbol.loadable,
            });
        }

        return symbols;
    } catch (const std::runtime_error &e) {
        throw symbol_collection_error(e.what());
    }
}

std::vector<std::string> library::sections() const {
    if (!m_handle)
        throw std::logic_error("Attempted to use a moved library object");

    try {
#ifdef _WIN32
        return dylib_detail::get_sections(m_handle, -1);
#else
        scoped_fd fd(m_path);

        return dylib_detail::get_sections(m_handle, fd.get());
#endif
    } catch (const std::runtime_error &e) {
        throw section_collection_error(e.what());
    }
}
