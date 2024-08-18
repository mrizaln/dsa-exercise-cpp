#include "test_util.hpp"

#include <concepts>
#include <dsa/fixed_array.hpp>

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

    "fixed_array construction"_test = [] {
        if constexpr (std::default_initializable<Type>) {
            auto array = dsa::FixedArray<Type>::sized(10);
            Type defaulted;

            expect(array.size() == 10_u);
            for (auto i : rv::iota(0uz, array.size())) {
                auto& element = array.at(i);
                expect(that % element.value() == defaulted.value());
                expect(that % element.stat().nocopy());
                expect(that % element.stat().nomove());
                expect(that % element.stat().defaulted());
            }
        }

        std::array            values{ 11, 1220, 237, 1 };
        dsa::FixedArray<Type> array{ Type{ 11 }, Type{ 1220 }, Type{ 237 }, Type{ 1 } };

        expect(array.size() == 4_u);
        for (auto i : rv::iota(0uz, array.size())) {
            auto& element = array.at(i);
            expect(that % element.value() == values[i]);
            expect(that % element.stat().nocopy() or not Type::s_movable) << element.stat();
            expect(that % not element.stat().defaulted());
        }
    };

    // unbalanced constructor/destructor means there is a bug in the code
    assert(Type::activeInstanceCount() == 0);
}

int main()
{
#ifdef DSA_TEST_EXTRA_TYPES
    test_util::forEach<test_util::NonTrivialPermutations>([]<typename T>() {
        if constexpr (dsa::FixedArrayElement<T>) {
            test<T>();
        }
    });
#else
    test<test_util::Regular>();
    test<test_util::MovableOnly<>>();
    test<test_util::CopyableOnly<>>();
#endif
}
