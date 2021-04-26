/**
 * \file DyLib.hpp
 * \brief Cross-platform Dynamic Library Loader
 * \author Martin Olivier
 * \version 1.2
 * 
 * MIT License
 * Copyright (c) 2021 Martin Olivier
 */

#pragma once

#if __cplusplus >= 201103L

#include <string>
#include <functional>
#include <exception>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

class DyLib
{
private:
#ifdef _WIN32
    HINSTANCE m_handle{nullptr};
    static HINSTANCE m_openLib(const char *path)
    {
        return LoadLibrary(TEXT(path));
    }
    FARPROC m_getSymbol(const char *name) const
    {
        return GetProcAddress(m_handle, name);
    }
    void m_closeLib()
    {
        FreeLibrary(m_handle);
    }
#else
    void *m_handle{nullptr};
    static void *m_openLib(const char *path)
    {
        return dlopen(path, RTLD_NOW | RTLD_LOCAL);
    }
    void *m_getSymbol(const char *name) const
    {
        return dlsym(m_handle, name);
    }
    void m_closeLib()
    {
        dlclose(m_handle);
    }
#endif
public:

/**
 *  This exception is thrown when the DyLib library encountered an error. 
 *
 *  @return error message by calling what() member function
 */
    class exception : public std::exception
    {
    protected:
        const std::string m_error;
    public:
        explicit exception(std::string message) : m_error(std::move(message)) {};
        const char *what() const noexcept override {return m_error.c_str();};
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
        explicit handle_error(const std::string &message) : exception(message) {};
    };

/**
 *  This exception is thrown when the library failed to load a symbol. 
 *  This usualy happens when you forgot to mark a library function or variable as extern "C"
 *
 *  @param message error message
 */
    class symbol_error : public exception
    {
    public:
        explicit symbol_error(const std::string &message) : exception(message) {};
    };

/** Creates a dynamic library object
 */
    DyLib() noexcept = default;
    DyLib(const DyLib&) = delete;
    DyLib(DyLib &&) = delete;
    DyLib& operator=(const DyLib&) = delete;
    DyLib& operator=(DyLib &&) = delete;

/**
 *  Creates a dynamic library instance
 *
 *  @param path path to the dynamic library to load (.so, .dll, .dylib)
 */
    explicit DyLib(const char *path)
    {
        this->open(path);
    }

    explicit DyLib(const std::string &path)
    {
        this->open(path.c_str());
    }

    ~DyLib()
    {
        this->close();
    }

/**
 *  Load a dynamic library into the object. 
 *  If a dynamic library was already opened, it will be unload and replaced
 *
 *  @param path path to the dynamic library to load (.so, .dll, .dylib)
 */
    void open(const char *path)
    {
        this->close();
        if (!path)
            throw handle_error("Error while loading the dynamic library : (nullptr)");
        m_handle = m_openLib(path);
        if (!m_handle)
            throw handle_error("Error while loading the dynamic library : " + std::string(path));
    }

    void open(const std::string &path)
    {
        open(path.c_str());
    }

/**
 *  Get a function from the dynamic library currently loaded in the object
 *
 *  @param template_Type the template argument must be the function prototype
 *  it must be the same pattern as the template of std::function
 *  @param name symbol name of the function to get from the dynamic library
 *
 *  @returns std::function<Type> that contains the function
 */
    template<typename Type>
    std::function<Type> getFunction(const char *name) const
    {
        if (!m_handle)
            throw handle_error("Error : no dynamic library loaded");
        if (!name)
            throw symbol_error("Error while loading function : (nullptr)");
        auto sym = m_getSymbol(name);
        if (!sym)
            throw symbol_error("Error while loading function : " + std::string(name));
        return reinterpret_cast<Type *>(sym);
    }

    template<typename Type>
    std::function<Type> getFunction(const std::string &name) const
    {
        return getFunction<Type>(name.c_str());
    }

/**
 *  Get a global variable from the dynamic library currently loaded in the object
 *
 *  @param template_Type type of the global variable
 *  @param name name of the global variable to get from the dynamic library
 *
 *  @returns global variable of type <Type>
 */
    template<class Type>
    Type getVariable(const char *name) const
    {
        if (!m_handle)
            throw handle_error("Error : no dynamic library loaded");
        if (!name)
            throw symbol_error("Error while loading global variable : (nullptr)");
        auto sym = m_getSymbol(name);
        if (!sym)
            throw symbol_error("Error while loading global variable : " + std::string(name));
        return *(Type *)sym;
    }

    template<class Type>
    Type getVariable(const std::string &name) const
    {
        return getVariable<Type>(name.c_str());
    }

/** Close the dynamic library currently loaded in the object. 
 *  This function will be automatically called by the class destructor
 */
    void close() noexcept
    {
        if (m_handle)
            m_closeLib();
        m_handle = nullptr;
    }
};

#else
    #error DyLib needs at least a C++11 compliant compiler
#endif