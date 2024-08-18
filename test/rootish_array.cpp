#include "test_util.hpp"

#include <dsa/rootish_array.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>

#include <cassert>
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
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    Type::resetActiveInstanceCount();

    "iterator should be a random access iterator"_test = [] {
        using Iter = dsa::RootishArray<Type>::template Iterator<false>;
        static_assert(std::random_access_iterator<Iter>);

        using ConstIter = dsa::RootishArray<Type>::template Iterator<true>;
        static_assert(std::random_access_iterator<ConstIter>);
    };

    "array_list with 0 capacity is usable"_test = [] {
        dsa::RootishArray<int> list{};
        expect(list.size() == 0_i);

        expect(nothrow([&] { list.push_back(42); })) << "pushing to an empty list should grow the capacity";
        expect(that % list.size() == 1_i) << fmt::format("list: {}", list);

        expect(nothrow([&] { list.pop_back(); })) << "pop from a list with 1 element";
    };

    "push_back should add an element to the end of the list"_test = [] {
        dsa::RootishArray<Type> list{};

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

        list.clear();

        // back-inserter
        rr::fill_n(std::back_inserter(list), 10, 42);
        for (const auto& value : list) {
            expect(value.value() == 42_i);
        }
    };

    "pop_back should remove the last element from the list"_test = [] {
        std::vector<int>        values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::RootishArray<Type> list{};
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

    "insert should inserts value at specified position"_test = [] {
        std::vector<int>        values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::RootishArray<Type> list{};

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
        std::vector<int>        values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::RootishArray<Type> list{};
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

        expect(throws([&] { list.remove(0); })) << "removing element of an empty list should throw";
    };

    "move should leave list into an empty state that is usable"_test = [] {
        dsa::RootishArray<Type> list{};
        populateContainer(list, rv::iota(0, 10));

        expect(list.size() == 10_i);
        expect(equalUnderlying<Type>(list, rv::iota(0, 10)));

        auto list2 = std::move(list);
        expect(list.size() == 0_i);

        expect(nothrow([&] { list.push_back(42); })) << "should throw when push to empty list";
        expect(that % list.size() == 1_i);

        expect(nothrow([&] { list.pop_back(); })) << "pop from a list with 1 element";
    };

    if constexpr (std::copyable<Type>) {
        "copy should copy each element exactly"_test = [] {
            dsa::RootishArray<Type> list{};
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

    // TODO: add parapeterized constructor to RootishArray
    // if constexpr (std::default_initializable<Type>) {
    //     "non-default constructed array_list should default initialize exactly N elements"_test = [] {
    //         dsa::RootishArray<Type> list{ 41 };
    //         expect(list.size() == 41_i);
    //         expect(rr::all_of(list, [](const auto& value) { return value.stat().defaulted(); }));
    //     };
    // }

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

        dsa::RootishArray<Type> list{};
        for (auto i : rv::iota(0uz, operations.size())) {
            auto operation = operations.at(i);
            for (auto j : rv::iota(0uz, static_cast<std::size_t>(operation))) {
                if (i % 2 == 0) {
                    if (static_cast<int>(j) <= operation / 2) {
                        list.push_back(int(j + i + (std::size_t)operation / 3uz));
                    } else {
                        list.insert(test_util::random(0uz, list.size()), static_cast<int>(operation));
                    }
                } else {
                    list.pop_back();
                }
            }
        }
        expect(list.size() == alternateSum);

        // each block should have a capacity of i + 1 and fully filled except the last two blocks
        for (auto size = list.blocks().size(); auto i : rv::iota(0uz, size)) {
            expect(that % list.block(i).capacity() == i + 1);
            if (i < size - 2) {
                expect(that % list.block(i).size() == list.block(i).capacity());
            }
        }

        // print once, cause I want to see the blocks, lol
        if constexpr (std::same_as<Type, test_util::Regular>) {
            // fmt::print("list [{}@{}]: {}\n", list.size(), list.blocks().size(), list.blocks());
            std::string listStr{};
            for (const auto& block : list.blocks()) {
                listStr += fmt::format("\t{:>3}: {}\n", block.size(), block);
            }
            fmt::println("list [{}@{}]: {{\n{}}}", list.size(), list.blocks().size(), listStr);
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
        if constexpr (dsa::RootishArrayElement<T>) {
            test<T>();
        }
    });
#else
    test<test_util::Regular>();
    test<test_util::MovableOnly<>>();
    test<test_util::CopyableOnly<>>();
#endif
}
