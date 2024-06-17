#include <dsa/queue.hpp>
#include <dsa/circular_buffer.hpp>
#include <dsa/array_list.hpp>
#include <dsa/linked_list.hpp>
#include <dsa/doubly_linked_list.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>

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

int main()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    "LinkedList and DoublyLinkedList should be able to be used as Queue backend"_test = [] {
        static_assert(dsa::QueueCompatible<dsa::LinkedList, NonTrivial>);
        static_assert(dsa::QueueCompatible<dsa::DoublyLinkedList, NonTrivial>);
        static_assert(dsa::QueueCompatible<dsa::CircularBuffer, NonTrivial>);
    };

    "ArrayList should not be able to be used as Queue backend"_test = [] {
        static_assert(not dsa::QueueCompatible<dsa::ArrayList, NonTrivial>);
    };

    "Queue should be able to be constructed with a backend"_test = [] {
        dsa::Queue<dsa::LinkedList, NonTrivial>       queue;
        dsa::Queue<dsa::DoublyLinkedList, NonTrivial> queue2;

        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ReplaceOldest,
        };
        dsa::Queue<dsa::CircularBuffer, NonTrivial> queue3{ 10, policy };
    };

    "Queue with LinkedList backend should be able to push and pop"_test = [] {
        dsa::Queue<dsa::LinkedList, NonTrivial> queue;

        for (auto i : rv::iota(0, 10)) {
            auto v = queue.push(i);
            expect(that % v == i);
            expect(that % queue.back() == i);
        }

        expect(queue.size() == 10_i);

        for (auto i : rv::iota(0, 10)) {
            expect(that % queue.front() == i);
            expect(that % queue.pop() == i);
        }
        expect(queue.empty());
    };

    "Queue with DoublyLinkedList backend should be able to push and pop"_test = [] {
        dsa::Queue<dsa::DoublyLinkedList, NonTrivial> queue;

        for (auto i : rv::iota(0, 10)) {
            auto v = queue.push(i);
            expect(that % v == i);
            expect(that % queue.back() == i);
        }

        expect(queue.size() == 10_i);

        for (auto i : rv::iota(0, 10)) {
            expect(that % queue.front() == i);
            expect(that % queue.pop() == i);
        }
        expect(queue.empty());
    };

    "Queue with CircularBuffer backend should be able to push and pop"_test = [] {
        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ReplaceOldest,
        };
        dsa::Queue<dsa::CircularBuffer, NonTrivial> queue{ 10, policy };

        for (auto i : rv::iota(0, 10)) {
            auto v = queue.push(i);
            expect(that % v == i);
            expect(that % queue.back() == i);
        }

        expect(queue.size() == 10_i);

        for (auto i : rv::iota(0, 10)) {
            expect(that % queue.front() == i);
            expect(that % queue.pop() == i);
        }
        expect(queue.empty());
    };
}
