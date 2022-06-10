#include <iostream>
#include "dylib.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

extern "C" {

LIB_EXPORT double pi_value = 3.14159;
LIB_EXPORT void *ptr = (void *)1;

LIB_EXPORT double adder(double a, double b) {
    return a + b;
}

LIB_EXPORT extern void print_hello() {
    std::cout << "Hello" << std::endl;
}

} // extern "C"