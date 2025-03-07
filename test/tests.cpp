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

TEST(exemple, exemple_test)
{
    OSRedirector oss(std::cout);

    try {
        dylib lib(std::string("./dynlib") + std::string(dylib::extension));

        auto adder = lib.get_function<double(double, double)>("adder");
        EXPECT_EQ(adder(5, 10), 15);

        auto printer = lib.get_function<void()>("print_hello");
        printer();
        EXPECT_EQ(oss.getContent(), "Hello!\n");

        double pi_value = lib.get_variable<double>("pi_value");
        EXPECT_EQ(pi_value, 3.14159);

        void *ptr = lib.get_variable<void *>("ptr");
        EXPECT_EQ(ptr, (void *)1);
    }
    catch (const dylib::exception &) {
        EXPECT_EQ(true, false);
    }
}

TEST(ctor, bad_library)
{
    try {
        dylib lib("./null.so");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::exception &e) {
        e.what();
        EXPECT_EQ(true, true);
    }
}

TEST(dtor, mutiple_open_close)
{
    try {
        dylib lib;
        lib.close();
        lib.close();
        lib.open(std::string("./dynlib") + std::string(dylib::extension));
        lib.open(std::string("./dynlib") + std::string(dylib::extension));
        EXPECT_EQ(lib.get_function<double(double, double)>("adder")(1, 1), 2);
        lib.close();
        lib.close();
        lib.close();
        auto fn = lib.get_function<double(double, double)>("adder");
    }
    catch (const dylib::exception &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_function, bad_handler)
{
    try {
        dylib lib(std::string("./dynlib") + std::string(dylib::extension));
        lib.close();
        auto adder = lib.get_function<double(double, double)>("adder");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_function, bad_symbol)
{
    try {
        dylib lib(std::string("./dynlib") + std::string(dylib::extension));
        auto adder = lib.get_function<double(double, double)>("unknown");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_variable, bad_handler)
{
    try {
        dylib lib(std::string("./dynlib") + std::string(dylib::extension));
        lib.close();
        lib.get_variable<double>("pi_value");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_variable, bad_symbol)
{
    try {
        dylib lib(std::string("./dynlib") + std::string(dylib::extension));
        lib.get_variable<double>("unknown");
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(get_variable, alter_variables)
{
    try {
        dylib lib(std::string("./dynlib"), dylib::extension);
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

TEST(bad_arguments, null_pointer)
{
    try {
        dylib lib(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const dylib::handle_error &) {
        EXPECT_EQ(true, true);
    }
    try {
        dylib lib(std::string("./dynlib") + std::string(dylib::extension));
        auto nothing = lib.get_function<void()>(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
    try {
        dylib lib(std::string("./dynlib") + std::string(dylib::extension));
        lib.get_variable<void *>(nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const dylib::symbol_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(bad_arguments, handle_and_ext)
{
    try {
        dylib lib("./badlib", dylib::extension);
        EXPECT_EQ(true, false);
    }
    catch (const dylib::handle_error &) {
        EXPECT_EQ(true, true);
    }
    try {
        dylib lib(std::string("./dynlib"), nullptr);
        EXPECT_EQ(true, false);
    }
    catch (const dylib::handle_error &) {
        EXPECT_EQ(true, true);
    }
}

TEST(os_detector, basic_test)
{
    try {
        dylib lib(std::string("./dynlib"), dylib::extension);
        auto pi = lib.get_variable<double>("pi_value");
        EXPECT_EQ(pi, 3.14159);
    }
    catch (const dylib::exception &) {
        EXPECT_EQ(true, false);
    }
}

TEST(std_move, basic_test)
{
    try {
        dylib lib(std::string("./dynlib"), dylib::extension);
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

int main(int ac, char **av)
{
    testing::InitGoogleTest(&ac, av);
    return RUN_ALL_TESTS();
}