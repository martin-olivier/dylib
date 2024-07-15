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
#include <algorithm>
#endif

#include <fcntl.h>
#include <string.h>

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

/* PRIVATE */

std::string get_demangled_name(const char *symbol);

#if (defined(_WIN32) || defined(_WIN64))
std::vector<std::string> get_symbols(HMODULE hModule, bool demangle);
#else
std::vector<std::string> get_symbols(int fd, bool demangle);
#endif

void replace_occurrences(std::string &input, const std::string &keyword, const std::string &replacement) {
    size_t pos = 0;

    if (keyword.empty())
        return;

    while ((pos = input.find(keyword, pos)) != std::string::npos) {
        input.replace(pos, keyword.length(), replacement);
        pos += replacement.length();
    }
}

void add_space_after_comma(std::string &input) {
    std::string result;

    for (char c : input) {
        if (c == ',') {
            result += ", ";
        } else {
            result += c;
        }
    }

    input = result;
}

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
    constexpr const size_t buf_size = 512;
    auto error_code = GetLastError();
    if (!error_code)
        return "Unknown error (GetLastError failed)";
    char description[512];
    auto lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    const DWORD length =
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_code, lang, description, buf_size, nullptr);
    return (length == 0) ? "Unknown error (FormatMessage failed)" : description;
#else
    auto description = dlerror();
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
        throw load_error("Could not load library");
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
    std::vector<std::string> matching_symbols;
    std::vector<std::string> all_symbols;

    if (!symbol_name)
        throw std::invalid_argument("Null parameter");
    if (!m_handle)
        throw std::logic_error("The dynamic library handle is null");

    auto symbol = locate_symbol(m_handle, symbol_name);

    if (symbol == nullptr) {
        all_symbols = symbols();

        for (auto &sym : all_symbols) {
            auto demangled = get_demangled_name(sym.c_str());

            if (demangled.empty())
                continue;

            if (demangled.find(symbol_name) == 0 &&
                (demangled[strlen(symbol_name)] == '(' ||
                 demangled[strlen(symbol_name)] == '\0'))
                matching_symbols.push_back(sym);
        }

        if (matching_symbols.size() == 0) {
            throw symbol_error("Could not get symbol '" + std::string(symbol_name) + "'\n" + get_error_description());
        } else if (matching_symbols.size() == 1) {
            symbol = locate_symbol(m_handle, matching_symbols.front().c_str());
            if (symbol == nullptr)
                throw symbol_error("Could not get symbol '" + std::string(symbol_name) + "'\n" + get_error_description());
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

bool dylib::has_symbol(const std::string &symbol) const noexcept {
    return has_symbol(symbol.c_str());
}

dylib::native_handle_type dylib::native_handle() noexcept {
    return m_handle;
}

std::vector<std::string> dylib::symbols(bool demangle) const {
    try {
#if !(defined(_WIN32) || defined(_WIN64))
        return get_symbols(m_fd, demangle);
#else
        return get_symbols(m_handle, demangle);
#endif
    } catch (const std::string &e) {
        throw symbol_error(e);
    }
}
