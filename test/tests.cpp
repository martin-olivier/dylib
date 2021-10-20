#include <gtest/gtest.h>
#include <utility>
#include "DyLib.hpp"

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

TEST(exemple, exemple_test)
{
    OSRedirector oss(std::cout);

    try {
        DyLib lib(std::string("./dynlib") + std::string(DyLib::extension));

        auto adder = lib.getFunction<double(double, double)>("adder");
        EXPECT_EQ(adder(5, 10), 15);

        auto printer = lib.getFunction<void()>("printHello");
        printer();
        EXPECT_EQ(oss.getContent(), "Hello!\n");

        double pi_value = lib.getVariable<double>("pi_value");
        EXPECT_EQ(pi_value, 3.14159);

        void *ptr = lib.getVariable<void *>("ptr");
        EXPECT_EQ(ptr, (void *)1);
    }
    catch (const DyLib::exception &) {
        EXPECT_EQ(true, false);
    }
}

TEST(ctor, bad_library)
{
    try {
        DyLib lib("./null.so");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::exception &e) {
        EXPECT_EQ(true, true);
    }
}

TEST(dtor, mutiple_open_close)
{
    try {
        DyLib lib;
        lib.close();
        lib.close();
        lib.open(std::string("./dynlib") + std::string(DyLib::extension));
        lib.open(std::string("./dynlib") + std::string(DyLib::extension));
        EXPECT_EQ(lib.getFunction<double(double, double)>("adder")(1, 1), 2);
        lib.close();
        lib.close();
        lib.close();
        auto fn = lib.getFunction<double(double, double)>("adder");
    }
    catch (const DyLib::exception &) {
        EXPECT_EQ(true, true);
    }
}

TEST(getFunction, bad_handler)
{
    try {
        DyLib lib(std::string("./dynlib") + std::string(DyLib::extension));
        lib.close();
        auto adder = lib.getFunction<double(double, double)>("adder");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(getFunction, bad_symbol)
{
    try {
        DyLib lib(std::string("./dynlib") + std::string(DyLib::extension));
        auto adder = lib.getFunction<double(double, double)>("unknow");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(getVariable, bad_handler)
{
    try {
        DyLib lib(std::string("./dynlib") + std::string(DyLib::extension));
        lib.close();
        lib.getVariable<double>("pi_value");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(getVariable, bad_symbol)
{
    try {
        DyLib lib(std::string("./dynlib") + std::string(DyLib::extension));
        lib.getVariable<double>("unknow");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(getVariable, alter_variables)
{
    try {
        DyLib lib(std::string("./dynlib"), DyLib::extension);
        DyLib other(std::move(lib));
        auto &pi = other.getVariable<double>("pi_value");
        EXPECT_EQ(pi, 3.14159);
        pi = 123;
        auto &pi1 = other.getVariable<double>("pi_value");
        EXPECT_EQ(pi1, 123);

        auto &ptr = other.getVariable<void *>("ptr");
        EXPECT_EQ(ptr, (void *)1);
        ptr = &lib;
        auto &ptr1 = other.getVariable<void *>("ptr");
        EXPECT_EQ(ptr1, &lib);
    }
    catch (const DyLib::handle_error &) {
        EXPECT_EQ(true, false);
    }
}

TEST(bad_arguments, null_pointer)
{
    try {
        DyLib lib(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::handle_error &) {
        EXPECT_EQ(true, true);
    }
    try {
        DyLib lib(std::string("./dynlib") + std::string(DyLib::extension));
        auto nothing = lib.getFunction<void()>(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
    try {
        DyLib lib(std::string("./dynlib") + std::string(DyLib::extension));
        lib.getVariable<void *>(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(bad_arguments, handle_and_ext)
{
    try {
        DyLib lib("./badlib", DyLib::extension);
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::handle_error &) {
        EXPECT_EQ(true, true);
    }
    try {
        DyLib lib(std::string("./dynlib"), nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(os_detector, basic_test)
{
    try {
        DyLib lib(std::string("./dynlib"), DyLib::extension);
        auto pi = lib.getVariable<double>("pi_value");
        EXPECT_EQ(pi, 3.14159);
    }
    catch (const DyLib::exception &) {
        EXPECT_EQ(true, false);
    }
}

TEST(std_move, basic_test)
{
    try {
        DyLib lib(std::string("./dynlib"), DyLib::extension);
        DyLib other(std::move(lib));
        auto pi = other.getVariable<double>("pi_value");
        EXPECT_EQ(pi, 3.14159);
        lib = std::move(other);
        auto ptr = lib.getVariable<void *>("ptr");
        EXPECT_EQ(ptr, (void *)1);
        other.getVariable<double>("pi_value");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

int main(int ac, char **av)
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}