#include <gtest/gtest.h>
#include <utility>
#include "dylib.hpp"

TEST(example, example_test) {
    testing::internal::CaptureStdout();
    dylib lib("./", "dynamic_lib");

    auto adder = lib.get_function<double(double, double)>("adder");
    EXPECT_EQ(adder(5, 10), 15);

    auto printer = lib.get_function<void()>("print_hello");
    printer();
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "Hello\n");

    double pi_value = lib.get_variable<double>("pi_value");
    EXPECT_EQ(pi_value, 3.14159);

    void *ptr = lib.get_variable<void *>("ptr");
    EXPECT_EQ(ptr, (void *)1);
}

TEST(ctor, bad_library) {
    EXPECT_THROW({
        dylib lib("./", "no_such_library");
    }, dylib::load_error);
}

TEST(multiple_handles, basic_test) {
    dylib libA("./", "dynamic_lib");
    dylib libB("./", "dynamic_lib");
}

TEST(get_function, bad_symbol) {
    EXPECT_THROW({
        dylib lib("./", "dynamic_lib");
        lib.get_function<double(double, double)>("unknown");
    }, dylib::symbol_error);
}

TEST(get_variable, bad_symbol) {
    EXPECT_THROW({
        dylib lib("./", "dynamic_lib");
        lib.get_variable<double>("unknown");
    }, dylib::symbol_error);
}

TEST(get_variable, alter_variables) {
    dylib lib("./", "dynamic_lib");

    auto &pi = lib.get_variable<double>("pi_value");
    EXPECT_EQ(pi, 3.14159);
    pi = 123;
    auto &pi1 = lib.get_variable<double>("pi_value");
    EXPECT_EQ(pi1, 123);

    auto &ptr = lib.get_variable<void *>("ptr");
    EXPECT_EQ(ptr, (void *)1);
    ptr = &lib;
    auto &ptr1 = lib.get_variable<void *>("ptr");
    EXPECT_EQ(ptr1, &lib);
}

TEST(invalid_argument, null_pointer) {
    EXPECT_THROW({
        dylib(nullptr);
    }, std::invalid_argument);

    EXPECT_THROW({
        dylib(nullptr, "dynamic_lib");
    }, std::invalid_argument);

    EXPECT_THROW({
        dylib lib("./", "dynamic_lib");
        lib.get_function<void()>(nullptr);
    }, std::invalid_argument);

    EXPECT_THROW({
        dylib lib("./", "dynamic_lib");
        lib.get_variable<void *>(nullptr);
    }, std::invalid_argument);
}

TEST(manual_decorations, basic_test) {
    dylib lib(".", dylib::filename_components::prefix + std::string("dynamic_lib") + dylib::filename_components::suffix, dylib::no_filename_decorations);
    auto pi = lib.get_variable<double>("pi_value");
    EXPECT_EQ(pi, 3.14159);
}

TEST(std_move, basic_test) {
    EXPECT_THROW({
        dylib lib("./", "dynamic_lib");
        dylib other(std::move(lib));
        auto pi = other.get_variable<double>("pi_value");
        EXPECT_EQ(pi, 3.14159);
        lib = std::move(other);
        auto ptr = lib.get_variable<void *>("ptr");
        EXPECT_EQ(ptr, (void *)1);
        other.get_variable<double>("pi_value");
    }, std::logic_error);
}

TEST(has_symbol, basic_test) {
    dylib dummy("./", "dynamic_lib");
    dylib lib(std::move(dummy));

    EXPECT_TRUE(lib.has_symbol("pi_value"));
    EXPECT_FALSE(lib.has_symbol("bad_symbol"));
    EXPECT_FALSE(lib.has_symbol(nullptr));
    EXPECT_FALSE(dummy.has_symbol("pi_value"));
}

TEST(handle_management, basic_test) {
    dylib lib("./", "dynamic_lib");
    EXPECT_FALSE(lib.native_handle() == nullptr);
    auto handle = lib.native_handle();
#if (defined(_WIN32) || defined(_WIN64))
    auto sym = GetProcAddress(handle, "adder");
#else
    auto sym = dlsym(handle, "adder");
#endif
    EXPECT_FALSE(sym == nullptr);
#if (defined(__GNUC__) && __GNUC__ >= 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    auto res = ((double (*)(double, double))(sym))(10, 10);
#if (defined(__GNUC__) && __GNUC__ >= 8)
#pragma GCC diagnostic pop
#endif
    EXPECT_EQ(res, 20);
}

TEST(system_lib, basic_test) {
#if (defined(_WIN32) || defined(_WIN64))
    dylib lib("kernel32");
    lib.get_function<DWORD()>("GetCurrentThreadId");
#elif defined(__APPLE__)
    dylib lib("ssh2");
    lib.get_function<const char *(int)>("libssh2_version");
#endif
}

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
TEST(filesystem, basic_test) {
    bool has_sym = dylib(std::filesystem::path("."), "dynamic_lib").has_symbol("pi_value");
    EXPECT_TRUE(has_sym);

    bool found = false;
    for (const auto &file : std::filesystem::recursive_directory_iterator(".")) {
        if (file.path().extension() == dylib::filename_components::suffix) {
            try {
                dylib lib(file.path());
                if (lib.has_symbol("pi_value"))
                    found = true;
            } catch (const std::exception &) {}
        }
    }
    EXPECT_TRUE(found);
}
#endif

int main(int ac, char **av) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}
