# Dylib - C++ cross-platform dynamic library loader  
[![Dylib](https://img.shields.io/badge/Dylib-v1.8.3-blue.svg)](https://github.com/martin-olivier/dylib/releases/tag/v1.8.3)
[![MIT license](https://img.shields.io/badge/License-MIT-orange.svg)](https://github.com/martin-olivier/dylib/blob/main/LICENSE)
[![CPP Version](https://img.shields.io/badge/C++-11_and_above-darkgreen.svg)](https://isocpp.org/)

[![GitHub watchers](https://img.shields.io/github/watchers/martin-olivier/dylib?style=social)](https://github.com/martin-olivier/dylib/watchers/)
[![GitHub forks](https://img.shields.io/github/forks/martin-olivier/dylib?style=social)](https://github.com/martin-olivier/dylib/network/members/)
[![GitHub stars](https://img.shields.io/github/stars/martin-olivier/dylib?style=social)](https://github.com/martin-olivier/dylib/stargazers/)

[![workflow](https://github.com/martin-olivier/dylib/actions/workflows/CI.yml/badge.svg)](https://github.com/martin-olivier/dylib/actions/workflows/CI.yml)
[![codecov](https://codecov.io/gh/martin-olivier/dylib/branch/main/graph/badge.svg?token=4V6A9B7PII)](https://codecov.io/gh/martin-olivier/dylib)

[![GitHub download](https://img.shields.io/github/downloads/martin-olivier/dylib/total?style=for-the-badge)](https://github.com/martin-olivier/dylib/releases/download/v1.8.3/dylib.hpp)

The goal of this C++ library is to load dynamic libraries (.so, .dll, .dylib) and access its functions and global variables at runtime.  

`⭐ Don't forget to put a star if you like the project!`

# Compatibility
Works on `Linux`, `Windows`, `MacOS`

# Installation

You can fetch `dylib` to your project using `CMake`:
```cmake
include(FetchContent)

FetchContent_Declare(
    dylib
    GIT_REPOSITORY "https://github.com/martin-olivier/dylib"
    GIT_TAG        "v1.8.3"
)

FetchContent_MakeAvailable(dylib)
```

You can also click [HERE](https://github.com/martin-olivier/dylib/releases/download/v1.8.3/dylib.hpp) to download the `dylib` header file.  

# Documentation

## Dylib class

The `dylib` class can load a dynamic library at runtime:
```c++
dylib lib("./dynamic_lib.so");
```
The `dylib` class can detect the file extension of the actual os using `dylib::extension`:
```c++
dylib lib("./dynamic_lib", dylib::extension);
```
or
```c++
dylib lib;
lib.open("./dynamic_lib", dylib::extension);
```

## Open and close

`open`  
Load a dynamic library into the object. If a dynamic library was already opened, it will be unloaded and replaced  

`close`  
Unload the dynamic library currently loaded in the object. This function will be automatically called by the class destructor
```c++
// Load ./dynamic_lib.so

dylib lib("./dynamic_lib.so");

// Unload ./dynamic_lib.so and load ./other_dynamic_lib.so

lib.open("./other_dynamic_lib.so");

// Unload ./other_dynamic_lib.so

lib.close();
```

## Get a function or a variable 

`get_function`  
Get a function from the dynamic library currently loaded in the object  

`get_variable`  
Get a global variable from the dynamic library currently loaded in the object
```c++
// Load ./dynamic_lib.so

dylib lib("./dynamic_lib.so");

// Get the function adder

auto adder = lib.get_function<double(double, double)>("adder");

// Get the global variable pi_value

double pi = lib.get_variable<double>("pi_value");

// Use the function adder with pi_value

double result = adder(pi, pi);
```

## Miscellaneous tools

`operator bool`  
Returns true if a dynamic library is currently loaded in the object, false otherwise  

`has_symbol`  
Returns true if the symbol passed as parameter exists in the dynamic library, false otherwise  

`native_handle`  
Returns the dynamic library handle  
```c++
void example(dylib &lib)
{
    if (lib)
        std::cout << "Something is currently loaded in the dylib object" << std::endl;

    if (lib.has_symbol("GetModule"))
        std::cout << "GetModule symbol has been found" << std::endl;

    dylib::native_handle_type handle = lib.native_handle();
}
```

## Dylib exceptions

`handle_error`  
This exception is raised when the library failed to load or the library encountered symbol resolution issues  

`symbol_error`  
This exception is raised when the library failed to load a symbol.
This usually happens when you forgot to put `DYLIB_API` before a library function or variable  

Those exceptions inherit from `dylib::exception`
```c++
try {
    dylib lib("./dynamic_lib.so");
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

A full example about the usage of the `dylib` library is available [HERE](example)

# Tips

## Remove the lib prefix

> If you use CMake to build a dynamic library, running the below CMake rule will allow you to remove the prefix `lib` for macOS and linux, ensuring that the library shares the same name on all the different OS:

```cmake
set_target_properties(target PROPERTIES PREFIX "")
```

|         | Without CMake rule    | With CMake rule |
|:-------:|:----------------------|:----------------|
|  Linux  | ***lib***malloc.so    | malloc.so       |
|  MacOS  | ***lib***malloc.dylib | malloc.dylib    |
| Windows | malloc.dll            | malloc.dll      |

## Build and run unit tests

To build the unit tests, enter the following commands:
```sh
cmake . -B build -DBUILD_TESTS=ON
cmake --build build
```

To run the unit tests, enter the following command:
```sh
# on unix
./unit_tests

# on windows
./Debug/unit_tests.exe
```
