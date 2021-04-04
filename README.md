# DyLib - Dynamic Library Loader for C++  
[![DyLib](https://img.shields.io/badge/DyLib-v0.1-red.svg)](https://github.com/tocola/IO-TESTER/releases/tag/v1.6.2)
[![MIT license](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/tocola/IO-TESTER/blob/main/LICENSE)
[![CPP Version](https://img.shields.io/badge/C++-11/14/17/20-green.svg)](https://github.com/tocola/IO-TESTER/blob/main/LICENSE)

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
// main.cpp

#include <iostream>
#include "DyLib.hpp"

int main(int ac, char **av)
{
    DyLib lib("./myLib.so");

    auto adder = lib.getFunction<int, int, int>("adder");
    std::cout << adder(5, 10) << std::endl;

    auto printer = lib.getFunction<void>("printHello");
    printer();

    return 0;
}
```

Lets build :  
`g++ -std=c++11 main.cpp -ldl`

Result :
```
15
Hello!
```