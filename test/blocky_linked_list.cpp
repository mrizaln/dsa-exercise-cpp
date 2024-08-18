#include "test_util.hpp"

#include <dsa/blocky_linked_list.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <cassert>
#include <iterator>
#include <ranges>
#include <string>

namespace ut = boost::ut;
namespace rr = std::ranges;
namespace rv = rr::views;

using test_util::equalUnderlying;
using test_util::populateContainer;
using test_util::subrange;

template <test_util::TestClass T>
auto blockRange(const dsa::BlockyLinkedList<T>& list)
{
    return std::ranges::subrange{ list.head(), list.tail()++ };
}

template <test_util::TestClass T>
std::string compare(const dsa::BlockyLinkedList<T>& list)
{
    return fmt::format("{} vs u {}", list, blockRange(list));
}

// TODO: check whether copy happens on operations that should not copy (unless type is not movable)
template <test_util::TestClass Type>
void test()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    Type::resetActiveInstanceCount();

    "iterator should be a bidirectional iterator"_test = [] {
        // non-const iterator
        using Iter = dsa::BlockyLinkedList<int>::Iterator<false>;
        static_assert(std::bidirectional_iterator<Iter>);

        // const iterator
        using ConstIter = dsa::BlockyLinkedList<int>::Iterator<true>;
        static_assert(std::bidirectional_iterator<ConstIter>);
    };

    "push_back should add an element to the end of the list"_test = [] {
        dsa::BlockyLinkedList<Type> list{};

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

        std::array<Type, 10> values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        expect(rr::equal(list, values)) << compare(list);

        list.clear();

        // back-inserter
        rr::fill_n(std::back_inserter(list), 10, 42);
        for (const auto& value : list) {
            expect(value.value() == 42_i);
        }
        expect(list.size() == 10_i);
    };

    "push_front should add an element to the head of the list"_test = [] {
        dsa::BlockyLinkedList<Type> list{};

        // first push
        auto size = list.size();
        list.push_front(42);
        expect(that % list.size() == size + 1);
        expect(list.back().value() == 42_i);
        expect(list.front().value() == 42_i);

        for (auto i : rv::iota(0, 9)) {
            expect(nothrow([&] { list.push_front(i); }))
                << fmt::format("push_front({}) : ", i) << compare(list);
        }

        expect(list.size() == 10_i);
        expect(list.back().value() == 42_i);
        expect(list.front().value() == 8_i);

        std::array<Type, 10> values = { 8, 7, 6, 5, 4, 3, 2, 1, 0, 42 };
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
        std::vector<int>            values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::BlockyLinkedList<Type> list;
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
        std::vector<int>            values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::BlockyLinkedList<Type> list;
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
            expect(nothrow([&] { return list.pop_front().value() == i; })) << "should not throw 1";
        }
        expect(list.size() == 1_i);
        expect(nothrow([&] { return list.pop_front().value() == 8_i; })) << "should not throw 2";
        expect(list.size() == 0_i);

        expect(throws([&] { list.pop_front(); })) << "should throw when pop from empty list";
    };

    "insert should inserts value at specified position"_test = [] {
        std::vector<int>            values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::BlockyLinkedList<Type> list;

        // first insert
        auto& value = list.insert(0, 42);
        expect(that % value.value() == list.front().value());
        expect(&value == &list.front()) << "insert should return reference to inserted value (same address)";

        expect(list.size() == 1_i);
        expect(list.front().value() == 42_i);
        expect(list.back().value() == 42_i);

        expect(throws([&] { list.insert(2, -1); })) << "out of bound insert should throws";

        for (auto i : rv::iota(0, 9)) {
            auto& value = list.insert(list.size(), i);
            expect(that % value.value() == list.back().value());
            expect(&value == &list.back())
                << "insert should return reference to inserted value (same address)";
        }
        expect(list.size() == 10_i);
        expect(equalUnderlying<Type>(list, values));

        // insertion at front
        list.insert(0, -1);
        expect(list.front().value() == -1_i);
        expect(list.size() == 11_i);
        expect(equalUnderlying<Type>(list | rv::drop(1), values));
        expect(list.pop_front().value() == -1_i);

        // insertion at back
        list.insert(list.size(), -1);
        expect(list.back().value() == -1_i);
        expect(list.size() == 11_i);
        expect(equalUnderlying<Type>(list | rv::take(10), values));
        expect(list.pop_back().value() == -1_i);

        // insertion in the middle (approach from head: pos <= size / 2)
        list.clear();
        populateContainer(list, values);
        list.insert(4, -1);
        expect(list.size() == 11_i);
        expect(equalUnderlying<Type>(subrange(list, 0, 4), subrange(values, 0, 4)));
        expect(std::next(list.cbegin(), 4)->value() == -1_i);
        expect(equalUnderlying<Type>(subrange(list, 5, 11), subrange(values, 4, 10)));

        // insertion in the middle (approach from tail: pos > size / 2)
        list.clear();
        populateContainer(list, values);
        list.insert(7, -1);
        expect(list.size() == 11_i);
        expect(equalUnderlying<Type>(subrange(list, 0, 7), subrange(values, 0, 7)));
        expect(std::next(list.cbegin(), 7)->value() == -1_i);
        expect(equalUnderlying<Type>(subrange(list, 8, 11), subrange(values, 7, 10)));
    };

    "remove should removes value at specified position"_test = [] {
        std::vector<int>            values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::BlockyLinkedList<Type> list;

        // remove first element
        populateContainer(list, values);
        auto value = list.remove(0);
        expect(that % value.value() == values[0]);
        expect(list.size() == 9_i);
        expect(equalUnderlying<Type>(list, subrange(values, 1, 10)));

        // remove last element
        list.clear();
        populateContainer(list, values);
        value = list.remove(list.size() - 1);
        expect(that % value.value() == values[9]);
        expect(list.size() == 9_i);
        expect(equalUnderlying<Type>(list, subrange(values, 0, 9)));

        // remove element in the middle (approach from head: pos <= size / 2)
        list.clear();
        populateContainer(list, values);
        value = list.remove(4);
        expect(that % value.value() == values[4]);
        expect(list.size() == 9_i);
        expect(equalUnderlying<Type>(subrange(list, 0, 4), subrange(values, 0, 4)));
        expect(equalUnderlying<Type>(subrange(list, 4, 9), subrange(values, 5, 10)));

        // remove element in the middle (approach from tail: pos > size / 2)
        list.clear();
        populateContainer(list, values);
        value = list.remove(7);
        expect(that % value.value() == values[7]);
        expect(list.size() == 9_i);
        expect(equalUnderlying<Type>(subrange(list, 0, 7), subrange(values, 0, 7)));
        expect(equalUnderlying<Type>(subrange(list, 7, 9), subrange(values, 8, 10)));

        // out of bound removal
        expect(throws([&] { list.remove(list.size()); })) << "out of bound removal should throws";

        // empty list removal
        list.clear();
        expect(throws([&] { list.remove(0); })) << "removing element of an empty list";
    };

    "move should leave list into an empty but usable state"_test = [] {
        dsa::BlockyLinkedList<Type> list;
        populateContainer(list, rv::iota(0, 10));

        expect(list.size() == 10_i);
        expect(equalUnderlying<Type>(list, rv::iota(0, 10)));

        auto list2 = std::move(list);
        expect(list.size() == 0_i);

        expect(nothrow([&] { list.push_back(42); })) << "push_back should work after move";
    };

    if constexpr (Type::s_copyable) {
        "copy should copy each element exactly"_test = [] {
            dsa::BlockyLinkedList<Type> list;
            populateContainer(list, rv::iota(0, 10));

            auto list2 = list;
            expect(list2.size() == 10_i);
            expect(rr::equal(list2, list));

            auto list3 = list2;
            expect(list3.size() == 10_i);
            expect(rr::equal(list3, list));
        };
    }

    "the list should be correct after operated on"_test = [] {
        constexpr std::array operations{ 987, 567, 417, 765, 12, 1, 75, 112 };
        static_assert(operations.size() == 8);

        constexpr auto alternateSum = [&]() {
            auto sum = 0;
            for (auto i : rv::iota(0uz, operations.size())) {
                sum += i % 2 == 0 ? operations.at(i) : -operations.at(i);
            }
            return sum;
        }();
        static_assert(alternateSum == 46);

        dsa::BlockyLinkedList<Type> list{};
        for (auto i : rv::iota(0uz, operations.size())) {
            auto operation = operations.at(i);
            for (auto j : rv::iota(0uz, static_cast<std::size_t>(operation))) {
                if (i % 2 == 0) {
                    if (static_cast<int>(j) <= operation / 2) {
                        expect(nothrow([&] { list.push_back(int(j + i + (std::size_t)operation / 3uz)); }));
                    } else {
                        expect(nothrow([&] {
                            list.insert(test_util::random(0uz, list.size()), int(operation));
                        }));
                    }
                } else {
                    if (static_cast<int>(j) <= operation / 2) {
                        list.pop_back();
                    } else {
                        expect(nothrow([&] { list.remove(test_util::random(0uz, list.size() - 1)); }));
                    }
                }
            }
        }
        expect(list.size() == alternateSum);

        // each block should have a capacity of b - 1 to b + 1
        for (const auto& block : blockRange(list) | rv::reverse | rv::drop(1) | rv::reverse) {
            auto actualBlockSize = block.size();
            auto blocksize       = list.blockSize();
            expect(that % actualBlockSize >= blocksize - 1 and actualBlockSize <= blocksize + 1);
        }

        // print once, cause I want to see the blocks, lol
        if constexpr (std::same_as<Type, test_util::Regular>) {
            std::string listStr{};
            for (const auto& block : blockRange(list)) {
                listStr += fmt::format("\t{:>3}: {}\n", block.size(), block);
            }
            fmt::println("list [{}]: {{\n{}}}", list.size(), listStr);
        }

        // check the values
        fmt::println("stat for {}:", ut::reflection::type_name<Type>());
        for (const auto& value : list) {
            fmt::print("\t{}\n", value.stat());
        }
    };

    // unbalanced constructor/destructor means there is a bug in the code
    assert(Type::activeInstanceCount() == 0);
}

int main()
{
#ifdef DSA_TEST_EXTRA_TYPES
    test_util::forEach<test_util::NonTrivialPermutations>([]<typename T>() {
        if constexpr (dsa::BlockyLinkedListElement<T>) {
            test<T>();
        }
    });
#else
    test<test_util::Regular>();
    test<test_util::MovableOnly<>>();
    test<test_util::CopyableOnly<>>();
#endif
}
