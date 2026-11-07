#pragma once

#include "macoro/work_item.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#ifdef MACORO_CPP_20
#include <span>
#endif

namespace macoro
{
    namespace detail
    {
        template<class Executor, class Local, class Fn>
        class bulk_chunked_awaitable
        {
            // The awaitable can live in a coroutine frame whose allocation does
            // not support extended alignment. Only over-aligned callables need
            // separate storage; ordinary callables stay inline.
            static constexpr bool indirectFunction = alignof(Fn) > alignof(std::max_align_t);
            using function_storage = std::conditional_t<indirectFunction, std::unique_ptr<Fn>, Fn>;

            template<class F>
            static function_storage storeFunction(F&& fn)
            {
                if constexpr (indirectFunction)
                    return std::make_unique<Fn>(std::forward<F>(fn));
                else
                    return std::forward<F>(fn);
            }

            Fn& function() noexcept
            {
                if constexpr (indirectFunction) return *mFunction;
                else return mFunction;
            }

            struct partition
            {
                bulk_chunked_awaitable* owner = nullptr;
                std::size_t index = 0;
                std::exception_ptr error;
            };

            Executor& mExecutor;
            std::size_t mCount;
            Local* mLocals;
            function_storage mFunction;
            std::vector<partition> mPartitions;
            std::vector<work_item> mItems;
            std::atomic<std::size_t> mRemaining{0};
            coroutine_handle<> mContinuation;
            bool mStarted = false;

            static std::size_t partitionCount(std::size_t count, Local* locals,
                std::size_t localCount)
            {
                if (count && (!locals || !localCount))
                    throw std::invalid_argument("macoro::bulk requires local storage for nonempty work");
                const auto n = std::min(count, localCount);
                if (n == std::numeric_limits<std::size_t>::max())
                    throw std::length_error("macoro::bulk has too many partitions");
                return n;
            }

            static void execute(void* context) noexcept
            {
                auto& part = *static_cast<partition*>(context);
                auto& self = *part.owner;
                const auto workers = self.mPartitions.size();
                const auto width = self.mCount / workers;
                const auto extra = self.mCount % workers;
                // Balanced contiguous ranges without multiplying count by rank.
                const auto begin = part.index * width + std::min(part.index, extra);
                const auto end = begin + width + (part.index < extra);
                try
                {
                    auto& local = self.mLocals[part.index];
                    auto& fn = self.function();
                    fn(begin, end, local);
                }
                catch (...) { part.error = std::current_exception(); }

                // Publish this partition's writes, including its exception. The
                // final signal may destroy self/part: do not access either again.
                if (self.mRemaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
                    self.mContinuation.resume();
            }

        public:
            template<class F>
            bulk_chunked_awaitable(Executor& executor, std::size_t count, Local* locals,
                std::size_t localCount, F&& fn)
                : mExecutor(executor), mCount(count), mLocals(locals),
                  mFunction(storeFunction(std::forward<F>(fn))),
                  mPartitions(partitionCount(count, locals, localCount)),
                  mItems(mPartitions.size())
            {
                static_assert(std::is_same<decltype(std::declval<Fn&>()(
                    std::size_t{}, std::size_t{}, std::declval<Local&>())), void>::value,
                    "bulk_chunked callbacks must be synchronous and return void");
            }

            bulk_chunked_awaitable(const bulk_chunked_awaitable&) = delete;
            bulk_chunked_awaitable& operator=(const bulk_chunked_awaitable&) = delete;
            bulk_chunked_awaitable& operator=(bulk_chunked_awaitable&&) = delete;

            // Macoro's awaiter adapters may move an unstarted operation. Bind
            // partition pointers only in await_suspend, at its final address.
            bulk_chunked_awaitable(bulk_chunked_awaitable&& other)
                noexcept(std::is_nothrow_move_constructible<function_storage>::value)
                : mExecutor(other.mExecutor), mCount(other.mCount), mLocals(other.mLocals),
                  mFunction(std::move(other.mFunction)),
                  mPartitions(std::move(other.mPartitions)), mItems(std::move(other.mItems))
            {
                assert(!other.mStarted);
            }

            ~bulk_chunked_awaitable()
            {
                // Structured lifetime: a started operation must finish before
                // its awaiter or the coroutine containing it is destroyed.
                assert(mRemaining.load(std::memory_order_relaxed) == 0);
            }

            bool await_ready() const noexcept
            {
                assert(!mStarted);
                return mCount == 0;
            }

            bool await_suspend(coroutine_handle<> continuation)
            {
                assert(!mStarted);
                mStarted = true;
                mContinuation = continuation;
                // The extra arrival prevents resumption during publication.
                mRemaining.store(mPartitions.size() + 1, std::memory_order_relaxed);
                for (std::size_t i = 0; i < mPartitions.size(); ++i)
                {
                    mPartitions[i].owner = this;
                    mPartitions[i].index = i;
                    mItems[i] = {&mPartitions[i], &execute};
                }
                try { mExecutor.post_batch(mItems.data(), mItems.size()); }
                catch (...)
                {
                    // Executor contract: throwing submission published no work.
                    mRemaining.store(0, std::memory_order_relaxed);
                    throw;
                }
                // As in when_all: either finish inline, or let the final child
                // resume us. No access to this is allowed after this decrement.
                return mRemaining.fetch_sub(1, std::memory_order_acq_rel) != 1;
            }

#ifdef MACORO_CPP_20
            template<class Promise>
            bool await_suspend(std::coroutine_handle<Promise> continuation)
            {
                return await_suspend(coroutine_handle<>(continuation));
            }
#endif

            void await_resume()
            {
                for (auto& part : mPartitions)
                    if (part.error) std::rethrow_exception(part.error);
            }
        };
    }

    // Lazy, single-use chunked parallel loop. Executor::post_batch must copy
    // descriptors and publish all or none on failure; execution may be inline.
    // Nonempty work uses min(count, localCount) balanced contiguous partitions.
    // Each exclusively borrows locals[partition] and invokes fn(begin, end, local)
    // once for its nonempty half-open range. Zero work invokes no callbacks.
    // The callable is owned, shared by partitions, and must permit concurrent
    // calls. No OS-thread affinity is implied. All callback scopes finish before
    // completion, including on failure, which rethrows the lowest-index
    // partition's exception. Other partitions still execute their callbacks.
    // Executor, locals and anything borrowed by fn must outlive completion.
    // A started operation must be awaited to completion, never dropped while active.
    template<class Executor, class Local, class Fn>
    MACORO_NODISCARD auto bulk_chunked(Executor& executor, std::size_t count,
        Local* locals, std::size_t localCount, Fn&& fn)
    {
        return detail::bulk_chunked_awaitable<Executor, Local, typename std::decay<Fn>::type>(
            executor, count, locals, localCount, std::forward<Fn>(fn));
    }

    namespace detail
    {
        // Like stdexec's item-to-chunk adapter: no separate execution engine.
        // Storing Fn as a member preserves its alignment for the awaitable's
        // indirect-storage decision, including move-only over-aligned functors.
        template<class Fn>
        struct bulk_item_function
        {
            Fn fn;
            template<class Local>
            void operator()(std::size_t begin, std::size_t end, Local& local)
                noexcept(noexcept(fn(begin, local)))
            {
                static_assert(std::is_same<decltype(fn(begin, local)), void>::value,
                    "bulk callbacks must be synchronous and return void");
                for (auto i = begin; i != end; ++i) fn(i, local);
            }
        };
    }

    // Per-item convenience form of bulk_chunked. On success every index runs
    // once. A throwing item skips the remaining items of that partition;
    // every other partition finishes before the error propagates.
    template<class Executor, class Local, class Fn>
    MACORO_NODISCARD auto bulk(Executor& executor, std::size_t count,
        Local* locals, std::size_t localCount, Fn&& fn)
    {
        return bulk_chunked(executor, count, locals, localCount,
            detail::bulk_item_function<typename std::decay<Fn>::type>{std::forward<Fn>(fn)});
    }

#ifdef MACORO_CPP_20
    template<class Executor, class Local, std::size_t Extent, class Fn>
    MACORO_NODISCARD auto bulk_chunked(Executor& executor, std::size_t count,
        std::span<Local, Extent> locals, Fn&& fn)
    {
        return bulk_chunked(executor, count, locals.data(), locals.size(), std::forward<Fn>(fn));
    }

    template<class Executor, class Local, std::size_t Extent, class Fn>
    MACORO_NODISCARD auto bulk(Executor& executor, std::size_t count,
        std::span<Local, Extent> locals, Fn&& fn)
    {
        return bulk(executor, count, locals.data(), locals.size(), std::forward<Fn>(fn));
    }
#endif
}
