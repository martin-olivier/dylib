/**
 * \file DyLib.hpp
 * \brief Dynamic Library Loader
 * \author Martin Olivier
 * \version 0.3
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

    class exception : std::exception
    {
    protected:
        const std::string m_error;
    public:
        exception(const char* message) : m_error(message) {};
        exception(std::string message) : m_error(std::move(message)) {};
        const char *what() const noexcept override {return m_error.c_str();};
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
            throw exception("Error while loading dynamic library : " + std::string(path));
    #else
        m_handle = dlopen(path, RTLD_NOW);
        if (!m_handle)
            throw exception(dlerror());
    #endif
    }

    void load(const std::string &path)
    {
        load(path.c_str());
    }

/**
 *  Get a function from the Dynamic Library inside the object.
 *
 *  @param template_Ret the first template arg must be the return value of the function
 *  @param template_Args next template arguments must be the arguments of the function
 *  @param name function to get from the Dynamic Library
 *
 *  @returns std::function<Ret, ...Args> that contains the symbol
 */
    template<class Ret, class ...Args>
    std::function<Ret(Args...)> getFunction(const char *name)
    {
    #ifdef _WIN32
        if (!m_handle)
            throw exception("Error : no Dynamic Library loaded");
        auto sym = GetProcAddress(m_handle, name);
        if (!sym)
            throw exception("Error while loading function : " + std::string(name));
    #else
        if (!m_handle)
            throw exception(dlerror());
        void *sym = dlsym(m_handle, name);
        if (!sym)
            throw exception(dlerror());
    #endif
        return ((Ret (*)(Args...)) sym);
    }

    template<class Ret, class ...Args>
    std::function<Ret(Args...)> getFunction(const std::string &name)
    {
        return getFunction<Ret, Args...>(name.c_str());
    }

/**
 *  Get a global variable from the Dynamic Library inside the object.
 *
 *  @param template_Type Type of the global variable
 *  @param name name of the global variable to get from the Dynamic Library
 *
 *  @returns global variable of type <Type>
 */
    template<class Type>
    Type getVariable(const char *name)
    {
    #ifdef _WIN32
        if (!m_handle)
            throw exception("Error : no Dynamic Library loaded");
        auto sym = GetProcAddress(m_handle, name);
        if (!sym)
            throw exception("Error while loading global variable : " + std::string(name));
    #else
        if (!m_handle)
            throw exception(dlerror());
        void *sym = dlsym(m_handle, name);
        if (!sym)
            throw exception(dlerror());
    #endif
        return *(Type*)sym;
    }

    template<class Type>
    Type getVariable(const std::string &name)
    {
        return getVariable<Type>(name.c_str());
    }

/** Close the Dynamic Library currently loaded into the object.
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