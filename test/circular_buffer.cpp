#include <dsa/circular_buffer.hpp>

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

auto subrange(auto&& range, std::size_t start, std::size_t end)
{
    return range | rv::drop(start) | rv::take(end - start);
}

template <typename T, std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, T>
void populateBuffer(dsa::CircularBuffer<T>& buffer, R&& range)
{
    for (auto value : range) {
        buffer.push_back(std::move(value));
    }
}

template <typename T>
std::string compare(const dsa::CircularBuffer<T>& buffer)
{
    return fmt::format("{} vs u {}", buffer, std::span{ buffer.data(), buffer.size() });
}

constexpr auto g_policyPermutations = std::tuple{
    dsa::BufferPolicy{ dsa::BufferCapacityPolicy::FixedCapacity, dsa::BufferStorePolicy::ReplaceOldest },
    dsa::BufferPolicy{ dsa::BufferCapacityPolicy::DynamicCapacity, dsa::BufferStorePolicy::ReplaceOldest },
    dsa::BufferPolicy{ dsa::BufferCapacityPolicy::FixedCapacity, dsa::BufferStorePolicy::ThrowOnFull },
    dsa::BufferPolicy{ dsa::BufferCapacityPolicy::DynamicCapacity, dsa::BufferStorePolicy::ThrowOnFull },
};

int main()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    "iterator should be a forward iterator"_test = [] {
        using Iter = dsa::CircularBuffer<NonTrivial>::Iterator<false>;
        static_assert(std::forward_iterator<Iter>);

        using ConstIter = dsa::CircularBuffer<NonTrivial>::Iterator<true>;
        static_assert(std::forward_iterator<ConstIter>);
    };

    "push_back should add an element to the back"_test = [](dsa::BufferPolicy policy) {
        dsa::CircularBuffer<NonTrivial> buffer{ 10, policy };    // default policy

        // first push
        auto size  = buffer.size();
        auto value = buffer.push_back(42);
        expect(that % buffer.size() == size + 1);
        expect(value == 42_i);
        expect(buffer.back() == 42_i);
        expect(buffer.front() == 42_i);

        for (auto i : rv::iota(0, 9)) {
            value = buffer.push_back(i);
            expect(that % value == i);
            expect(that % buffer.back() == i) << compare(buffer);
            expect(that % buffer.front() == 42);
        }

        std::array<NonTrivial, 10> expected{ 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        expect(rr::equal(buffer, expected));

        buffer.clear();
        expect(buffer.size() == 0_i);

        rr::fill_n(std::back_inserter(buffer), 10, 42);
        for (const auto& value : buffer) {
            expect(value == 42_i);
        }
        expect(buffer.size() == 10_i);
    } | g_policyPermutations;

    "push_back with ReplaceOldest policy should replace the oldest element when buffer is full"_test = [] {
        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ReplaceOldest,
        };
        dsa::CircularBuffer<NonTrivial> buffer{ 10, policy };

        // fully populate buffer
        populateBuffer(buffer, rv::iota(0, 10));
        expect(buffer.size() == 10_i);
        expect(that % buffer.size() == buffer.capacity());
        expect(rr::equal(buffer, rv::iota(0, 10)));

        // replace old elements (4 times)
        for (auto i : rv::iota(21, 25)) {
            auto value = buffer.push_back(i);
            expect(that % value == i);
            expect(that % buffer.back() == i);
            expect(buffer.capacity() == 10_i);
            expect(buffer.size() == 10_i);
        }

        // the circular-buffer iterator
        expect(rr::equal(subrange(buffer, 0, 6), rv::iota(0, 10) | rv::drop(4)));
        expect(rr::equal(subrange(buffer, 6, 10), rv::iota(21, 25))) << compare(buffer);

        // the underlying array
        auto underlying = std::span{ buffer.data(), buffer.capacity() };
        expect(rr::equal(subrange(underlying, 0, 4), rv::iota(21, 25))) << compare(buffer);
        expect(rr::equal(subrange(underlying, 4, 10), rv::iota(0, 10) | rv::drop(4))) << compare(buffer);
    };

    "push_back with ThrowOnFull policy should throw when buffer is full"_test = [] {
        dsa::BufferPolicy policy{
            .m_capacity = dsa::BufferCapacityPolicy::FixedCapacity,
            .m_store    = dsa::BufferStorePolicy::ThrowOnFull,
        };
        dsa::CircularBuffer<NonTrivial> buffer{ 10, policy };

        // fully populate buffer
        populateBuffer(buffer, rv::iota(0, 10));
        expect(buffer.size() == 10_i);
        expect(that % buffer.size() == buffer.capacity());
        expect(rr::equal(buffer, rv::iota(0, 10)));

        expect(throws([&] { buffer.push_back(42); })) << "should throw when push to full buffer";
    };

    "push_back with DynamicCapacity should never replace old elements nor throws on a full buffer"_test =
        [](dsa::BufferStorePolicy storePolicy) {
            dsa::BufferPolicy throwOnFullPolicy{
                .m_capacity = dsa::BufferCapacityPolicy::DynamicCapacity,
                .m_store    = storePolicy,
            };
            dsa::CircularBuffer<NonTrivial> buffer2{ 10, throwOnFullPolicy };

            // fully populate buffer
            populateBuffer(buffer2, rv::iota(0, 10));
            expect(buffer2.size() == 10_i);
            expect(that % buffer2.size() == buffer2.capacity());
            expect(rr::equal(buffer2, rv::iota(0, 10)));

            expect(nothrow([&] { buffer2.push_back(42); })) << "should not throw when push to full buffer";
            expect(buffer2.back() == 42_i);
            expect(buffer2.size() == 11_i);
            expect(buffer2.capacity() > 10_i);
            expect(rr::equal(subrange(buffer2, 0, 10), rv::iota(0, 10)));
        }
        | std::tuple{ dsa::BufferStorePolicy::ReplaceOldest, dsa::BufferStorePolicy::ThrowOnFull };

    "pop_front should remove the first element on the buffer"_test = [](dsa::BufferPolicy policy) {
        std::vector<NonTrivial>         values = { 42, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        dsa::CircularBuffer<NonTrivial> buffer{ 10, policy };
        for (auto value : values) {
            buffer.push_back(std::move(value));
        }
        expect(that % buffer.size() == values.size());

        // first pop
        auto size  = buffer.size();
        auto value = buffer.pop_front();
        expect(that % buffer.size() == size - 1);
        expect(value == 42_i);

        for (auto i : rv::iota(0, 8)) {
            expect(buffer.pop_front() == i);
        }
        expect(buffer.size() == 1_i);
        expect(buffer.pop_front() == 8_i);
        expect(buffer.size() == 0_i);

        expect(throws([&] { buffer.pop_front(); })) << "should throw when pop from empty buffer";
    } | g_policyPermutations;

    // TODO: add test for pop_front on DynamicCapacity policy; check when will it double or halve its capacity

    "default initialized CircularBuffer is basically useless"_test = [] {
        dsa::CircularBuffer<int> buffer;
        expect(buffer.size() == 0_i);
        expect(buffer.capacity() == 0_i);
        expect(throws([&] { buffer.push_back(42); })) << "should throw when push to empty buffer";
        expect(throws([&] { buffer.pop_front(); })) << "should throw when pop from empty buffer";
    };

    // TODO: add test for edge case: 1 digit capacity

    "move should leave buffer into an empty state that is not usable"_test = [] {
        dsa::CircularBuffer<NonTrivial> buffer{ 20 };
        populateBuffer(buffer, rv::iota(0, 10));

        expect(buffer.size() == 10_i);
        expect(rr::equal(buffer, rv::iota(0, 10)));

        auto buffer2 = std::move(buffer);
        expect(buffer.size() == 0_i);
        expect(buffer.capacity() == 0_i);
        expect(throws([&] { buffer.push_back(42); })) << "should throw when push to empty buffer";
        expect(throws([&] { buffer.pop_front(); })) << "should throw when pop from empty buffer";
    };

    "copy should copy each element exactly"_test = [] {
        dsa::CircularBuffer<NonTrivial> buffer{ 20 };
        populateBuffer(buffer, rv::iota(0, 10));

        expect(buffer.size() == 10_i);
        expect(rr::equal(buffer, rv::iota(0, 10)));

        auto buffer2 = buffer;
        expect(buffer2.size() == 10_i);
        expect(rr::equal(buffer2, buffer));

        auto buffer3 = buffer2;
        expect(buffer3.size() == 10_i);
        expect(rr::equal(buffer3, buffer));
    };
}
