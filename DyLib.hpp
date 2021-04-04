/**
 * \file DyLib.hpp
 * \brief Dynamic Library Loader
 * \author Martin Olivier
 * \version 0.2
 * 
 * MIT License
 * Copyright (c) 2021 Martin Olivier
 */

#pragma once

#if __cplusplus >= 201103L

#include <string>
#include <functional>
#include <exception>
#ifdef __WIN32__
#include <windows.h> 
#else
#include <dlfcn.h>
#endif

class DyLib
{
private:
#ifdef __WIN32__
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
        exception(const std::string &error) : m_error(error) {};
        const char *what() const noexcept override {return m_error.c_str();};
    };
/** Creates a Dynamic Library object.
 */
    DyLib() noexcept = default;
    DyLib(const DyLib&) = delete;
    DyLib& operator=(const DyLib&) = delete;

#ifdef __WIN32__
/**
 *  Creates a Dynamic Library instance.
 *
 *  @param path path to the dynamic library to load (.so, .dll, .dylib)
 */
    DyLib(const std::string &path)
    {
        this->load(path);
    }
#else
/**
 *  Creates a Dynamic Library instance.
 *
 *  @param path path to the dynamic library to load (.so, .dll, .dylib)
 *  @param mode dl lib flag -> RTLD_NOW by default
 */
    DyLib(const std::string &path, int mode = RTLD_NOW)
    {
        this->load(path, mode);
    }
#endif

    ~DyLib()
    {
        this->close();
    }

#ifdef __WIN32__
/**
 *  Load a Dynamic Library into the object. 
 *  If a Dynamic Library was Already opened, it will be unload and replaced
 *
 *  @param path path to the dynamic library to load (.so, .dylib)
 */
    void load(const std::string &path)
    {
        this->close();
        m_handle = LoadLibrary(TEXT(path.c_str()));
        if (!m_handle)
            throw exception("Error while loading dynamic library : " + path);
    }
#else
/**
 *  Load a Dynamic Library into the object. 
 *  If a Dynamic Library was Already opened, it will be unload and replaced
 *
 *  @param path path to the dynamic library to load (.so, .dylib)
 *  @param mode dl lib flag -> RTLD_NOW by default
 */
    void load(const std::string &path, int mode = RTLD_NOW)
    {
        this->close();
        m_handle = dlopen(path.c_str(), mode);
        if (!m_handle)
            throw exception(dlerror());
    }
#endif

/**
 *  Get a function from the Dynamic Library inside the object.
 *
 *  @param template_Ret the first template arg must be the return value of the function
 *  @param template_Args next template arguments must be the arguments of the function
 *  @param name function to get from the Dynamic Library
 *
 *  @returns std::function<Ret, ...Args> that contains the symbol
 */
#ifdef __WIN32__
    template<class Ret, class ...Args>
    std::function<Ret(Args...)> getFunction(const std::string &name)
    {
        if (!m_handle)
            throw exception("Error : no Dynamic Library loaded");
        auto sym = GetProcAddress(m_handle, name.c_str());
        if (!sym)
            throw exception("Error while loading function : " + name);
        return ((Ret (*)(Args...)) sym);
    }
#else
    template<class Ret, class ...Args>
    std::function<Ret(Args...)> getFunction(const std::string &name)
    {
        if (!m_handle)
            throw exception(dlerror());
        void *sym = dlsym(m_handle, name.c_str());
        if (!sym)
            throw exception(dlerror());
        return ((Ret (*)(Args...)) sym);
    }
#endif

/**
 *  Get a global variable from the Dynamic Library inside the object.
 *
 *  @param template_Type Type of the global variable
 *  @param name name of the global variable to get from the Dynamic Library
 *
 *  @returns global variable of type <Type>
 */
#ifdef __WIN32__
    template<class Type>
    Type getVariable(const std::string &name)
    {
        if (!m_handle)
            throw exception("Error : no Dynamic Library loaded");
        auto sym = GetProcAddress(m_handle, name.c_str());
        if (!sym)
            throw exception("Error while loading global variable : " + name);
        return *(Type*)sym;
    }
#else
    template<class Type>
    Type getVariable(const std::string &name)
    {
        if (!m_handle)
            throw exception(dlerror());
        void *sym = dlsym(m_handle, name.c_str());
        if (!sym)
            throw exception(dlerror());
        return *(Type*)sym;
    }
#endif

/** Close the Dynamic Library currently loaded into the object.
 */
#ifdef __WIN32__
    void close() noexcept
    {
        if (m_handle)
            FreeLibrary(m_handle);
        m_handle = nullptr;
    }
#else
    void close() noexcept
    {
        if (m_handle)
            dlclose(m_handle);
        m_handle = nullptr;
    }
#endif
};

#else
    #error DyLib needs at least a C++11 compliant compiler
#endif