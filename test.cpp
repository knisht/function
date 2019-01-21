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

template <typename F>
using tested_function = exam::function<F>;

void fv(){};
int fi() { return 42; }

TEST(correctness, stable)
{
    tested_function<void()> my_f(fv);
    my_f();
}

TEST(correctness, int)
{
    tested_function<int()> my_f(fi);
    EXPECT_EQ(my_f(), 42);
}

TEST(correctness, lambda)
{
    tested_function<int()> my_f([]() -> int { return 1337; });
    EXPECT_EQ(my_f(), 1337);
}

TEST(correctness, capture_lambda)
{
    int x = 256;
    tested_function<int()> my_f([x]() -> int { return x; });
    EXPECT_EQ(my_f(), 256);
}

int fii(int x) { return x * x; }

TEST(correctness, args)
{
    tested_function<int(int)> my_f(fii);
    EXPECT_EQ(my_f(100), 10000);
}

TEST(correctness, multiple_args)
{
    std::string x = "hello";
    tested_function<int(int, long)> my_f(
        [x](int a, long b) -> int { return a + b; });
    EXPECT_EQ(static_cast<bool>(my_f), true);
    EXPECT_EQ(my_f(100, 200), 300);
}

std::string fs() { return "hello"; }

TEST(correctness, non_scalar)
{
    tested_function<std::string()> my_f(fs);
    EXPECT_EQ(my_f(), "hello");
}

TEST(correctness, non_scalar2)
{
    tested_function<std::string(std::string)> my_f(
        [](std::string s) -> std::string { return s + "!"; });

    EXPECT_EQ(static_cast<bool>(my_f), true);
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
    tested_function<Huge(Huge)> my_f([](Huge h) {
        h.data[0] = 1;
        return h;
    });
    Huge h;
    h.data[0] = 1;
    EXPECT_EQ(static_cast<bool>(my_f), true);
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

    Funct(Funct &&other) noexcept : id(other.id) {}

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
    tested_function<int(int)> my_f(Funct{});
    EXPECT_EQ(my_f(2), 2);
}

TEST(correctness, default_ctor)
{
    tested_function<void()> my_f{};
    EXPECT_EQ(static_cast<bool>(my_f), false);
}

TEST(correctness, nullptr_ctor)
{
    tested_function<void()> my_f{nullptr};
    EXPECT_EQ(static_cast<bool>(my_f), false);
}

TEST(correctness, copy_ctor)
{
    tested_function<int(int)> my_f{Funct{}};
    tested_function<int(int)> my_f2(my_f);
    EXPECT_EQ(my_f2(42), 42);
}

TEST(correctness, move_ctor)
{
    Funct f = Funct{};
    tested_function<int(int)> my_f{std::move(f)};
    tested_function<int(int)> my_f2(std::move(my_f));
    EXPECT_EQ(static_cast<bool>(my_f), false);
    EXPECT_EQ(static_cast<bool>(my_f2), true);
    EXPECT_EQ(my_f2(84), 84);
}

int fii2(int x) { return x * x * x; }

TEST(correctness, copy_assign)
{
    tested_function<int(int)> my_f{fii2};
    tested_function<int(int)> my_f2{fii};
    my_f2 = my_f;
    EXPECT_EQ(static_cast<bool>(my_f), true);
    EXPECT_EQ(static_cast<bool>(my_f2), true);
    EXPECT_EQ(my_f2(10), 1000);
    EXPECT_EQ(my_f(10), 1000);
}

TEST(correctness, move_assign)
{
    tested_function<int(int)> my_f{fii2};
    tested_function<int(int)> my_f2{fii};
    my_f2 = std::move(my_f);
    EXPECT_EQ(static_cast<bool>(my_f), false);
    EXPECT_EQ(static_cast<bool>(my_f2), true);
    EXPECT_EQ(my_f2(10), 1000);
}

TEST(correctness, huge_swaps)
{
    Huge h;
    auto t = [h](int x) { return x + 1; };
    tested_function<int(int)> my_f_big(t);
    tested_function<int(int)> my_f_small(fii);
    EXPECT_EQ(my_f_big.operator bool(), true);
    EXPECT_EQ(my_f_small.operator bool(), true);
    EXPECT_EQ(my_f_big(2), 3);
    EXPECT_EQ(my_f_small(2), 4);
    //    swap(my_f_big, my_f_small);
    my_f_big.swap(my_f_small);

    EXPECT_EQ(static_cast<bool>(my_f_big), true);
    EXPECT_EQ(static_cast<bool>(my_f_small), true);

    EXPECT_EQ(my_f_big(2), 4);
    EXPECT_EQ(my_f_small(2), 3);
}

struct tricky_movable {
    tricky_movable *pointer;
    tricky_movable() { pointer = this; }
    tricky_movable(tricky_movable const &) {}
    tricky_movable(tricky_movable &&other) noexcept
    {
        pointer = this;
        other.pointer = nullptr;
    }
    ptrdiff_t operator()() const noexcept
    {
        return reinterpret_cast<ptrdiff_t>(pointer) -
               reinterpret_cast<ptrdiff_t>(this);
    }
    ~tricky_movable() {}
};

TEST(correctness, tricky_movable_ctor)
{
    tested_function<ptrdiff_t()> my_f{tricky_movable{}};
    tested_function<ptrdiff_t()> my_f2{std::move(my_f)};
    EXPECT_EQ(my_f2(), 0);
}

TEST(correctness, tricky_movable_assign)
{
    tested_function<ptrdiff_t()> my_f{tricky_movable{}};
    tested_function<ptrdiff_t()> my_f2;
    my_f2 = std::move(my_f);
    EXPECT_EQ(my_f2(), 0);
}

enum class detection_policy { COPY, MOVE, BOTH };

template <bool Noexcept>
struct detector {
    static bool watcher;
    static detection_policy policy;

    detector() = default;

    detector(detector const &other)
    {
        if (policy == detection_policy::COPY ||
            policy == detection_policy::BOTH) {
            watcher = true;
        }
    }

    detector(detector &&other) noexcept(Noexcept)
    {
        if (policy == detection_policy::MOVE ||
            policy == detection_policy::BOTH) {
            watcher = true;
        }
    }

    int operator()(int x) const { return x; }
    ~detector() {}
};
template <>
bool detector<true>::watcher = false;
template <>
detection_policy detector<true>::policy = detection_policy::BOTH;
template <>
bool detector<false>::watcher = false;
template <>
detection_policy detector<false>::policy = detection_policy::BOTH;

TEST(noexcept, nothrow_move_ctor)
{
    detector<true>::policy = detection_policy::COPY;
    tested_function<int(int)> f1(detector<true>{});
    detector<true>::watcher = false;
    tested_function<int(int)> f2 = std::move(f1);
    EXPECT_EQ(detector<true>::watcher, false);
}

TEST(noexcept, throwing_move_ctor)
{
    detector<false>::policy = detection_policy::BOTH;
    tested_function<int(int)> f1(detector<false>{});
    detector<false>::watcher = false;
    tested_function<int(int)> f2 = std::move(f1);
    EXPECT_EQ(detector<false>::watcher, false);
}

TEST(noexcept, nothrow_move_assign)
{
    detector<true>::policy = detection_policy::COPY;
    tested_function<int(int)> f1(detector<true>{});
    detector<true>::watcher = false;
    tested_function<int(int)> f2;
    f2 = std::move(f1);
    EXPECT_EQ(detector<true>::watcher, false);
}

TEST(noexcept, throwing_move_assign)
{
    detector<false>::policy = detection_policy::BOTH;
    tested_function<int(int)> f1(detector<false>{});
    detector<false>::watcher = false;
    tested_function<int(int)> f2;
    f2 = std::move(f1);
    EXPECT_EQ(detector<false>::watcher, false);
}

TEST(noexcept, swap_noexcept)
{
    detector<true>::policy = detection_policy::COPY;
    tested_function<int(int)> f1(detector<true>{});
    tested_function<int(int)> f2(detector<true>{});
    detector<true>::watcher = false;
    f2.swap(f1);
    EXPECT_EQ(detector<true>::watcher, false);
}

TEST(noexcept, swap_throwing)
{
    detector<false>::policy = detection_policy::BOTH;
    tested_function<int(int)> f1(detector<false>{});
    tested_function<int(int)> f2(detector<false>{});
    detector<false>::watcher = false;
    f2.swap(f1);
    EXPECT_EQ(detector<false>::watcher, false);
}

struct heapless {
    static bool new_watcher;
    heapless() = default;
    void operator()(int) const {}
    void *operator new(size_t)
    {
        new_watcher = true;
        return ::new heapless();
    }
    void operator delete(void *p) { free(p); }
};
bool heapless::new_watcher = false;

TEST(small_object, basic)
{
    heapless::new_watcher = false;
    tested_function<void(int)> my_f2{heapless{}};
    EXPECT_EQ(heapless::new_watcher, false);
}

TEST(small_object, copy_ctor)
{
    heapless::new_watcher = false;
    tested_function<void(int)> my_f{heapless{}};
    tested_function<void(int)> my_f2{my_f};
    EXPECT_EQ(heapless::new_watcher, false);
}

TEST(small_object, copy_assign)
{
    heapless::new_watcher = false;
    tested_function<void(int)> my_f{heapless{}};
    tested_function<void(int)> my_f2;
    my_f2 = my_f;
    EXPECT_EQ(heapless::new_watcher, false);
}

TEST(small_object, move_ctor)
{
    heapless::new_watcher = false;
    tested_function<void(int)> my_f{heapless{}};
    tested_function<void(int)> my_f2{std::move(my_f)};
    EXPECT_EQ(heapless::new_watcher, false);
}

TEST(small_object, move_assign)
{
    heapless::new_watcher = false;
    tested_function<void(int)> my_f{heapless{}};
    tested_function<void(int)> my_f2;
    my_f2 = {std::move(my_f)};
    EXPECT_EQ(heapless::new_watcher, false);
}

TEST(small_object, swap)
{
    heapless::new_watcher = false;
    tested_function<void(int)> my_f{heapless{}};
    tested_function<void(int)> my_f2{heapless{}};
    my_f.swap(my_f2);
    EXPECT_EQ(heapless::new_watcher, false);
    EXPECT_EQ(static_cast<bool>(my_f), true);
    EXPECT_EQ(static_cast<bool>(my_f2), true);
}
