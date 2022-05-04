/**
 * \file dylib.hpp
 * \brief C++ cross-platform dynamic library loader
 * \link https://github.com/martin-olivier/dylib
 * \author Martin Olivier
 * \version 1.8.3
 * 
 * MIT License
 * Copyright (c) 2022 Martin Olivier
 */

#pragma once

#include <string>
#include <functional>
#include <exception>
#include <utility>
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define DYLIB_API extern "C" __declspec(dllexport)
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#define DYLIB_API extern "C"
#include <dlfcn.h>
#endif

/**
 *  The dylib class can hold a dynamic library instance and interact with it 
 *  by getting its symbols like functions or global variables
 */
class dylib
{
public:
#if defined(_WIN32) || defined(_WIN64)
    static constexpr const char *extension = ".dll";
    using native_handle_type = HINSTANCE;
#elif defined(__APPLE__)
    static constexpr const char *extension = ".dylib";
    using native_handle_type = void *;
#else
    static constexpr const char *extension = ".so";
    using native_handle_type = void *;
#endif

    /**
     *  This exception is raised when the dylib class encountered an error
     *
     *  @return the error message by calling what() member function
     */
    class exception : public std::exception
    {
    protected:
        const std::string m_error;
    public:
        explicit exception(std::string &&message) : m_error(std::move(message)) {}
        const char *what() const noexcept override {return m_error.c_str();}
    };

    /**
     *  This exception is raised when the library failed to load 
     *  or encountered symbol resolution issues
     *
     *  @param message the error message
     */
    class handle_error : public exception
    {
    public:
        explicit handle_error(std::string &&message) : exception(std::move(message)) {}
    };

    /**
     *  This exception is raised when the library failed to load a symbol. 
     *  This usually happens when you forgot to put <DYLIB_API> before a library function or variable
     *
     *  @param message the error message
     */
    class symbol_error : public exception
    {
    public:
        explicit symbol_error(std::string &&message) : exception(std::move(message)) {}
    };

    dylib(const dylib&) = delete;
    dylib& operator=(const dylib&) = delete;

    dylib(dylib &&other) noexcept : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }

    dylib& operator=(dylib &&other) noexcept
    {
        if (this != &other) {
            close();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    dylib() noexcept = default;

    /**
     *  Creates a dynamic library instance
     *
     *  @param path path to the dynamic library to load
     *  @param ext use dylib::extension to specify the os extension (optional parameter)
     */
    explicit dylib(const char *path)
    {
        open(path);
    }

    explicit dylib(const std::string &path)
    {
        open(path.c_str());
    }

    dylib(const std::string &path, const char *ext)
    {
        open(path, ext);
    }

    ~dylib()
    {
        close();
    }

    /**
     *  Load a dynamic library into the object. 
     *  If a dynamic library was already opened, it will be unload and replaced
     *
     *  @param path the path of the dynamic library to load
     *  @param ext use dylib::extension to detect the current os extension (optional parameter)
     */
    void open(const char *path)
    {
        close();
        if (!path)
            throw handle_error(get_handle_error("(nullptr)"));
        m_handle = open_lib(path);
        if (!m_handle)
            throw handle_error(get_handle_error(path));
    }

    void open(const std::string &path)
    {
        open(path.c_str());
    }

    void open(const std::string &path, const char *ext)
    {
        if (!ext)
            throw handle_error("dylib: failed to load \"" + path + "\", bad extension: (nullptr)");
        open(path + ext);
    }

    /**
     *  Get a function from the dynamic library currently loaded in the object
     *
     *  @param T the template argument must be the function prototype. 
     *  it must be the same pattern as the template of std::function
     *  @param name the symbol name of the function to get from the dynamic library
     *
     *  @return std::function<T> that contains the function
     */
    template<typename T>
    std::function<T> get_function(const char *name) const
    {
        if (!name)
            throw symbol_error(get_symbol_error("(nullptr)"));
        if (!m_handle)
            throw handle_error(get_missing_handle_error(name));
        auto sym = get_symbol(name);
        if (!sym)
            throw symbol_error(get_symbol_error(name));
        return reinterpret_cast<T *>(sym);
    }

    template<typename T>
    std::function<T> get_function(const std::string &name) const
    {
        return get_function<T>(name.c_str());
    }

    /**
     *  Get a global variable from the dynamic library currently loaded in the object
     *
     *  @param T type of the global variable
     *  @param name the name of the global variable to get from the dynamic library
     *
     *  @return global variable of type <T>
     */
    template<typename T>
    T &get_variable(const char *name) const
    {
        if (!name)
            throw symbol_error(get_symbol_error("(nullptr)"));
        if (!m_handle)
            throw handle_error(get_missing_handle_error(name));
        auto sym = get_symbol(name);
        if (!sym)
            throw symbol_error(get_symbol_error(name));
        return *reinterpret_cast<T *>(sym);
    }

    template<typename T>
    T &get_variable(const std::string &name) const
    {
        return get_variable<T>(name.c_str());
    }

    /**
     *  Check if a symbol exists in the currently loaded dynamic library. 
     *  This method will return false if no dynamic library is currently loaded or if the symbol equals nullptr
     *
     *  @param symbol the symbol name to look for
     *
     *  @return true if the symbol exists in the dynamic library, false otherwise
     */
    bool has_symbol(const char *symbol) const noexcept
    {
        if (!symbol)
            return false;
        if (!m_handle)
            return false;
        return get_symbol(symbol) != nullptr;
    }

    bool has_symbol(const std::string &symbol) const noexcept
    {
        return has_symbol(symbol.c_str());
    }

    /**
     *  @return the dynamic library handle
     */
    native_handle_type native_handle() noexcept
    {
        return m_handle;
    }

    /**
     *  @return true if a dynamic library is currently loaded in the object, false otherwise
     */
    operator bool() const noexcept
    {
        return m_handle != nullptr;
    }

    /**
     *  Close the dynamic library currently loaded in the object. 
     *  This function will be automatically called by the class destructor
     */
    void close() noexcept
    {
        if (m_handle) {
            close_lib();
            m_handle = nullptr;
        }
    }

private:
    native_handle_type m_handle{nullptr};
#if defined(_WIN32) || defined(_WIN64)
    static native_handle_type open_lib(const char *path) noexcept
    {
        return LoadLibraryA(path);
    }
    FARPROC get_symbol(const char *name) const noexcept
    {
        return GetProcAddress(m_handle, name);
    }
    void close_lib() noexcept
    {
        FreeLibrary(m_handle);
    }
    static char *get_error_message() noexcept
    {
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
    static native_handle_type open_lib(const char *path) noexcept
    {
        return dlopen(path, RTLD_NOW | RTLD_LOCAL);
    }
    void *get_symbol(const char *name) const noexcept
    {
        return dlsym(m_handle, name);
    }
    void close_lib() noexcept
    {
        dlclose(m_handle);
    }
    static char *get_error_message() noexcept
    {
        return dlerror();
    }
#endif
    static std::string get_handle_error(const std::string &name)
    {
        std::string msg = "dylib: error while loading dynamic library \"" + name + "\"";
        auto err = get_error_message();
        if (!err)
            return msg;
        return msg + '\n' + err;
    }
    static std::string get_symbol_error(const std::string &name)
    {
        std::string msg = "dylib: error while loading symbol \"" + name + "\"";
        auto err = get_error_message();
        if (!err)
            return msg;
        return msg + '\n' + err;
    }
    static std::string get_missing_handle_error(const std::string &symbol_name)
    {
        return "dylib: could not get symbol \"" + symbol_name + "\", no dynamic library currently loaded";
    }
};