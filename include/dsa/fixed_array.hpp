#pragma once

#include "dsa/common.hpp"

#include <concepts>
#include <memory>
#include <type_traits>

namespace dsa
{
    template <typename T>
    concept FixedArrayElement = std::movable<T> or std::copyable<T>;

    template <FixedArrayElement T>
    class FixedArray
    {
    public:
        using Element = T;

        using value_type = Element;    // STL compliance

        FixedArray()  = default;
        ~FixedArray() = default;

        FixedArray(FixedArray&& other) noexcept            = default;
        FixedArray& operator=(FixedArray&& other) noexcept = default;

        FixedArray(const FixedArray&)            = delete;
        FixedArray& operator=(const FixedArray&) = delete;

        template <typename... Ts>
            requires(sizeof...(Ts) > 0) and (std::same_as<std::decay_t<Ts>, T> and ...)
        FixedArray(Ts&&... args)
            : m_data{ new T[sizeof...(args)]{ std::forward<Ts>(args)... } }
            , m_size{ sizeof...(args) }
        {
        }

        static FixedArray sized(std::size_t size)
            requires std::default_initializable<T>
        {
            return { SizedTag{}, size };
        }

        auto*  data(this auto&& self) noexcept { return &self.at(0); }
        auto&& at(this auto&& self, std::size_t pos) noexcept { return derefConst<T>(self.m_data, pos); }

        auto* begin(this auto&& self) { return &self.at(0); }
        auto* end(this auto&& self) { return &self.at(self.m_size); }

        const T* cbegin() { return begin(); }
        const T* cend() { return end(); }

        std::size_t size() const noexcept { return m_size; }

    private:
        struct SizedTag
        {
        };

        FixedArray(SizedTag, std::size_t size)
            : m_data{ new T[size]{} }
            , m_size{ size }
        {
        }

        std::unique_ptr<T[]> m_data = nullptr;
        std::size_t          m_size = 0;
    };
}
