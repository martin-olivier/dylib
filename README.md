# DyLib - Dynamic Library Loader for C++  
[![DyLib](https://img.shields.io/badge/DyLib-v1.7.0-blue.svg)](https://github.com/tocola/DyLib/releases/tag/v1.7.0)
[![MIT license](https://img.shields.io/badge/License-MIT-orange.svg)](https://github.com/tocola/DyLib/blob/main/LICENSE)
[![CPP Version](https://img.shields.io/badge/C++-11-darkgreen.svg)](https://isocpp.org/)

[![GitHub watchers](https://img.shields.io/github/watchers/tocola/DyLib?style=social)](https://github.com/tocola/DyLib/watchers/)
[![GitHub forks](https://img.shields.io/github/forks/tocola/DyLib?style=social)](https://github.com/tocola/DyLib/network/members/)
[![GitHub stars](https://img.shields.io/github/stars/tocola/DyLib?style=social)](https://github.com/tocola/DyLib/stargazers/)

[![workflow](https://github.com/tocola/DyLib/actions/workflows/unit_tests.yml/badge.svg)](https://github.com/tocola/DyLib/actions/workflows/unit_tests.yml)
[![codecov](https://codecov.io/gh/tocola/DyLib/branch/main/graph/badge.svg?token=4V6A9B7PII)](https://codecov.io/gh/tocola/DyLib)

[![GitHub download](https://img.shields.io/github/downloads/tocola/DyLib/total?style=for-the-badge)](https://github.com/tocola/DyLib/releases/download/v1.7.0/DyLib.hpp)

The goal of this C++ Library is to load dynamic libraries (.so, .dll, .dylib) and access its functions and global variables at runtime.

# Compatibility
Works on `Linux`, `Windows`, `MacOS`

# Installation

Click [HERE](https://github.com/tocola/DyLib/releases/download/v1.7.0/DyLib.hpp) to download the DyLib header file  
`⭐ Don't forget to put a star if you like the project!`

# Documentation

## DyLib Class

The DyLib class can load a dynamic library at runtime :
```c++
DyLib lib("./myDynLib.so");
```
The DyLib class can detect the file extension of the actual os using `DyLib::extension` :
```c++
DyLib lib("./myDynLib", DyLib::extension);
```
or
```c++
DyLib lib;
lib.open("./myDynLib", DyLib::extension);
```

## Open and Close

`open`  
Load a dynamic library into the object. If a dynamic library was already opened, it will be unload and replaced  

`close`  
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

`getFunction`  
Get a function from the dynamic library currently loaded in the object.  

`getVariable`  
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

`handle_error`  
This exception is thrown when the library failed to load or the library encountered symbol resolution issues  

`symbol_error`  
This exception is thrown when the library failed to load a symbol.
This usualy happens when you forgot to put `export_symbol` before a library function or variable  


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
#include "DyLib.hpp"

export_symbol double pi_value = 3.14159;
export_symbol void *ptr = (void *)1;

export_symbol double adder(double a, double b)
{
    return a + b;
}

export_symbol void printHello()
{
    std::cout << "Hello!" << std::endl;
}
```

Lets build our code into a dynamic library :  

`g++ -std=c++11 -fPIC -shared myDynLib.cpp -o myDynLib.so`

Lets try to access the functions and global variables of our dynamic library at runtime with this code :
```c++
// main.cpp

#include <iostream>
#include "DyLib.hpp"

int main()
{
    try {
        DyLib lib("./myDynLib.so");

        auto adder = lib.getFunction<double(double, double)>("adder");
        std::cout << adder(5, 10) << std::endl;

        auto printer = lib.getFunction<void()>("printHello");
        printer();

        double pi_value = lib.getVariable<double>("pi_value");
        std::cout << pi_value << std::endl;

        auto &ptr = lib.getVariable<void *>("ptr");
        if (ptr == (void *)1)
            std::cout << "1" << std::endl;
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
1
```

# Tips

> If you are using CMake to build a dynamic library, you can remove the prefix `lib` for macOS and linux to ensure the library has the same name on all platforms with this CMake rule :

```cmake
set_target_properties(target PROPERTIES PREFIX "")
```

## Results

|          | Without CMake rule    | With CMake rule |
| :------: | :-------------------- | :-------------- |
| Linux    | ***lib***malloc.so    | malloc.so       |
| MacOS    | ***lib***malloc.dylib | malloc.dylib    |
| Windows  | malloc.dll            | malloc.dll      |
