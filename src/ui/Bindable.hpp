#pragma once

#include "Common.hpp"

namespace ZenitUI
{


    template <typename T>
    class Bindable
    {
    public:
        class Subscription
        {
        public:
            Subscription() = default;

            ~Subscription()
            {
                unsubscribe();
            }

            Subscription(Subscription &&o) noexcept
                : ctrl_(std::move(o.ctrl_)), token_(o.token_)
            {
                o.ctrl_.reset();
                o.token_ = 0;
            }

            Subscription &operator=(Subscription &&o) noexcept
            {
                if (this != &o)
                {
                    unsubscribe();
                    ctrl_ = std::move(o.ctrl_);
                    token_ = o.token_;
                    o.ctrl_.reset();
                    o.token_ = 0;
                }
                return *this;
            }

            Subscription(const Subscription &) = delete;
            Subscription &operator=(const Subscription &) = delete;

            void unsubscribe()
            {
                if (!ctrl_ || !ctrl_->alive_ || !ctrl_->owner_)
                    return;
                ctrl_->owner_->unsubscribe_(token_);
                ctrl_->owner_ = nullptr;
            }

            bool active() const
            {
                return ctrl_ && ctrl_->alive_ && ctrl_->owner_ && token_ != 0;
            }

        private:
            friend class Bindable;

            struct ControlBlock
            {
                Bindable *owner_{nullptr};
                bool alive_{true};
            };

            Subscription(std::shared_ptr<ControlBlock> ctrl, uint64_t token)
                : ctrl_(std::move(ctrl)), token_(token) {}

            std::shared_ptr<ControlBlock> ctrl_;
            uint64_t token_{0};
        };


        Bindable() = default;

        explicit Bindable(T initial)
            : value_(std::move(initial)) {}

        ~Bindable()
        {
            if (ctrl_)
                ctrl_->alive_ = false;
        }

        Bindable(const Bindable &) = delete;
        Bindable &operator=(const Bindable &) = delete;
        Bindable(Bindable &&) = delete;
        Bindable &operator=(Bindable &&) = delete;


        const T &get() const { return value_; }

        void set(T v)
        {
            if (v == value_)
                return;
            value_ = std::move(v);

            auto snapshot = subscribers_;
            for (const auto &entry : snapshot)
            {
                if (isStillSubscribed(entry.token))
                    entry.cb(value_);
            }
        }


        Subscription onChanged(std::function<void(const T &)> cb,
                               bool fireImmediately = false)
        {
            if (!ctrl_)
                ctrl_ = std::make_shared<typename Subscription::ControlBlock>();
            ctrl_->owner_ = this;

            uint64_t token = nextToken_++;
            subscribers_.push_back({token, std::move(cb)});

            Subscription sub(ctrl_, token);

            if (fireImmediately)
            {
                if (auto it = findEntry(token))
                    it->cb(value_);
            }

            return sub;
        }

        template <typename W, typename Arg>
        Subscription bindTo(std::weak_ptr<W> widget,
                            void (W::*method)(Arg),
                            bool fireImmediately = false)
        {
            return onChanged(
                [widget, method](const T &val)
                {
                    if (auto w = widget.lock())
                        (w.get()->*method)(val);
                },
                fireImmediately);
        }

        template <typename W, typename Arg>
        Subscription bindTo(std::shared_ptr<W> widget,
                            void (W::*method)(Arg),
                            bool fireImmediately = false)
        {
            return bindTo(std::weak_ptr<W>(widget), method, fireImmediately);
        }

    private:
        struct Entry
        {
            uint64_t token;
            std::function<void(const T &)> cb;
        };

        std::vector<Entry> subscribers_;
        std::shared_ptr<typename Subscription::ControlBlock> ctrl_;
        uint64_t nextToken_{1};
        T value_{};

        void unsubscribe_(uint64_t token)
        {
            subscribers_.erase(
                std::remove_if(
                    subscribers_.begin(), subscribers_.end(),
                    [token](const Entry &e)
                    { return e.token == token; }),
                subscribers_.end());
        }

        bool isStillSubscribed(uint64_t token) const
        {
            for (const auto &e : subscribers_)
                if (e.token == token)
                    return true;
            return false;
        }

        Entry *findEntry(uint64_t token)
        {
            for (auto &e : subscribers_)
                if (e.token == token)
                    return &e;
            return nullptr;
        }
    };

} // namespace ZenitUI