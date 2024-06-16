#pragma once

// NOTE: Stack implementation based on reference 1

#include <concepts>
#include <utility>

namespace dsa
{
    template <template <typename> typename C, typename T>
    concept StackCompatible = requires(C<T> c, const C<T> cc, T&& t) {
        { cc.back() } -> std::same_as<const T&>;
        { cc.size() } -> std::same_as<std::size_t>;
        { c.back() } -> std::same_as<T&>;
        { c.push_back(std::move(t)) } -> std::same_as<T&>;
        { c.pop_back() } -> std::same_as<T>;
    };

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
        T&          push(T&& t) { return m_container.push_back(std::move(t)); }
        T           pop() { return m_container.pop_back(); }
        T&          top() { return m_container.back(); }
        const T&    top() const { return m_container.back(); }

        Container&       underlying() { return m_container; }
        const Container& underlying() const { return m_container; }

    private:
        Container m_container;
    };
}
