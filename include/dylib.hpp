/**
 * @file dylib.hpp
 * @version 3.0.0 alpha
 * @brief C++ cross-platform wrapper around dynamic loading of shared libraries
 * @link https://github.com/martin-olivier/dylib
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2024 Martin Olivier
 *
 * This library is released under MIT license
 */

#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <utility>

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#define DYLIB_CPP17
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

/**
 *  The dylib class can hold a dynamic library instance and interact with it
 *  by getting its symbols like functions or global variables
 */
class dylib {
public:
    struct filename_components {
        static constexpr const char *prefix = DYLIB_WIN_OTHER("", "lib");
        static constexpr const char *suffix = DYLIB_WIN_MAC_OTHER(".dll", ".dylib", ".so");
    };
    using native_handle_type = DYLIB_WIN_OTHER(HINSTANCE, void *);
    using native_symbol_type = DYLIB_WIN_OTHER(FARPROC, void *);

    static_assert(std::is_pointer<native_handle_type>::value, "Expecting HINSTANCE to be a pointer");
    static_assert(std::is_pointer<native_symbol_type>::value, "Expecting FARPROC to be a pointer");

    static constexpr bool add_filename_decorations = true;
    static constexpr bool no_filename_decorations = false;

    struct symbol_params {
        symbol_params(): demangle(false), loadable(false) {}
        bool demangle;
        bool loadable;
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

    dylib(const dylib &) = delete;
    dylib &operator=(const dylib &) = delete;

    dylib(dylib &&other) noexcept;
    dylib &operator=(dylib &&other) noexcept;

    /**
     *  @brief Loads a dynamic library
     *
     *  @throws dylib::load_error if the library could not be opened (including
     *  the case of the library file not being found)
     *
     *  @param dir_path the directory path where is located the dynamic library you want to load
     *  @param lib_name the name of the dynamic library to load
     *  @param decorations add os decorations to the library name
     */
    dylib(const char *dir_path, const char *lib_name, bool decorations = add_filename_decorations);

    dylib(const std::string &dir_path, const std::string &lib_name, bool decorations = add_filename_decorations)
        : dylib(dir_path.c_str(), lib_name.c_str(), decorations) {}

    dylib(const std::string &dir_path, const char *lib_name, bool decorations = add_filename_decorations)
        : dylib(dir_path.c_str(), lib_name, decorations) {}

    dylib(const char *dir_path, const std::string &lib_name, bool decorations = add_filename_decorations)
        : dylib(dir_path, lib_name.c_str(), decorations) {}

    explicit dylib(const std::string &lib_name, bool decorations = add_filename_decorations)
        : dylib("", lib_name.c_str(), decorations) {}

    explicit dylib(const char *lib_name, bool decorations = add_filename_decorations)
        : dylib("", lib_name, decorations) {}

#ifdef DYLIB_CPP17
    explicit dylib(const std::filesystem::path &lib_path)
        : dylib("", lib_path.string().c_str(), no_filename_decorations) {}

    dylib(const std::filesystem::path &dir_path, const std::string &lib_name, bool decorations = add_filename_decorations)
        : dylib(dir_path.string().c_str(), lib_name.c_str(), decorations) {}

    dylib(const std::filesystem::path &dir_path, const char *lib_name, bool decorations = add_filename_decorations)
        : dylib(dir_path.string().c_str(), lib_name, decorations) {}
#endif

    ~dylib();

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

#undef DYLIB_WIN_MAC_OTHER
#undef DYLIB_WIN_OTHER
#undef DYLIB_CPP17
