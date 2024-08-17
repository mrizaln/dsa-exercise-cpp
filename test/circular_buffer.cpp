#include "test_util.hpp"

#include <dsa/circular_buffer.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <cassert>
#include <ranges>
#include <concepts>
#include <vector>

namespace ut = boost::ut;
namespace rr = std::ranges;
namespace rv = rr::views;

using test_util::equalUnderlying;
using test_util::populateContainer;
using test_util::populateContainerFront;
using test_util::subrange;

template <typename T>
std::string compare(const dsa::CircularBuffer<T>& buffer)
{
    return fmt::format("{} vs u {}", buffer, std::span{ buffer.data(), buffer.capacity() });
}

constexpr auto g_policyPermutations = std::tuple{
    dsa::BufferPolicy{ dsa::BufferCapacityPolicy::FixedCapacity, dsa::BufferStorePolicy::ReplaceOnFull },
    dsa::BufferPolicy{ dsa::BufferCapacityPolicy::DynamicCapacity, dsa::BufferStorePolicy::ReplaceOnFull },
    dsa::BufferPolicy{ dsa::BufferCapacityPolicy::FixedCapacity, dsa::BufferStorePolicy::ThrowOnFull },
    dsa::BufferPolicy{ dsa::BufferCapacityPolicy::DynamicCapacity, dsa::BufferStorePolicy::ThrowOnFull },
};

// TODO: check whether copy happens on operations that should not copy (unless type is not movable)
// TODO: add test for pop_front on DynamicCapacity policy; check when will it double or halve its capacity
// TODO: add test for edge case: 1 digit capacity
// TODO: add test for insertion with DiscardTail policy and DynamicCapacity and ThrowOnFull + FixedCapacity
template <test_util::TestClass Type>
void test()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    Type::resetActiveInstanceCount();

    "iterator should be a random access iterator iterator"_test = [] {
        using Iter = dsa::CircularBuffer<Type>::template Iterator<false>;
        static_assert(std::random_access_iterator<Iter>);

        using ConstIter = dsa::CircularBuffer<Type>::template Iterator<true>;
        static_assert(std::random_access_iterator<ConstIter>);
    };

    "push_back should add an element to the back"_test = [](dsa::BufferPolicy policy) {
        dsa::CircularBuffer<Type> buffer{ 10, policy };    // default policy

        // first push
        auto  size  = buffer.size();
        auto& value = buffer.push_back(42);
        expect(that % buffer.size() == size + 1);
        expect(value.value() == 42_i);
        expect(buffer.back().value() == 42_i);
        expect(buffer.front().value() == 42_i);

        for (auto i : rv::iota(0, 9)) {
            auto& value = buffer.push_back(i);
            expect(that % value.value() == i);
            expect(that % buffer.back().value() == i) << compare(buffer);
            expect(that % buffer.front().value() == 42);
        }

        std::array expected{ 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        expect(equalUnderlying<Type>(buffer, expected));

        buffer.clear();
        expect(buffer.size() == 0_i);

        rr::fill_n(std::back_inserter(buffer), 10, 42);
        for (const auto& value : buffer) {
            expect(value.value() == 42_i);
        }
        expect(buffer.size() == 10_i);
    } | g_policyPermutations;

    "push_back with ReplaceOnFullepolicy should replace the adjacent element when buffer is full"_test = [] {
        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ReplaceOnFull,
        };
        dsa::CircularBuffer<Type> buffer{ 10, policy };

        // fully populate buffer
        populateContainer(buffer, rv::iota(0, 10));
        expect(buffer.size() == 10_i);
        expect(that % buffer.size() == buffer.capacity());
        expect(equalUnderlying<Type>(buffer, rv::iota(0, 10)));

        // replace old elements (4 times)
        for (auto i : rv::iota(21, 25)) {
            auto& value = buffer.push_back(i);
            expect(that % value.value() == i);
            expect(that % buffer.back().value() == i);
            expect(buffer.capacity() == 10_i);
            expect(buffer.size() == 10_i);
        }

        // the circular-buffer iterator
        expect(equalUnderlying<Type>(subrange(buffer, 0, 6), rv::iota(0, 10) | rv::drop(4)));
        expect(equalUnderlying<Type>(subrange(buffer, 6, 10), rv::iota(21, 25))) << compare(buffer);

        // the underlying array
        auto underlying = std::span{ buffer.data(), buffer.capacity() };
        expect(equalUnderlying<Type>(subrange(underlying, 0, 4), rv::iota(21, 25))) << compare(buffer);
        expect(equalUnderlying<Type>(subrange(underlying, 4, 10), rv::iota(0, 10) | rv::drop(4)))
            << compare(buffer);
    };

    "push_back with ThrowOnFull policy should throw when buffer is full"_test = [] {
        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ThrowOnFull,
        };
        dsa::CircularBuffer<Type> buffer{ 10, policy };

        // fully populate buffer
        populateContainer(buffer, rv::iota(0, 10));
        expect(buffer.size() == 10_i);
        expect(that % buffer.size() == buffer.capacity());
        expect(equalUnderlying<Type>(buffer, rv::iota(0, 10)));

        expect(throws([&] { buffer.push_back(42); })) << "should throw when push to full buffer";
    };

    "push_back with DynamicCapacity should never replace old elements nor throws on a full buffer"_test =
        [](dsa::BufferStorePolicy storePolicy) {
            dsa::BufferPolicy policy{
                .m_capacity = dsa::BufferCapacityPolicy::DynamicCapacity,
                .m_store    = storePolicy,
            };
            dsa::CircularBuffer<Type> buffer{ 10, policy };

            // fully populate buffer
            populateContainer(buffer, rv::iota(0, 10));
            expect(buffer.size() == 10_i);
            expect(that % buffer.size() == buffer.capacity());
            expect(equalUnderlying<Type>(buffer, rv::iota(0, 10)));

            expect(nothrow([&] { buffer.push_back(42); })) << "should not throw when push to full buffer";
            expect(buffer.back().value() == 42_i);
            expect(buffer.size() == 11_i);
            expect(buffer.capacity() > 10_i);
            expect(equalUnderlying<Type>(subrange(buffer, 0, 10), rv::iota(0, 10)));
        }
        | std::tuple{ dsa::BufferStorePolicy::ReplaceOnFull, dsa::BufferStorePolicy::ThrowOnFull };

    "push_front should add an element to the front"_test = [](dsa::BufferPolicy policy) {
        dsa::CircularBuffer<Type> buffer{ 10, policy };    // default policy

        // first push
        auto  size  = buffer.size();
        auto& value = buffer.push_front(42);
        expect(that % buffer.size() == size + 1);
        expect(value.value() == 42_i);
        expect(buffer.front().value() == 42_i);
        expect(buffer.back().value() == 42_i);

        for (auto i : rv::iota(0, 9)) {
            auto& value = buffer.push_front(i);
            expect(that % value.value() == i);
            expect(that % buffer.front().value() == i) << compare(buffer);
            expect(that % buffer.back().value() == 42);
        }

        std::array expected{ 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        expect(equalUnderlying<Type>(buffer | rv::reverse, expected)) << compare(buffer);

        buffer.clear();
        expect(buffer.size() == 0_i);

        rr::fill_n(std::back_inserter(buffer), 10, 42);
        for (const auto& value : buffer) {
            expect(value.value() == 42_i);
        }
        expect(buffer.size() == 10_i);
    } | g_policyPermutations;

    "push_front with ReplaceOnFullepolicy should replace the adjacent element when buffer is full"_test = [] {
        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ReplaceOnFull,
        };
        dsa::CircularBuffer<Type> buffer{ 10, policy };

        // fully populate buffer
        populateContainerFront(buffer, rv::iota(0, 10));
        expect(buffer.size() == 10_i);
        expect(that % buffer.size() == buffer.capacity());
        expect(equalUnderlying<Type>(buffer | rv::reverse, rv::iota(0, 10))) << compare(buffer);

        // replace old elements (4 times)
        for (auto i : rv::iota(21, 25)) {
            auto& value = buffer.push_front(i);
            expect(that % value.value() == i);
            expect(that % buffer.front().value() == i);
            expect(buffer.capacity() == 10_i);
            expect(buffer.size() == 10_i);
        }

        // the circular-buffer iterator
        auto rbuffer = buffer | rv::reverse;
        expect(equalUnderlying<Type>(subrange(rbuffer, 0, 6), rv::iota(0, 10) | rv::drop(4)));
        expect(equalUnderlying<Type>(subrange(rbuffer, 6, 10), rv::iota(21, 25))) << compare(buffer);

        // the underlying array
        auto runderlying = std::span{ buffer.data(), buffer.capacity() } | rv::reverse;
        expect(equalUnderlying<Type>(subrange(runderlying, 0, 4), rv::iota(21, 25))) << compare(buffer);
        expect(equalUnderlying<Type>(subrange(runderlying, 4, 10), rv::iota(0, 10) | rv::drop(4)))
            << compare(buffer);
    };

    "push_front with ThrowOnFull policy should throw when buffer is full"_test = [] {
        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ThrowOnFull,
        };
        dsa::CircularBuffer<Type> buffer{ 10, policy };

        // fully populate buffer
        populateContainerFront(buffer, rv::iota(0, 10));
        expect(buffer.size() == 10_i);
        expect(that % buffer.size() == buffer.capacity());
        expect(equalUnderlying<Type>(buffer | rv::reverse, rv::iota(0, 10))) << compare(buffer);

        expect(throws([&] { buffer.push_front(42); })) << "should throw when push to full buffer";
    };

    "push_front with DynamicCapacity should never replace old elements nor throws on a full buffer"_test =
        [](dsa::BufferStorePolicy storePolicy) {
            dsa::BufferPolicy policy{
                .m_capacity = dsa::BufferCapacityPolicy::DynamicCapacity,
                .m_store    = storePolicy,
            };
            dsa::CircularBuffer<Type> buffer{ 10, policy };

            // fully populate buffer
            populateContainerFront(buffer, rv::iota(0, 10));
            expect(buffer.size() == 10_i);
            expect(that % buffer.size() == buffer.capacity());
            expect(equalUnderlying<Type>(buffer | rv::reverse, rv::iota(0, 10))) << compare(buffer);

            expect(nothrow([&] { buffer.push_front(42); })) << "should not throw when push to full buffer";
            expect(buffer.front().value() == 42_i);
            expect(buffer.size() == 11_i);
            expect(buffer.capacity() > 10_i);
            expect(equalUnderlying<Type>(subrange(buffer | rv::reverse, 0, 10), rv::iota(0, 10)));
        }
        | std::tuple{ dsa::BufferStorePolicy::ReplaceOnFull, dsa::BufferStorePolicy::ThrowOnFull };

    "pop_front should remove the first element on the buffer"_test = [](dsa::BufferPolicy policy) {
        std::vector<int>          values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::CircularBuffer<Type> buffer{ 10, policy };
        for (auto value : values) {
            buffer.push_back(std::move(value));
        }
        expect(that % buffer.size() == values.size());

        // first pop
        auto size  = buffer.size();
        auto value = buffer.pop_front();
        expect(that % buffer.size() == size - 1);
        expect(value.value() == 42_i);

        for (auto i : rv::iota(0, 8)) {
            expect(buffer.pop_front().value() == i);
        }
        expect(buffer.size() == 1_i);
        expect(buffer.pop_front().value() == 8_i);
        expect(buffer.size() == 0_i);

        expect(throws([&] { buffer.pop_front(); })) << "should throw when pop from empty buffer";
    } | g_policyPermutations;

    "insertion in the middle should move the elements around with FixedCapacity and ReplaceOnFull"_test = [] {
        // full buffer condition
        {
            dsa::CircularBuffer<Type> buffer{ 10 };    // default policy
            populateContainer(buffer, rv::iota(0, 10));

            buffer.insert(3, 42, dsa::BufferInsertPolicy::DiscardHead);
            expect(buffer.size() == 10_i);
            expect(equalUnderlying<Type>(subrange(buffer, 0, 3), rv::iota(1, 4))) << compare(buffer);
            expect(buffer.at(3).value() == 42_i) << compare(buffer);
            expect(equalUnderlying<Type>(subrange(buffer, 4, 10), rv::iota(4, 10))) << compare(buffer);

            auto expected = std::vector{ 1, 2, 3, 42, 4, 5, 6, 7, 8, 9 };
            expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);

            buffer.insert(0, 42, dsa::BufferInsertPolicy::DiscardHead);
            expected = std::vector{ 42, 2, 3, 42, 4, 5, 6, 7, 8, 9 };
            expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);

            buffer.insert(9, 32748, dsa::BufferInsertPolicy::DiscardHead);
            expected = std::vector{ 2, 3, 42, 4, 5, 6, 7, 8, 9, 32748 };
            expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);
        }

        // partially filled buffer condition
        {
            dsa::CircularBuffer<Type> buffer{ 10 };    // default policy
            populateContainer(buffer, rv::iota(0, 15));
            for (auto _ : rv::iota(0, 5)) {
                buffer.pop_front();
            }

            auto expected = std::vector{ 10, 11, 12, 13, 14 };
            expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);

            buffer.insert(2, -42, dsa::BufferInsertPolicy::DiscardHead);
            expected = std::vector{ 10, 11, -42, 12, 13, 14 };
            expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);

            buffer.insert(0, -42, dsa::BufferInsertPolicy::DiscardHead);
            expected = std::vector{ -42, 10, 11, -42, 12, 13, 14 };
            buffer.insert(buffer.size(), -42, dsa::BufferInsertPolicy::DiscardHead);
        }
    };

    "removal should be able to remove value anywhere in the buffer"_test = [] {
        dsa::CircularBuffer<Type> buffer{ 10 };    // default policy
        populateContainer(buffer, rv::iota(0, 15));
        std::vector expected{ 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
        expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);

        auto value = buffer.remove(3);
        expected   = std::vector{ 5, 6, 7, 9, 10, 11, 12, 13, 14 };
        expect(value.value() == 8_i);
        expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);

        auto end = buffer.size() - 1;
        value    = buffer.remove(end);
        expected = std::vector{ 5, 6, 7, 9, 10, 11, 12, 13 };
        expect(value.value() == 14_i);
        expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);

        value    = buffer.remove(0);
        expected = std::vector{ 6, 7, 9, 10, 11, 12, 13 };
        expect(value.value() == 5_i);
        expect(equalUnderlying<Type>(buffer, expected)) << compare(buffer);
    };

    "default initialized CircularBuffer is basically useless"_test = [] {
        dsa::CircularBuffer<int> buffer;
        expect(buffer.size() == 0_i);
        expect(buffer.capacity() == 0_i);
        expect(throws([&] { buffer.push_back(42); })) << "should throw when push to empty buffer";
        expect(throws([&] { buffer.pop_front(); })) << "should throw when pop from empty buffer";
    };

    "move should leave buffer into an empty state that is not usable"_test = [] {
        dsa::CircularBuffer<Type> buffer{ 20 };
        populateContainer(buffer, rv::iota(0, 10));

        expect(buffer.size() == 10_i);
        expect(equalUnderlying<Type>(buffer, rv::iota(0, 10)));

        auto buffer2 = std::move(buffer);
        expect(buffer.size() == 0_i);
        expect(buffer.capacity() == 0_i);
        expect(throws([&] { buffer.push_back(42); })) << "should throw when push to empty buffer";
        expect(throws([&] { buffer.pop_front(); })) << "should throw when pop from empty buffer";
    };

    if constexpr (std::copyable<Type>) {
        "copy should copy each element exactly"_test = [] {
            dsa::CircularBuffer<Type> buffer{ 20 };
            populateContainer(buffer, rv::iota(0, 10));

            expect(buffer.size() == 10_i);
            expect(equalUnderlying<Type>(buffer, rv::iota(0, 10)));

            auto buffer2 = buffer;
            expect(buffer2.size() == 10_i);
            expect(rr::equal(buffer2, buffer));

            auto buffer3 = buffer2;
            expect(buffer3.size() == 10_i);
            expect(rr::equal(buffer3, buffer));
        };
    }

    // unbalanced constructor/destructor means there is a bug in the code
    assert(Type::activeInstanceCount() == 0);
}

int main()
{
    // main types
    test<test_util::Regular>();
    test<test_util::MovableOnly<>>();
    test<test_util::CopyableOnly<>>();

    // extra
    test_util::forEachType<test_util::NonTrivialPermutations>([]<typename T>() {
        if constexpr (dsa::CircularBufferElement<T>) {
            test<T>();
        }
    });
}
