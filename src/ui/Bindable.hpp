#pragma once

#include "Common.hpp"

namespace ZenitUI
{

    // =========================================================================
    //  Bindable<T>
    //
    //  A plain value with observer semantics. Not a Layout, not a widget,
    //  not involved in the frame loop. Holds a value of type T, and
    //  notifies subscribers when set() changes it.
    //
    //  Thread safety: none. Single-thread only. If a background thread
    //  produces values, post them to the main thread before calling set().
    //
    //  Requirements on T:
    //    - operator== (set() uses it to detect real changes)
    //    - default-constructible if you use the default constructor of
    //      Bindable itself. Bindable<T>{initial} works for non-default-
    //      constructible T.
    //    - copy-constructible / move-constructible as set() and get()
    //      require.
    //
    //  Design notes:
    //    - Subscribers are stored in std::vector<Entry>, preserving
    //      insertion order. Notification order is guaranteed by the
    //      standard for std::vector iteration, unlike unordered_map.
    //    - A Subscription holds a shared_ptr to a ControlBlock. ~Bindable
    //      sets ControlBlock::alive_ = false, so a Subscription that
    //      outlives its Bindable can be safely destroyed or unsubscribed
    //      without touching freed memory.
    //    - Bindable is non-copyable and non-movable, so the ControlBlock
    //      is unique per Bindable. This makes subscription identity stable
    //      across the Bindable's lifetime.
    // =========================================================================

    template <typename T>
    class Bindable
    {
    public:
        // =====================================================================
        //  Subscription
        //
        //  RAII token for one subscription. Destroying it unsubscribes.
        //  Move-only; non-copyable.
        // =====================================================================
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

            // Shared between the Bindable and every Subscription it hands
            // out. `alive_` is set to false by ~Bindable, so a dangling
            // Subscription knows not to touch `owner_`.
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

        // =====================================================================
        //  Construction
        // =====================================================================

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

        // =====================================================================
        //  Value access
        // =====================================================================

        const T &get() const { return value_; }

        void set(T v)
        {
            if (v == value_)
                return;
            value_ = std::move(v);

            // Snapshot: a callback may call unsubscribe, which modifies
            // subscribers_ during iteration. Iterating a copy is the
            // simplest safe approach; N is expected to be small.
            auto snapshot = subscribers_;
            for (const auto &entry : snapshot)
            {
                if (isStillSubscribed(entry.token))
                    entry.cb(value_);
            }
        }

        // =====================================================================
        //  Subscription
        // =====================================================================

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

        // Convenience: bind to a widget method, with lifetime safety.
        //
        // `Arg` is deduced, so the method can take the parameter by value
        // (void(ProgressBar::*)(float)) or by const-ref
        // (void(Layout::*)(const std::string&)) — both work without a
        // wrapper lambda. Without the deduction, every call site would
        // need a wrapper because C++ does not convert method pointer
        // signatures implicitly.
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

        // Convenience overload for shared_ptr. Users typically hold a
        // shared_ptr (from X::create()); forcing a weak_ptr cast at every
        // call site would be noise.
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