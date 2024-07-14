# dylib example

Here is an example about the usage of the `dylib` library in a project

The functions and variables of our forthcoming dynamic library are located inside [lib.cpp](lib.cpp)

```c++
// lib.cpp

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
```

The code that will load functions and global variables of our dynamic library at runtime is located inside [main.cpp](main.cpp)

```c++
// main.cpp

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
```

Then, we want a build system that will:

- Fetch `dylib` into the project
- Build [lib.cpp](lib.cpp) into a dynamic library
- Build [main.cpp](main.cpp) into an executable

This build system is located inside [CMakeLists.txt](CMakeLists.txt)

```cmake
# CMakeLists.txt

cmake_minimum_required(VERSION 3.14)

project(dylib_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# dylib fetch

include(FetchContent)

FetchContent_Declare(
    dylib
    GIT_REPOSITORY  "https://github.com/martin-olivier/dylib"
    GIT_TAG         "v2.2.1"
)

FetchContent_MakeAvailable(dylib)

# build lib.cpp into a shared library

add_library(dynamic_lib SHARED lib.cpp)

# build main.cpp into an executable

add_executable(dylib_example main.cpp)
target_link_libraries(dylib_example PRIVATE dylib)
```

Let's build our code:
> Make sure to type the following commands inside the `example` folder

```sh
cmake . -B build
cmake --build build
```

Let's run our code:

```sh
# on unix, run the following command inside "build" folder
./dylib_example

# on windows, run the following command inside "build/Debug" folder
./dylib_example.exe
```

You will have the following result:

```sh
15
Hello
3.14159
1
```
