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

#include <fcntl.h>
#include <cstring>

#include "dylib.hpp"

using dylib::library;
using dylib::native_handle_type;
using dylib::native_symbol_type;

std::vector<std::string> get_symbols(native_handle_type handle, int fd, bool demangle, bool loadable);
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

    length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_code,
                            lang, description, 512, nullptr);

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
        throw std::invalid_argument("Could not load library '" + lib + "': invalid path");

    lib_name = decorations.decorate(lib_name);
    lib = lib_dir + '/' + lib_name;

    m_handle = open_lib(lib.c_str());
    if (!m_handle)
        throw load_error("Could not load library '" + lib + "'\n" + get_error_description());

#if defined(__APPLE__)
    m_fd = open(lib.c_str(), O_RDONLY);
    if (m_fd < 0)
        throw load_error("Could not open file '" + lib + "': " + strerror(errno));
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
    native_symbol_type symbol;
    size_t symbol_name_len;

    if (!symbol_name)
        throw std::invalid_argument("The symbol name to lookup is null");
    if (symbol_name[0] == '\0')
        throw std::invalid_argument("The symbol name to lookup is an empty string");
    if (!m_handle)
        throw std::logic_error("Attempted to use a moved library object.");

    symbol_name_len = strlen(symbol_name);

    symbol = locate_symbol(m_handle, symbol_name);
    if (symbol == nullptr) {
        std::string initial_error = get_error_description();
        std::vector<std::string> matching_symbols;

        for (auto &sym : symbols()) {
            std::string demangled = demangle_symbol(sym.c_str());

            if (demangled.find(symbol_name) == 0 &&
                (demangled.size() == symbol_name_len || demangled[symbol_name_len] == '('))
                matching_symbols.push_back(sym);
        }

        if (matching_symbols.size() == 0) {
            throw symbol_error("Could not get symbol '" + std::string(symbol_name) + "':\n" + initial_error);
        } else if (matching_symbols.size() == 1) {
            symbol = locate_symbol(m_handle, matching_symbols.front().c_str());
            if (symbol == nullptr)
                throw symbol_error("Could not get symbol '" + std::string(symbol_name) + "':\n" + get_error_description());
        } else {
            std::string error = "Could not get symbol '" + std::string(symbol_name) + "', multiple matches:\n";
            for (auto &sym : matching_symbols)
                error += "- " + sym + '\n';

            throw symbol_error(error);
        }
    }

    return symbol;
}

native_symbol_type library::get_symbol(const std::string &symbol_name) const {
    return get_symbol(symbol_name.c_str());
}

bool library::has_symbol(const char *symbol_name) const noexcept {
    if (!m_handle || !symbol_name)
        return false;
    return locate_symbol(m_handle, symbol_name) != nullptr;
}

bool library::has_symbol(const std::string &symbol_name) const noexcept {
    return has_symbol(symbol_name.c_str());
}

native_handle_type library::native_handle() noexcept {
    return m_handle;
}

std::vector<std::string> library::symbols(symbol_params params) const {
    if (!m_handle)
        throw std::logic_error("Attempted to use a moved library object.");

    try {
        return get_symbols(
            m_handle,
            DYLIB_WIN_MAC_OTHER(-1, m_fd, -1),
            params.demangle,
            params.loadable
        );
    } catch (const std::string &e) {
        throw symbol_error(e);
    }
}
