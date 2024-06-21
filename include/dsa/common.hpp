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
        { *t } -> std::same_as<U&>;
    };

    template <typename T, typename U>
    concept Indexable = requires(T t, std::size_t i) {
        { t[i] } -> std::same_as<U&>;
    };

    // dereference From while propagating the constness of From to the dereferenced type (To)
    template <typename To, typename From>
        requires Dereferencable<From, To>
    decltype(auto) derefConst(From&& from)
    {
        if constexpr (std::is_const_v<std::remove_reference_t<From>>) {
            return std::as_const(*from);
        } else {
            return *from;
        }
    }

    template <typename To, typename From>
        requires Indexable<From, To>
    decltype(auto) derefConst(From&& from, std::size_t i)
    {
        if constexpr (std::is_const_v<std::remove_reference_t<From>>) {
            return std::as_const(from[i]);
        } else {
            return from[i];
        }
    }

    template <template <bool IsConst> typename Iterator, typename Self, typename... Ts>
        requires std::constructible_from<Iterator<true>, Ts...>
              or std::constructible_from<Iterator<false>, Ts...>
    auto makeIter(Ts&&... ts)
    {
        if constexpr (std::is_const_v<std::remove_reference_t<Self>>) {
            return Iterator<true>{ std::forward<Ts>(ts)... };
        } else {
            return Iterator<false>{ std::forward<Ts>(ts)... };
        }
    }

    template <typename T, typename U>
    std::pair<T&, U&> makePairRef(T&& t, U&& u)
    {
        return { t, u };
    }
}
