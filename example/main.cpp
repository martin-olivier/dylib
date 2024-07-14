#include <iostream>
#include "dylib.hpp"

int main() {
    dylib lib("./", "dynamic_lib");

    auto adder = lib.get_function<double(double, double)>("adder");
    std::cout << adder(5, 10) << std::endl;

    auto printer = lib.get_function<void()>("print_hello");
    printer();

    double pi_value = lib.get_variable<double>("pi_value");
    std::cout << pi_value << std::endl;

    void *ptr = lib.get_variable<void *>("ptr");
    if (ptr == (void *)1) std::cout << 1 << std::endl;

    return EXIT_SUCCESS;
}