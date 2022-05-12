#include <iostream>
#include "dylib.hpp"

int main() {
    try {
        dylib lib("./dynamic_lib", dylib::extension);

        auto adder = lib.get_symbol<double(double, double)>("adder");
        std::cout << adder(5, 10) << std::endl;

        auto printer = lib.get_symbol<void()>("print_hello");
        printer();

        double pi_value = lib.get_symbol<double>("pi_value");
        std::cout << pi_value << std::endl;

        void *ptr = lib.get_symbol<void *>("ptr");
        if (ptr == (void *)1) std::cout << 1 << std::endl;
    }
    catch (const dylib::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}