#include "test_util.hpp"

#include <dsa/raw_buffer.hpp>

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
    using ut::expect, ut::that;

    Type::resetActiveInstanceCount();

    "nrvo should happen"_test = [] {
        dsa::RawBuffer<Type> buffer{ 10 };
        for (auto i : rv::iota(0uz, 10uz)) {
            buffer.construct(i, 10 - i + 1);
        }

        for (auto i : rv::iota(0uz, 10uz)) {
            auto&& value = buffer.at(i);
            expect(that % value.value() == 10 - i + 1);
            fmt::println("stat: {}", value.stat());
        }

        if constexpr (Type::s_movable or Type::s_copyable) {
            for (auto i : rv::iota(0uz, 10uz)) {
                auto value = std::move(buffer.at(i));
                expect(that % value.value() == 10 - i + 1);
                expect(that % value.stat().nocopy() or not Type::s_movable) << fmt::format(
                    "copy shouldn't be made on '{}': {}", ut::reflection::type_name<Type>(), value.stat()
                );
                buffer.destroy(i);
            }
        } else {
            for (auto i : rv::iota(0uz, 10uz)) {
                buffer.destroy(i);
            }
        }
    };

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
    test_util::forEachType<test_util::NonTrivialPermutations>([]<typename T>() { test<T>(); });
}
