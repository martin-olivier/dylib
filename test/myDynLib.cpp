#include <iostream>
#include "DyLib.hpp"

export_symbol double pi_value = 3.14159;
export_symbol void *ptr = (void *)1;

export_symbol double adder(double a, double b)
{
    return a + b;
}

export_symbol void printHello()
{
    std::cout << "Hello!" << std::endl;
}