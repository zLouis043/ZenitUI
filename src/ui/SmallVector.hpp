#pragma once

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <vector>

namespace ZenitUI
{

// A container with N elements of inline storage, and a std::vector
// fallback used only when N is exceeded.
//
// Used by `Value` to avoid the heap allocation of std::vector for the
// common case of 1-2 terms (Px(8), calc(100% - 20px)). Once the
// container grows beyond N, it permanently switches to the vector and
// never returns to inline storage — `clear()` resets it back.
//
// This is intentionally NOT a general-purpose container. Known limits:
//
//   - `inline_` is default-initialized eagerly. For `Value::Term`
//     (a float and an enum) this is zero-cost; for a type with a
//     non-trivial default constructor it would construct N dummy
//     objects on every SmallVector instantiation. Don't reuse this
//     class for such types without addressing it.
//   - The initial switch to heap uses `assign`, which copies for
//     lvalues and moves for rvalues. Move-only types are supported
//     only in the rvalue `push_back`.
//   - No erase / insert / reverse iterators. The API is the minimal
//     subset of std::vector that `Value` actually uses.
//
// Size cost: with N=2, the class occupies ~48 bytes (16 bytes of
// inline storage + 24 bytes of std::vector + 8 bytes size). A plain
// std::vector would be 24 bytes. Since `ComputedStyle` has ~30
// `Value` fields, this costs ~720 bytes per ComputedStyle. In exchange
// it eliminates the heap allocation for every 0-2 term Value. The
// trade-off favors contiguous memory over allocator traffic.
template <typename T, std::size_t N>
class SmallVector
{
public:
    SmallVector() = default;

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

    SmallVector &operator=(std::initializer_list<T> init)
    {
        clear();
        reserve(init.size());
        for (const auto &t : init)
            push_back(t);
        return *this;
    }

    std::size_t size()  const { return size_; }
    bool        empty() const { return size_ == 0; }

    void clear()
    {
        size_ = 0;
        heap_.clear();
    }

    // `heap_` holds *all* elements once the container has switched to
    // the heap storage, not just the overflow. So reserve(n) reserves
    // for n elements in the vector, not n - N.
    void reserve(std::size_t n)
    {
        if (n > N)
            heap_.reserve(n);
    }

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
    T       *data()       { return size_ <= N ? inline_ : heap_.data(); }
    const T *data() const { return size_ <= N ? inline_ : heap_.data(); }

    T              inline_[N]{};
    std::vector<T> heap_;
    std::size_t    size_{0};
};

} // namespace ZenitUI