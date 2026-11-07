#include "bulk_tests.h"
#include "macoro/bulk.h"
#include "macoro/inline_scheduler.h"
#include "macoro/thread_pool.h"
#include "macoro/task.h"
#include "macoro/sync_wait.h"
#include "macoro/when_all.h"

#include <array>
#include <cstdint>
#include <memory>
#include <numeric>

namespace macoro::tests
{
    namespace
    {
        void check(bool value)
        {
            if (!value) throw std::runtime_error("bulk test failed");
        }

        struct Pool
        {
            thread_pool pool;
            thread_pool::work work;
            explicit Pool(std::size_t threads) : work(pool.make_work())
            {
                try { pool.create_threads(threads); }
                catch (...) { work.reset(); throw; }
            }
            ~Pool() { work.reset(); pool.join(); }
        };

        task<> record(thread_pool& pool, std::vector<int>& values)
        {
            MC_BEGIN(task<>, &pool, &values);
            MC_AWAIT(pool.schedule());
            values.push_back(2);
            MC_AWAIT(pool.dispatch());
            values.push_back(3);
            MC_END();
        }

        struct AllocationState { int untilFailure = -1; std::size_t outstanding = 0; };
        template<class T>
        struct FailingAllocator
        {
            using value_type = T;
            AllocationState* state;
            explicit FailingAllocator(AllocationState& s) : state(&s) {}
            template<class U>
            FailingAllocator(const FailingAllocator<U>& other) : state(other.state) {}
            T* allocate(std::size_t n)
            {
                if (state->untilFailure == 0) throw std::bad_alloc();
                if (state->untilFailure > 0) --state->untilFailure;
                auto* p = std::allocator<T>{}.allocate(n);
                ++state->outstanding;
                return p;
            }
            void deallocate(T* p, std::size_t n)
            {
                --state->outstanding;
                std::allocator<T>{}.deallocate(p, n);
            }
            template<class U> bool operator==(const FailingAllocator<U>& b) const { return state == b.state; }
            template<class U> bool operator!=(const FailingAllocator<U>& b) const { return !(*this == b); }
        };

        struct alignas(64) Local
        {
            std::size_t count = 0;
            std::atomic_flag busy = ATOMIC_FLAG_INIT;
        };

        struct RejectExecutor
        {
            void post_batch(const work_item*, std::size_t) { throw std::bad_alloc(); }
        };

        struct alignas(64) AlignedFunction
        {
            std::shared_ptr<int> lifetime;
            std::unique_ptr<int> value = std::make_unique<int>(1);
            bool fail;

            AlignedFunction(std::shared_ptr<int> owner, bool throwOnZero = false)
                : lifetime(std::move(owner)), fail(throwOnZero) {}
            AlignedFunction(AlignedFunction&&) = default;
            AlignedFunction(const AlignedFunction&) = delete;

            void operator()(std::size_t i, std::size_t& local)
            {
                check(reinterpret_cast<std::uintptr_t>(this) % alignof(AlignedFunction) == 0);
                check(value && *value == 1);
                ++local;
                if (fail && i == 0) throw std::runtime_error("aligned callback failure");
            }
        };

        // Also exercise the raw chunk callback with the same move-only,
        // over-aligned callable and exact per-item failure behavior.
        struct AlignedChunkFunction : AlignedFunction
        {
            using AlignedFunction::AlignedFunction;
            void operator()(std::size_t begin, std::size_t end, std::size_t& local)
            {
                for (auto i = begin; i != end; ++i) AlignedFunction::operator()(i, local);
            }
        };
        using AlignedChunkBulk = decltype(bulk_chunked(std::declval<inline_scheduler&>(),
            std::size_t{}, static_cast<std::size_t*>(nullptr), std::size_t{},
            std::declval<AlignedChunkFunction>()));
        static_assert(alignof(AlignedChunkBulk) <= alignof(std::max_align_t),
            "bulk_chunked must not impose extended alignment on coroutine frames");

        template<class Executor>
        task<> alignedChunkMacro(Executor& executor, AlignedChunkFunction& fn,
            std::array<std::size_t, 3>& locals)
        {
            MC_BEGIN(task<>, &executor, &fn, &locals);
            MC_AWAIT(bulk_chunked(executor, 7, locals.data(), locals.size(), std::move(fn)));
            MC_END();
        }
#ifdef MACORO_CPP_20
        template<class Executor>
        task<> alignedChunkNative(Executor& executor, AlignedChunkFunction& fn,
            std::array<std::size_t, 3>& locals)
        {
            co_await bulk_chunked(executor, 7, std::span{locals}, std::move(fn));
        }
#endif

        using AlignedBulk = decltype(bulk(std::declval<inline_scheduler&>(),
            std::size_t{}, static_cast<std::size_t*>(nullptr), std::size_t{},
            std::declval<AlignedFunction>()));
        static_assert(alignof(AlignedBulk) <= alignof(std::max_align_t),
            "bulk must not impose extended alignment on the parent coroutine frame");

        // Keep the aligned object outside coroutine frames; bulk must safely
        // take ownership of it in both native and macro coroutine awaiters.
        template<class Executor>
        task<> alignedMacro(Executor& executor, AlignedFunction& fn,
            std::array<std::size_t, 3>& locals)
        {
            MC_BEGIN(task<>, &executor, &fn, &locals);
            MC_AWAIT(bulk(executor, 7, locals.data(), locals.size(), std::move(fn)));
            MC_END();
        }
#ifdef MACORO_CPP_20
        template<class Executor>
        task<> alignedNative(Executor& executor, AlignedFunction& fn,
            std::array<std::size_t, 3>& locals)
        {
            co_await bulk(executor, 7, std::span{locals}, std::move(fn));
        }
#endif

        // Exercises a macro coroutine awaiting the new operation as well as
        // nested scheduling on the very same (possibly single-threaded) pool.
        task<> nested(thread_pool& pool, std::array<std::size_t, 3>& locals,
            std::array<unsigned, 37>& seen)
        {
            auto process = [&seen](std::size_t i, std::size_t& local) { ++seen[i]; ++local; };
            MC_BEGIN(task<>, &pool, &locals, &seen, process);
            MC_AWAIT(pool.schedule());
            MC_AWAIT(bulk(pool, seen.size(), locals.data(), locals.size(), process));
            MC_END();
        }
    }

    void thread_pool_work_items_test()
    {
        thread_pool pool;
        std::vector<int> values;
        pool.post(work_item{&values, [](void* p) noexcept {
            static_cast<std::vector<int>*>(p)->push_back(1);
        }});
        // Reserve before any callbacks execute: low-level callbacks are noexcept.
        values.reserve(5);
        auto child = make_eager(record(pool, values));
        std::array<work_item, 2> items{{
            {&values, [](void* p) noexcept { static_cast<std::vector<int>*>(p)->push_back(4); }},
            {&values, [](void* p) noexcept { static_cast<std::vector<int>*>(p)->push_back(5); }}
        }};
        pool.post_batch(items.data(), items.size());
        pool.post_batch(nullptr, 0);
        items = {}; // Submission copies descriptors, not their backing array.
        pool.run();
        sync_wait(child);
        check(values == std::vector<int>({1, 2, 3, 4, 5}));
    }

    void thread_pool_batch_failure_test()
    {
        AllocationState allocations;
        {
            std::deque<work_item, FailingAllocator<work_item>> queue{
                FailingAllocator<work_item>(allocations)};
            int original = 0, added = 0;
            auto increment = [](void* p) noexcept { ++*static_cast<int*>(p); };
            queue.push_back({&original, increment});
            std::array<work_item, 1024> items;
            items.fill({&added, increment});
            // Exercise both immediate and later allocation failures, after an
            // inserted prefix. Existing queued work must survive every rollback.
            for (int failAfter : {0, 1, 2})
            {
                allocations.untilFailure = failAfter;
                bool rejected = false;
                try { detail::append_work_batch(queue, items.data(), items.size()); }
                catch (const std::bad_alloc&) { rejected = true; }
                check(rejected && queue.size() == 1 && queue.front().context == &original);
            }
            allocations.untilFailure = -1;
            detail::append_work_batch(queue, items.data(), items.size());
            for (auto item : queue) item();
            check(original == 1 && added == 1024);
        }
        check(allocations.outstanding == 0);
    }

    void bulk_coverage_test()
    {
        Pool executor(3);
        for (std::size_t count : {0u, 1u, 2u, 3u, 11u, 257u})
        {
            std::array<Local, 3> locals;
            std::array<std::atomic<unsigned>, 257> seen{};
            std::array<std::size_t, 257> owners{};
            const auto caller = std::this_thread::get_id();
            auto operation = bulk(executor.pool, count, locals.data(), locals.size(),
                [&, token = std::make_unique<int>(42)](std::size_t i, Local& local) {
                    check(*token == 42 && std::this_thread::get_id() != caller);
                    check(!local.busy.test_and_set());
                    ++local.count;
                    ++seen[i];
                    owners[i] = &local - locals.data();
                    local.busy.clear();
                });
            // Laziness and moving the operation before start.
            for (auto& local : locals) check(local.count == 0);
            sync_wait(std::move(operation));
            for (std::size_t i = 0; i < seen.size(); ++i) check(seen[i] == (i < count ? 1u : 0u));
            std::size_t total = 0;
            const auto partitions = std::min(count, locals.size());
            for (std::size_t p = 0; p < partitions; ++p)
            {
                const auto length = count / partitions + (p < count % partitions);
                check(locals[p].count == length);
                for (auto i = total; i < total + length; ++i) check(owners[i] == p);
                total += length;
            }
            check(total == count);
        }
    }

    void bulk_inline_submission_test()
    {
        inline_scheduler executor;
        std::array<std::size_t, 3> locals{};
        auto increment = [](std::size_t, std::size_t& local) { ++local; };
        sync_wait(bulk(executor, 11, locals.data(), locals.size(), increment));
        check(locals == std::array<std::size_t, 3>{4, 4, 3});
        sync_wait(bulk(executor, 0, static_cast<std::size_t*>(nullptr), 0, increment));
        bool invalid = false;
        try { auto op = bulk(executor, 1, static_cast<std::size_t*>(nullptr), 0, increment); }
        catch (const std::invalid_argument&) { invalid = true; }
        check(invalid);
        RejectExecutor rejected;
        bool failed = false;
        try { sync_wait(bulk(rejected, 11, locals.data(), locals.size(), increment)); }
        catch (const std::bad_alloc&) { failed = true; }
        check(failed && locals == std::array<std::size_t, 3>{4, 4, 3});
        // All partitions must finish even when inline execution throws.
        failed = false;
        try { sync_wait(bulk(executor, 3, locals.data(), locals.size(),
            [](std::size_t i, std::size_t& local) { ++local; if (i == 0) throw 42; })); }
        catch (int error) { failed = error == 42; }
        check(failed && locals == std::array<std::size_t, 3>{5, 5, 4});
    }

    void bulk_exception_join_test()
    {
        // Force the completion order without sleeps: an exception must not
        // complete the parent while any published partition is still pending.
        struct ManualExecutor
        {
            std::vector<work_item> pending;
            void post_batch(const work_item* items, std::size_t count)
            {
                pending.assign(items, items + count);
            }
        } manual;
        std::array<std::size_t, 3> manualLocals{};
        auto pending = make_eager(bulk(manual, 3, manualLocals.data(), manualLocals.size(),
            [](std::size_t i, std::size_t& local) { ++local; if (i == 0) throw 42; }));
        manual.pending[0]();
        const bool afterError = pending.is_ready();
        manual.pending[2]();
        const bool beforeLast = pending.is_ready();
        manual.pending[1]();
        bool manualFailed = false;
        try { sync_wait(pending); }
        catch (int error) { manualFailed = error == 42; }
        check(!afterError && !beforeLast && pending.is_ready() && manualFailed);
        check(manualLocals == std::array<std::size_t, 3>{1, 1, 1});

        Pool executor(3);
        std::array<std::size_t, 3> locals{};
        // Each partition throws after modifying private state. Every write must
        // be visible before the selected exception reaches the parent.
        bool failed = false;
        try { sync_wait(bulk(executor.pool, 3, locals.data(), locals.size(),
            [](std::size_t i, std::size_t& local) { ++local; throw int(i); })); }
        catch (int error) { failed = error == 0; }
        check(failed && locals == std::array<std::size_t, 3>{1, 1, 1});
        // Reuse after failure, with no stale completion/error state.
        sync_wait(bulk(executor.pool, 6, locals.data(), locals.size(),
            [](std::size_t, std::size_t& local) { ++local; }));
        check(locals == std::array<std::size_t, 3>{3, 3, 3});
    }

    void bulk_composition_test()
    {
        Pool executor(1);
        std::array<std::size_t, 3> localsA{}, localsB{};
        std::array<unsigned, 37> seenA{}, seenB{};
        auto results = sync_wait(when_all_ready(
            nested(executor.pool, localsA, seenA), nested(executor.pool, localsB, seenB)));
        std::get<0>(results).result();
        std::get<1>(results).result();
        for (auto value : seenA) check(value == 1);
        for (auto value : seenB) check(value == 1);
#ifdef MACORO_CPP_20
        // Native coroutine and the span overload, alongside the macro path above.
        auto native = [&]() -> task<> {
            co_await executor.pool.schedule();
            co_await bulk(executor.pool, 3, std::span{localsA},
                [](std::size_t, std::size_t& local) { ++local; });
        };
        sync_wait(native());
        check(std::accumulate(localsA.begin(), localsA.end(), std::size_t{}) == 40);
#endif
    }

    void bulk_chunked_test()
    {
        Pool executor(3);
        inline_scheduler inlineExecutor;
        for (std::size_t count : {0u, 1u, 2u, 3u, 11u, 257u})
        {
            std::array<Local, 3> locals;
            std::array<unsigned, 257> seen{};
            std::array<std::size_t, 3> begins{}, ends{};
            auto operation = bulk_chunked(executor.pool, count, locals.data(), locals.size(),
                [&, token = std::make_unique<int>(42)](std::size_t begin, std::size_t end, Local& local) {
                    check(*token == 42 && begin < end && end <= count);
                    check(reinterpret_cast<std::uintptr_t>(&local) % alignof(Local) == 0);
                    check(!local.busy.test_and_set());
                    const auto p = &local - locals.data();
                    begins[p] = begin; ends[p] = end;
                    ++local.count; // Exactly one invocation per active partition.
                    for (auto i = begin; i != end; ++i) ++seen[i];
                    local.busy.clear();
                });
            for (auto& local : locals) check(local.count == 0);
            sync_wait(std::move(operation));
            const auto partitions = std::min(count, locals.size());
            for (std::size_t p = 0; p < locals.size(); ++p)
            {
                check(locals[p].count == (p < partitions ? 1u : 0u));
                if (p < partitions)
                {
                    check(begins[p] == (p ? ends[p - 1] : 0));
                    check(ends[p] - begins[p] == count / partitions + (p < count % partitions));
                }
            }
            if (partitions) check(ends[partitions - 1] == count);
            for (std::size_t i = 0; i < seen.size(); ++i) check(seen[i] == (i < count ? 1u : 0u));
        }
        auto addRange = [](std::size_t begin, std::size_t end, std::size_t& local) { local += end - begin; };
        std::array<std::size_t, 3> locals{};
        sync_wait(bulk_chunked(inlineExecutor, 11, locals.data(), locals.size(), addRange));
        check(locals == std::array<std::size_t, 3>{4, 4, 3});
        sync_wait(bulk_chunked(inlineExecutor, 0, static_cast<std::size_t*>(nullptr), 0, addRange));
        bool invalid = false;
        try { auto op = bulk_chunked(inlineExecutor, 1, static_cast<std::size_t*>(nullptr), 0, addRange); }
        catch (const std::invalid_argument&) { invalid = true; }
        check(invalid);
        RejectExecutor rejected;
        bool failed = false;
        try { sync_wait(bulk_chunked(rejected, 11, locals.data(), locals.size(), addRange)); }
        catch (const std::bad_alloc&) { failed = true; }
        check(failed && locals == std::array<std::size_t, 3>{4, 4, 3});

        // Manual completion order: the resumed parent must see all callback
        // scope destructors, even when multiple chunks throw in reverse order.
        struct ManualExecutor
        {
            std::vector<work_item> pending;
            void post_batch(const work_item* items, std::size_t count)
            { pending.assign(items, items + count); }
        } manual;
        std::array<unsigned, 3> cleaned{};
        auto chunks = [&] {
            return bulk_chunked(manual, 11, locals.data(), locals.size(),
                [&cleaned, &locals](std::size_t begin, std::size_t, std::size_t& local) {
                    struct Cleanup { unsigned& flag; ~Cleanup() { ++flag; } } cleanup{cleaned[&local - locals.data()]};
                    throw int(begin);
                });
        };
#ifdef MACORO_CPP_20
        auto parent = [&]() -> task<> {
            try { co_await chunks(); }
            catch (int error) {
                check(error == 0 && cleaned == std::array<unsigned, 3>{1, 1, 1});
                co_return;
            }
            check(false);
        };
        auto pending = make_eager(parent());
#else
        auto pending = make_eager(chunks());
#endif
        manual.pending[2](); check(!pending.is_ready());
        manual.pending[1](); check(!pending.is_ready());
        manual.pending[0]();
#ifdef MACORO_CPP_20
        sync_wait(pending);
#else
        bool caught = false;
        try { sync_wait(pending); } catch (int error) { caught = error == 0; }
        check(caught);
#endif
        check(cleaned == std::array<unsigned, 3>{1, 1, 1});
    }

    void bulk_alignment_test()
    {
        inline_scheduler inlineExecutor;
        Pool executor(2);
        for (bool fail : {false, true})
        {
            for (bool native : {false, true})
            {
                auto lifetime = std::make_shared<int>(0);
                std::weak_ptr<int> weak = lifetime;
                AlignedFunction fn(std::move(lifetime), fail);
                std::array<std::size_t, 3> locals{};
                bool threw = false;
                try
                {
#ifdef MACORO_CPP_20
                    if (native) sync_wait(alignedNative(executor.pool, fn, locals));
                    else
#endif
                        sync_wait(alignedMacro(inlineExecutor, fn, locals));
                }
                catch (const std::runtime_error& error)
                {
                    check(std::string(error.what()) == "aligned callback failure");
                    threw = true;
                }
                check(threw == fail && weak.expired());
                check(locals == (fail ? std::array<std::size_t, 3>{1, 2, 2}
                                     : std::array<std::size_t, 3>{3, 2, 2}));
            }
        }
        auto lifetime = std::make_shared<int>(0);
        std::weak_ptr<int> weak = lifetime;
        std::size_t local = 0;
        {
            AlignedFunction fn(std::move(lifetime));
            auto operation = bulk(inlineExecutor, 1, &local, 1, std::move(fn));
            auto moved = std::move(operation);
            check(!weak.expired());
        }
        check(weak.expired() && local == 0);
    }
    void bulk_chunked_alignment_test()
    {
        inline_scheduler inlineExecutor;
        Pool executor(2);
        for (bool fail : {false, true})
        for (bool native : {false, true})
        {
            auto lifetime = std::make_shared<int>(0);
            std::weak_ptr<int> weak = lifetime;
            AlignedChunkFunction fn(std::move(lifetime), fail);
            std::array<std::size_t, 3> locals{};
            bool threw = false;
            try
            {
#ifdef MACORO_CPP_20
                if (native) sync_wait(alignedChunkNative(executor.pool, fn, locals));
                else
#endif
                    sync_wait(alignedChunkMacro(inlineExecutor, fn, locals));
            }
            catch (const std::runtime_error& error)
            { check(std::string(error.what()) == "aligned callback failure"); threw = true; }
            check(threw == fail && weak.expired());
            check(locals == (fail ? std::array<std::size_t, 3>{1, 2, 2}
                                 : std::array<std::size_t, 3>{3, 2, 2}));
        }
        auto lifetime = std::make_shared<int>(0);
        std::weak_ptr<int> weak = lifetime;
        std::size_t local = 0;
        {
            AlignedChunkFunction fn(std::move(lifetime));
            auto operation = bulk_chunked(inlineExecutor, 1, &local, 1, std::move(fn));
            auto moved = std::move(operation);
            check(!weak.expired());
        }
        check(weak.expired() && local == 0);
    }

}
