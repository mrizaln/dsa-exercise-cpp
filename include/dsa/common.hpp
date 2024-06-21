#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace dsa
{
    template <typename T, typename R = std::size_t>
    concept HasSizeMethod = requires(T t) {
        { t.size() } -> std::same_as<R>;
    };

    template <typename T, typename U>
    concept Dereferencable = requires(T t) {
        { *t } -> std::same_as<U>;
    };

    // dereference From while propagating the constness of From to the dereferenced type (To)
    template <typename To, typename From>
        requires Dereferencable<From, std::decay_t<To>&>
    decltype(auto) derefConst(From&& from)
    {
        if constexpr (std::is_const_v<std::remove_reference_t<From>>) {
            return std::as_const(*from);
        } else {
            return *from;
        }
    }
}
