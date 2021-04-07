# DyLib - Dynamic Library Loader for C++  
[![DyLib](https://img.shields.io/badge/DyLib-v0.5-blue.svg)](https://github.com/tocola/DyLib/releases/tag/v0.5)
[![MIT license](https://img.shields.io/badge/License-MIT-orange.svg)](https://github.com/tocola/DyLib/blob/main/LICENSE)
[![CPP Version](https://img.shields.io/badge/C++-11/14/17/20-darkgreen.svg)](https://isocpp.org/)

[![GitHub watchers](https://img.shields.io/github/watchers/tocola/DyLib?style=social)](https://github.com/tocola/DyLib/watchers/)
[![GitHub forks](https://img.shields.io/github/forks/tocola/DyLib?style=social)](https://github.com/tocola/DyLib/network/members/)
[![GitHub stars](https://img.shields.io/github/stars/tocola/DyLib?style=social)](https://github.com/tocola/DyLib/stargazers/)

[![example workflow](https://github.com/tocola/DyLib/actions/workflows/unit_tests.yml/badge.svg)](https://github.com/tocola/DyLib/actions/workflows/unit_tests.yml)

[![GitHub download](https://img.shields.io/github/downloads/tocola/DyLib/total?style=for-the-badge)](https://github.com/tocola/DyLib/releases/download/v0.5/DyLib.hpp)

The goal of this C++ Library is to load dynamic libraries (.so, .dll, .dylib) and access its functions and global variables at runtime.

# Compatibility
Works on `Linux`, `Windows`, `MacOS`

# Installation

1. Click [HERE](https://github.com/tocola/DyLib/releases/download/v0.5/DyLib.hpp) to download the DyLib header file
2. Put the DyLib header file in your project directory

# Documentation

## DyLib Class

The DyLib class can load a dynamic library at runtime :
```c++
DyLib lib("./myDynLib.so");
```
or
```c++
DyLib lib;
lib.open("./myDynLib.so");
```

## Open and Close

`Open:`  
Load a dynamic library into the object. If a dynamic library was already opened, it will be unload and replaced  
`Close:`  
Close the dynamic library currently loaded in the object. This function will be automatically called by the class destructor
```c++
// Load ./myDynLib.so

DyLib lib("./myDynLib.so");

// Unload ./myDynLib.so and load ./otherLib.so

lib.open("./otherLib.so");

// Close ./otherLib.so

lib.close();
```

## Get a Function or a Variable

`getFunction:`  
Get a function from the dynamic library currently loaded in the object.  
`getVariable:`  
Get a global variable from the dynamic library currently loaded in the object.
```c++
// Load ./myDynLib.so

DyLib lib("./myDynLib.so");

// Get the global function adder

auto adder = lib.getFunction<double(double, double)>("adder");

// Get the global variable pi_value

double pi = lib.getVariable<double>("pi_value");

// Use the function adder with pi_value

double result = adder(pi, pi);
```

## DyLib Exceptions

`handle_error:`  
This exception is thrown when the library failed to load or the library encountered symbol resolution issues  
`symbol_error:`  
This exception is thrown when the library failed to load a symbol.
This usualy happens when you forgot to mark a library function or variable as `extern "C"`  


Those exceptions inherits from `DyLib::exception`
```c++
try {
    DyLib lib("./myDynLib.so");
    double pi_value = lib.getVariable<double>("pi_value");
    std::cout << pi_value << std::endl;
}
catch (const DyLib::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
return EXIT_SUCCESS;
```

# Exemple

Lets write some functions in our future dynamic library :
```c++
// myDynLib.cpp

#include <iostream>

extern "C" {
    double pi_value = 3.14159;
    void *ptr = nullptr;

    double adder(double a, double b)
    {
        return a + b;
    }

    void printHello()
    {
        std::cout << "Hello!" << std::endl;
    }
}
```

Lets build our code into a dynamic library :  

`g++ -std=c++11 -fPIC -shared myDynLib.cpp -o myDynLib.so`

Lets try to access the functions and global variables of our dynamic library at runtime with this code :
```c++
// main.cpp

#include <iostream>
#include "DyLib.hpp"

int main(int ac, char **av)
{
    try {
        DyLib lib("./myDynLib.so");

        auto adder = lib.getFunction<double(double, double)>("adder");
        std::cout << adder(5, 10) << std::endl;

        auto printer = lib.getFunction<void()>("printHello");
        printer();

        double pi_value = lib.getVariable<double>("pi_value");
        std::cout << pi_value << std::endl;

        void *ptr = lib.getVariable<void *>("ptr");
        if (ptr == nullptr)
            std::cout << "nullptr" << std::endl;
    }
    catch (const DyLib::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

Lets build and run our code :  
`g++ -std=c++11 main.cpp -o out -ldl`  
`./out`

Output :
```
15
Hello!
3.14159
nullptr
```
