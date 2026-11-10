#pragma once

/**
 * \file SmallVector.hpp
 * \brief Fixed-capacity inline-storage vector with heap fallback.
 */

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <vector>

namespace ZenitUI
{

/**
 * \class SmallVector
 * \tparam T Element type. Must be default-constructible for the inline array.
 * \tparam N Number of elements stored inline before a heap allocation is made.
 *
 * \brief A vector whose first \p N elements live inside the object.
 *
 * Behaves like `std::vector<T>` but avoids a heap allocation for small
 * payloads (typically 2–8 elements). When `size()` exceeds \p N, the inline
 * contents are moved into an internal `std::vector` and all further growth
 * uses that heap buffer.
 *
 * The `inline_` array is always present, so the object size is at least
 * `N * sizeof(T)`. Choose \p N accordingly.
 *
 * \note Copy and move operations are defaulted and forward to the inline array
 *       and the internal heap vector as appropriate. The inline → heap
 *       promotion is not exception-safe against throwing move constructors
 *       of `T`; for trivially-copyable element types (the intended use case)
 *       this is irrelevant.
 *
 * \warning Iterators returned by `begin()`/`end()` are invalidated whenever
 *          the vector transitions from inline to heap storage, and on the
 *          usual heap reallocation.
 */
template <typename T, std::size_t N>
class SmallVector
{
public:
    SmallVector() = default;

    /** \brief Construct from an initializer list. */
    SmallVector(std::initializer_list<T> init)
    {
        reserve(init.size());
        for (const auto &t : init)
            push_back(t);
    }

    SmallVector(const SmallVector &) = default;
    SmallVector(SmallVector &&) noexcept = default;
    SmallVector &operator=(const SmallVector &) = default;
    SmallVector &operator=(SmallVector &&) noexcept = default;
    ~SmallVector() = default;

    /** \brief Replace contents with the given initializer list. */
    SmallVector &operator=(std::initializer_list<T> init)
    {
        clear();
        reserve(init.size());
        for (const auto &t : init)
            push_back(t);
        return *this;
    }

    /** \brief Number of elements currently stored. */
    std::size_t size()  const { return size_; }
    /** \brief `true` when there are no elements. */
    bool        empty() const { return size_ == 0; }

    /** \brief Remove all elements. Does not free the heap buffer if one exists. */
    void clear()
    {
        size_ = 0;
        heap_.clear();
    }

    /**
     * \brief Reserve storage for at least \p n elements.
     *
     * Values `n <= N` are no-ops: inline storage is already sufficient.
     */
    void reserve(std::size_t n)
    {
        if (n > N)
            heap_.reserve(n);
    }

    /** \brief Append a copy of \p value. Triggers inline → heap promotion on overflow. */
    void push_back(const T &value)
    {
        if (size_ < N)
        {
            inline_[size_++] = value;
            return;
        }
        if (size_ == N)
            heap_.assign(inline_, inline_ + N);
        heap_.push_back(value);
        ++size_;
    }

    /** \brief Append a moved \p value. */
    void push_back(T &&value)
    {
        if (size_ < N)
        {
            inline_[size_++] = std::move(value);
            return;
        }
        if (size_ == N)
        {
            heap_.assign(std::make_move_iterator(inline_),
                         std::make_move_iterator(inline_ + N));
        }
        heap_.push_back(std::move(value));
        ++size_;
    }

    T       &operator[](std::size_t i)       { return data()[i]; }
    const T &operator[](std::size_t i) const { return data()[i]; }

    T       *begin()       { return data(); }
    T       *end()         { return data() + size_; }
    const T *begin() const { return data(); }
    const T *end()   const { return data() + size_; }

    bool operator==(const SmallVector &o) const
    {
        if (size_ != o.size_)
            return false;
        for (std::size_t i = 0; i < size_; ++i)
            if (!((*this)[i] == o[i]))
                return false;
        return true;
    }
    bool operator!=(const SmallVector &o) const { return !(*this == o); }

private:
    /// \brief Returns the active storage: inline array or heap vector.
    T       *data()       { return size_ <= N ? inline_ : heap_.data(); }
    const T *data() const { return size_ <= N ? inline_ : heap_.data(); }

    T              inline_[N]{};  ///< Backing store while `size_ <= N`.
    std::vector<T> heap_;         ///< Backing store once promoted to heap.
    std::size_t    size_{0};      ///< Logical element count.
};

} // namespace ZenitUI