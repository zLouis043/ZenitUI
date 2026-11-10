#pragma once

/**
 * \file Bindable.hpp
 * \brief Single-value observable used for one-way data binding between
 *        models and UI components.
 *
 * `Bindable<T>` is deliberately minimal:
 *
 *  - It stores one value.
 *  - `set()` notifies subscribers if the value changed.
 *  - `onChanged()` returns a `Subscription` RAII handle that unsubscribes
 *    on destruction, so subscribers do not need to keep track of tokens.
 *  - `bindTo()` builds a `Subscription` that calls a method on a widget.
 *
 * Lifetime model: `Bindable` is **not copyable or movable**. Its
 * `Subscription` handles hold a `shared_ptr<ControlBlock>` whose `alive_`
 * flag is flipped in the destructor, so a `Subscription` outliving its
 * `Bindable` is safe (it just becomes a no-op). Conversely, `set()` is
 * re-entrancy-safe: the subscriber list is snapshotted before iteration, so
 * a subscriber that (un)subscribes from inside its own callback does not
 * invalidate the loop.
 *
 * Non-goals: thread safety, computed/derived bindings, batching. Wrap in a
 * mutex or compose externally if those are needed.
 */

#include "Common.hpp"

namespace ZenitUI
{

    /**
     * \class Bindable
     * \tparam T Value type; must be copy-constructible and support `==`.
     *
     * \brief Observable single-value container.
     */
    template <typename T>
    class Bindable
    {
    public:
        /**
         * \class Subscription
         * \brief RAII handle to an `onChanged` registration.
         *
         * Move-only. Destruction (or explicit `unsubscribe`) removes the
         * callback from the owning `Bindable`. Safe to destroy after the
         * `Bindable` has already been destroyed: the shared `ControlBlock`
         * outlives both.
         */
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

            /** \brief Remove the callback from the owning `Bindable`. Idempotent. */
            void unsubscribe()
            {
                if (!ctrl_ || !ctrl_->alive_ || !ctrl_->owner_)
                    return;
                ctrl_->owner_->unsubscribe_(token_);
                ctrl_->owner_ = nullptr;
            }

            /** \brief `true` while the underlying callback is still registered. */
            bool active() const
            {
                return ctrl_ && ctrl_->alive_ && ctrl_->owner_ && token_ != 0;
            }

        private:
            friend class Bindable;

            /**
             * \struct ControlBlock
             * \brief Shared state between a `Bindable` and its `Subscription`s.
             *
             * `owner_` points back to the live `Bindable`, or is null after
             * either side has gone away. `alive_` is set to false by the
             * `Bindable` destructor so that subscriptions can detect the
             * situation without dereferencing `owner_`.
             */
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

        /** \brief Construct with an initial value. */
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


        /** \brief Read the current value. */
        const T &get() const { return value_; }

        /**
         * \brief Replace the value and notify subscribers if it changed.
         *
         * Equality is compared on `T`; setting the same value is a no-op.
         * Subscribers are snapshotted before dispatch, so a subscriber that
         * (un)subscribes from within its callback does not invalidate the loop.
         */
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


        /**
         * \brief Register a callback fired on value changes.
         * \param cb              Callback invoked with the new value.
         * \param fireImmediately When `true`, invoke \p cb right away with the
         *                        current value (before returning).
         * \return A `Subscription` that unregisters on destruction.
         */
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

        /**
         * \brief Bind this value to a widget method.
         * \tparam W     Widget type.
         * \tparam Arg   Argument type of the widget method; must be
         *               implicitly constructible from `T`.
         * \param widget          Weak pointer to the target widget.
         * \param method          Member function to call on change.
         * \param fireImmediately When `true`, invoke \p method right away.
         * \return A `Subscription` that unregisters on destruction.
         *
         * The weak pointer is locked on every notification, so the widget can
         * be destroyed independently of the subscription.
         */
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

        /** \brief Overload for `shared_ptr` widgets; forwards to the weak-pointer version. */
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

        /** \brief Remove the entry with the given token. */
        void unsubscribe_(uint64_t token)
        {
            subscribers_.erase(
                std::remove_if(
                    subscribers_.begin(), subscribers_.end(),
                    [token](const Entry &e)
                    { return e.token == token; }),
                subscribers_.end());
        }

        /** \brief `true` if a subscriber with the given token is still registered. */
        bool isStillSubscribed(uint64_t token) const
        {
            for (const auto &e : subscribers_)
                if (e.token == token)
                    return true;
            return false;
        }

        /** \brief Look up the entry with the given token, or `nullptr`. */
        Entry *findEntry(uint64_t token)
        {
            for (auto &e : subscribers_)
                if (e.token == token)
                    return &e;
            return nullptr;
        }
    };

} // namespace ZenitUI