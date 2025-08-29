/**
 * @file dylib.cpp
 *
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2025 Martin Olivier
 *
 * This library is released under MIT license
 */

#if !defined(_WIN32)
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <cstring>
#include <fcntl.h>

#include "dylib.hpp"

using dylib::library;
using dylib::native_handle_type;
using dylib::native_symbol_type;
using dylib::symbol_info;

/*
 * internal_symbol_type and internal_symbol_info are needed
 * because the namespace 'dylib' conflicts with the 'dylib'
 * struct from <mach-o/loader.h> witch is needed on macOS.
 */

enum internal_symbol_type : std::uint8_t {
    C,
    CPP,
};

struct internal_symbol_info {
    std::string name;
    std::string demangled_name;
    internal_symbol_type type;
    bool loadable;
};

std::vector<internal_symbol_info> get_symbols(native_handle_type handle, int fd);
std::string demangle_symbol(const char *symbol);

static native_handle_type open_lib(const char *path) noexcept {
#if defined(_WIN32)
    return LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

static native_symbol_type locate_symbol(native_handle_type lib, const char *name) noexcept {
#if defined(_WIN32)
    return GetProcAddress(lib, name);
#else
    return dlsym(lib, name);
#endif
}

static void close_lib(native_handle_type lib) noexcept {
#if defined(_WIN32)
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
}

static std::string get_error_description() noexcept {
#if defined(_WIN32)
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

library::library(library &&other) noexcept {
    std::swap(m_handle, other.m_handle);
#if defined(__APPLE__)
    std::swap(m_fd, other.m_fd);
#endif
}

library &library::operator=(library &&other) noexcept {
    if (this != &other) {
        std::swap(m_handle, other.m_handle);
#if defined(__APPLE__)
        std::swap(m_fd, other.m_fd);
#endif
    }
    return *this;
}

library::library(const char *lib_path, dylib::decorations decorations) {
    std::string lib_name;
    std::string lib_dir;
    std::string lib;

    if (!lib_path)
        throw std::invalid_argument("The library path to lookup is null");

    lib = lib_path;

#if defined(_WIN32)
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
    if (!m_handle)
        throw load_error("Could not load library '" + lib + "':\n" + get_error_description());

#if defined(__APPLE__)
    m_fd = open(lib.c_str(), O_RDONLY);
    if (m_fd < 0)
        throw load_error("Could not open file '" + lib + "':\n" + strerror(errno));
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
#if defined(__APPLE__)
    if (m_fd > -1)
        close(m_fd);
#endif
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
        if (!sym.loadable)
            continue;

        std::string demangled = demangle_symbol(sym.name.c_str());

        if (demangled.find(symbol_name) == 0 &&
            (demangled.size() == symbol_name_len || demangled[symbol_name_len] == '('))
            matching_symbols.push_back(sym.name);
    }

    switch (matching_symbols.size()) {
    case 0:
        throw symbol_not_found(symbol_name, initial_error);
    case 1:
        return locate_symbol(m_handle, matching_symbols.front().c_str());
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

native_handle_type library::native_handle() noexcept {
    return m_handle;
}

std::vector<symbol_info> library::symbols() const {
    std::vector<internal_symbol_info> internal_symbols;
    std::vector<symbol_info> symbols;

    if (!m_handle)
        throw std::logic_error("Attempted to use a moved library object");

    try {
        internal_symbols = get_symbols(m_handle, DYLIB_WIN_MAC_OTHER(-1, m_fd, -1));

        symbols.reserve(internal_symbols.size());

        for (const auto &symbol : internal_symbols) {
            symbols.push_back(symbol_info{
                symbol.name,
                symbol.demangled_name,
                static_cast<symbol_type>(symbol.type),
                symbol.loadable,
            });
        }

        return symbols;
    } catch (const std::runtime_error &e) {
        throw symbol_collection_error(e.what());
    }
}
