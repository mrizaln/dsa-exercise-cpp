#include <dsa/list/doubly_linked_list.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <ranges>
#include <concepts>

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

auto subrange(auto&& range, std::size_t start, std::size_t end)
{
    return range | rv::drop(start) | rv::take(end - start);
}

template <typename T, std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, T>
void populateList(dsa::DoublyLinkedList<T>& list, R&& range)
{
    for (auto value : range) {
        list.push_back(std::move(value));
    }
}

int main()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    "iterator should be a forward iterator"_test = [] {
        // non-const iterator
        using Iter = dsa::DoublyLinkedList<int>::Iterator<false>;
        static_assert(std::forward_iterator<Iter>);

        // const iterator
        using ConstIter = dsa::DoublyLinkedList<int>::Iterator<true>;
        static_assert(std::forward_iterator<ConstIter>);
    };

    "push_back should add an element to the end of the list"_test = [] {
        dsa::DoublyLinkedList<NonTrivial> list{};

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
        expect(rr::equal(list, values));

        list.clear();

        // back-inserter
        rr::fill_n(std::back_inserter(list), 10, 42);
        for (const auto& value : list) {
            expect(value.value() == 42_i);
        }
        expect(list.size() == 10_i);
    };

    "push_front should add an element to the head of the list"_test = [] {
        dsa::DoublyLinkedList<NonTrivial> list{};

        // first push
        auto size = list.size();
        list.push_front(42);
        expect(that % list.size() == size + 1);
        expect(list.back().value() == 42_i);
        expect(list.front().value() == 42_i);

        for (auto i : rv::iota(0, 9)) {
            list.push_front(i);
        }

        expect(list.size() == 10_i);
        expect(list.back().value() == 42_i);
        expect(list.front().value() == 8_i);

        std::array<NonTrivial, 10> values = { 8, 7, 6, 5, 4, 3, 2, 1, 0, 42 };
        expect(rr::equal(list, values));

        list.clear();

        // front-inserter
        rr::fill_n(std::front_inserter(list), 10, 42);
        for (const auto& value : list) {
            expect(value.value() == 42_i);
        }
        expect(list.size() == 10_i);
    };

    "pop_back should remove the last element from the list"_test = [] {
        std::vector<NonTrivial>           values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::DoublyLinkedList<NonTrivial> list;
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
            expect(that % list.pop_back().value() == i);
        }
        expect(list.size() == 1_i);
        expect(list.pop_back().value() == 42_i);
        expect(list.size() == 0_i);

        expect(throws([&] { list.pop_back(); })) << "should throw when pop from empty list";
    };

    "pop_front should remove the first element on the list"_test = [] {
        std::vector<NonTrivial>           values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::DoublyLinkedList<NonTrivial> list;
        for (auto value : values) {
            list.push_back(std::move(value));
        }
        expect(that % list.size() == values.size());

        // first pop
        auto size  = list.size();
        auto value = list.pop_front();
        expect(that % list.size() == size - 1);
        expect(value.value() == 42_i);

        for (auto i : rv::iota(0, 8)) {
            expect(list.pop_front().value() == i);
        }
        expect(list.size() == 1_i);
        expect(list.pop_front().value() == 8_i);
        expect(list.size() == 0_i);

        expect(throws([&] { list.pop_front(); })) << "should throw when pop from empty list";
    };

    "insert should inserts value at specified position"_test = [] {
        std::vector<NonTrivial>           values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::DoublyLinkedList<NonTrivial> list;

        // first insert
        auto& value = list.insert(0, 42);
        expect(that % value == list.front());
        expect(&value == &list.front()) << "insert should return reference to inserted value (same address)";

        expect(list.size() == 1_i);
        expect(list.front().value() == 42_i);
        expect(list.back().value() == 42_i);

        expect(throws([&] { list.insert(2, -1); })) << "out of bound insert should throws";

        for (auto i : rv::iota(0, 9)) {
            auto& value = list.insert(list.size(), i);
            expect(that % value == list.back());
            expect(&value == &list.back())
                << "insert should return reference to inserted value (same address)";
        }
        expect(list.size() == 10_i);
        expect(rr::equal(list, values));

        // insertion at front
        list.insert(0, -1);
        expect(list.front() == -1_i);
        expect(list.size() == 11_i);
        expect(rr::equal(list | rv::drop(1), values));
        expect(list.pop_front() == -1_i);

        // insertion at back
        list.insert(list.size(), -1);
        expect(list.back() == -1_i);
        expect(list.size() == 11_i);
        expect(rr::equal(list | rv::take(10), values));
        expect(list.pop_back() == -1_i);

        // insertion in the middle (approach from head: pos <= size / 2)
        list.clear();
        populateList(list, values);
        list.insert(4, -1);
        expect(list.size() == 11_i);
        expect(rr::equal(subrange(list, 0, 4), subrange(values, 0, 4)));
        expect(*std::next(list.cbegin(), 4) == -1_i);
        expect(rr::equal(subrange(list, 5, 11), subrange(values, 4, 10)));

        // insertion in the middle (approach from tail: pos > size / 2)
        list.clear();
        populateList(list, values);
        list.insert(7, -1);
        expect(list.size() == 11_i);
        expect(rr::equal(subrange(list, 0, 7), subrange(values, 0, 7)));
        expect(*std::next(list.cbegin(), 7) == -1_i);
        expect(rr::equal(subrange(list, 8, 11), subrange(values, 7, 10)));
    };

    "remove should removes value at specified position"_test = [] {
        std::vector<NonTrivial>           values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::DoublyLinkedList<NonTrivial> list;

        // remove first element
        populateList(list, values);
        auto value = list.remove(0);
        expect(that % value == values[0]);
        expect(list.size() == 9_i);
        expect(rr::equal(list, subrange(values, 1, 10)));

        // remove last element
        list.clear();
        populateList(list, values);
        value = list.remove(list.size() - 1);
        expect(that % value == values[9]);
        expect(list.size() == 9_i);
        expect(rr::equal(list, subrange(values, 0, 9)));

        // remove element in the middle (approach from head: pos <= size / 2)
        list.clear();
        populateList(list, values);
        value = list.remove(4);
        expect(that % value == values[4]);
        expect(list.size() == 9_i);
        expect(rr::equal(subrange(list, 0, 4), subrange(values, 0, 4)));
        expect(rr::equal(subrange(list, 4, 9), subrange(values, 5, 10)));

        // remove element in the middle (approach from tail: pos > size / 2)
        list.clear();
        populateList(list, values);
        value = list.remove(7);
        expect(that % value == values[7]);
        expect(list.size() == 9_i);
        expect(rr::equal(subrange(list, 0, 7), subrange(values, 0, 7)));
        expect(rr::equal(subrange(list, 7, 9), subrange(values, 8, 10)));

        // out of bound removal
        expect(throws([&] { list.remove(list.size()); })) << "out of bound removal should throws";

        // empty list removal
        list.clear();
        expect(throws([&] { list.remove(0); })) << "removing element of an empty list";
    };

    "move should leave list into an empty but usable state"_test = [] {
        dsa::DoublyLinkedList<NonTrivial> list;
        populateList(list, rv::iota(0, 10));

        expect(list.size() == 10_i);
        expect(rr::equal(list, rv::iota(0, 10)));

        auto list2 = std::move(list);
        expect(list.size() == 0_i);

        expect(nothrow([&] { list.push_back(42); })) << "push_back should work after move";
    };

    "copy should copy each element exactly"_test = [] {
        dsa::DoublyLinkedList<NonTrivial> list;
        populateList(list, rv::iota(0, 10));

        auto list2 = list;
        expect(list2.size() == 10_i);
        expect(rr::equal(list2, list));

        auto list3 = list2;
        expect(list3.size() == 10_i);
        expect(rr::equal(list3, list));
    };
}
