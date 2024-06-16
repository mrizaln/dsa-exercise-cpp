#include <dsa/deque.hpp>

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

template <typename Deque>
std::string compare(const Deque& deque)
{
    const auto&& [front, back] = deque.underlying();
    return fmt::format("f: {} vs b: {}", front.underlying(), back.underlying());
}

int main()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    "Deque should be able to be constructed with a backend"_test = [] {
        dsa::Deque<NonTrivial> deque{ 10 };    // 10 for each front and back stack
    };

    "Deque with ArrayList backend should be able to push and pop"_test = [] {
        dsa::Deque<NonTrivial> deque{ 10 };
        expect(deque.underlying().first.underlying().capacity() == 10_u);
        expect(deque.underlying().second.underlying().capacity() == 10_u);

        {    // push_back and pop_back
            for (auto i : rv::iota(0, 10)) {
                deque.push_back(i);
            }
            expect(deque.size() == 10_u) << compare(deque);

            for (auto i : rv::iota(0, 10) | rv::reverse) {
                expect(that % deque.front() == 0) << "front value should be unchanged";
                expect(that % deque.back() == i);
                expect(that % deque.pop_back() == i);
            }
        }

        {    // push_front and pop_front
            for (auto i : rv::iota(0, 10)) {
                deque.push_front(i);
            }
            expect(deque.size() == 10_u);

            for (auto i : rv::iota(0, 10) | rv::reverse) {
                expect(that % deque.back() == 0) << "back value should be unchanged";
                expect(that % deque.front() == i);
                expect(that % deque.pop_front() == i);
            }
        }

        {    // push_back and pop_front
            for (auto i : rv::iota(0, 10)) {
                deque.push_back(i);
            }
            expect(deque.size() == 10_u);

            for (auto i : rv::iota(0, 10)) {
                expect(that % deque.back() == 9) << "back value should be unchanged";
                expect(that % deque.front() == i);
                expect(that % deque.pop_front() == i);
            }
        }

        {    // push_front and pop_back
            for (auto i : rv::iota(0, 10)) {
                deque.push_front(i);
            }
            expect(deque.size() == 10_u);

            for (auto i : rv::iota(0, 10)) {
                expect(that % deque.front() == 9) << "front value should be unchanged";
                expect(that % deque.back() == i);
                expect(that % deque.pop_back() == i);
            }
        }
    };

    // TODO: add tests for exceptional cases (e.g. pop from empty deque, push to full deque, etc.)
}
