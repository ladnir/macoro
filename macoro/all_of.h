#pragma once
#include "macoro/detail/await_callback.h"
#include <algorithm>
#include <atomic>
#include <functional>
#include <limits>
#include <stdexcept>

#ifdef MACORO_CPP_20
#include <span>

namespace macoro
{
    namespace detail
    {
        // Tests count shared storage without global hooks. Native continuation
        // frames and allocations made by the predicate/executor are separate.
        struct all_of_allocation
        {
            static void* allocate(std::size_t bytes, std::size_t alignment)
            { return ::operator new(bytes, std::align_val_t(alignment)); }
            static void deallocate(void* ptr, std::size_t alignment) noexcept
            { ::operator delete(ptr, std::align_val_t(alignment)); }
        };

        template<class Executor, class Local, class Fn, class Allocation = all_of_allocation>
        class all_of_operation
        {
            using result = std::invoke_result_t<Fn&, std::size_t, Local&>;
            static constexpr bool async = requires { get_awaiter(std::declval<result>()).await_resume(); };
            struct state;
            struct partition : predicate_storage<result, async>
            {
                state* owner = nullptr;
                Local* local = nullptr;
                std::size_t next = 0, end = 0;
                std::exception_ptr error;

                void finish() noexcept
                {
                    auto* s = owner;
                    // The callback coroutine suspends before entering the driver,
                    // so it can be destroyed here, before publishing completion.
                    if constexpr (async) this->continuation.reset();
                    // No access to this partition or its owner after publication.
                    if (s->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
                        s->continuation.resume();
                }
                void reject() noexcept { owner->stop.store(true, std::memory_order_relaxed); }

                // Returns true when finished, false when awaiting a continuation.
                bool advance() noexcept
                {
                    try
                    {
                        if constexpr (async)
                        {
                            if (this->active)
                            {
                                bool accepted = static_cast<bool>(this->awaiter.value().await_resume());
                                this->reset();
                                if (!accepted) { reject(); return true; }
                                ++next;
                            }
                        }
                        while (next != end && !owner->stop.load(std::memory_order_relaxed))
                        {
                            if constexpr (!async)
                            {
                                if (!static_cast<bool>(std::invoke(owner->fn, next, *local)))
                                { reject(); break; }
                                ++next;
                            }
                            else
                            {
                                this->expression.construct([&]() -> decltype(auto) {
                                    return std::invoke(owner->fn, next, *local);
                                });
                                this->awaiter.construct([&]() -> decltype(auto) {
                                    return get_awaiter(this->expression.get());
                                });
                                this->active = true;
                                auto& a = this->awaiter.value();
                                if (!a.await_ready())
                                {
                                    auto h = this->continuation.handle();
                                    using suspension = decltype(a.await_suspend(h));
                                    if constexpr (std::is_void_v<suspension>)
                                    { a.await_suspend(h); return false; }
                                    else if constexpr (std::is_same_v<suspension, bool>)
                                    { if (a.await_suspend(h)) return false; }
                                    else
                                    {
                                        auto target = a.await_suspend(h);
                                        target.resume();
                                        return false;
                                    }
                                }
                                bool accepted = static_cast<bool>(a.await_resume());
                                this->reset();
                                if (!accepted) { reject(); break; }
                                ++next;
                            }
                        }
                    }
                    catch (...)
                    {
                        error = std::current_exception();
                        if constexpr (async) this->reset();
                        reject();
                    }
                    return true;
                }

                static void run(void* ptr) noexcept
                {
                    auto& p = *static_cast<partition*>(ptr);
                    if constexpr (!async)
                    {
                        p.advance();
                        p.finish();
                    }
                    else
                    {
                        if (p.driving.fetch_add(1, std::memory_order_acq_rel) != 0) return;
                        do
                        {
                            if (p.advance())
                            {
                                assert(p.driving.load(std::memory_order_relaxed) == 1);
                                p.driving.store(0, std::memory_order_relaxed);
                                p.finish();
                                return;
                            }
                        } while (p.driving.fetch_sub(1, std::memory_order_acq_rel) != 1);
                    }
                }
            };

            struct state
            {
                Executor& executor;
                Fn fn;
                std::size_t count;
                partition* partitions;
                work_item* items;
                std::atomic<bool> stop{false};
                std::atomic<std::size_t> remaining{0};
                coroutine_handle<> continuation;
                bool started = false;
                template<class F> state(Executor& e, F&& f, std::size_t n)
                    : executor(e), fn(std::forward<F>(f)), count(n) {}
            };

            static constexpr std::size_t alignment = std::max({alignof(state), alignof(partition), alignof(work_item)});
            state* mState = nullptr;
            [[no_unique_address]] Allocation mAllocation;

            static std::size_t reserve(std::size_t& bytes, std::size_t align, std::size_t n, std::size_t size)
            {
                constexpr auto max = std::numeric_limits<std::size_t>::max();
                if (bytes > max - (align - 1)) throw std::length_error("all_of storage overflow");
                const auto offset = (bytes + align - 1) & ~(align - 1);
                if (n > (max - offset) / size) throw std::length_error("all_of storage overflow");
                bytes = offset + n * size;
                return offset;
            }

        public:
            template<class F>
            all_of_operation(Executor& executor, std::size_t count, Local* locals,
                std::size_t localCount, F&& fn, Allocation allocation = {})
                : mAllocation(std::move(allocation))
            {
                static_assert(std::is_nothrow_default_constructible_v<partition>);
                static_assert(std::is_nothrow_move_constructible_v<Allocation>);
                if (count && (!locals || !localCount))
                    throw std::invalid_argument("all_of requires local storage for nonempty work");
                const auto n = std::min(count, localCount);
                if (n == std::numeric_limits<std::size_t>::max())
                    throw std::length_error("all_of has too many partitions");
                std::size_t bytes = sizeof(state);
                auto partsOffset = reserve(bytes, alignof(partition), n, sizeof(partition));
                auto itemsOffset = reserve(bytes, alignof(work_item), n, sizeof(work_item));
                auto* storage = static_cast<std::byte*>(mAllocation.allocate(bytes, alignment));
                try { mState = ::new (storage) state(executor, std::forward<F>(fn), n); }
                catch (...) { mAllocation.deallocate(storage, alignment); throw; }
                mState->partitions = reinterpret_cast<partition*>(storage + partsOffset);
                mState->items = reinterpret_cast<work_item*>(storage + itemsOffset);
                std::size_t constructed = 0;
                try
                {
                    for (std::size_t i = 0; i != n; ++i)
                    {
                        auto& p = *::new (mState->partitions + i) partition;
                        ++constructed;
                        p.owner = mState;
                        p.local = locals + i;
                        auto width = count / n, extra = count % n;
                        p.next = i * width + std::min(i, extra);
                        p.end = p.next + width + (i < extra);
                        ::new (mState->items + i) work_item{&p, &partition::run};
                        if constexpr (async)
                            p.continuation = await_callback::create({&p, &partition::run});
                    }
                }
                catch (...)
                {
                    while (constructed) mState->partitions[--constructed].~partition();
                    mState->~state();
                    mAllocation.deallocate(storage, alignment);
                    throw;
                }
            }
            all_of_operation(const all_of_operation&) = delete;
            all_of_operation& operator=(const all_of_operation&) = delete;
            all_of_operation(all_of_operation&& other) noexcept(std::is_nothrow_move_constructible_v<Allocation>)
                : mState(std::exchange(other.mState, nullptr)), mAllocation(std::move(other.mAllocation)) {}
            ~all_of_operation()
            {
                if (!mState) return;
                assert(mState->remaining.load(std::memory_order_relaxed) == 0);
                for (std::size_t i = 0; i != mState->count; ++i) mState->partitions[i].~partition();
                mState->~state();
                mAllocation.deallocate(mState, alignment);
            }
            bool await_ready() const noexcept
            { assert(mState && !mState->started); return mState->count == 0; }
            bool await_suspend(coroutine_handle<> continuation)
            {
                auto& s = *mState;
                assert(!s.started);
                s.started = true;
                s.continuation = continuation;
                s.remaining.store(s.count + 1, std::memory_order_relaxed);
                try
                {
                    if (s.count == 1) s.items[0]();
                    else s.executor.post_batch(s.items, s.count);
                }
                catch (...)
                {
                    // post_batch must publish all descriptors or none on failure.
                    s.remaining.store(0, std::memory_order_relaxed);
                    throw;
                }
                return s.remaining.fetch_sub(1, std::memory_order_acq_rel) != 1;
            }
            template<class Promise> bool await_suspend(std::coroutine_handle<Promise> continuation)
            { return await_suspend(coroutine_handle<>(continuation)); }
            bool await_resume()
            {
                for (std::size_t i = 0; i != mState->count; ++i)
                    if (mState->partitions[i].error) std::rethrow_exception(mState->partitions[i].error);
                return !mState->stop.load(std::memory_order_relaxed);
            }
        };
    }

    // Lazy, single-use, bounded all_of. fn(index, local) returns bool-like or an
    // awaitable yielding bool-like. One aligned allocation owns the callable,
    // partitions and submission descriptors. Async predicates additionally use
    // one real coroutine frame per partition, reused across its items.
    // Executor::post_batch must copy descriptors and publish all or none on failure.
    // Each partition exclusively borrows one local and walks a balanced contiguous
    // range. One partition runs inline. False requests stopping between items;
    // started predicates finish. Exceptions take precedence over false, and the
    // lowest failing partition wins. Child storage is cleaned before completion.
    // The callable is shared and must support concurrent calls. Executor, locals,
    // and callable referents are borrowed until completion; no thread affinity is
    // promised. Started operations must finish before destruction. Predicate and
    // executor allocations are separate from the shared block and bridge frames.
    template<class Executor, class Local, class Fn>
    MACORO_NODISCARD auto all_of(Executor& executor, std::size_t count,
        Local* locals, std::size_t localCount, Fn&& fn)
    {
        return detail::all_of_operation<Executor, Local, std::decay_t<Fn>>(
            executor, count, locals, localCount, std::forward<Fn>(fn));
    }
    template<class Executor, class Local, std::size_t Extent, class Fn>
    MACORO_NODISCARD auto all_of(Executor& executor, std::size_t count,
        std::span<Local, Extent> locals, Fn&& fn)
    { return all_of(executor, count, locals.data(), locals.size(), std::forward<Fn>(fn)); }
}
#endif
