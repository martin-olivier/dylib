/**
 * @file dylib.hpp
 * @version 2.0.0
 * @brief C++ cross-platform wrapper around dynamic loading of shared libraries
 * @link https://github.com/martin-olivier/dylib
 * 
 * @author Martin Olivier <martin.olivier@live.fr>
 * @copyright (c) 2022 Martin Olivier
 *
 * This library is released under MIT license
 */

#pragma once

#include <string>
#include <stdexcept>
#include <utility>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <dlfcn.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
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

    static_assert(std::is_pointer<native_handle_type>::value, "Expecting HINSTANCE to be a pointer");

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

    dylib(const dylib&) = delete;
    dylib& operator=(const dylib&) = delete;

    dylib(dylib &&other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    dylib& operator=(dylib &&other) noexcept {
        if (this != &other)
            std::swap(m_handle, other.m_handle);
        return *this;
    }

    /**
     *  @brief Loads a dynamic library
     *
     *  @throws dylib::load_error if the library could not be opened (including
     *  the case of the library file not being found)
     *
     *  @param dir_path the directory path where is located the dynamic library you want to load
     *  @param name the name of the dynamic library to load
     *  @param decorations add os decorations to the library name
     */
    ///@{
    dylib(const char *dir_path, const char *name, bool decorations = true) {
        if (!dir_path || !name)
            throw std::invalid_argument("Null parameter");

        std::string final_name = name;
        std::string final_path = dir_path;

        if (decorations)
            final_name = filename_components::prefix + final_name + filename_components::suffix;

        if (final_path != "" && final_path.find_last_of('/') != final_path.size() - 1)
            final_path += '/';

        m_handle = _open((final_path + final_name).c_str());

        if (!m_handle)
            throw load_error("Could not load library \"" + final_path + final_name + "\":\n" + _get_error_description());
    }

    dylib(const std::string &dir_path, const std::string &name, bool decorations = true)
        : dylib(dir_path.c_str(), name.c_str(), decorations) {}

    dylib(const std::string &dir_path, const char *name, bool decorations = true)
        : dylib(dir_path.c_str(), name, decorations) {}

    dylib(const char *dir_path, const std::string &name, bool decorations = true)
        : dylib(dir_path, name.c_str(), decorations) {}

    explicit dylib(const std::string &name, bool decorations = true)
        : dylib("", name.c_str(), decorations) {}

    explicit dylib(const char *name, bool decorations = true)
        : dylib("", name, decorations) {}
    ///@}


    ~dylib() {
        if (m_handle)
            _close(m_handle);
    }

    /**
     *  Get a function from the dynamic library currently loaded in the object
     * 
     *  @throws dylib::symbol_error if the symbol could not be found
     *
     *  @param T the template argument must be the function prototype to get
     *  @param name the symbol name of a function to get from the dynamic library
     *
     *  @return a pointer to the requested function
     */
    template<typename T>
    T *get_function(const char *name) const {
        return reinterpret_cast<T *>(locate_symbol(name));
    }

    template<typename T>
    T *get_function(const std::string &name) const {
        return get_function<T>(name.c_str());
    }

    /**
     *  Get a variable from the dynamic library currently loaded in the object
     * 
     *  @throws dylib::symbol_error if the symbol could not be found
     *
     *  @param T the template argument must be the type of the variable to get
     *  @param name the symbol name of a variable to get from the dynamic library
     *
     *  @return a reference to the requested variable
     */
    template<typename T>
    T &get_variable(const char *name) const {
        return *reinterpret_cast<T *>(locate_symbol(name));
    }

    template<typename T>
    T &get_variable(const std::string &name) const {
        return get_variable<T>(name.c_str());
    }

    /**
     *  Check if a symbol exists in the currently loaded dynamic library. 
     *  This method will return false if no dynamic library is currently loaded 
     *  or if the symbol equals nullptr
     *
     *  @param symbol the symbol name to look for
     *
     *  @return true if the symbol exists in the dynamic library, false otherwise
     */
    bool has_symbol(const char *symbol) const noexcept {
        if (!symbol)
            return false;
        if (!m_handle)
            return false;
        return _get_symbol(m_handle, symbol) != nullptr;
    }

    bool has_symbol(const std::string &symbol) const noexcept {
        return has_symbol(symbol.c_str());
    }

    /**
     *  @return the dynamic library handle
     */
    native_handle_type native_handle() noexcept {
        return m_handle;
    }

protected:
    native_handle_type m_handle{nullptr};

    static native_handle_type _open(const char *path) noexcept {
#if defined(_WIN32) || defined(_WIN64)
        return LoadLibraryA(path);
#else
        return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
    }

    void *locate_symbol(const char *name) const {
        if (!name)
            throw std::invalid_argument("Null parameter");
        if (!m_handle)
            throw std::logic_error("The dynamic library handle is null");

        auto symbol = _get_symbol(m_handle, name);

        if (symbol == nullptr)
            throw symbol_error("Could not locate symbol \"" + std::string(name) + "\":\n" + _get_error_description());
        return symbol;
    }

    static DYLIB_WIN_OTHER(FARPROC, void *)
    _get_symbol(native_handle_type lib, const char *name) noexcept {
        return DYLIB_WIN_OTHER(GetProcAddress, dlsym)(lib, name);
    }

    static void _close(native_handle_type lib) noexcept {
        DYLIB_WIN_OTHER(FreeLibrary, dlclose)(lib);
    }

    static std::string _get_error_description() noexcept {
#if defined(_WIN32) || defined(_WIN64)
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
};

#undef DYLIB_WIN_MAC_OTHER
#undef DYLIB_WIN_OTHER
