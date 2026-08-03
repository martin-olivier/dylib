#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__)
// suppress '#warning <ciso646> is deprecated in C++17' for mingw and ucrt
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#endif
#include <gtest/gtest.h>
#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "dylib.hpp"
#include "lib.hpp"

#ifndef _WIN32
#include <dlfcn.h>
#endif

#define GET_SYM DYLIB_WIN_OTHER(GetProcAddress, dlsym)

TEST(library, path) {
    EXPECT_THROW(dylib::library(nullptr), std::invalid_argument);
    EXPECT_THROW(dylib::library(""), std::invalid_argument);

    EXPECT_THROW(dylib::library("/", dylib::decorations::os_default()), std::invalid_argument);
    EXPECT_THROW(dylib::library("/", dylib::decorations::none()), std::invalid_argument);

    EXPECT_THROW(dylib::library("/lib", dylib::decorations::os_default()), dylib::load_error);
    EXPECT_THROW(dylib::library("/lib", dylib::decorations::none()), dylib::load_error);

    EXPECT_THROW(dylib::library("///", dylib::decorations::os_default()), std::invalid_argument);
    EXPECT_THROW(dylib::library("///", dylib::decorations::none()), std::invalid_argument);

    EXPECT_NO_THROW(dylib::library(".///dynamic_lib", dylib::decorations::os_default()));
    EXPECT_NO_THROW(dylib::library("./././dynamic_lib", dylib::decorations::os_default()));

    EXPECT_THROW(dylib::library("/usr/bin/", dylib::decorations::os_default()),
                 std::invalid_argument);
    EXPECT_THROW(dylib::library("/usr/bin/", dylib::decorations::none()), std::invalid_argument);
}

TEST(library, multiple_handles) {
    dylib::library libA("./dynamic_lib", dylib::decorations::os_default());
    dylib::library libB("./dynamic_lib", dylib::decorations::os_default());
}

TEST(library, std_move) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    dylib::library other(std::move(lib));

    auto pi = other.get_variable<double>("pi_value_c");
    EXPECT_EQ(pi, 3.14159);

    lib = std::move(other);

    auto ptr = lib.get_variable<void *>("ptr_c");
    EXPECT_EQ(ptr, (void *)1);

    EXPECT_THROW(other.get_variable<double>("pi_value_c"), std::logic_error);
}

TEST(library, std_move_assign_empties_the_source) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    dylib::library other("./dynamic_lib", dylib::decorations::os_default());

    /*
     * Unlike the std_move test above, the destination already holds a library
     * here, which is what distinguishes a move from a swap.
     */
    lib = std::move(other);

    EXPECT_EQ(lib.get_variable<double>("pi_value_c"), 3.14159);

    EXPECT_THROW(other.get_variable<double>("pi_value_c"), std::logic_error);
    EXPECT_THROW(other.symbols(), std::logic_error);
    EXPECT_THROW(other.sections(), std::logic_error);
    EXPECT_EQ(other.native_handle(), nullptr);
}

TEST(library, std_move_self_assign) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    dylib::library *self = &lib;

    lib = std::move(*self);

    EXPECT_EQ(lib.get_variable<double>("pi_value_c"), 3.14159);
}

TEST(library, not_a_library) {
    const char *path = "./not_a_library.txt";

    {
        std::ofstream file(path);
        file << "this is not a dynamic library" << std::endl;
    }

    EXPECT_THROW(dylib::library(path, dylib::decorations::none()), dylib::load_error);

    std::remove(path);
}

TEST(library, manual_decorations) {
    dylib::decorations os_decorations = dylib::decorations::os_default();
    dylib::library lib(std::string("./") + os_decorations.prefix + std::string("dynamic_lib") +
                           os_decorations.suffix,
                       dylib::decorations::none());

    auto pi = lib.get_variable<double>("pi_value_c");
    EXPECT_EQ(pi, 3.14159);
}

TEST(library, handle_management) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    auto handle = lib.native_handle();
    auto sym = GET_SYM(handle, "adder");

    EXPECT_TRUE(!!handle);
    EXPECT_TRUE(!!sym);

#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
#ifdef __clang__
#if __has_warning("-Wcast-function-type-mismatch")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif
#endif
    auto res = ((double (*)(double, double))(sym))(10, 10);
#ifdef __clang__
#if __has_warning("-Wcast-function-type-mismatch")
#pragma clang diagnostic pop
#endif
#endif
#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    EXPECT_EQ(res, 20);
}

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
TEST(library, std_filesystem) {
    dylib::library(std::filesystem::path("./dynamic_lib"), dylib::decorations::os_default());

    for (const auto &file : std::filesystem::recursive_directory_iterator(".")) {
        if (file.path().extension() == dylib::decorations::os_default().suffix)
            dylib::library(file.path());
    }
}
#endif

TEST(symbols, bad_symbol) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    EXPECT_THROW(lib.get_function<void()>(nullptr), std::invalid_argument);
    EXPECT_THROW(lib.get_variable<void *>(nullptr), std::invalid_argument);

    EXPECT_THROW(lib.get_function<void()>(""), std::invalid_argument);
    EXPECT_THROW(lib.get_variable<void *>(""), std::invalid_argument);
    EXPECT_THROW(lib.get_symbol(std::string()), std::invalid_argument);

    EXPECT_THROW(lib.get_function<double()>("unknown"), dylib::symbol_not_found);
    EXPECT_THROW(lib.get_variable<double>("unknown"), dylib::symbol_not_found);
}

TEST(symbols, moved) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    dylib::library other(std::move(lib));

    EXPECT_THROW(lib.symbols(), std::logic_error);
    EXPECT_THROW(lib.get_symbol("adder"), std::logic_error);
}

TEST(symbols, metadata) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    std::vector<dylib::symbol_info> symbols = lib.symbols();
    const dylib::symbol_info *c_symbol = nullptr;
    const dylib::symbol_info *cpp_symbol = nullptr;

    for (const auto &symbol : symbols) {
        if (symbol.name == "adder")
            c_symbol = &symbol;
        else if (symbol.demangled_name == "tools::adder(double, double)")
            cpp_symbol = &symbol;
    }

    ASSERT_NE(c_symbol, nullptr);
    EXPECT_TRUE(c_symbol->loadable);
    // An unmangled name is reported as is.
    EXPECT_EQ(c_symbol->demangled_name, "adder");

    ASSERT_NE(cpp_symbol, nullptr);
    EXPECT_TRUE(cpp_symbol->loadable);
    EXPECT_EQ(cpp_symbol->type, dylib::symbol_type::CPP);
    // A mangled name has to differ from its demangled form.
    EXPECT_NE(cpp_symbol->name, cpp_symbol->demangled_name);
}

TEST(symbols, no_duplicates) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    std::vector<dylib::symbol_info> symbols = lib.symbols();
    std::vector<std::string> names;

    names.reserve(symbols.size());
    for (const auto &symbol : symbols) {
        EXPECT_FALSE(symbol.name.empty());
        names.push_back(symbol.name);
    }

    std::sort(names.begin(), names.end());
    EXPECT_EQ(std::adjacent_find(names.begin(), names.end()), names.end());
}

TEST(symbols, functions) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    auto adder = lib.get_function<double(double, double)>("adder");
    EXPECT_EQ(adder(5, 10), 15);

    auto hello_world = lib.get_function<const char *()>("hello_world");
    EXPECT_STREQ(hello_world(), "Hello World!");
}

TEST(symbols, variables) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    auto &pi = lib.get_variable<double>("pi_value_c");
    EXPECT_EQ(pi, 3.14159);
    pi = 123;
    auto &pi1 = lib.get_variable<double>("pi_value_c");
    EXPECT_EQ(pi1, 123);

    auto &ptr = lib.get_variable<void *>("ptr_c");
    EXPECT_EQ(ptr, (void *)1);
    ptr = &lib;
    auto &ptr1 = lib.get_variable<void *>("ptr_c");
    EXPECT_EQ(ptr1, &lib);
}

#define STD_STRING "std::basic_string<char, std::char_traits<char>, std::allocator<char>>"
#define STD_VECTOR_STRING "std::vector<" STD_STRING ", std::allocator<" STD_STRING ">>"

TEST(cpp_symbols, variables) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    auto meaning = lib.get_variable<double>("meaning_of_life");
    EXPECT_EQ(meaning, 42);

    auto secret = lib.get_variable<const char *>("secret");
    EXPECT_EQ(strcmp(secret, "12345"), 0);
}

TEST(cpp_symbols, variables_namespace) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    auto pi = lib.get_variable<double>("tools::pi_value");
    EXPECT_EQ(pi, 3.14159);
}

TEST(cpp_symbols, functions) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    auto list_new_find = lib.get_function<std::vector<std::string>()>("list_new_string");
    auto list_new_full = lib.get_function<std::vector<std::string>()>("list_new_string(void)");

    EXPECT_EQ(list_new_find, list_new_full);

    auto list_add_find =
        lib.get_function<void(std::vector<std::string> &, std::string)>("list_add_string");
    auto list_add_full = lib.get_function<void(std::vector<std::string> &, std::string)>(
        "list_add_string(" STD_VECTOR_STRING " &, " STD_STRING ")");

    EXPECT_EQ(list_add_find, list_add_full);

    std::vector<std::string> list = list_new_find();

    list_add_find(list, "Hello");
    list_add_find(list, "World");

    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list[0], "Hello");
    EXPECT_EQ(list[1], "World");
}

#define TEST_TEXT "bla,bla,bla..."

TEST(cpp_symbols, functions_overload_namespace) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    EXPECT_THROW(lib.get_function<void()>("tools::adder"), dylib::symbol_multiple_matches);

    auto v_adder = lib.get_function<double(void)>("tools::adder(void)");
    EXPECT_EQ(v_adder(), 0);

    auto d_adder = lib.get_function<double(double, double)>("tools::adder(double, double)");
    EXPECT_EQ(d_adder(11, 11), 22);

    auto s_adder = lib.get_function<std::string(std::string, std::string)>(
        "tools::adder(" STD_STRING ", " STD_STRING ")");
    EXPECT_EQ(s_adder("Hello", "World"), "HelloWorld");

    auto ptr_format =
        lib.get_function<std::string(const char *)>("tools::string::format(char const *)");
    EXPECT_EQ(ptr_format(TEST_TEXT), "ptr: " TEST_TEXT);

    auto cpy_format =
        lib.get_function<std::string(std::string)>("tools::string::format(" STD_STRING ")");
    EXPECT_EQ(cpy_format(TEST_TEXT), "cpy: " TEST_TEXT);

    auto ref_format = lib.get_function<std::string(const std::string &)>(
        "tools::string::format(" STD_STRING " const &)");
    EXPECT_EQ(ref_format(TEST_TEXT), "ref: " TEST_TEXT);

    auto mov_format =
        lib.get_function<std::string(std::string &&)>("tools::string::format(" STD_STRING " &&)");
    auto mov_text = std::string(TEST_TEXT);
    EXPECT_EQ(mov_format(std::move(mov_text)), "mov: " TEST_TEXT);

    auto int_ref_println = lib.get_function<std::string(const unsigned int &)>(
        "tools::string::format(unsigned int const &)");
    EXPECT_EQ(int_ref_println(123), "ref: 123");
}

TEST(cpp_symbols, struct) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    auto fmt_position = lib.get_function<std::string(Position &)>("fmt_position(Position &)");
    Position pos = {1, 2};
    EXPECT_EQ(fmt_position(pos), "1, 2");
}

class Cat : public IAnimal {
public:
    std::string sound() override {
        return "Meow";
    }
};

TEST(cpp_symbols, interface) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    Cat cat;

    auto get_animal_sound = lib.get_function<std::string(IAnimal *)>("get_animal_sound(IAnimal *)");

    EXPECT_EQ(get_animal_sound(&cat), "Meow");
}

TEST(cpp_symbols, callback) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());

    auto adder = lib.get_function<double(double, double)>("tools::adder(double, double)");
    auto callback = lib.get_function<double(double, double, double (*)(double, double))>(
        "callback(double, double, double (*)(double, double))");

    EXPECT_EQ(callback(10, 10, adder), 20);
}

TEST(cpp_symbols, loadable) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    std::vector<dylib::symbol_info> symbols;

    symbols = lib.symbols();

    for (auto &symbol : symbols) {
        if (symbol.loadable) {
            EXPECT_TRUE(!!GET_SYM(lib.native_handle(), symbol.name.c_str()));
        }
    }
}

TEST(cpp_symbols, demangle) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    std::vector<dylib::symbol_info> symbols;

    symbols = lib.symbols();

    for (auto &symbol : symbols) {
        try {
            if (symbol.loadable) {
                EXPECT_TRUE(!!lib.get_symbol(symbol.name));
            }
        } catch (dylib::symbol_multiple_matches &) {
        }
    }
}

TEST(sections, lookup) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    std::vector<std::string> sections;

    sections = lib.sections();

    EXPECT_NE(std::find(sections.begin(), sections.end(), "testsec"), sections.end());

    std::vector<std::string> sorted_sections = sections;
    std::sort(sorted_sections.begin(), sorted_sections.end());
    EXPECT_EQ(std::adjacent_find(sorted_sections.begin(), sorted_sections.end()),
              sorted_sections.end());
}

TEST(sections, moved) {
    dylib::library lib("./dynamic_lib", dylib::decorations::os_default());
    dylib::library other(std::move(lib));

    EXPECT_THROW(lib.sections(), std::logic_error);
}

int main(int ac, char **av) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
