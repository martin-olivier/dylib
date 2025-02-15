#include <string>
#include <vector>

#if defined(_WIN32)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

// C functions and variables

extern "C" {

LIB_EXPORT double pi_value = 3.14159;

LIB_EXPORT const char *hello_world() {
    return "Hello World!";
}

}

// C++ functions and global variables

namespace example {
    LIB_EXPORT unsigned int magic = 0xcafebabe;

    namespace dylib {
        LIB_EXPORT std::string info() {
            return "dylib - v3.0.0";
        }
    }

    LIB_EXPORT double adder(double a, double b) {
        return a + b;
    }

    LIB_EXPORT std::string adder(const std::string &a, const std::string &b) {
        return a + b;
    }

    LIB_EXPORT std::vector<std::string> adder(const std::vector<std::string> &a, const std::vector<std::string> &b) {
        std::vector<std::string> result = a;

        result.insert(result.end(), b.begin(), b.end());

        return result;
    }
}
