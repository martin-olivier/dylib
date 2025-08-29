#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cassert>

#include "dylib.hpp"

#define STD_STRING "std::basic_string<char, std::char_traits<char>, std::allocator<char>>"
#define STD_VECTOR(T) "std::vector<" T ", std::allocator<" T ">>"

int main() {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    // C global variable
    auto pi_value = lib.get_variable<double>("pi_value");

    // C function
    auto hello_world = lib.get_function<const char *()>("hello_world");

    // C++ global variable
    auto magic = lib.get_variable<unsigned int>("example::magic");

    // C++ function without overloads
    auto dylib_info = lib.get_function<std::string()>("example::dylib::info");
    auto dylib_info_args = lib.get_function<std::string()>("example::dylib::info(void)");

    assert(dylib_info == dylib_info_args);

    // C++ function overloaded
    auto adder = lib.get_function<double(double, double)>
        ("example::adder(double, double)");

    auto adder_str = lib.get_function<std::string(const std::string &, const std::string &)>
        ("example::adder(" STD_STRING " const &, " STD_STRING " const &)");

    auto adder_vec = lib.get_function<std::vector<std::string>(const std::vector<std::string> &, const std::vector<std::string> &)>
        ("example::adder(" STD_VECTOR(STD_STRING) " const &, " STD_VECTOR(STD_STRING) " const &)");
    auto result_vec = adder_vec({"abc", "def"}, {"ghi", "jkl"});

    auto operation = lib.get_function<double(double, double, double (*)(double, double))>
        ("example::operation(double, double, double (*)(double, double))");


    std::cout << hello_world() << std::endl;
    std::cout << dylib_info() << std::endl;
    std::cout << "pi value: " << pi_value << std::endl;
    std::cout << "magic value: " << std::hex << magic << std::endl;
    std::cout << "10 + 10 = " << adder(10, 10) << std::endl;
    std::cout << "string = " << adder_str("abc", "def") << std::endl;
    std::cout << "vector:" << std::endl;
    for (const auto &str : result_vec) {
        std::cout << "- " << str << std::endl;
    }
    std::cout << "cb operation with (10, 5), using adder: " << operation(10, 5, adder) << std::endl;

    return EXIT_SUCCESS;
}
