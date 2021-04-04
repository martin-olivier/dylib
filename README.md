# DyLib - Dynamic Library Loader for C++  
[![DyLib](https://img.shields.io/badge/DyLib-v0.1-blue.svg)](https://github.com/tocola/DyLib)
[![MIT license](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/tocola/DyLib/blob/main/LICENSE)
[![CPP Version](https://img.shields.io/badge/C++-11/14/17/20-darkgreen.svg)](https://isocpp.org/)

The goal of this Library is to load Dynamic Libraries (.so, .dylib, ...) and access its functions and symbols at runtime.

## Compatibility
Tested on `MacOS`, `Ubuntu`, `Fedora`, `ArchLinux`

## Installation

1. Clone this repository
2. Go to the `DyLib` folder
3. Grab the `DyLib.hpp` file

## Usage

Lets write some functions in our lib
```c++
// myLib.cpp

#include <iostream>

extern "C" int adder(int a, int b)
{
    return a + b;
}

extern "C" void printHello()
{
    std::cout << "Hello!" << std::endl;
}
```

Lets build our lib :  

`g++ -fPIC -shared myLib.cpp -o myLib.so`

Lets try to access the functions of our dynamic lib at runtime with this main :
```c++
#include "DyLib.hpp"
#include <iostream>

int main(int ac, char **av)
{
    try {
        DyLib lib("./myLib.so");

        // getFunction<ReturnValue, arg1, arg2>("functionName")
        auto adder = lib.getFunction<int, int, int>("adder");
        std::cout << adder(5, 10) << std::endl;

        // getFunction<ReturnValue>("functionName")
        auto printer = lib.getFunction<void>("printHello");
        printer();
    }
    catch (const DyLib::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

Lets build :  
`g++ -std=c++11 main.cpp -ldl`

Result :
```
15
Hello!
```
