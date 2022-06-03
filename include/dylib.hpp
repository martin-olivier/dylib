/**
 * @file dylib.hpp
 * @brief C++ cross-platform dynamic library loader
 * @link https://github.com/martin-olivier/dylib
 *
 * @author Martin Olivier
 * @author Eyal Rozenberg
 *
 * @license This library is released under MIT license
 *
 * @copyright (c) 2022 Martin Olivier
 * @copyright (c) 2022 Eyal Rozenberg
 */

#pragma once

#include <string>
#include <exception>
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
    using native_handle_type = DYLIB_WIN_OTHER(HINSTANCE, void*);

    static_assert(std::is_pointer<native_handle_type>::value, "Expecting HISTANCE to be some kind of pointer");

    dylib(const dylib&) = delete;
    dylib& operator=(const dylib&) = delete;

    dylib(dylib &&other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    dylib& operator=(dylib &&other) noexcept {
        std::swap(m_handle, other.m_handle);
        return *this;
    }

    /**
     *  @brief Loads a dynamic library
     *
     *  @throws std::runtime_error if the library could not be opened (including
     *  the case of the library file not being found).
     *
     *  @param dir_path the directory path where is located the dynamic library you want to load
     *  @param name the name of the dynamic library to load
     *  @param decorations add os decorations to the library name
     */
    ///@{
    dylib(const char *dir_path, const char *name, bool decorations = true) {
        std::string final_name = name;
        std::string final_path = dir_path;

        if (decorations)
            final_name = filename_components::prefix + final_name + filename_components::suffix;

        if (final_path != "" && final_path.find_last_of('/') != final_path.size() - 1)
            final_path += '/';

        m_handle = _open((final_path + final_name).c_str());

        if (!m_handle) {
            throw std::runtime_error(
                std::string("Failed loading dynamic library ") + final_path + final_name + ": " + _get_error_description()
            );
        }
    }

    dylib(const std::string &dir_path, const std::string &name, bool decorations = true)
        : dylib(dir_path.c_str(), name.c_str(), decorations) { }

    dylib(const std::string &dir_path, const char *name, bool decorations = true)
        : dylib(dir_path.c_str(), name, decorations) { }

    dylib(const char *dir_path, const std::string &name, bool decorations = true)
        : dylib(dir_path, name.c_str(), decorations) { }

    explicit dylib(const std::string &name, bool decorations = true)
        : dylib("", name.c_str(), decorations) { }

    explicit dylib(const char *name, bool decorations = true)
        : dylib("", name, decorations) { }
    ///@}


    ~dylib() {
        if (m_handle)
            _close(m_handle);
    }

    void *locate_symbol(const char *name) const {
        if (!name)
            throw std::invalid_argument("Null symbol name");
        if (!m_handle)
            throw std::logic_error("The dynamic library handle is null");

        auto symbol = _get_symbol(m_handle, name);

        if (symbol == nullptr) {
            auto msg = std::string("Failed locating symbol ") + name + " in a dynamic library: " + _get_error_description();
            throw std::runtime_error(msg);
        }
        return symbol;
    }

    /**
     *  Get a function from the dynamic library currently loaded in the object
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

    static DYLIB_WIN_MAC_OTHER(FARPROC, void*, void*)
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
            return "Unknown error (GetLastError() failed)";
        char description[512];
        auto lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        const DWORD length =
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_code, lang, description, buf_size, nullptr);
        return (length == 0) ? "Unknown error (FormatMessage() failed)" : description;
#else
        auto description = dlerror();
        return (description == nullptr) ? "Unknown error (dlerror() failed)" : description;
#endif
    }
};

#undef DYLIB_WIN_MAC_OTHER
#undef DYLIB_WIN_OTHER
