/**
 * @file dylib.hpp
 * @version 3.0.0 alpha
 * @brief C++ cross-platform wrapper around dynamic loading of shared libraries
 * @link https://github.com/martin-olivier/dylib
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2025 Martin Olivier
 *
 * This library is released under MIT license
 */

#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <utility>

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#include <filesystem>
#endif

#if (defined(_WIN32) || defined(_WIN64))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define DYLIB_UNDEFINE_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#define DYLIB_UNDEFINE_NOMINMAX
#endif
#include <windows.h>
#ifdef DYLIB_UNDEFINE_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef DYLIB_UNDEFINE_LEAN_AND_MEAN
#endif
#ifdef DYLIB_UNDEFINE_NOMINMAX
#undef NOMINMAX
#undef DYLIB_UNDEFINE_NOMINMAX
#endif
#endif

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

namespace dylib {

using native_handle_type = DYLIB_WIN_OTHER(HINSTANCE, void *);
using native_symbol_type = DYLIB_WIN_OTHER(FARPROC, void *);

static_assert(std::is_pointer<native_handle_type>::value, "Expecting HINSTANCE to be a pointer");
static_assert(std::is_pointer<native_symbol_type>::value, "Expecting FARPROC to be a pointer");

struct decorations {
    const char *prefix{""};
    const char *suffix{""};

    static decorations none() noexcept {
        return {};
    }

    static decorations os_default() noexcept {
        return {
            DYLIB_WIN_OTHER("", "lib"),
            DYLIB_WIN_MAC_OTHER(".dll", ".dylib", ".so"),
        };
    }

    std::string decorate(const std::string &lib_name) const {
        return prefix + lib_name + suffix;
    }
};

struct symbol_params {
    bool demangle{false};
    bool loadable{false};
};

/**
 *  This exception is raised when the library failed to load a dynamic library or a symbol
 *
 *  @param message the error message
 */
class exception : public std::runtime_error {
public:
    explicit exception(const std::string &message) : std::runtime_error(message) {}
};

/**
 *  This exception is raised when the library failed to load or encountered symbol resolution issues
 *
 *  @param message the error message
 */
class load_error : public exception {
public:
    explicit load_error(const std::string &message) : exception(message) {}
};

/**
 *  This exception is raised when the library failed to load a symbol
 *
 *  @param message the error message
 */
class symbol_error : public exception {
public:
    explicit symbol_error(const std::string &message) : exception(message) {}
};

/**
 *  The dylib::library class can hold a dynamic library instance and interact with it
 *  by getting its symbols like functions or global variables
 */
class library {
public:
    library(const library &) = delete;
    library &operator=(const library &) = delete;

    library(library &&other) noexcept;
    library &operator=(library &&other) noexcept;

    /**
     *  @brief Loads a dynamic library
     *
     *  @throws dylib::load_error if the library could not be opened (including
     *  the case of the library file not being found)
     *
     *  @param lib_path the path to the dynamic library to load
     *  @param decorations os decorations to append to the library name
     */
    explicit library(const char *lib_path, decorations decorations = decorations::none());
    explicit library(const std::string &lib_path, decorations decorations = decorations::none());
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
    explicit library(const std::filesystem::path &lib_path, decorations decorations = decorations::none());
#endif

    ~library();

    /**
     *  Get a symbol from the dynamic library currently loaded in the object
     *
     *  @throws dylib::symbol_error if the symbol could not be found
     *
     *  @param symbol_name the symbol name to get from the dynamic library
     *
     *  @return a pointer to the requested symbol
     */
    native_symbol_type get_symbol(const char *symbol_name) const;
    native_symbol_type get_symbol(const std::string &symbol_name) const;

    /**
     *  Check if a symbol exists in the currently loaded dynamic library.
     *  This method will return false if no dynamic library is currently loaded
     *  or if the symbol name is nullptr
     *
     *  @param symbol_name the symbol name to look for
     *
     *  @return true if the symbol exists in the dynamic library, false otherwise
     */
    bool has_symbol(const char *symbol_name) const noexcept;
    bool has_symbol(const std::string &symbol_name) const noexcept;

    /**
     *  Get a function from the dynamic library currently loaded in the object
     *
     *  @throws dylib::symbol_error if the symbol could not be found
     *
     *  @param T the template argument must be the function prototype to get
     *  @param symbol_name the symbol name of a function to get from the dynamic library
     *
     *  @return a pointer to the requested function
     */
    template <typename T>
    T *get_function(const char *symbol_name) const {
#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
#if defined __clang__
#if __has_warning("-Wcast-function-type-mismatch")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif
#endif
        return reinterpret_cast<T *>(get_symbol(symbol_name));
#if defined __clang__
#if __has_warning("-Wcast-function-type-mismatch")
#pragma clang diagnostic pop
#endif
#endif
#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    }

    template <typename T>
    T *get_function(const std::string &symbol_name) const {
        return get_function<T>(symbol_name.c_str());
    }

    /**
     *  Get a variable from the dynamic library currently loaded in the object
     *
     *  @throws dylib::symbol_error if the symbol could not be found
     *
     *  @param T the template argument must be the type of the variable to get
     *  @param symbol_name the symbol name of a variable to get from the dynamic library
     *
     *  @return a reference to the requested variable
     */
    template <typename T>
    T &get_variable(const char *symbol_name) const {
        return *reinterpret_cast<T *>(get_symbol(symbol_name));
    }

    template <typename T>
    T &get_variable(const std::string &symbol_name) const {
        return get_variable<T>(symbol_name.c_str());
    }

    /**
     *  Get the list of symbols from the dynamic library currently loaded in the object
     *
     *  @throws dylib::symbol_error if an error occurred during symbols collection
     *
     *  @param symbol_params::demangle if true, returns demangled symbols
     *  @param symbol_params::loadable if true, returns only loadable symbols
     *
     *  @return the list of symbols in the dynamic library
     */
    std::vector<std::string> symbols(symbol_params params = symbol_params()) const;

    /**
     *  @return the dynamic library handle
     */
    native_handle_type native_handle() noexcept;

protected:
    native_handle_type m_handle{nullptr};
#if !(defined(_WIN32) || defined(_WIN64))
    int m_fd{-1};
#endif
};

} // namespace dylib
