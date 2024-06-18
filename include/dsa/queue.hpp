#pragma once

// NOTE: Queue implementation based on reference 1

#include <concepts>
#include <utility>

#include "common.hpp"

namespace dsa
{
    template <template <typename> typename C, typename T>
    concept HasPushBackAndPopFront = requires(C<T> c, T&& t) {
        { c.push_back(std::move(t)) } -> std::same_as<T&>;
        { c.pop_front() } -> std::same_as<T>;
    };

    template <template <typename> typename C, typename T>
    concept HasPushFrontAndPopBack = requires(C<T> c, T&& t) {
        { c.push_front(std::move(t)) } -> std::same_as<T&>;
        { c.pop_back() } -> std::same_as<T>;
    };

    template <template <typename> typename C, typename T>
    concept HasFrontAndBackAccess = requires(C<T> c, const C<T> cc) {
        { cc.back() } -> std::same_as<const T&>;
        { cc.front() } -> std::same_as<const T&>;
        { c.back() } -> std::same_as<T&>;
        { c.front() } -> std::same_as<T&>;
    };

    template <template <typename> typename C, typename T>
    concept QueueCompatible = (HasPushBackAndPopFront<C, T> or HasPushFrontAndPopBack<C, T>)
                          and HasFrontAndBackAccess<C, T> and HasSizeMethod<const C<T>>;

    template <template <typename> typename C, typename T>
        requires QueueCompatible<C, T>
    class Queue
    {
    public:
        using Container = C<T>;
        using Element   = T;

        template <typename... Args>
            requires std::constructible_from<Container, Args...>
        Queue(Args&&... args)
            : m_container(std::forward<Args>(args)...)
        {
        }

        bool        empty() const { return m_container.size() == 0; }
        std::size_t size() const { return m_container.size(); }

        auto&& front(this auto&& self) { return self.m_container.front(); }
        auto&& back(this auto&& self) { return self.m_container.back(); }
        auto&& underlying(this auto&& self) { return self.m_container; }

        T& push(T&& t)
        {
            if constexpr (HasPushBackAndPopFront<C, T>) {
                return m_container.push_back(std::move(t));
            } else if constexpr (HasPushFrontAndPopBack<C, T>) {
                return m_container.push_front(std::move(t));
            }
        }

        T pop()
        {
            if constexpr (HasPushBackAndPopFront<C, T>) {
                return m_container.pop_front();
            } else if constexpr (HasPushFrontAndPopBack<C, T>) {
                return m_container.pop_back();
            }
        }

    private:
        Container m_container;
    };
}
