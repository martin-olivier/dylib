#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

extern "C" {

LIB_EXPORT double pi_value_c = 3.14159;
LIB_EXPORT void *ptr_c = (void *)1;

LIB_EXPORT double adder(double a, double b) {
    return a + b;
}

LIB_EXPORT void print_hello() {
    std::cout << "Hello" << std::endl;
}

} // extern "C"

LIB_EXPORT double meaning_of_life = 42;
LIB_EXPORT const char *secret = "12345";

LIB_EXPORT std::vector<std::string> list_new_string() {
    return std::vector<std::string>();
}

LIB_EXPORT void list_add_string(std::vector<std::string> &cont, std::string elem) {
    cont.push_back(elem);
}

namespace tools {
    LIB_EXPORT double adder() {
        return 0;
    }

    LIB_EXPORT double adder(double a, double b) {
        return a + b;
    }

    LIB_EXPORT std::string adder(std::string a, std::string b) {
        return a + b;
    }

    namespace string {
        LIB_EXPORT std::string format(const char *str) {
            return std::string("ptr: ") + str;
        }

        LIB_EXPORT std::string format(const std::string str) {
            return std::string("cpy: ") + str;
        }

        LIB_EXPORT std::string format(const std::string &str) {
            return std::string("ref: ") + str;
        }

        LIB_EXPORT std::string format(std::string &&str) {
            return std::string("mov: ") + str;
        }

        LIB_EXPORT std::string format(const unsigned int& val) {
            return std::string("ref: ") + std::to_string(val);
        }
    }
}
