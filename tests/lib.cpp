#include <iostream>
#include "dylib.hpp"

DYLIB_API inline double pi_value = 3.14159;
DYLIB_API inline void *ptr = (void *)1;

DYLIB_API double adder(double a, double b) {
    return a + b;
}

DYLIB_API void print_hello() {
    std::cout << "Hello" << std::endl;
}