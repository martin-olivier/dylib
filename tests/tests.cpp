#include <gtest/gtest.h>
#include <utility>
#include "dylib.hpp"

class OSRedirector {
    private:
        std::ostringstream _oss{};
        std::streambuf *_backup{};
        std::ostream &_c;

    public:
        OSRedirector(OSRedirector &) = delete;
        OSRedirector &operator=(OSRedirector &) = delete;

        OSRedirector(std::ostream &c) : _c(c) {
            _backup = _c.rdbuf();
            _c.rdbuf(_oss.rdbuf());
        }

        ~OSRedirector() {
            _c.rdbuf(_backup);
        }

        const std::string getContent() {
            _oss << std::flush;
            return _oss.str();
        }
};

TEST(exemple, exemple_test) {
    OSRedirector oss(std::cout);

    try {
        dylib lib(dylib::working_directory, "dynamic_lib");

        auto adder = lib.get_function<double(double, double)>("adder");
        EXPECT_EQ(adder(5, 10), 15);

        auto printer = lib.get_function<void()>("print_hello");
        printer();
        EXPECT_EQ(oss.getContent(), "Hello\n");

        double pi_value = lib.get_variable<double>("pi_value");
        EXPECT_EQ(pi_value, 3.14159);

        void *ptr = lib.get_variable<void *>("ptr");
        EXPECT_EQ(ptr, (void *)1);
    }
    catch (const dylib::exception &) {
        EXPECT_EQ(true, false);
    }
}

TEST(ctor, bad_library) {
    try {
        dylib lib(dylib::working_directory, "null");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::exception &e) {
        EXPECT_EQ(true, true);
    }
}

TEST(multiple_handles, basic_test) {
    dylib libA(dylib::working_directory, "dynamic_lib");
    dylib libB(dylib::working_directory, "dynamic_lib");
}

TEST(get_function, bad_symbol) {
    try {
        dylib lib(dylib::working_directory, "dynamic_lib");
        lib.get_function<double(double, double)>("unknown");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_variable, bad_symbol) {
    try {
        dylib lib(dylib::working_directory, "dynamic_lib");
        lib.get_variable<double>("unknown");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_variable, alter_variables) {
    try {
        dylib lib(dylib::working_directory, "dynamic_lib");
        dylib other(std::move(lib));
        auto &pi = other.get_variable<double>("pi_value");
        EXPECT_EQ(pi, 3.14159);
        pi = 123;
        auto &pi1 = other.get_variable<double>("pi_value");
        EXPECT_EQ(pi1, 123);

        auto &ptr = other.get_variable<void *>("ptr");
        EXPECT_EQ(ptr, (void *)1);
        ptr = &lib;
        auto &ptr1 = other.get_variable<void *>("ptr");
        EXPECT_EQ(ptr1, &lib);
    }
    catch (const dylib::handle_error &) {
        EXPECT_EQ(true, false);
    }
}

TEST(bad_arguments, null_pointer) {
    try {
        dylib lib(dylib::working_directory, "dynamic_lib");
        lib.get_function<void()>(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
    try {
        dylib lib(dylib::working_directory, "dynamic_lib");
        lib.get_variable<void *>(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(bad_arguments, handle_and_ext) {
    try {
        dylib lib(dylib::working_directory, "badlib");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(os_detector, basic_test) {
    try {
        dylib lib(dylib::working_directory, "dynamic_lib");
        auto pi = lib.get_variable<double>("pi_value");
        EXPECT_EQ(pi, 3.14159);
    }
    catch (const dylib::exception &) {
        EXPECT_EQ(true, false);
    }
}

TEST(std_move, basic_test) {
    try {
        dylib lib(dylib::working_directory, "dynamic_lib");
        dylib other(std::move(lib));
        auto pi = other.get_variable<double>("pi_value");
        EXPECT_EQ(pi, 3.14159);
        lib = std::move(other);
        auto ptr = lib.get_variable<void *>("ptr");
        EXPECT_EQ(ptr, (void *)1);
        other.get_variable<double>("pi_value");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(has_symbol, basic_test) {
    dylib lib(dylib::working_directory, "dynamic_lib");
    EXPECT_TRUE(lib.has_symbol("pi_value"));
    EXPECT_FALSE(lib.has_symbol("bad_symbol"));
}

TEST(handle_management, basic_test) {
    dylib lib(dylib::working_directory, "dynamic_lib");
    EXPECT_FALSE(lib.native_handle() == nullptr);
    auto handle = lib.native_handle();
#if defined(_WIN32) || defined(_WIN64)
    auto sym = GetProcAddress(handle, "adder");
#else
    auto sym = dlsym(handle, "adder");
#endif
    EXPECT_FALSE(sym == nullptr);
    auto res = ((double (*)(double, double))(sym))(10, 10);
    EXPECT_EQ(res, 20);
}

int main(int ac, char **av) {
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}