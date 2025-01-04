#include <gtest/gtest.h>
#include <utility>

#include "dylib.hpp"

#if !(defined(_WIN32) || defined(_WIN64))
#include <dlfcn.h>
#endif

TEST(example, example_test) {
    testing::internal::CaptureStdout();
    dylib::library lib("./", "dynamic_lib");

    auto adder = lib.get_function<double(double, double)>("adder_c");
    EXPECT_EQ(adder(5, 10), 15);

    auto printer = lib.get_function<void()>("print_hello_c");
    printer();
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "Hello\n");

    double pi_value = lib.get_variable<double>("pi_value_c");
    EXPECT_EQ(pi_value, 3.14159);

    void *ptr = lib.get_variable<void *>("ptr_c");
    EXPECT_EQ(ptr, (void *)1);
}

TEST(ctor, bad_library) {
    try {
        dylib::library lib("./", "no_such_library");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::load_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(multiple_handles, basic_test) {
    dylib::library libA("./", "dynamic_lib");
    dylib::library libB("./", "dynamic_lib");
}

TEST(get_function, bad_symbol) {
    try {
        dylib::library lib("./", "dynamic_lib");
        lib.get_function<double(double, double)>("unknown");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_variable, bad_symbol) {
    try {
        dylib::library lib("./", "dynamic_lib");
        lib.get_variable<double>("unknown");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_variable, alter_variables) {
    dylib::library lib("./", "dynamic_lib");

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

TEST(invalid_argument, null_pointer) {
    try {
        dylib::library(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const std::invalid_argument &) {
        EXPECT_EQ(true, true);
    }
    try {
        dylib::library lib("./", "dynamic_lib");
        lib.get_function<void()>(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const std::invalid_argument &) {
        EXPECT_EQ(true, true);
    }
    try {
        dylib::library lib("./", "dynamic_lib");
        lib.get_variable<void *>(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const std::invalid_argument &) {
        EXPECT_EQ(true, true);
    }
}

TEST(manual_decorations, basic_test) {
    dylib::library lib(".", dylib::filename_components::prefix + std::string("dynamic_lib") + dylib::filename_components::suffix, false);
    auto pi = lib.get_variable<double>("pi_value_c");
    EXPECT_EQ(pi, 3.14159);
}

TEST(std_move, basic_test) {
    try {
        dylib::library lib("./", "dynamic_lib");
        dylib::library other(std::move(lib));
        auto pi = other.get_variable<double>("pi_value_c");
        EXPECT_EQ(pi, 3.14159);
        lib = std::move(other);
        auto ptr = lib.get_variable<void *>("ptr_c");
        EXPECT_EQ(ptr, (void *)1);
        other.get_variable<double>("pi_value_c");
        EXPECT_EQ(true, false);
    }
    catch (const std::logic_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(has_symbol, basic_test) {
    dylib::library dummy("./", "dynamic_lib");
    dylib::library lib(std::move(dummy));

    EXPECT_TRUE(lib.has_symbol("pi_value_c"));
    EXPECT_FALSE(lib.has_symbol("bad_symbol"));
    EXPECT_FALSE(lib.has_symbol(nullptr));
    EXPECT_FALSE(dummy.has_symbol("pi_value_c"));
}

TEST(handle_management, basic_test) {
    dylib::library lib("./", "dynamic_lib");
    EXPECT_FALSE(lib.native_handle() == nullptr);
    auto handle = lib.native_handle();
#if (defined(_WIN32) || defined(_WIN64))
    auto sym = GetProcAddress(handle, "adder_c");
#else
    auto sym = dlsym(handle, "adder_c");
#endif
    EXPECT_FALSE(sym == nullptr);

#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
#if defined __clang__
#if __has_warning("-Wcast-function-type-mismatch")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif
#endif
    auto res = ((double (*)(double, double))(sym))(10, 10);
#if defined __clang__
#if __has_warning("-Wcast-function-type-mismatch")
#pragma clang diagnostic pop
#endif
#endif
#if defined(__GNUC__) && __GNUC__ >= 8 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    EXPECT_EQ(res, 20);
}

#if (defined(_WIN32) || defined(_WIN64))
TEST(system_lib, basic_test) {
    dylib::library lib("kernel32");
    lib.get_function<DWORD()>("GetCurrentThreadId");
}
#endif

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
TEST(filesystem, basic_test) {
    bool has_sym = dylib::library(std::filesystem::path("."), "dynamic_lib").has_symbol("pi_value_c");
    EXPECT_TRUE(has_sym);

    bool found = false;
    for (const auto &file : std::filesystem::recursive_directory_iterator(".")) {
        if (file.path().extension() == dylib::filename_components::suffix) {
            try {
                dylib::library lib(file.path());
                if (lib.has_symbol("pi_value_c"))
                    found = true;
            } catch (const std::exception &) {}
        }
    }
    EXPECT_TRUE(found);
}
#endif

TEST(cpp_symbols, basic_test) {
    dylib::library lib("./", "dynamic_lib");

    auto mean = lib.get_variable<double>("meaning_of_life");
    EXPECT_EQ(mean, 42);

    auto secret = lib.get_variable<const char *>("secret");
    EXPECT_EQ(strcmp(secret, "12345"), 0);

    EXPECT_THROW(lib.get_function<void()>("tools::adder"), dylib::symbol_error);

    auto n_adder = lib.get_function<double(void)>("tools::adder(void)");
    EXPECT_EQ(n_adder(), 0);

    auto d_adder = lib.get_function<double(double, double)>("tools::adder(double, double)");
    EXPECT_EQ(d_adder(11, 11), 22);

    auto str_rep = "std::basic_string<char, std::char_traits<char>, std::allocator<char>>";

    auto s_adder = lib.get_function<std::string(std::string, std::string)>
        (std::string("tools::adder(") + str_rep + ", " + str_rep + ")");
    EXPECT_EQ(s_adder("Hello", "World"), "HelloWorld");

    auto text = std::string("bla,bla,bla...");

    auto ptr_format = lib.get_function<std::string(const char *)>("tools::string::format(char const *)");
    EXPECT_EQ(ptr_format(text.c_str()), std::string("ptr: bla,bla,bla..."));

    auto ref_format = lib.get_function<std::string(const std::string &)>("tools::string::format(std::basic_string<char, std::char_traits<char>, std::allocator<char>> const &)");
    EXPECT_EQ(ref_format(text), std::string("ref: bla,bla,bla..."));

    auto mov_format = lib.get_function<std::string(std::string &&)>("tools::string::format(std::basic_string<char, std::char_traits<char>, std::allocator<char>> &&)");
    EXPECT_EQ(mov_format(std::move(text)), std::string("mov: bla,bla,bla..."));

    auto int_ref_println = lib.get_function<std::string(const unsigned int &)>("tools::string::format(unsigned int const &)");
    EXPECT_EQ(int_ref_println(123), std::string("ref: 123"));
}

int main(int ac, char **av) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
