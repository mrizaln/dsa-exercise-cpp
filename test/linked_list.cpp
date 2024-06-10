#include <dsa/list/linked_list.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

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

static_assert(std::movable<NonTrivial> and std::copyable<NonTrivial>);

int main()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws;

    "iterator should be a forward iterator"_test = [] {
        // non-const iterator
        using Iter = dsa::LinkedList<int>::Iterator<false>;
        static_assert(std::forward_iterator<Iter>);

        // const iterator
        using ConstIter = dsa::LinkedList<int>::Iterator<true>;
        static_assert(std::forward_iterator<ConstIter>);
    };

    "push_back should add an element to the end of the list"_test = [] {
        dsa::LinkedList<NonTrivial> list{};

        // first push
        auto size = list.size();
        list.push_back(42);
        expect(that % list.size() == size + 1);
        expect(list.back().value() == 42_i);
        expect(list.front().value() == 42_i);

        for (auto i : rv::iota(0, 9)) {
            list.push_back(i);
        }
        expect(list.size() == 10_i);
        expect(list.back().value() == 8_i);
        expect(list.front().value() == 42_i);

        std::array<NonTrivial, 10> values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        rr::equal(list, values);

        list.clear();

        // back-inserter
        rr::fill_n(std::back_inserter(list), 10, 42);
        for (const auto& value : list) {
            expect(value.value() == 42_i);
        }
        expect(list.size() == 10_i);
    };

    "pop_back should remove the last element from the list"_test = [] {
        std::vector<NonTrivial>     values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::LinkedList<NonTrivial> list;
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
}
