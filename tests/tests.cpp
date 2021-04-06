#include <gtest/gtest.h>
#include "../DyLib.hpp"

class OSRedirector {
    private:
        std::ostringstream _oss;
        std::streambuf *_backup;
        std::ostream &_c;

    public:
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
        DyLib lib("./myDynLib.so");

        auto adder = lib.getFunction<double(double, double)>("adder");
        EXPECT_EQ(adder(5, 10), 15);

        auto printer = lib.getFunction<void()>("printHello");
        printer();
        EXPECT_EQ(oss.getContent(), "Hello!\n");

        double pi_value = lib.getVariable<double>("pi_value");
        EXPECT_EQ(pi_value, 3.14159);

        void *ptr = lib.getVariable<void *>("ptr");
        EXPECT_EQ(ptr, nullptr);
    }
    catch (const DyLib::exception &e) {
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
        EXPECT_EQ(std::string(e.what()), "Error while loading the dynamic library : ./null.so");
    }
}

TEST(dtor, mutiple_open_close)
{
    try {
        DyLib lib;
        lib.close();
        lib.close();
        lib.open("./myDynLib.so");
        lib.open("./myDynLib.so");
        EXPECT_EQ(lib.getFunction<double(double, double)>("adder")(1, 1), 2);
        lib.close();
        lib.close();
        lib.close();
        auto fn = lib.getFunction<double(double, double)>("adder");
    }
    catch (const DyLib::exception &e) {
        EXPECT_EQ(true, true);
    }
}

TEST(getFunction, bad_handler)
{
    try {
        DyLib lib("./myDynLib.so");
        lib.close();
        auto adder = lib.getFunction<double(double, double)>("adder");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::handle_error &e) {
        EXPECT_EQ(true, true);
    }
}

TEST(getFunction, bad_symbol)
{
    try {
        DyLib lib("./myDynLib.so");
        auto adder = lib.getFunction<double(double, double)>("unknow");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::symbol_error &e) {
        EXPECT_EQ(true, true);
    }
}

TEST(getVariable, bad_handler)
{
    try {
        DyLib lib("./myDynLib.so");
        lib.close();
        auto pi = lib.getVariable<double>("pi_value");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::handle_error &e) {
        EXPECT_EQ(true, true);
    }
}

TEST(getVariable, bad_symbol)
{
    try {
        DyLib lib("./myDynLib.so");
        auto pi = lib.getVariable<double>("unknow");
        EXPECT_EQ(true, false);
    }
    catch (const DyLib::symbol_error &e) {
        EXPECT_EQ(true, true);
    }
}

int main(int ac, char **av)
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}