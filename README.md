# dylib

[![version](https://img.shields.io/badge/Version-3.0.0-blue.svg)](https://github.com/martin-olivier/dylib/releases/tag/v3.0.0)
[![license](https://img.shields.io/badge/License-MIT-orange.svg)](https://github.com/martin-olivier/dylib/blob/main/LICENSE)
[![cpp](https://img.shields.io/badge/Compatibility-C++11-darkgreen.svg)](https://isocpp.org)
[![ci](https://github.com/martin-olivier/dylib/actions/workflows/CI.yml/badge.svg)](https://github.com/martin-olivier/dylib/actions/workflows/CI.yml)

The goal of this C++ library is to load dynamic libraries (.so, .dll, .dylib) and access its C and C++ functions and global variables at runtime.  

`⭐ Don't forget to put a star if you like the project!`

## Compatibility

- **OS**:
Works on `Linux`, `MacOS` and `Windows`

- **Compilers**:
Works with `GCC`, `Clang`, `MSVC` and `MinGW`

## Installation

### Using a package manager

You can install `dylib` from [vcpkg](https://vcpkg.io/en) or [conan center](https://conan.io/center):

```sh
vcpkg install dylib
```

```sh
conan install --requires=dylib/3.0.0
```

### Using CMake Fetch

You can also fetch `dylib` to your project using `CMake`:

```cmake
include(FetchContent)

FetchContent_Declare(
    dylib
    GIT_REPOSITORY "https://github.com/martin-olivier/dylib"
    GIT_TAG        "v3.0.0"
)

FetchContent_MakeAvailable(dylib)
```

## Documentation

### Constructor

The `dylib::library` class can load a dynamic library from a relative or a full path:

```c++
// Load "foo" library from relative path "./plugins"

dylib::library lib("./plugins/libfoo.so");

// Load "foo" library from full path "/lib"

dylib::library lib("/lib/libfoo.so");
```

The `dylib::library` class can add filename decorations to the library name.
You can use `dylib::decorations::os_default()` to add default OS decorations to the library name.
You can also use `dylib::decorations` to add custom decorations to the library name.

```c++
// No decorations added
// Windows -> "foo.lib"
// MacOS   -> "foo.lib"
// Linux   -> "foo.lib"

dylib::library lib("./foo.lib");

// Default OS decorations added
// Windows -> "foo.dll"
// MacOS   -> "libfoo.dylib"
// Linux   -> "libfoo.so"

dylib::library lib("./foo", dylib::decorations::os_default());

// Custom OS decorations added
// Windows -> "foo.dll"
// MacOS   -> "libfoo.so"
// Linux   -> "libfoo.so"

auto custom_decorations = dylib::decorations(
    DYLIB_WIN_OTHER("", "lib"), DYLIB_WIN_OTHER(".dll", ".so")
);
dylib::library lib("./foo", custom_decorations);
```

### Get a function or a variable

`get_function`  
Get a function from the dynamic library currently loaded in the object  

`get_variable`  
Get a global variable from the dynamic library currently loaded in the object

#### C symbols

```c++
// Load "foo" dynamic library

dylib::library lib("./foo", dylib::decorations::os_default());

// Get the C function "adder" (get_function<T> will return T*)

auto adder = lib.get_function<double(double, double)>("adder");

// Get the variable "pi_value" (get_variable<T> will return T&)

double pi = lib.get_variable<double>("pi_value");

// Use the function "adder" with "pi_value"

double result = adder(pi, pi);
```

#### C++ symbols

```c++
// Load "foo" dynamic library

dylib::library lib("./foo", dylib::decorations::os_default());

// Get the C++ functions "to_string" located in the namespace "tools"
// The format for the function arguments is the following:
// type [const] [volatile] [*|&|&&]

auto int_to_string = lib.get_function<std::string(int)>("tools::to_string(int)");
auto double_to_string = lib.get_function<std::string(double)>("tools::to_string(double)");

// Get the variable "pi_value" located in the namespace "global::math"

double pi = lib.get_variable<double>("global::math::pi_value");

std::string meaning_of_life_str = int_to_string(42);
std::string pi_str = double_to_string(pi);
```

### Gather library symbols

You can collect the symbols of a dynamic library using the `symbols` method:

```c++
// Load "foo" dynamic library

dylib::library lib("./foo", dylib::decorations::os_default());

// Iterate through symbols

for (auto &symbol : lib.symbols()) {
    if (symbol.loadable)
        std::cout << symbol.demangled_name << std::endl;
}
```

### Miscellaneous tools

`get_symbol`  
Get a C or C++ symbol from the dynamic library currently loaded in the object  

`native_handle`  
Returns the dynamic library handle

```c++
dylib::library lib("./foo", dylib::decorations::os_default());

dylib::native_handle_type handle = lib.native_handle();
dylib::native_symbol_type symbol = lib.get_symbol("pi_value");

assert(handle != nullptr && symbol != nullptr);
assert(symbol == dlsym(handle, "pi_value"));
```

### Exceptions

`load_error`  
This exception is raised when the library failed to load or the library encountered symbol resolution issues  

`symbol_error`  
This exception is raised when the library failed to load a symbol  

Those exceptions inherit from `dylib::exception`

```c++
try {
    dylib::library lib("./foo", dylib::decorations::os_default());
    double pi_value = lib.get_variable<double>("pi_value");
    std::cout << pi_value << std::endl;
} catch (const dylib::load_error &) {
    std::cerr << "failed to load 'foo' library" << std::endl;
} catch (const dylib::symbol_error &) {
    std::cerr << "failed to get 'pi_value' symbol" << std::endl;
}
```

## Example

A full example about the usage of the `dylib` library is available [HERE](example)

## Tests

To build unit tests, enter the following commands:

```sh
cmake . -B build -DDYLIB_BUILD_TESTS=ON
cmake --build build
```

To run unit tests, enter the following command inside `build` directory:

```sh
ctest
```

## Community

If you have any question about the usage of the library, do not hesitate to open a [discussion](https://github.com/martin-olivier/dylib/discussions)

If you want to report a bug or provide a feature, do not hesitate to open an [issue](https://github.com/martin-olivier/dylib/issues) or submit a [pull request](https://github.com/martin-olivier/dylib/pulls)
