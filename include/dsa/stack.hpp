#pragma once

// NOTE: Stack implementation based on reference 1

#include <concepts>
#include <utility>

#include "common.hpp"

namespace dsa
{
    template <template <typename> typename C, typename T>
    concept HasPushBackAndPopBack = requires(C<T> c, T&& t) {
        { c.push_back(std::move(t)) } -> std::same_as<T&>;
        { c.pop_back() } -> std::same_as<T>;
    };

    template <template <typename> typename C, typename T>
    concept HasBackAccess = requires(C<T> c, const C<T> cc) {
        { cc.back() } -> std::same_as<const T&>;
        { c.back() } -> std::same_as<T&>;
    };

    template <template <typename> typename C, typename T>
    concept HasPushFrontAndPopFront = requires(C<T> c, T&& t) {
        { c.push_front(std::move(t)) } -> std::same_as<T&>;
        { c.pop_front() } -> std::same_as<T>;
    };

    template <template <typename> typename C, typename T>
    concept HasFrontAccess = requires(C<T> c, const C<T> cc) {
        { cc.front() } -> std::same_as<const T&>;
        { c.front() } -> std::same_as<T&>;
    };

    template <template <typename> typename C, typename T>
    concept FrontStackCompatible = HasPushFrontAndPopFront<C, T> and HasFrontAccess<C, T>
                               and HasSizeMethod<const C<T>>;

    template <template <typename> typename C, typename T>
    concept BackStackCompatible = HasPushBackAndPopBack<C, T> and HasBackAccess<C, T>
                              and HasSizeMethod<const C<T>>;

    template <template <typename> typename C, typename T>
    concept StackCompatible = FrontStackCompatible<C, T> or BackStackCompatible<C, T>;

    template <template <typename> typename C, typename T>
        requires StackCompatible<C, T>
    class Stack
    {
    public:
        using Container = C<T>;
        using Element   = T;

        template <typename... Args>
            requires std::constructible_from<Container, Args...>
        Stack(Args&&... args)
            : m_container(std::forward<Args>(args)...)
        {
        }

        bool        empty() const { return m_container.size() == 0; }
        std::size_t size() const { return m_container.size(); }

        auto&& underlying(this auto&& self) { return self.m_container; }

        auto&& top(this auto&& self)
        {
            if constexpr (FrontStackCompatible<C, T>) {
                return self.m_container.front();
            } else if constexpr (BackStackCompatible<C, T>) {
                return self.m_container.back();
            }
        }

        T& push(T&& t)
        {
            if constexpr (FrontStackCompatible<C, T>) {
                return m_container.push_front(std::move(t));
            } else if constexpr (BackStackCompatible<C, T>) {
                return m_container.push_back(std::move(t));
            }
        }

        T pop()
        {
            if constexpr (FrontStackCompatible<C, T>) {
                return m_container.pop_front();
            } else if constexpr (BackStackCompatible<C, T>) {
                return m_container.pop_back();
            }
        }

    private:
        Container m_container;
    };
}
