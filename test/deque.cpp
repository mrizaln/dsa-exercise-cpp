#include "test_util.hpp"

#include <dsa/deque.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <cassert>
#include <ranges>

namespace ut = boost::ut;
namespace rr = std::ranges;
namespace rv = rr::views;

template <typename Deque>
std::string compare(const Deque& deque)
{
    const auto&& [front, back] = deque.underlying();
    return fmt::format("f: {} vs b: {}", front.underlying(), back.underlying());
}

// TODO: check whether copy happens on operations that should not copy (unless type is not movable, then copy
// should happen)
template <test_util::TestClass Type>
void test()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    Type::resetActiveInstanceCount();

    "Deque should be able to be constructed with a backend"_test = [] {
        dsa::Deque<Type> deque{};    //
    };

    "Deque with ArrayList backend should be able to push and pop"_test = [] {
        dsa::Deque<Type> deque{};    // dynamic capacity now

        {    // push_back and pop_back
            for (auto i : rv::iota(0, 10)) {
                deque.push_back(i);
            }
            expect(deque.size() == 10_u) << compare(deque);

            for (auto i : rv::iota(0, 10) | rv::reverse) {
                expect(that % deque.front().value() == 0) << "front value should be unchanged";
                expect(that % deque.back().value() == i);
                expect(that % deque.pop_back().value() == i);
            }
        }

        {    // push_front and pop_front
            for (auto i : rv::iota(0, 10)) {
                deque.push_front(i);
            }
            expect(deque.size() == 10_u);

            for (auto i : rv::iota(0, 10) | rv::reverse) {
                expect(that % deque.back().value() == 0) << "back value should be unchanged";
                expect(that % deque.front().value() == i);
                expect(that % deque.pop_front().value() == i);
            }
        }

        {    // push_back and pop_front
            for (auto i : rv::iota(0, 10)) {
                deque.push_back(i);
            }
            expect(deque.size() == 10_u);

            for (auto i : rv::iota(0, 10)) {
                expect(that % deque.back().value() == 9) << "back value should be unchanged";
                expect(that % deque.front().value() == i);
                expect(that % deque.pop_front().value() == i);
            }
        }

        {    // push_front and pop_back
            for (auto i : rv::iota(0, 10)) {
                deque.push_front(i);
            }
            expect(deque.size() == 10_u);

            for (auto i : rv::iota(0, 10)) {
                expect(that % deque.front().value() == 9) << "front value should be unchanged";
                expect(that % deque.back().value() == i);
                expect(that % deque.pop_back().value() == i);
            }
        }
    };

    // TODO: add tests for exceptional cases (e.g. pop from empty deque, push to full deque, etc.)

    // unbalanced constructor/destructor means there is a bug in the code
    assert(Type::activeInstanceCount() == 0);
}

int main()
{
#ifdef DSA_TEST_EXTRA_TYPES
    test_util::forEach<test_util::NonTrivialPermutations>([]<typename T>() {
        if constexpr (dsa::DequeElement<T>) {
            test<T>();
        }
    });
#else
    test<test_util::Regular>();
    test<test_util::MovableOnly<>>();
    test<test_util::CopyableOnly<>>();
#endif
}
