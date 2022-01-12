# Dylib - Dynamic Library Loader for C++  
[![Dylib](https://img.shields.io/badge/Dylib-v1.7.0-blue.svg)](https://github.com/tocola/dylib/releases/tag/v1.7.0)
[![MIT license](https://img.shields.io/badge/License-MIT-orange.svg)](https://github.com/tocola/dylib/blob/main/LICENSE)
[![CPP Version](https://img.shields.io/badge/C++-11_and_above-darkgreen.svg)](https://isocpp.org/)

[![GitHub watchers](https://img.shields.io/github/watchers/tocola/dylib?style=social)](https://github.com/tocola/dylib/watchers/)
[![GitHub forks](https://img.shields.io/github/forks/tocola/dylib?style=social)](https://github.com/tocola/dylib/network/members/)
[![GitHub stars](https://img.shields.io/github/stars/tocola/dylib?style=social)](https://github.com/tocola/dylib/stargazers/)

[![workflow](https://github.com/tocola/dylib/actions/workflows/CI.yml/badge.svg)](https://github.com/tocola/dylib/actions/workflows/CI.yml)
[![codecov](https://codecov.io/gh/tocola/dylib/branch/main/graph/badge.svg?token=4V6A9B7PII)](https://codecov.io/gh/tocola/dylib)

[![GitHub download](https://img.shields.io/github/downloads/tocola/dylib/total?style=for-the-badge)](https://github.com/tocola/dylib/releases/download/v1.7.0/dylib.hpp)

The goal of this C++ Library is to load dynamic libraries (.so, .dll, .dylib) and access its functions and global variables at runtime.

# Compatibility
Works on `Linux`, `Windows`, `MacOS`

# Installation

Click [HERE](https://github.com/tocola/dylib/releases/download/v1.7.0/dylib.hpp) to download the dylib header file  
`⭐ Don't forget to put a star if you like the project!`

# Documentation

## Dylib Class

The dylib class can load a dynamic library at runtime:
```c++
dylib lib("./myDynLib.so");
```
The dylib class can detect the file extension of the actual os using `dylib::extension`:
```c++
dylib lib("./myDynLib", dylib::extension);
```
or
```c++
dylib lib;
lib.open("./myDynLib", dylib::extension);
```

## Open and Close

`open`  
Load a dynamic library into the object. If a dynamic library was already opened, it will be unloaded and replaced  

`close`  
Close the dynamic library currently loaded in the object. This function will be automatically called by the class destructor
```c++
// Load ./myDynLib.so

dylib lib("./myDynLib.so");

// Unload ./myDynLib.so and load ./otherLib.so

lib.open("./otherLib.so");

// Close ./otherLib.so

lib.close();
```

## Get a Function or a Variable

`get_function`  
Get a function from the dynamic library currently loaded in the object.  

`get_variable`  
Get a global variable from the dynamic library currently loaded in the object.
```c++
// Load ./myDynLib.so

dylib lib("./myDynLib.so");

// Get the global function adder

auto adder = lib.get_function<double(double, double)>("adder");

// Get the global variable pi_value

double pi = lib.get_variable<double>("pi_value");

// Use the function adder with pi_value

double result = adder(pi, pi);
```

## Dylib Exceptions

`handle_error`  
This exception is raised when the library failed to load or the library encountered symbol resolution issues  

`symbol_error`  
This exception is raised when the library failed to load a symbol.
This usually happens when you forgot to put `DYLIB_API` before a library function or variable  


Those exceptions inherit from `dylib::exception`
```c++
try {
    dylib lib("./myDynLib.so");
    double pi_value = lib.get_variable<double>("pi_value");
    std::cout << pi_value << std::endl;
}
catch (const dylib::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
return EXIT_SUCCESS;
```

# Example

Let's write some functions in our forthcoming dynamic library:
```c++
// myDynLib.cpp

#include <iostream>
#include "dylib.hpp"

DYLIB_API double pi_value = 3.14159;
DYLIB_API void *ptr = (void *)1;

DYLIB_API double adder(double a, double b)
{
    return a + b;
}

DYLIB_API void print_hello()
{
    std::cout << "Hello!" << std::endl;
}
```

Let's build our code into a dynamic library:  

`g++ -std=c++11 -fPIC -shared myDynLib.cpp -o myDynLib.so`

Let's try to access the functions and global variables of our dynamic library at runtime with this code:
```c++
// main.cpp

#include <iostream>
#include "dylib.hpp"

int main()
{
    try {
        dylib lib("./myDynLib.so");

        auto adder = lib.get_function<double(double, double)>("adder");
        std::cout << adder(5, 10) << std::endl;

        auto printer = lib.get_function<void()>("print_hello");
        printer();

        double pi_value = lib.get_variable<double>("pi_value");
        std::cout << pi_value << std::endl;

        auto &ptr = lib.get_variable<void *>("ptr");
        if (ptr == (void *)1)
            std::cout << "1" << std::endl;
    }
    catch (const dylib::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

Let's build and run our code:  
`g++ -std=c++11 main.cpp -o out -ldl`  
`./out`

Output:
```
15
Hello!
3.14159
1
```

# Tips

> If you use CMake to build a dynamic library, running the below CMake rule will allow you to remove the prefix `lib` for macOS and linux, ensuring that the library shares the same name on all the different OS:

```cmake
set_target_properties(target PROPERTIES PREFIX "")
```

## Results

|         | Without CMake rule    | With CMake rule |
|:-------:|:----------------------|:----------------|
|  Linux  | ***lib***malloc.so    | malloc.so       |
|  MacOS  | ***lib***malloc.dylib | malloc.dylib    |
| Windows | malloc.dll            | malloc.dll      |
