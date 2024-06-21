#include "test_util.hpp"

#include <dsa/queue.hpp>
#include <dsa/circular_buffer.hpp>
#include <dsa/array_list.hpp>
#include <dsa/linked_list.hpp>
#include <dsa/doubly_linked_list.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>

#include <cassert>
#include <ranges>

namespace ut = boost::ut;
namespace rr = std::ranges;
namespace rv = rr::views;

template <test_util::TestClass Type>
void test()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    Type::resetActiveInstanceCount();

    "LinkedList and DoublyLinkedList should be able to be used as Queue backend"_test = [] {
        static_assert(dsa::QueueCompatible<dsa::LinkedList, Type>);
        static_assert(dsa::QueueCompatible<dsa::DoublyLinkedList, Type>);
        static_assert(dsa::QueueCompatible<dsa::CircularBuffer, Type>);
    };

    "ArrayList should not be able to be used as Queue backend"_test = [] {
        static_assert(not dsa::QueueCompatible<dsa::ArrayList, Type>);
    };

    "Queue should be able to be constructed with a backend"_test = [] {
        dsa::Queue<dsa::LinkedList, Type>       queue;
        dsa::Queue<dsa::DoublyLinkedList, Type> queue2;

        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ReplaceOnFull,
        };
        dsa::Queue<dsa::CircularBuffer, Type> queue3{ 10uz, policy };
    };

    "Queue with LinkedList backend should be able to push and pop"_test = [] {
        dsa::Queue<dsa::LinkedList, Type> queue;

        for (auto i : rv::iota(0, 10)) {
            auto& v = queue.push(i);
            expect(that % v.value() == i);
            expect(that % queue.back().value() == i);
        }

        expect(queue.size() == 10_i);

        for (auto i : rv::iota(0, 10)) {
            expect(that % queue.front().value() == i);
            expect(that % queue.pop().value() == i);
        }
        expect(queue.empty());
    };

    "Queue with DoublyLinkedList backend should be able to push and pop"_test = [] {
        dsa::Queue<dsa::DoublyLinkedList, Type> queue;

        for (auto i : rv::iota(0, 10)) {
            auto& v = queue.push(i);
            expect(that % v.value() == i);
            expect(that % queue.back().value() == i);
        }

        expect(queue.size() == 10_i);

        for (auto i : rv::iota(0, 10)) {
            expect(that % queue.front().value() == i);
            expect(that % queue.pop().value() == i);
        }
        expect(queue.empty());
    };

    "Queue with CircularBuffer backend should be able to push and pop"_test = [] {
        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ReplaceOnFull,
        };
        dsa::Queue<dsa::CircularBuffer, Type> queue{ 10uz, policy };

        for (auto i : rv::iota(0, 10)) {
            auto& v = queue.push(i);
            expect(that % v.value() == i);
            expect(that % queue.back().value() == i);
        }

        expect(queue.size() == 10_i);

        for (auto i : rv::iota(0, 10)) {
            expect(that % queue.front().value() == i);
            expect(that % queue.pop().value() == i);
        }
        expect(queue.empty());
    };

    // unbalanced constructor/destructor means there is a bug in the code
    assert(Type::activeInstanceCount() == 0);
}

int main()
{
    test<test_util::Regular>();
    test<test_util::MovableOnly<>>();
    test<test_util::CopyableOnly<>>();

    // extra
    test_util::forEachType<test_util::NonTrivialPermutations>([]<typename T>() {
        if constexpr (T::s_movable || T::s_copyable) {
            test<T>();
        }
    });
}
