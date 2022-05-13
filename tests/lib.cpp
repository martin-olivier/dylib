#include <iostream>
#include "dylib.hpp"

DYLIB_API double pi_value;
DYLIB_API void *ptr;

double pi_value = 3.14159;
void *ptr = (void *)1;

DYLIB_API double adder(double a, double b) {
    return a + b;
}

DYLIB_API void print_hello() {
    std::cout << "Hello" << std::endl;
}