/**
 * \file DyLib.hpp
 * \brief Mutiplatform Dynamic Library Loader
 * \author Martin Olivier
 * \version 0.4
 * 
 * MIT License
 * Copyright (c) 2021 Martin Olivier
 */

#pragma once

#if __cplusplus >= 201103L

#include <string>
#include <functional>
#include <exception>
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
#else
    void *m_handle{nullptr};
#endif
public:
    class handle_error : public std::runtime_error
    {
    public:
        handle_error(const char* message) : std::runtime_error(message) {};
        handle_error(const std::string &message) : std::runtime_error(message) {};
    };

    class symbol_error : public std::runtime_error
    {
    public:
        symbol_error(const char* message) : std::runtime_error(message) {};
        symbol_error(const std::string &message) : std::runtime_error(message) {};
    };

/** Creates a Dynamic Library object.
 */
    DyLib() noexcept = default;
    DyLib(const DyLib&) = delete;
    DyLib(DyLib &&) = delete;
    DyLib& operator=(const DyLib&) = delete;
    DyLib& operator=(DyLib &&) = delete;

/**
 *  Creates a Dynamic Library instance.
 *
 *  @param path path to the dynamic library to load (.so, .dll, .dylib)
 */
    DyLib(const char *path)
    {
        this->load(path);
    }

    DyLib(const std::string &path)
    {
        this->load(path.c_str());
    }

    ~DyLib()
    {
        this->close();
    }

/**
 *  Load a Dynamic Library into the object. 
 *  If a Dynamic Library was Already opened, it will be unload and replaced
 *
 *  @param path path to the dynamic library to load (.so, .dll, .dylib)
 */
    void load(const char *path)
    {
        this->close();
    #ifdef _WIN32
        m_handle = LoadLibrary(TEXT(path));
        if (!m_handle)
            throw handle_error("Error while loading the dynamic library : " + std::string(path));
    #else
        m_handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!m_handle)
            throw handle_error(dlerror());
    #endif
    }

    void load(const std::string &path)
    {
        load(path.c_str());
    }

/**
 *  Get a function from the Dynamic Library currently loaded in the object.
 *
 *  @param template_Type the template argument must be the function prototype
 *  it must be the same pattern as the template of std::function
 *  @param name symbol name of the function to get from the Dynamic Library
 *
 *  @returns std::function<Type> that contains the function
 */
    template<typename Type>
    std::function<Type> getFunction(const char *name)
    {
    #ifdef _WIN32
        if (!m_handle)
            throw handle_error("Error : no Dynamic Library loaded");
        auto sym = GetProcAddress(m_handle, name);
        if (!sym)
            throw symbol_error("Error while loading function : " + std::string(name));
    #else
        if (!m_handle)
            throw handle_error(dlerror());
        void *sym = dlsym(m_handle, name);
        if (!sym)
            throw symbol_error(dlerror());
    #endif
        return reinterpret_cast<Type *>(sym);
    }

    template<typename Type>
    std::function<Type> getFunction(const std::string &name)
    {
        return getFunction<Type>(name.c_str());
    }

/**
 *  Get a global variable from the Dynamic Library currently loaded in the object.
 *
 *  @param template_Type type of the global variable
 *  @param name name of the global variable to get from the Dynamic Library
 *
 *  @returns global variable of type <Type>
 */
    template<class Type>
    Type getVariable(const char *name)
    {
    #ifdef _WIN32
        if (!m_handle)
            throw handle_error("Error : no dynamic library loaded");
        auto sym = GetProcAddress(m_handle, name);
        if (!sym)
            throw symbol_error("Error while loading global variable : " + std::string(name));
    #else
        if (!m_handle)
            throw handle_error(dlerror());
        void *sym = dlsym(m_handle, name);
        if (!sym)
            throw symbol_error(dlerror());
    #endif
        return *(Type*)sym;
    }

    template<class Type>
    Type getVariable(const std::string &name)
    {
        return getVariable<Type>(name.c_str());
    }

/** Close the Dynamic Library currently loaded in the object.
 */
    void close() noexcept
    {
    #ifdef _WIN32
        if (m_handle)
            FreeLibrary(m_handle);
    #else
        if (m_handle)
            dlclose(m_handle);
    #endif
        m_handle = nullptr;
    }
};

#else
    #error DyLib needs at least a C++11 compliant compiler
#endif