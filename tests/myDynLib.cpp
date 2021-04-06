#include <iostream>

extern "C" {
    double pi_value = 3.14159;
    void *ptr = nullptr;

    double adder(double a, double b)
    {
        return a + b;
    }

    void printHello()
    {
        std::cout << "Hello!" << std::endl;
    }
}