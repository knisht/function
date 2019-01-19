#include "function.hpp"
#include "gtest/gtest.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <utility>
#include <vector>

template <typename F, typename... Args>
using func = exam::function<F(Args...)>;

void fv(){};
int fi() { return 42; }

TEST(correctness, stable)
{
    func<void> my_f(fv);
    my_f();
}

TEST(correctness, int)
{
    func<int> my_f(fi);
    EXPECT_EQ(my_f(), 42);
}

TEST(correctness, lambda)
{
    func<int> my_f([]() -> int { return 1337; });
    EXPECT_EQ(my_f(), 1337);
}

TEST(correctness, capture_lambda)
{
    int x = 256;
    func<int> my_f([x]() -> int { return x; });
    EXPECT_EQ(my_f(), 256);
}

int fii(int x) { return x * x; }

TEST(correctness, args)
{
    func<int, int> my_f(fii);
    EXPECT_EQ(my_f(100), 10000);
}

TEST(correctness, multiple_args)
{
    std::string x = "hello";
    func<int, int, long> my_f([x](int a, long b) -> int { return a + b; });
    EXPECT_EQ(my_f.operator bool(), true);
    EXPECT_EQ(my_f(100, 200), 300);
}

std::string fs() { return "hello"; }

TEST(correctness, non_scalar)
{
    func<std::string> my_f(fs);
    EXPECT_EQ(my_f(), "hello");
}

TEST(correctness, non_scalar2)
{
    func<std::string, std::string> my_f(
        [](std::string s) -> std::string { return s + "!"; });

    EXPECT_EQ(my_f.operator bool(), true);
    EXPECT_EQ(my_f("hello"), "hello!");
}

struct Huge {
    int data[100];
    Huge() { memset(data, 0, 100 * sizeof(int)); }
    bool operator==(Huge const &other) const
    {
        return memcmp(data, other.data, 100 * sizeof(int)) == 0;
    }
};

TEST(correctness, big_struct)
{
    func<Huge, Huge> my_f([](Huge h) {
        h.data[0] = 1;
        return h;
    });
    Huge h;
    h.data[0] = 1;
    EXPECT_EQ(my_f.operator bool(), true);
    EXPECT_EQ(my_f({}), h);
}

struct Funct {
    Funct() : id(0)
    {
#ifdef VERBOSE
        std::cout << "Funct built: " << id << std::endl;
#endif
    }
    Funct(Funct const &other) : id(other.id + 1)
    {
#ifdef VERBOSE
        std::cout << "Funct copied: " << id << std::endl;
#endif
    }

    int operator()(int x) const { return x; }

    size_t id;

    ~Funct()
    {
#ifdef VERBOSE
        std::cout << "Funct destroyed: " << id << std::endl;
#endif
    }
};

TEST(correctness, functor)
{
    func<int, int> my_f(Funct{});
    EXPECT_EQ(my_f(2), 2);
}

TEST(correctness, default_ctor)
{
    func<void> my_f{};
    EXPECT_EQ(my_f.operator bool(), false);
}

TEST(correctness, nullptr_ctor)
{
    func<void> my_f{nullptr};
    EXPECT_EQ(my_f.operator bool(), false);
}

TEST(correctness, copy_ctor)
{
    func<int, int> my_f{Funct{}};
    func<int, int> my_f2(my_f);
    EXPECT_EQ(my_f2(42), 42);
}

struct Non_copyable_Funct {
    Non_copyable_Funct() : id(0) {}
    Non_copyable_Funct(Non_copyable_Funct const &other) { id = 1000; }

    Non_copyable_Funct(Non_copyable_Funct &&other) noexcept : id{other.id}
    {
        ++id;
    }

    int operator()(int x) const { return x; }
    size_t id;
    ~Non_copyable_Funct() {}
};
TEST(correctness, move_ctor)
{
    Non_copyable_Funct f = Non_copyable_Funct{};
    func<int, int> my_f{std::move(f)};
    func<int, int> my_f2(std::move(my_f));
    EXPECT_EQ(my_f.operator bool(), false);
    EXPECT_EQ(my_f2.operator bool(), true);
    EXPECT_EQ(my_f2(84), 84);
    EXPECT_NE(f.id, 1000);
}

int fii2(int x) { return x * x * x; }

TEST(correctness, copy_assign)
{
    func<int, int> my_f{fii2};
    func<int, int> my_f2{fii};
    my_f2 = my_f;
    EXPECT_EQ(my_f.operator bool(), true);
    EXPECT_EQ(my_f2.operator bool(), true);
    EXPECT_EQ(my_f2(10), 1000);
    EXPECT_EQ(my_f(10), 1000);
}

TEST(correctness, move_assign)
{
    func<int, int> my_f{fii2};
    func<int, int> my_f2{fii};
    my_f2 = std::move(my_f);
    EXPECT_EQ(my_f.operator bool(), false);
    EXPECT_EQ(my_f2.operator bool(), true);
    EXPECT_EQ(my_f2(10), 1000);
}

TEST(correctness, huge_swawps)
{
    Huge h;
    auto t = [h](int x) { return x + 1; };
    func<int, int> my_f_big(t);
    func<int, int> my_f_small(fii);

    EXPECT_EQ(my_f_big.operator bool(), true);
    EXPECT_EQ(my_f_small.operator bool(), true);
    EXPECT_EQ(my_f_big(2), 3);
    EXPECT_EQ(my_f_small(2), 4);
    swap(my_f_big, my_f_small);

    EXPECT_EQ(my_f_big.operator bool(), true);
    EXPECT_EQ(my_f_small.operator bool(), true);

    EXPECT_EQ(my_f_big(2), 4);
    EXPECT_EQ(my_f_small(2), 3);
}
