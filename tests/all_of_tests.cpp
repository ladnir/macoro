#include "all_of_tests.h"
#include "macoro/all_of.h"
#include "macoro/task.h"
#ifdef MACORO_CPP_20
#include "macoro/inline_scheduler.h"
#include "macoro/thread_pool.h"
#include "macoro/sync_wait.h"
#include "macoro/manual_reset_event.h"
#include <array>
#include <cstdint>
#include <limits>
#include <semaphore>
#include <thread>

namespace macoro::tests
{
    namespace
    {
        void check(bool value) { if (!value) throw std::runtime_error("all_of regression failed"); }
        struct Pool
        {
            thread_pool pool;
            thread_pool::work work;
            explicit Pool(std::size_t n) : work(pool.make_work())
            {
                try { pool.create_threads(n); }
                catch (...) { work.reset(); throw; }
            }
            ~Pool() { work.reset(); pool.join(); }
        };
        struct alignas(64) Local
        {
            std::size_t calls = 0;
            std::atomic<bool> busy{false};
        };
        struct Lease
        {
            Local& local;
            explicit Lease(Local& l) : local(l) { check(!local.busy.exchange(true)); }
            ~Lease() { local.busy.store(false); }
        };
        struct CountingExecutor
        {
            std::size_t submissions = 0;
            void post_batch(const work_item* items, std::size_t n)
            {
                submissions += n;
                for (std::size_t i = 0; i != n; ++i) items[i]();
            }
        };
        struct AllocationCounts { unsigned allocated = 0, freed = 0; };
        struct CountAllocation
        {
            AllocationCounts* counts;
            void* allocate(std::size_t bytes, std::size_t alignment)
            {
                auto* p = detail::all_of_allocation::allocate(bytes, alignment);
                ++counts->allocated;
                return p;
            }
            void deallocate(void* p, std::size_t alignment) noexcept
            {
                ++counts->freed;
                detail::all_of_allocation::deallocate(p, alignment);
            }
        };
        struct AwaitCounts { unsigned alive = 0, calls = 0, depth = 0; };
        struct RaceState
        {
            std::coroutine_handle<> continuation;
            std::binary_semaphore resumed{0};
        };
        struct EarlyResume
        {
            thread_pool& pool;
            RaceState& race;
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h)
            {
                race.continuation = h;
                pool.post(work_item{&race, [](void* ptr) noexcept {
                    auto& r = *static_cast<RaceState*>(ptr);
                    r.continuation.resume();
                    r.resumed.release();
                }});
                // Force resumption on a different thread BEFORE await_suspend
                // returns. The partition must not consume/destroy this awaiter yet.
                race.resumed.acquire();
            }
            bool await_resume() const noexcept { return true; }
        };
        template<int Mode>
        struct alignas(128) Immediate
        {
            AwaitCounts& counts;
            explicit Immediate(AwaitCounts& c) : counts(c) { ++counts.alive; }
            Immediate(Immediate&&) = delete;
            ~Immediate() { --counts.alive; }
            bool await_ready()
            {
                check(reinterpret_cast<std::uintptr_t>(this) % alignof(Immediate) == 0);
                if constexpr (Mode == 4) throw 42;
                return Mode == 0;
            }
            template<class Promise> auto await_suspend(std::coroutine_handle<Promise> h)
            {
                if constexpr (Mode == 1) return false;
                else if constexpr (Mode == 2)
                {
                    check(++counts.depth == 1);
                    h.resume();
                    // The driver must not destroy us during await_suspend.
                    check(counts.alive == 1);
                    --counts.depth;
                }
                else if constexpr (Mode == 5) { throw 42; return false; }
                else return h;
            }
            bool await_resume()
            {
                if constexpr (Mode == 6) throw 42;
                ++counts.calls;
                return true;
            }
        };
        template<int Mode> void checkImmediate()
        {
            inline_scheduler executor;
            std::array<Local, 3> locals;
            AwaitCounts counts;
            AllocationCounts allocations;
            auto predicate = [&](std::size_t, Local&) { return Immediate<Mode>(counts); };
            using Operation = detail::all_of_operation<inline_scheduler, std::span<Local>, decltype(predicate), CountAllocation>;
            {
                Operation op(executor, 4096, std::span(locals), predicate, CountAllocation{&allocations});
                check(allocations.allocated == 1 && allocations.freed == 0 && counts.alive == 0);
                int caught = 0;
                try { check(sync_wait(std::move(op))); }
                catch (int value) { caught = value; }
                check(caught == (Mode >= 4 ? 42 : 0));
                check(counts.alive == 0 && counts.depth == 0);
                check(counts.calls == (Mode >= 4 ? 0 : 4096));
            }
            check(allocations.allocated == 1 && allocations.freed == 1);
        }
        struct EmptyResource {};
        struct CountOnly
        {
            std::size_t count;
            std::size_t size() const { return count; }
            EmptyResource operator[](std::size_t) const { return {}; }
        };
        struct IndirectLocals
        {
            std::array<Local*, 3> locals;
            std::size_t size() const { return locals.size(); }
            Local& operator[](std::size_t i) const { return *locals[i]; }
        };
        struct alignas(256) OwnedResource
        {
            unsigned* alive;
            const unsigned* viewAlive;
            unsigned calls = 0;
            OwnedResource(unsigned& a, const unsigned& v) : alive(&a), viewAlive(&v) { ++*alive; }
            OwnedResource(OwnedResource&&) = delete;
            ~OwnedResource() { --*alive; }
        };
        struct OwningView
        {
            unsigned* alive;
            std::shared_ptr<unsigned> owner = std::make_shared<unsigned>(42);
            std::size_t throwAt = 3;
            std::size_t size() const { return 3; }
            OwnedResource operator[](std::size_t i)
            {
                if (i == throwAt) throw 19;
                return OwnedResource(*alive, *owner);
            }
        };
        task<bool> borrowResource(async_manual_reset_event& gate, OwnedResource& resource)
        {
            co_await gate;
            check(reinterpret_cast<std::uintptr_t>(&resource) % alignof(OwnedResource) == 0);
            check(*resource.viewAlive == 42 && *resource.alive != 0);
            ++resource.calls;
            co_return true;
        }
        void checkIndexedResources()
        {
            inline_scheduler executor;
            AllocationCounts allocations;
            unsigned calls = 0;
            auto predicate = [&](std::size_t, EmptyResource&) { ++calls; return true; };
            using Operation = detail::all_of_operation<inline_scheduler, CountOnly,
                decltype(predicate), CountAllocation>;
            {
                Operation operation(executor, 37, CountOnly{3}, predicate, CountAllocation{&allocations});
                check(calls == 0 && allocations.allocated == 1);
                check(sync_wait(std::move(operation)) && calls == 37);
            }
            check(allocations.freed == 1);
            Local first, second, third;
            check(sync_wait(all_of(executor, 8, IndirectLocals{{&third, &first, &second}},
                [](std::size_t, Local& local) { ++local.calls; return true; })));
            check(third.calls == 3 && first.calls == 3 && second.calls == 2);

            // The span header is only needed while partition references bind.
            std::array<Local, 3> backing;
            auto fromLocalSpan = [&]() {
                auto view = std::span(backing);
                return all_of(executor, 7, view,
                    [](std::size_t, Local& local) { ++local.calls; return true; });
            };
            check(sync_wait(fromLocalSpan()));
            check(backing[0].calls == 3 && backing[1].calls == 2 && backing[2].calls == 2);

            unsigned alive = 0;
            std::weak_ptr<unsigned> weak;
            auto makeOperation = [&](async_manual_reset_event& gate) {
                OwningView view{&alive};
                weak = view.owner;
                return all_of(executor, 7, std::move(view),
                    [&](std::size_t, OwnedResource& local) { return borrowResource(gate, local); });
            };
            async_manual_reset_event gate;
            {
                auto operation = makeOperation(gate);
                check(alive == 3 && !weak.expired());
                bool finished = false;
                auto controller = [&]() -> task<> {
                    check(co_await std::move(operation));
                    check(alive == 0); // materialized arguments die before parent completion
                    finished = true;
                };
                auto running = make_eager(controller());
                check(!finished && alive == 3 && !weak.expired());
                gate.set();
                sync_wait(running);
                check(finished && alive == 0);
            }
            check(weak.expired());
            { auto unstarted = makeOperation(gate); check(alive == 3); }
            check(alive == 0 && weak.expired());
            allocations = {};
            auto ownedPredicate = [](std::size_t, OwnedResource&) { return true; };
            using ThrowingOperation = detail::all_of_operation<inline_scheduler, OwningView,
                decltype(ownedPredicate), CountAllocation>;
            int caught = 0;
            try
            {
                ThrowingOperation operation(executor, 7, OwningView{&alive, std::make_shared<unsigned>(42), 1},
                    ownedPredicate, CountAllocation{&allocations});
            }
            catch (int value) { caught = value; }
            check(caught == 19 && alive == 0 && allocations.allocated == 1 && allocations.freed == 1);
        }
        // A move-only coroutine parameter, whose destruction tests frame cleanup
        // rather than merely block-local scope cleanup.
        struct Cleanup
        {
            bool* done;
            explicit Cleanup(bool& value) : done(&value) {}
            Cleanup(Cleanup&& other) : done(std::exchange(other.done, nullptr)) {}
            ~Cleanup() { if (done) *done = true; }
        };
        task<bool> gated(async_manual_reset_event& gate, Cleanup cleanup, bool result, int error)
        {
            co_await gate;
            if (error) throw error;
            co_return result;
        }
        template<class Fn>
        task<bool> onPool(thread_pool& pool, std::span<Local> locals, Fn fn)
        {
            co_await pool.schedule();
            co_return co_await all_of(pool, 37, locals, fn);
        }
        struct alignas(64) AlignedPredicate
        {
            std::shared_ptr<int> owner;
            std::unique_ptr<int> value = std::make_unique<int>(42);
            async_manual_reset_event* gate;
            explicit AlignedPredicate(std::shared_ptr<int> p, async_manual_reset_event* g = nullptr)
                : owner(std::move(p)), gate(g) {}
            AlignedPredicate(AlignedPredicate&&) = default;
            AlignedPredicate(const AlignedPredicate&) = delete;
            task<bool> operator()(std::size_t, Local& local)
            {
                if (gate) co_await *gate;
                check(reinterpret_cast<std::uintptr_t>(this) % alignof(AlignedPredicate) == 0);
                check(owner && *owner == 1 && value && *value == 42);
                ++local.calls;
                co_return true;
            }
        };
    }

    void all_of_coverage_test()
    {
        CountingExecutor executor;
        for (std::size_t count : {0u, 1u, 2u, 37u})
        {
            std::array<Local, 4> locals;
            std::array<unsigned, 37> seen{};
            auto predicate = [&](std::size_t i, Local& local) {
                Lease lease(local);
                check(reinterpret_cast<std::uintptr_t>(&local) % alignof(Local) == 0);
                ++local.calls;
                ++seen[i];
                return true;
            };
            auto operation = all_of(executor, count, std::span(locals), predicate);
            check(executor.submissions == 0 && locals[0].calls == 0);
            auto moved = std::move(operation);
            check(sync_wait(std::move(moved)));
            for (std::size_t i = 0; i != seen.size(); ++i) check(seen[i] == (i < count));
            check(executor.submissions == (count <= 1 ? 0 : std::min(count, locals.size())));
            executor.submissions = 0;
        }
        check(sync_wait(all_of(executor, 0, std::span<Local>{},
            [](std::size_t, Local&) { throw 99; return true; })));
        bool rejected = false;
        try { auto bad = all_of(executor, 1, std::span<Local>{},
            [](std::size_t, Local&) { return true; }); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected);

        std::array<Local, 3> locals;
        int caught = 0;
        try { sync_wait(all_of(executor, 1, std::span(locals),
            [](std::size_t, Local&) -> bool { throw 17; })); }
        catch (int e) { caught = e; }
        check(caught == 17 && executor.submissions == 0);
        check(!sync_wait(all_of(executor, std::numeric_limits<std::size_t>::max(), std::span(locals),
            [](std::size_t i, Local& local) { check(i == 0); ++local.calls; return false; })));
        check(locals[0].calls == 1 && locals[1].calls == 0 && locals[2].calls == 0);
    }

    void all_of_async_test()
    {
        for (auto n : {1u, 2u})
        {
            Pool executor(n);
            std::array<Local, 4> locals;
            std::array<unsigned, 37> seen{};
            auto predicate = [&](std::size_t i, Local& local) -> task<bool> {
                Lease lease(local);
                co_await executor.pool.schedule();
                ++local.calls;
                ++seen[i];
                co_return true;
            };
            check(sync_wait(onPool(executor.pool, locals, predicate)));
            for (auto count : seen) check(count == 1);
            for (const auto& local : locals) check(!local.busy.load());
        }
    }

    void all_of_stop_join_test()
    {
        // Two started predicates: rejection/error must not resume the controller
        // while the other predicate is suspended. Exercise both completion orders.
        for (bool rejectFirst : {false, true})
        for (int error : {0, 42})
        for (int secondError : {0, 77})
        {
            inline_scheduler executor;
            std::array<Local, 2> locals;
            async_manual_reset_event first, second;
            std::array<bool, 2> destroyed{false, false};
            std::array<unsigned, 8> seen{};
            bool finished = false, answer = true;
            int caught = 0;
            auto predicate = [&](std::size_t i, Local&) {
                ++seen[i];
                const auto partition = i / 4;
                return gated(partition ? second : first, Cleanup(destroyed[partition]),
                    partition == 0, partition == 0 ? error : secondError);
            };
            auto controller = [&]() -> task<> {
                try { answer = co_await all_of(executor, 8, std::span(locals), predicate); }
                catch (int e) { caught = e; }
                check(destroyed[0] && destroyed[1]);
                finished = true;
            };
            auto operation = make_eager(controller());
            check(!finished && seen[0] == 1 && seen[4] == 1);
            (rejectFirst ? second : first).set();
            check(!finished);
            (rejectFirst ? first : second).set();
            sync_wait(operation);
            const int expectedError = error ? error : secondError;
            check(finished && caught == expectedError && (expectedError || !answer));
            if (rejectFirst || error) check(seen[1] == 0);
            check(seen[5] == 0);
        }
    }

    void all_of_ownership_test()
    {
        checkIndexedResources();
        inline_scheduler executor;
        std::array<Local, 3> locals;
        std::weak_ptr<int> weak;
        auto makeOperation = [&](async_manual_reset_event* gate = nullptr) {
            auto owner = std::make_shared<int>(1);
            weak = owner;
            AlignedPredicate predicate(owner, gate);
            return all_of(executor, 7, std::span(locals), std::move(predicate));
        };
        {
            auto unstarted = makeOperation();
            check(!weak.expired());
        }
        check(weak.expired());
        {
            auto operation = makeOperation();
            auto moved = std::move(operation);
            check(!weak.expired() && locals[0].calls == 0);
            check(sync_wait(std::move(moved)));
        }
        check(weak.expired());
        check(locals[0].calls + locals[1].calls + locals[2].calls == 7);
        {
            async_manual_reset_event gate;
            // The original callable is gone before start. Every worker then
            // suspends inside its owned callable before reading its members.
            auto operation = make_eager(makeOperation(&gate));
            check(!weak.expired());
            check(locals[0].calls + locals[1].calls + locals[2].calls == 7);
            auto moved = std::move(operation);
            gate.set();
            check(sync_wait(moved));
        }
        check(weak.expired());
        check(locals[0].calls + locals[1].calls + locals[2].calls == 14);
    }

    void all_of_submission_failure_test()
    {
        struct Executor
        {
            void post_batch(const work_item*, std::size_t) { throw 7; }
        } executor;
        std::array<Local, 3> locals;
        async_manual_reset_event gate;
        bool destroyed = false, finished = false;
        auto predicate = [&](std::size_t, Local&) { return gated(gate, Cleanup(destroyed), true, 0); };
        auto controller = [&]() -> task<> {
            int caught = 0;
            try { co_await all_of(executor, 3, std::span(locals), predicate); }
            catch (int value) { caught = value; }
            check(caught == 7 && !destroyed);
            finished = true;
        };
        auto operation = make_eager(controller());
        check(finished && !destroyed);
        sync_wait(operation);
        check(finished);

        // A named operation survives the failed await. Child resources must
        // already be gone in the parent's catch, for both predicate modes.
        auto checkRetained = [&](auto retainedPredicate) {
            unsigned alive = 0;
            auto retained = all_of(executor, 3, OwningView{&alive}, retainedPredicate);
            check(alive == 3);
            bool caught = false;
            auto awaitRetained = [&]() -> task<> {
                try { co_await retained; }
                catch (int value) { check(value == 7 && alive == 0); caught = true; }
                check(caught && alive == 0);
            };
            sync_wait(awaitRetained());
            check(caught && alive == 0);
        };
        unsigned calls = 0;
        checkRetained([&](std::size_t, OwnedResource&) { ++calls; return true; });
        checkRetained([&](std::size_t, OwnedResource&) -> task<bool> { ++calls; co_return true; });
        check(calls == 0);
    }

    void all_of_adapter_test()
    {
        unsigned resumed = 0;
        auto callback = detail::await_callback::create(
            {&resumed, [](void* p) noexcept { ++*static_cast<unsigned*>(p); }});
        auto handle = callback.handle();
        check(!handle.done());
        auto moved = std::move(callback);
        check(!callback.handle() && moved.handle() == handle);
        for (unsigned i = 0; i != 32; ++i)
        {
            handle.resume();
            check(resumed == i + 1 && !handle.done());
        }
        moved.reset();
        check(!moved.handle());
        // The notification may destroy the bridge that just resumed. It must
        // already be suspended, with no later access to its coroutine frame.
        detail::await_callback selfDestroy;
        selfDestroy = detail::await_callback::create({&selfDestroy, [](void* p) noexcept {
            static_cast<detail::await_callback*>(p)->reset();
        }});
        selfDestroy.handle().resume();
        check(!selfDestroy.handle());

        // A bridge is suspended before notification, even while that notification
        // is still running. Resume it again on another thread and destroy it from
        // the second notification; the first must return without touching its frame.
        struct OverlappingNotification
        {
            detail::await_callback bridge;
            std::binary_semaphore entered{0}, release{0};
            std::atomic<unsigned> calls{0};
        } overlap;
        overlap.bridge = detail::await_callback::create({&overlap, [](void* p) noexcept {
            auto& state = *static_cast<OverlappingNotification*>(p);
            if (state.calls.fetch_add(1) == 0)
            {
                state.entered.release();
                state.release.acquire();
            }
            else state.bridge.reset();
        }});
        auto overlapHandle = overlap.bridge.handle();
        std::thread firstNotification([overlapHandle] { overlapHandle.resume(); });
        overlap.entered.acquire();
        overlapHandle.resume();
        overlap.release.release();
        firstNotification.join();
        check(overlap.calls == 2 && !overlap.bridge.handle());
        checkImmediate<0>(); // ready
        checkImmediate<1>(); // await_suspend returns false
        checkImmediate<2>(); // resumes inline before void await_suspend returns
        checkImmediate<3>(); // returns its continuation (handle transfer)
        checkImmediate<4>(); // await_ready throws
        checkImmediate<5>(); // await_suspend throws
        checkImmediate<6>(); // await_resume throws
        inline_scheduler executor;
        std::array<Local, 1> locals;
        AwaitCounts counts;
        Immediate<0> borrowed(counts);
        check(sync_wait(all_of(executor, 7, std::span(locals),
            [&](std::size_t, Local&) -> Immediate<0>& { return borrowed; })));
        check(counts.alive == 1 && counts.calls == 7);
        RaceState race;
        Pool pool(1); // destroyed/joined before the borrowed race state
        check(sync_wait(all_of(executor, 8, std::span(locals),
            [&](std::size_t, Local&) { return EarlyResume{pool.pool, race}; })));
    }

    void all_of_allocation_test()
    {
        inline_scheduler executor;
        std::array<Local, 3> locals;
        for (std::size_t n : {0u, 1u, 37u})
        {
            AllocationCounts counts;
            auto predicate = [](std::size_t, Local& local) { ++local.calls; return true; };
            using Operation = detail::all_of_operation<inline_scheduler, std::span<Local>, decltype(predicate), CountAllocation>;
            {
                Operation op(executor, n, std::span(locals), predicate, CountAllocation{&counts});
                check(counts.allocated == 1 && counts.freed == 0);
                check(sync_wait(std::move(op)));
            }
            check(counts.allocated == 1 && counts.freed == 1);
        }
        // The allocation hook counts the shared block only. Async partitions
        // also own real coroutine frames. Verify each reuses its own handle
        // across every item, instead of constructing a bridge per item.
        struct BridgeUse { void* address = nullptr; unsigned calls = 0; };
        struct ObserveBridge
        {
            BridgeUse& use;
            bool await_ready() const noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
            {
                if (!use.address) use.address = h.address();
                check(use.address == h.address());
                ++use.calls;
                return h;
            }
            bool await_resume() const noexcept { return true; }
        };
        std::array<BridgeUse, 3> bridges;
        AllocationCounts shared;
        auto asyncPredicate = [](std::size_t, BridgeUse& use) { return ObserveBridge{use}; };
        using AsyncOperation = detail::all_of_operation<inline_scheduler, std::span<BridgeUse>,
            decltype(asyncPredicate), CountAllocation>;
        {
            AsyncOperation op(executor, 37, std::span(bridges),
                asyncPredicate, CountAllocation{&shared});
            check(shared.allocated == 1 && shared.freed == 0);
            check(sync_wait(std::move(op)));
            check(bridges[0].calls == 13 && bridges[1].calls == 12 && bridges[2].calls == 12);
            check(bridges[0].address != bridges[1].address &&
                bridges[0].address != bridges[2].address && bridges[1].address != bridges[2].address);
        }
        check(shared.allocated == 1 && shared.freed == 1);
        AllocationCounts counts;
        struct ThrowOnMove
        {
            ThrowOnMove() = default;
            ThrowOnMove(ThrowOnMove&&) { throw 42; }
            bool operator()(std::size_t, Local&) { return true; }
        } fn;
        using Operation = detail::all_of_operation<inline_scheduler, std::span<Local>, ThrowOnMove, CountAllocation>;
        int caught = 0;
        try { Operation op(executor, 7, std::span(locals), std::move(fn), CountAllocation{&counts}); }
        catch (int value) { caught = value; }
        check(caught == 42 && counts.allocated == 1 && counts.freed == 1);
    }
}
#endif
