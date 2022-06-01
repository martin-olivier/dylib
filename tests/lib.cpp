#include <iostream>
#include "dylib.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define LIB_API __declspec(dllexport)
#else
#define LIB_API
#endif

extern "C" {

LIB_API double pi_value = 3.14159;
LIB_API void *ptr = (void *)1;

LIB_API double adder(double a, double b) {
    return a + b;
}

LIB_API extern void print_hello() {
    std::cout << "Hello" << std::endl;
}

} // extern "C"