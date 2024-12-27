/**
 * @file dylib.cpp
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2024 Martin Olivier
 *
 * This library is released under MIT license
 */

#if (defined(_WIN32) || defined(_WIN64))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <windows.h>
#endif
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <cstring>

#include "dylib.hpp"

#if (defined(_WIN32) || defined(_WIN64))
#define DYLIB_WIN_MAC_OTHER(win_def, mac_def, other_def) win_def
#define DYLIB_WIN_OTHER(win_def, other_def) win_def
#elif defined(__APPLE__)
#define DYLIB_WIN_MAC_OTHER(win_def, mac_def, other_def) mac_def
#define DYLIB_WIN_OTHER(win_def, other_def) other_def
#else
#define DYLIB_WIN_MAC_OTHER(win_def, mac_def, other_def) other_def
#define DYLIB_WIN_OTHER(win_def, other_def) other_def
#endif

std::string demangle_symbol(const char *symbol);

#if (defined(_WIN32) || defined(_WIN64))
std::vector<std::string> get_symbols(HMODULE handle, bool demangle, bool loadable);
#else
std::vector<std::string> get_symbols(void *handle, int fd, bool demangle, bool loadable);
#endif

static dylib::native_handle_type open_lib(const char *path) noexcept {
#if (defined(_WIN32) || defined(_WIN64))
    return LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

static dylib::native_symbol_type locate_symbol(dylib::native_handle_type lib, const char *name) noexcept {
    return DYLIB_WIN_OTHER(GetProcAddress, dlsym)(lib, name);
}

static void close_lib(dylib::native_handle_type lib) noexcept {
    DYLIB_WIN_OTHER(FreeLibrary, dlclose)(lib);
}

static std::string get_error_description() noexcept {
#if (defined(_WIN32) || defined(_WIN64))
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

dylib::dylib(dylib &&other) noexcept
    : m_handle(other.m_handle) {
    other.m_handle = nullptr;
}

dylib &dylib::operator=(dylib &&other) noexcept {
    if (this != &other)
        std::swap(m_handle, other.m_handle);
    return *this;
}

dylib::dylib(const char *dir_path, const char *lib_name, bool decorations) {
    if (!dir_path || !lib_name)
        throw std::invalid_argument("Null parameter");

    std::string final_name = lib_name;
    std::string final_path = dir_path;

    if (decorations)
        final_name = filename_components::prefix + final_name + filename_components::suffix;

    if (!final_path.empty() && final_path.find_last_of('/') != final_path.size() - 1)
        final_path += '/';

    m_handle = open_lib((final_path + final_name).c_str());

    if (!m_handle)
        throw load_error("Could not load library '" + final_path + final_name + "'\n" + get_error_description());

#if !(defined(_WIN32) || defined(_WIN64))
    m_fd = open((final_path + final_name).c_str(), O_RDONLY);

    if (m_fd < 0)
        throw load_error("Could not open library file");
#endif
}

dylib::~dylib() {
    if (m_handle)
        close_lib(m_handle);
#if !(defined(_WIN32) || defined(_WIN64))
    if (m_fd > -1)
        close(m_fd);
#endif
}

dylib::native_symbol_type dylib::get_symbol(const char *symbol_name) const {
    dylib::native_symbol_type symbol;

    if (!symbol_name)
        throw std::invalid_argument("Null parameter");
    if (!m_handle)
        throw std::logic_error("The dynamic library handle is null");

    symbol = locate_symbol(m_handle, symbol_name);
    if (symbol == nullptr) {
        std::string initial_error = get_error_description();
        std::vector<std::string> matching_symbols;

        for (auto &sym : symbols()) {
            std::string demangled = demangle_symbol(sym.c_str());

            if (demangled.find(symbol_name) == 0 &&
                (demangled[strlen(symbol_name)] == '(' ||
                 demangled[strlen(symbol_name)] == '\0'))
                matching_symbols.push_back(sym);
        }

        if (matching_symbols.size() == 0) {
            throw symbol_error("Could not get symbol '" +
                                std::string(symbol_name) +
                                "':\n" + initial_error);
        } else if (matching_symbols.size() == 1) {
            symbol = locate_symbol(m_handle, matching_symbols.front().c_str());
            if (symbol == nullptr)
                throw symbol_error("Could not get symbol '" +
                                    std::string(symbol_name) + "':\n"
                                    + get_error_description());
        } else {
            throw symbol_error(
                "Could not get symbol '" + std::string(symbol_name) + "': multiple matches");
        }
    }

    return symbol;
}

dylib::native_symbol_type dylib::get_symbol(const std::string &symbol_name) const {
    return get_symbol(symbol_name.c_str());
}

bool dylib::has_symbol(const char *symbol_name) const noexcept {
    if (!m_handle || !symbol_name)
        return false;
    return locate_symbol(m_handle, symbol_name) != nullptr;
}

bool dylib::has_symbol(const std::string &symbol_name) const noexcept {
    return has_symbol(symbol_name.c_str());
}

dylib::native_handle_type dylib::native_handle() noexcept {
    return m_handle;
}

std::vector<std::string> dylib::symbols(symbol_params params) const {
    try {
        return get_symbols(
            m_handle,
#if !(defined(_WIN32) || defined(_WIN64))
            m_fd,
#endif
            params.demangle,
            params.loadable
        );
    } catch (const std::string &e) {
        throw symbol_error(e);
    }
}
