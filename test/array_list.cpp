#include <dsa/list/array_list.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <iterator>
#include <ranges>
#include <concepts>
#include <random>

namespace ut = boost::ut;
namespace rr = std::ranges;
namespace rv = rr::views;

class NonTrivial
{
public:
    // clang-format off
    NonTrivial() = default;
    NonTrivial(int value) : m_value(value) { }

    NonTrivial(const NonTrivial& other)     : m_value(other.m_value) { }
    NonTrivial(NonTrivial&& other) noexcept : m_value(other.m_value) { }

    NonTrivial& operator=(const NonTrivial& other)     { m_value = other.m_value; return *this; }
    NonTrivial& operator=(NonTrivial&& other) noexcept { m_value = other.m_value; return *this; }
    int value() const { return m_value; }
    // clang-format on

    friend std::ostream& operator<<(std::ostream& os, const NonTrivial& nt) { return os << nt.value(); }
    friend int           format_as(const NonTrivial& nt) { return nt.value(); }

    auto operator<=>(const NonTrivial&) const = default;

private:
    int m_value = 0;
};

static_assert(std::movable<NonTrivial> and std::copyable<NonTrivial>);

template <typename T>
T random(T min, T max)
{
    thread_local static std::mt19937 gen{ std::random_device{}() };
    if constexpr (std::is_floating_point_v<T>) {
        return std::uniform_real_distribution<T>{ min, max }(gen);
    } else {
        return std::uniform_int_distribution<T>{ min, max }(gen);
    }
}

int main()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws;

    "iterator should be a random access iterator"_test = [] {
        using Iter = dsa::ArrayList<NonTrivial>::Iterator;
        static_assert(std::random_access_iterator<Iter>);
    };

    "push_back should add an element to the end of the list"_test = [] {
        dsa::ArrayList<NonTrivial> list{ 20 };

        // first push
        auto size = list.size();
        list.push_back(42);
        expect(that % list.size() == size + 1);
        expect(list.back().value() == 42_i);
        expect(list.front().value() == 42_i);

        // push until half of the capacity
        for (auto i : rv::iota(0, 9)) {
            list.push_back(i);
        }
        expect(list.size() == 10_i);
        expect(list.back().value() == 8_i);
        expect(list.front().value() == 42_i);

        std::array<NonTrivial, 10> values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        rr::equal(list, values);

        // push until full
        while (list.size() < 20) {
            list.push_back(42);
        }
        expect(list.size() == 20_i);
        for (auto& value : list | rv::reverse | rv::take(10)) {
            expect(value.value() == 42_i);
        }

        expect(throws([&] { list.push_back(42); })) << "should throw when push to full list";

        list.clear();

        // back-inserter
        rr::fill_n(std::back_inserter(list), 10, 42);
        for (const auto& value : list) {
            expect(value.value() == 42_i);
        }
    };

    "pop_back should remove the last element from the list"_test = [] {
        std::vector<NonTrivial>    values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::ArrayList<NonTrivial> list{ 20 };
        for (auto value : values) {
            list.push_back(std::move(value));
        }
        expect(that % list.size() == values.size());

        // first pop
        auto size  = list.size();
        auto value = list.pop_back();
        expect(that % list.size() == size - 1);
        expect(value.value() == 8_i);

        for (auto i : rv::iota(0, 8) | rv::reverse) {
            expect(list.pop_back().value() == i);
        }
        expect(list.size() == 1_i);
        expect(list.pop_back().value() == 42_i);
        expect(list.size() == 0_i);

        expect(throws([&] { list.pop_back(); })) << "should throw when pop from empty list";
    };

    "array_list with 0 capacity is basically useless"_test = [] {
        dsa::ArrayList<int> list{ 0 };
        expect(list.size() == 0_i);
        expect(list.capacity() == 0_i);
        expect(throws([&] { list.push_back(42); })) << "should throw when push to empty list";
        expect(throws([&] { list.pop_back(); })) << "should throw when pop from empty list";
    };

    "insert should inserts value at specified position"_test = [] {
        std::vector<NonTrivial>    values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::ArrayList<NonTrivial> list{ 20 };

        // first insert
        list.insert(0, 42);

        expect(list.size() == 1_i);
        expect(list.front().value() == 42_i);
        expect(list.back().value() == 42_i);

        expect(throws([&] { list.insert(2, -1); })) << "out of bound insert should throws";

        for (auto i : rv::iota(0, 9)) {
            list.insert(list.size(), i);
        }
        expect(list.size() == 10_i);
        expect(rr::equal(list, values));

        // insertion in the middle
        list.pop_back();    // to reserve one space
        list.insert(5, -1);

        expect(rr::equal(list | rv::take(4), values | rv::take(4)));
        expect(list[5] == -1_i);
        expect(rr::equal(list | rv::drop(6) | rv::take(4), values | rv::drop(5) | rv::take(4)));
    };

    "remove should removes value at specified position"_test = [] {
        std::vector<NonTrivial>    values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::ArrayList<NonTrivial> list{ 20 };
        for (auto value : values) {
            list.push_back(std::move(value));
        }
        expect(that % list.size() == values.size());

        auto value = list.remove(5);
        expect(that % value == values[5]);
        expect(list.size() == 9_i);
        expect(rr::equal(list | rv::take(5), values | rv::take(5)));
        expect(rr::equal(list | rv::drop(5) | rv::take(4), values | rv::drop(6) | rv::take(4)));

        list.clear();
        list.push_back(42);

        expect(throws([&] { list.remove(1); })) << "out of bound removal should throws";
        expect(list.remove(0) == 42_i);
        expect(list.size() == 0_i);

        expect(throws([&] { list.remove(0); })) << "removing element of an empty list";
    };
}
