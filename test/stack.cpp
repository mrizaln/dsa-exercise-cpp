#include <dsa/stack.hpp>
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

    "ArrayList, LinkedList, and DoublyLinkedList should be able to be used as Stack backend"_test = [] {
        static_assert(dsa::StackCompatible<dsa::ArrayList, NonTrivial>);
        static_assert(dsa::StackCompatible<dsa::LinkedList, NonTrivial>);
        static_assert(dsa::StackCompatible<dsa::DoublyLinkedList, NonTrivial>);
    };

    "CircularBuffer should not be able to be used as Stack backend"_test = [] {
        static_assert(not dsa::StackCompatible<dsa::CircularBuffer, NonTrivial>);
    };

    "Stack should be able to be constructed with a backend"_test = [] {
        dsa::Stack<dsa::ArrayList, NonTrivial>        stack{ 10 };    // limited capacity
        dsa::Stack<dsa::LinkedList, NonTrivial>       stack2;         // shouldn't be used; pop_back is O(n)
        dsa::Stack<dsa::DoublyLinkedList, NonTrivial> stack3;
    };

    "Stack with ArrayList backend should be able to push and pop"_test = [] {
        dsa::Stack<dsa::ArrayList, NonTrivial> stack{ 10 };

        for (auto i : rv::iota(0, 10)) {
            stack.push(i);
        }

        expect(stack.size() == 10_i);
        expect(throws([&] { stack.push(10); }));

        for (auto i : rv::iota(0, 10) | rv::reverse) {
            expect(that % stack.top() == i);    // just a view
            expect(that % stack.pop() == i);
        }
        expect(stack.empty()) << "stack should be empty after popping all elements";
    };

    // NOTE: Stack with LinkedList backend is incredibly inefficient since its pop_back is linear time [O(n)]
    "Stack with LinkedList backend should be able to push and pop"_test = [] {
        dsa::Stack<dsa::LinkedList, NonTrivial> stack;

        for (auto i : rv::iota(0, 10)) {
            stack.push(i);
        }

        expect(stack.size() == 10_i);

        for (auto i : rv::iota(0, 10) | rv::reverse) {
            expect(that % stack.top() == i);    // just a view
            expect(that % stack.pop() == i);
        }
        expect(stack.empty()) << "stack should be empty after popping all elements";
    };

    "Stack with DoublyLinkedList backend should be able to push and pop"_test = [] {
        dsa::Stack<dsa::DoublyLinkedList, NonTrivial> stack;

        for (auto i : rv::iota(0, 10)) {
            stack.push(i);
        }

        expect(stack.size() == 10_i);

        for (auto i : rv::iota(0, 10) | rv::reverse) {
            expect(that % stack.top() == i);    // just a view
            expect(that % stack.pop() == i);
        }
        expect(stack.empty()) << "stack should be empty after popping all elements";
    };
}
