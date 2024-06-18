#include "test_util.hpp"

#include <dsa/stack.hpp>
#include <dsa/circular_buffer.hpp>
#include <dsa/array_list.hpp>
#include <dsa/linked_list.hpp>
#include <dsa/doubly_linked_list.hpp>

#include <boost/ut.hpp>

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

    "ArrayList, LinkedList, and DoublyLinkedList should be able to be used as Stack backend"_test = [] {
        static_assert(dsa::StackCompatible<dsa::ArrayList, Type>);
        static_assert(dsa::StackCompatible<dsa::LinkedList, Type>);
        static_assert(dsa::StackCompatible<dsa::DoublyLinkedList, Type>);
    };

    "CircularBuffer should not be able to be used as Stack backend"_test = [] {
        static_assert(not dsa::StackCompatible<dsa::CircularBuffer, Type>);
    };

    "Stack should be able to be constructed with a backend"_test = [] {
        dsa::Stack<dsa::ArrayList, Type>        stack{ 10 };    // limited capacity
        dsa::Stack<dsa::LinkedList, Type>       stack2;
        dsa::Stack<dsa::DoublyLinkedList, Type> stack3;
    };

    "Stack with ArrayList backend should be able to push and pop"_test = [] {
        dsa::Stack<dsa::ArrayList, Type> stack{ 10 };

        for (auto i : rv::iota(0, 10)) {
            stack.push(i);
        }

        expect(stack.size() == 10_i);
        expect(throws([&] { stack.push(10); }));

        for (auto i : rv::iota(0, 10) | rv::reverse) {
            expect(that % stack.top().value() == i);

            if (Type::s_movable) {
                expect(that % stack.top().stat().nocopy())
                    << fmt::format("top() should not copy: {}", stack.top().stat());
            }

            auto value = stack.pop();

            expect(that % value.value() == i);
            if (Type::s_movable) {
                expect(that % value.stat().nocopy())    //
                    << fmt::format("pop() should not copy: {}", value.stat());
            }
        }
        expect(stack.empty()) << "stack should be empty after popping all elements";
    };

    // NOTE: Stack with LinkedList backend is incredibly inefficient since its pop_back is linear time [O(n)]
    "Stack with LinkedList backend should be able to push and pop"_test = [] {
        dsa::Stack<dsa::LinkedList, Type> stack;

        for (auto i : rv::iota(0, 10)) {
            stack.push(i);
        }

        expect(stack.size() == 10_i);

        for (auto i : rv::iota(0, 10) | rv::reverse) {
            expect(that % stack.top().value() == i);    // just a view
            expect(that % stack.pop().value() == i);
        }
        expect(stack.empty()) << "stack should be empty after popping all elements";
    };

    "Stack with DoublyLinkedList backend should be able to push and pop"_test = [] {
        dsa::Stack<dsa::DoublyLinkedList, Type> stack;

        for (auto i : rv::iota(0, 10)) {
            stack.push(i);
        }

        expect(stack.size() == 10_i);

        for (auto i : rv::iota(0, 10) | rv::reverse) {
            expect(that % stack.top().value() == i);    // just a view
            expect(that % stack.pop().value() == i);
        }
        expect(stack.empty()) << "stack should be empty after popping all elements";
    };
}

int main()
{
    // main types
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
