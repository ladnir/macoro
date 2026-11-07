#pragma once
#include "macoro/work_item.h"
#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#ifdef MACORO_CPP_20
namespace macoro::detail
{
    // A real coroutine, allocated once per async partition and reused for each
    // child continuation. Numerical/over-aligned state stays in the partition.
    struct await_callback
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        struct promise_type
        {
            await_callback get_return_object() noexcept
            { return await_callback(handle_type::from_promise(*this)); }
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void return_void() noexcept {}
            void unhandled_exception() noexcept { std::terminate(); }
        };

        await_callback() noexcept = default;
        await_callback(const await_callback&) = delete;
        await_callback& operator=(const await_callback&) = delete;
        await_callback(await_callback&& other) noexcept
            : mHandle(std::exchange(other.mHandle, {})) {}
        await_callback& operator=(await_callback&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                mHandle = std::exchange(other.mHandle, {});
            }
            return *this;
        }
        ~await_callback() { reset(); }
        void reset() noexcept
        {
            if (auto h = std::exchange(mHandle, {})) h.destroy();
        }
        handle_type handle() const noexcept { return mHandle; }

        static await_callback create(work_item callback)
        {
            for (;;) co_await notify_on_suspend{callback};
        }

    private:
        struct notify_on_suspend
        {
            work_item callback;
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<>) const noexcept
            {
                // Already suspended: notification may resume or destroy this
                // coroutine. Copy the descriptor and never touch the frame again.
                auto work = callback;
                work();
            }
            void await_resume() const noexcept {}
        };
        explicit await_callback(handle_type h) noexcept : mHandle(h) {}
        handle_type mHandle{};
    };

    // Direct construction also supports immovable awaitables. References borrow
    // their referents; values are destroyed explicitly before signaling completion.
    template<class T, bool = std::is_reference_v<T>>
    struct await_storage
    {
        alignas(T) std::byte bytes[sizeof(T)];
        bool live = false;
        template<class Factory> void construct(Factory&& factory)
        {
            ::new (static_cast<void*>(bytes)) T(factory());
            live = true;
        }
        T& value() noexcept { return *std::launder(reinterpret_cast<T*>(bytes)); }
        T&& get() noexcept { return static_cast<T&&>(value()); }
        void reset() noexcept { if (live) { value().~T(); live = false; } }
        ~await_storage() { reset(); }
    };

    template<class T>
    struct await_storage<T, true>
    {
        std::remove_reference_t<T>* ptr = nullptr;
        template<class Factory> void construct(Factory&& factory)
        {
            auto&& value = factory();
            ptr = std::addressof(value);
        }
        T get() noexcept { return static_cast<T>(*ptr); }
        auto& value() noexcept { return *ptr; }
        void reset() noexcept { ptr = nullptr; }
    };

    template<class Result, bool Async>
    struct predicate_storage {};

    template<class Result>
    struct predicate_storage<Result, true>
    {
        await_storage<Result> expression;
        await_storage<decltype(get_awaiter(std::declval<Result>()))> awaiter;
        await_callback continuation;
        // One driver owns the state. A racing or inline continuation contributes
        // an arrival and lets the current driver process it after await_suspend.
        std::atomic<unsigned> driving{0};
        bool active = false;
        void reset() noexcept
        {
            awaiter.reset();
            expression.reset();
            active = false;
        }
    };
}
#endif
