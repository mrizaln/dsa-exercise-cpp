#include "test_util.hpp"

#include <dsa/array_list.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <ranges>

namespace ut = boost::ut;
namespace rr = std::ranges;
namespace rv = rr::views;

using test_util::equalUnderlying;
using test_util::populateContainer;

// TODO: check whether copy happens on operations that should not copy (unless type is not movable, then copy
// should happen)
template <test_util::TestClass Type>
void test()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws;

    "iterator should be a random access iterator"_test = [] {
        using Iter = dsa::ArrayList<Type>::Iterator;
        static_assert(std::random_access_iterator<Iter>);
    };

    "push_back should add an element to the end of the list"_test = [] {
        dsa::ArrayList<Type> list{ 20 };

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

        std::array values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        equalUnderlying<Type>(list, values);

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
        std::vector<int>     values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::ArrayList<Type> list{ 20 };
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
        std::vector<int>     values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::ArrayList<Type> list{ 20 };

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
        expect(equalUnderlying<Type>(list, values));

        // insertion in the middle
        list.pop_back();    // to reserve one space
        list.insert(5, -1);

        expect(equalUnderlying<Type>(list | rv::take(4), values | rv::take(4)));
        expect(list.at(5).value() == -1_i);
        expect(equalUnderlying<Type>(list | rv::drop(6) | rv::take(4), values | rv::drop(5) | rv::take(4)));
    };

    "remove should removes value at specified position"_test = [] {
        std::vector<int>     values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::ArrayList<Type> list{ 20 };
        for (auto value : values) {
            list.push_back(std::move(value));
        }
        expect(that % list.size() == values.size());

        auto value = list.remove(5);
        expect(that % value.value() == values[5]);
        expect(list.size() == 9_i);
        expect(equalUnderlying<Type>(list | rv::take(5), values | rv::take(5)));
        expect(equalUnderlying<Type>(list | rv::drop(5) | rv::take(4), values | rv::drop(6) | rv::take(4)));

        list.clear();
        list.push_back(42);

        expect(throws([&] { list.remove(1); })) << "out of bound removal should throws";
        expect(list.remove(0).value() == 42_i);
        expect(list.size() == 0_i);

        expect(throws([&] { list.remove(0); })) << "removing element of an empty list";
    };

    "move should leave list into an empty state that is not usable"_test = [] {
        dsa::ArrayList<Type> list{ 20 };
        populateContainer(list, rv::iota(0, 10));

        expect(list.size() == 10_i);
        expect(equalUnderlying<Type>(list, rv::iota(0, 10)));

        auto list2 = std::move(list);
        expect(list.size() == 0_i);
        expect(list.capacity() == 0_i);

        expect(throws([&] { list.push_back(42); })) << "should throw when push to empty list";
    };

    if constexpr (std::copyable<Type>) {
        "copy should copy each element exactly"_test = [] {
            dsa::ArrayList<Type> list{ 20 };
            populateContainer(list, rv::iota(0, 10));

            expect(list.size() == 10_i);
            expect(equalUnderlying<Type>(list, rv::iota(0, 10)));

            auto list2 = list;
            expect(list2.size() == 10_i);
            expect(rr::equal(list2, list));

            auto list3 = list2;
            expect(list3.size() == 10_i);
            expect(rr::equal(list3, list));
        };
    }
}

int main()
{
    // main types
    test<test_util::Regular>();
    test<test_util::MovableOnly<>>();
    test<test_util::CopyableOnly<>>();

    // extra
    test_util::forEachType<test_util::NonTrivialPermutations>([]<typename T>() {
        if constexpr (dsa::ArrayElement<T>) {
            test<T>();
        }
    });
}
