#pragma once

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <vector>

namespace ZenitUI
{

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