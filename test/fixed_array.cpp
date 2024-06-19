#include "test_util.hpp"

#include <concepts>
#include <dsa/fixed_array.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>

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
}

int main()
{
    // main types
    test<test_util::Regular>();
    test<test_util::MovableOnly<>>();
    test<test_util::CopyableOnly<>>();

    // extra
    test_util::forEachType<test_util::NonTrivialPermutations>([]<typename T>() {
        if constexpr (dsa::FixedArrayElement<T>) {
            test<T>();
        }
    });
}
