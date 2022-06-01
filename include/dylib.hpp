/**
 * @file dylib.hpp
 * @brief C++ cross-platform dynamic library loader
 * @link https://github.com/martin-olivier/dylib
 * @author Martin Olivier
 * @author Eyal Rozenberg
 *
 * @license This library is released under the MIT license (see separate fil)
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

/**
 *  The dylib class can hold a dynamic library instance and interact with it 
 *  by getting its symbols like functions or global variables
 */
class dylib {
public:
#if defined(_WIN32) || defined(_WIN64)
    struct filename_components {
        static constexpr const char *prefix = "";
        static constexpr const char *suffix = ".dll";
    };
    using native_handle_type = HINSTANCE;
#elif defined(__APPLE__)
    struct filename_components {
        static constexpr const char *prefix = "lib";
        static constexpr const char *suffix = ".dylib";
    };
    using native_handle_type = void *;
#else
    struct filename_components {
        static constexpr const char *prefix = "lib";
        static constexpr const char *suffix = ".so";
    };
    using native_handle_type = void *;
#endif


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
     *  Creates a dynamic library instance
     *
     *  @param dir_path the directory path where is located the dynamic library you want to load
     *  @param name the name of the dynamic library to load
     *  @param decorations add os decorations to the library name
     */
    dylib(const std::string &dir_path, const std::string &name, bool decorations = true) {
        open(dir_path.c_str(), name.c_str(), decorations);
    }
    dylib(const char *dir_path, const char *name, bool decorations = true) {
        open(dir_path, name, decorations);
    }
    dylib(const std::string &dir_path, const char *name, bool decorations = true) {
        open(dir_path.c_str(), name, decorations);
    }
    dylib(const char *dir_path, const std::string &name, bool decorations = true) {
        open(dir_path, name.c_str(), decorations);
    }

    /**
     *  Creates a dynamic library instance
     *
     *  @param name the name of the dynamic library to load from the system library search path
     *  @param decorations add os decorations to the library name
     */
    explicit dylib(const std::string &name, bool decorations = true) {
        open("", name.c_str(), decorations);
    }
    explicit dylib(const char *name, bool decorations = true) {
        open("", name, decorations);
    }

    ~dylib() {
        if (m_handle)
            _close(m_handle);
    }

    void* locate_symbol(const char* name) const noexcept(false) {
        if (!name) {
            throw std::invalid_argument("null symbol name");
        }
        if (!m_handle) {
            throw std::logic_error("The dynamic (shared) library handle is null. This should not be possible");
        }

        auto symbol = _get_symbol(m_handle, name);
        if (symbol == nullptr) {
            auto msg = std::string("Failed locating symbol ") + name + " in a dynamic (shared) library: " + _get_error();
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
        auto symbol_addr = locate_symbol(name);
        return reinterpret_cast<T *>(symbol_addr);
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
        auto symbol_addr = locate_symbol(name);
        return *reinterpret_cast<T *>(symbol_addr);
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
#if defined(_WIN32) || defined(_WIN64)
    static native_handle_type _open(const char *path) noexcept {
        return LoadLibraryA(path);
    }
    static FARPROC _get_symbol(native_handle_type lib, const char *name) noexcept {
        return GetProcAddress(lib, name);
    }
    static void _close(native_handle_type lib) noexcept {
        FreeLibrary(lib);
    }
    static char *_get_error() noexcept {
        constexpr size_t buf_size = 512;
        auto error_code = GetLastError();
        if (!error_code)
            return nullptr;
        static char msg[buf_size];
        auto lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        const DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_code, lang, msg, buf_size, nullptr);
        if (len > 0)
            return msg;
        return nullptr;
    }
#else
    static native_handle_type _open(const char *path) noexcept {
        return dlopen(path, RTLD_NOW | RTLD_LOCAL);
    }
    static void *_get_symbol(native_handle_type lib, const char *name) noexcept {
        return dlsym(lib, name);
    }
    static void _close(native_handle_type lib) noexcept {
        dlclose(lib);
    }
    static char *_get_error() noexcept {
        return dlerror();
    }
#endif
    void open(const char *path, const char *name, bool decorations) {
        std::string final_name = name;
        std::string final_path = path;

        if (decorations)
            final_name = filename_components::prefix + final_name + filename_components::suffix;

        if (final_path != "" && final_path.find_last_of('/') != final_path.size() - 1)
            final_path += '/';

        m_handle = _open((final_path + final_name).c_str());

        if (!m_handle) {
            throw std::runtime_error(
                std::string("Failed loading dynamic (shared) library ") + final_path + final_name + ": " + _get_error()
            );
        }
    }
};
