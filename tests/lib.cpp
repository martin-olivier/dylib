#include <iostream>

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

LIB_EXPORT void print_hello() {
    std::cout << "Hello" << std::endl;
}

} // extern "C"

LIB_EXPORT double do_you_find_me(double a, double b) {
    return a + b;
}

namespace toto {
    LIB_EXPORT double and_now(double a, double b) {
        return a + b;
    }

    LIB_EXPORT double and_now(double a, std::string b) {
        return a + b.size();
    }
}

LIB_EXPORT double zaza = 12;

namespace tata {
    LIB_EXPORT double zozo = 11;
}