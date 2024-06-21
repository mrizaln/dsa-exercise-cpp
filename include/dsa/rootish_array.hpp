#pragma once

// NOTE: RootishArray implementation based on reference 1 and reference 2 (ArrayQueue)

#include "dsa/array_list.hpp"
#include "dsa/common.hpp"

#include <cmath>
#include <concepts>
#include <iterator>
#include <stdexcept>

namespace dsa
{
    template <typename T>
    concept RootishArrayElement = std::movable<T> or std::copyable<T>;

    // NOTE: the class has 3 invariants:
    //  1. there is at most 1 empty block at the end. if there are more than 1, then the array will be shrunk.
    //  2. special case when the array is empty, then there is no block not even an empty one.
    //  3. the first push_back() will create 2 blocks; thus the 1st invariant is satisfied.
    //  4. blocks have a size of 1, 2, 3, 4, 5, ..., b.
    template <RootishArrayElement T>
    class RootishArray
    {
    public:
        template <bool IsConst>
        class [[nodiscard]] Iterator;

        friend class Iterator<false>;
        friend class Iterator<true>;

        using Element    = T;
        using value_type = Element;    // STL compliance

        using Block = ArrayList<T>;

        RootishArray()  = default;
        ~RootishArray() = default;

        RootishArray(RootishArray&&) noexcept            = default;
        RootishArray& operator=(RootishArray&&) noexcept = default;

        RootishArray(const RootishArray&)            = default;
        RootishArray& operator=(const RootishArray&) = default;

        void swap(RootishArray& other) noexcept;
        void clear() noexcept;

        T& insert(std::size_t pos, T&& element);
        T  remove(std::size_t pos);

        T& push_back(T&& value) { return insert(size(), std::move(value)); }
        T  pop_back() { return remove(size() - 1); }

        auto&& at(this auto&& self, std::size_t pos);
        auto&& block(this auto&& self, std::size_t pos) { return self.m_blocks.at(pos); }

        auto&& front(this auto&& self) { return self.at(0); }
        auto&& back(this auto&& self) { return self.at(self.size() - 1); }

        std::size_t size() const noexcept;
        const auto& blocks() const noexcept { return m_blocks; }

        auto begin(this auto&& self) noexcept { return makeIter<Iterator, decltype(self)>(&self, 0uz); }
        auto end(this auto&& self) noexcept { return makeIter<Iterator, decltype(self)>(&self, npos); }

        Iterator<true> cbegin() const noexcept { return begin(); }
        Iterator<true> cend() const noexcept { return begin(); }

    private:
        struct ElementIndex
        {
            std::size_t block = 0;
            std::size_t index = 0;    // local to the block
        };

        static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

        ArrayList<Block> m_blocks = {};

        ElementIndex elementIndex(std::size_t pos) const noexcept;
        auto&&       nearEmpty(this auto&& self) { return self.block(self.m_blocks.size() - 2); }

        void grow();
        void shrink();
    };
}

// -----------------------------------------------------------------------------
// implementation detail
// -----------------------------------------------------------------------------

namespace dsa
{
    template <RootishArrayElement T>
    void RootishArray<T>::swap(RootishArray& other) noexcept
    {
        m_blocks.swap(other.m_blocks);
    }

    template <RootishArrayElement T>
    void RootishArray<T>::clear() noexcept
    {
        m_blocks.clear();
    }

    template <RootishArrayElement T>
    T& RootishArray<T>::insert(std::size_t pos, T&& value)
    {
        if (pos > size()) {
            throw std::out_of_range{
                std::format("Cannot insert element at position greater than size ({} > {})", pos, size())
            };
        }

        // NOTE: 3rd invariant
        if (size() == 0) {
            grow();
            grow();

            return block(0).push_back(std::move(value));
        }

        if (nearEmpty().size() == nearEmpty().capacity()) {
            grow();
        }

        auto [blockIdx, localIdx] = elementIndex(pos);

        for (auto i = m_blocks.size() - 2; i > blockIdx; --i) {
            Block& current = block(i);
            Block& prev    = block(i - 1);

            current.insert(0, prev.pop_back());
        }

        return block(blockIdx).insert(localIdx, std::move(value));
    }

    template <RootishArrayElement T>
    T RootishArray<T>::remove(std::size_t pos)
    {
        if (pos >= size()) {
            throw std::out_of_range{ "Cannot remove element at position greater than or equal to size" };
        }

        auto [blockIdx, localIdx] = elementIndex(pos);
        auto value                = block(blockIdx).remove(localIdx);

        for (auto i = blockIdx; i < m_blocks.size() - 2; ++i) {
            Block& current = block(i);
            Block& next    = block(i + 1);

            current.push_back(next.remove(0));
        }

        // NOTE: 3rd invariant means the minimum block size is 2 when not empty
        if (nearEmpty().size() == 0 and m_blocks.size() > 2) {
            shrink();
        }

        return value;
    }

    template <RootishArrayElement T>
    auto&& RootishArray<T>::at(this auto&& self, std::size_t pos)
    {
        auto [block, local] = self.elementIndex(pos);
        return self.block(block).at(local);
    }

    template <RootishArrayElement T>
    std::size_t RootishArray<T>::size() const noexcept
    {
        if (m_blocks.size() == 0) {
            return 0;
        }

        auto numBlocks     = m_blocks.size() - 1;
        auto lastBlockSize = nearEmpty().size();

        return (numBlocks * (numBlocks + 1) / 2) - (numBlocks - lastBlockSize);
    }

    // NOTE: see reference 2
    template <RootishArrayElement T>
    RootishArray<T>::ElementIndex RootishArray<T>::elementIndex(std::size_t pos) const noexcept
    {
        auto blockApprox = (-3 + std::sqrt(9 + 8 * pos)) / 2;
        auto block       = static_cast<std::size_t>(std::ceil(blockApprox));

        auto local = pos - block * (block + 1) / 2;

        return { block, local };
    }

    template <RootishArrayElement T>
    void RootishArray<T>::grow()
    {
        auto& block = m_blocks.push_back({});
        block.reserve(m_blocks.size());
    }

    template <RootishArrayElement T>
    void RootishArray<T>::shrink()
    {
        std::ignore = m_blocks.pop_back();
    }

    template <RootishArrayElement T>
    template <bool IsConst>
    class RootishArray<T>::Iterator
    {
    public:
        // STL compatibility
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = typename RootishArray::Element;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference         = std::conditional_t<IsConst, const value_type&, value_type&>;

        using ArrayPtr = std::conditional_t<IsConst, const RootishArray*, RootishArray*>;

        Iterator() noexcept                      = default;
        Iterator(const Iterator&)                = default;
        Iterator& operator=(const Iterator&)     = default;
        Iterator(Iterator&&) noexcept            = default;
        Iterator& operator=(Iterator&&) noexcept = default;

        Iterator(ArrayPtr array, std::size_t pos) noexcept
            : m_array{ array }
            , m_pos{ pos }
            , m_size{ array->size() }
        {
        }

        // for const iterator construction from iterator
        Iterator(Iterator<false>& other)
            : m_array{ other.m_array }
            , m_pos{ other.m_pos }
            , m_size{ other.m_size }
        {
        }

        // just a pointer comparison
        auto operator<=>(const Iterator&) const = default;

        Iterator& operator+=(difference_type n)
        {
            // casted n possibly become very large if it was negative, but when it was added to m_pos, m_pos
            // will wraparound anyway since it was unsigned
            m_pos += static_cast<std::size_t>(n);

            // detect wrap-around then set to sentinel value
            if (m_pos >= m_size) {
                m_pos = RootishArray::npos;
            }

            return *this;
        }

        Iterator& operator-=(difference_type n)
        {
            // to allow reverse iteration
            if (m_pos == RootishArray::npos) {
                m_pos = m_size;
            }

            return *this += -n;
        }

        Iterator& operator++() { return (*this) += 1; }
        Iterator& operator--() { return (*this) -= 1; }

        Iterator operator++(int)
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        Iterator operator--(int)
        {
            auto copy = *this;
            --(*this);
            return copy;
        }

        reference operator*() const
        {
            if (m_array == nullptr || m_pos == RootishArray::npos) {
                throw std::out_of_range{ "Iterator is out of range" };
            }
            return m_array->at(m_pos);
        }

        pointer operator->() const
        {
            if (m_array == nullptr || m_pos == RootishArray::npos) {
                throw std::out_of_range{ "Iterator is out of range" };
            }
            return &m_array->at(m_pos);
        }

        reference operator[](difference_type n) const { return *(*this + n); }

        friend Iterator operator+(const Iterator& lhs, difference_type n) { return auto{ lhs } += n; }
        friend Iterator operator+(difference_type n, const Iterator& rhs) { return rhs + n; }
        friend Iterator operator-(const Iterator& lhs, difference_type n) { return auto{ lhs } -= n; }

        friend difference_type operator-(const Iterator& lhs, const Iterator& rhs)
        {
            auto lpos = lhs.m_pos == RootishArray::npos ? lhs.m_size : lhs.m_pos;
            auto rpos = rhs.m_pos == RootishArray::npos ? rhs.m_size : rhs.m_pos;

            return static_cast<difference_type>(lpos) - static_cast<difference_type>(rpos);
        }

    private:
        ArrayPtr    m_array = nullptr;
        std::size_t m_pos   = RootishArray::npos;
        std::size_t m_size  = 0;
    };
}
