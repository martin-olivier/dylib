/**
 * \file DyLib.hpp
 * \brief Cross-platform Dynamic Library Loader
 * \author Martin Olivier
 * \version 1.6.0
 * 
 * MIT License
 * Copyright (c) 2021 Martin Olivier
 */

#pragma once

#include <string>
#include <functional>
#include <exception>
#include <utility>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

class DyLib
{
private:
#if defined(_WIN32) || defined(_WIN64)
    HINSTANCE m_handle{nullptr};
    static HINSTANCE openLib(const char *path) noexcept
    {
        return LoadLibrary(TEXT(path));
    }
    static std::string getHandleError(const std::string &name)
    {
        return "error while loading dynamic library \"" + name + "\": error code " + std::to_string(GetLastError());
    }
    static std::string getSymbolError(const std::string &name)
    {
        return "error while loading symbol \"" + name + "\": error code " + std::to_string(GetLastError());
    }
    FARPROC getSymbol(const char *name) const noexcept
    {
        return GetProcAddress(m_handle, name);
    }
    void closeLib() noexcept
    {
        FreeLibrary(m_handle);
    }
#else
    void *m_handle{nullptr};
    static void *openLib(const char *path) noexcept
    {
        return dlopen(path, RTLD_NOW | RTLD_LOCAL);
    }
    static std::string getHandleError(const std::string &name)
    {
        auto err = dlerror();
        if (!err)
            return "error while loading dynamic library \"" + name + "\"";
        return err;
    }
    static std::string getSymbolError(const std::string &name)
    {
        auto err = dlerror();
        if (!err)
            return "error while loading symbol \"" + name + "\"";
        return err;
    }
    void *getSymbol(const char *name) const noexcept
    {
        return dlsym(m_handle, name);
    }
    void closeLib() noexcept
    {
        dlclose(m_handle);
    }
#endif
public:

#if defined(_WIN32) || defined(_WIN64)
    static constexpr auto extension = ".dll";
#elif defined(__APPLE__)
    static constexpr auto extension = ".dylib";
#else
    static constexpr auto extension = ".so";
#endif

    /**
     *  This exception is thrown when the DyLib class encountered an error.
     *
     *  @return error message by calling what() member function
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
     *  This exception is thrown when the library failed to load 
     *  or the library encountered symbol resolution issues
     *
     *  @param message error message
     */
    class handle_error : public exception
    {
    public:
        explicit handle_error(std::string &&message) : exception(std::move(message)) {}
    };

    /**
     *  This exception is thrown when the library failed to load a symbol. 
     *  This usually happens when you forgot to mark a library function or variable as extern "C"
     *
     *  @param message error message
     */
    class symbol_error : public exception
    {
    public:
        explicit symbol_error(std::string &&message) : exception(std::move(message)) {}
    };

    /**
     *  Creates a dynamic library object
     */
    constexpr DyLib() noexcept = default;

    DyLib(const DyLib&) = delete;
    DyLib& operator=(const DyLib&) = delete;

    /**
     *  Move constructor : move a dynamic library instance to build this object
     *
     *  @param other ref on rvalue of the other DyLib (use std::move)
     */
    DyLib(DyLib &&other) noexcept
    {
        m_handle = other.m_handle;
        other.m_handle = nullptr;
    }

    DyLib& operator=(DyLib &&other) noexcept
    {
        if (this != &other) {
            close();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    /**
     *  Creates a dynamic library instance
     *
     *  @param path path to the dynamic library to load
     *  @param ext use DyLib::extension to specify the os extension (optional parameter)
     */
    explicit DyLib(const char *path)
    {
        open(path);
    }

    explicit DyLib(const std::string &path)
    {
        open(path.c_str());
    }

    DyLib(std::string path, const char *ext)
    {
        open(std::move(path), ext);
    }

    ~DyLib()
    {
        close();
    }

    /**
     *  Load a dynamic library into the object.
     *  If a dynamic library was already opened, it will be unload and replaced
     *
     *  @param path path to the dynamic library to load
     *  @param ext use DyLib::extension to specify the os extension (optional parameter)
     */
    void open(const char *path)
    {
        close();
        if (!path)
            throw handle_error(getHandleError("(nullptr)"));
        m_handle = openLib(path);
        if (!m_handle)
            throw handle_error(getHandleError(path));
    }

    void open(const std::string &path)
    {
        open(path.c_str());
    }

    void open(std::string path, const char *ext)
    {
        close();
        if (!ext)
            throw handle_error(getHandleError("bad extension name : (nullptr)"));
        path += ext;
        m_handle = openLib(path.c_str());
        if (!m_handle)
            throw handle_error(getHandleError(path));
    }

    /**
     *  Get a function from the dynamic library currently loaded in the object
     *
     *  @param T the template argument must be the function prototype. 
     *  it must be the same pattern as the template of std::function
     *  @param name symbol name of the function to get from the dynamic library
     *
     *  @returns std::function<T> that contains the function
     */
    template<typename T>
    std::function<T> getFunction(const char *name) const
    {
        if (!m_handle)
            throw handle_error("error : no dynamic library loaded");
        if (!name)
            throw symbol_error(getSymbolError("(nullptr)"));
        auto sym = getSymbol(name);
        if (!sym)
            throw symbol_error(getSymbolError(name));
        return reinterpret_cast<T *>(sym);
    }

    template<typename T>
    std::function<T> getFunction(const std::string &name) const
    {
        return getFunction<T>(name.c_str());
    }

    /**
     *  Get a global variable from the dynamic library currently loaded in the object
     *
     *  @param T type of the global variable
     *  @param name name of the global variable to get from the dynamic library
     *
     *  @returns global variable of type <T>
     */
    template<typename T>
    T &getVariable(const char *name) const
    {
        if (!m_handle)
            throw handle_error("error : no dynamic library loaded");
        if (!name)
            throw symbol_error(getSymbolError("(nullptr)"));
        auto sym = getSymbol(name);
        if (!sym)
            throw symbol_error(getSymbolError(name));
        return *reinterpret_cast<T *>(sym);
    }

    template<typename T>
    T &getVariable(const std::string &name) const
    {
        return getVariable<T>(name.c_str());
    }

    /**
     *  Close the dynamic library currently loaded in the object.
     *  This function will be automatically called by the class destructor
     */
    void close() noexcept
    {
        if (m_handle) {
            closeLib();
            m_handle = nullptr;
        }
    }
};