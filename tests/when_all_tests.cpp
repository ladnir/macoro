#include "when_all_tests.h"
#include "macoro/when_all.h"
#include "macoro/task.h"
#include "macoro/sync_wait.h"
#include "macoro/manual_reset_event.h"
#include "macoro/async_scope.h"
#include "macoro/when_all_scope.h"
#include <algorithm>
#include <array>

namespace macoro
{
	namespace tests
	{

		auto f = []() -> task<int> {
			//std::cout << "trace\n" << (co_await get_trace{}).str() << std::endl;
			co_return(42);
			};


		auto g = []() -> task<bool> {
			MC_BEGIN(task<bool>);
			MC_RETURN(true);
			MC_END();
			};

		bool b = true;
		auto h = []() -> task<bool&> {
			MC_BEGIN(task<bool&>);
			MC_RETURN(b);
			MC_END();
			};

		void when_all_basic_tests()
		{
			static_assert(
				is_awaitable<
				task<int>
				>::value
				, "");

			static_assert(std::is_same<
				remove_reference_and_wrapper_t<task<int>>,
				task<int>
			>::value, "");


			static_assert(
				is_awaitable<
				remove_reference_and_wrapper_t<task<int>>
				>::value
				, "");

			static_assert(
				conjunction<
				is_awaitable<
				remove_reference_and_wrapper_t<task<int>>
				>,
				is_awaitable<
				remove_reference_and_wrapper_t<task<bool>>
				>
				>::value
				, "");

			auto ff = f();
			auto gg = g();

			auto t = [&]()->task<
				std::tuple <
				detail::when_all_task<int>,
				detail::when_all_task<bool&>
				>>
			{
				co_return co_await when_all_ready(std::move(ff), h());
			};

			auto r2 = sync_wait(t());

			detail::when_all_task<int> r0 = std::move(std::get<0>(r2));
			detail::when_all_task<bool&> r1 = std::move(std::get<1>(r2));

			if (r0.result() != 42)
				throw std::runtime_error(MACORO_LOCATION);
			if (&r1.result() != &b)
				throw std::runtime_error(MACORO_LOCATION);

		}


  void when_all_scope_test()
    {
      auto retained = std::make_shared<int>(0);
      auto unstarted = [](std::shared_ptr<int> owner) -> when_all_scope {
        co_return;
      };
      {
        auto scope = unstarted(retained);
        auto moved = std::move(scope);
        if (retained.use_count() != 2) throw std::runtime_error(MACORO_LOCATION);
      }
      if (retained.use_count() != 1) throw std::runtime_error(MACORO_LOCATION);

      auto f = []() -> task<int> {
        co_return 42;
        };
  
      auto g = []() -> task<std::unique_ptr<int>> {
        co_return new int{ 10 };
        };
  
      // a coroutine with RAII eager subtasks.
      // the eager subtasks will be joined before
      // the scope exits.
      auto t = [&]()->task<> {
  
        co_await f();
  
        // start a eager scope. All scoped
        // tasks in the when_all_scope will 
        // be joined before scope exit.
        co_await [&]()->when_all_scope {
  
          // start the work eagerly
          scoped_task<int> r = co_await scoped(f());
  
          // start the work eagerly
          scoped_task<std::unique_ptr<int>> h = co_await scoped(g());
  
          // join one of them
          int i = co_await std::move(r);
  
          if (i == 42)
            throw std::runtime_error("meaning of life");
  
          // h wont be joined explicitly if i == 42. However, 
          // when_all_scope will suspend until h completes. 
          // exceptions will be propegated out of the scope.
          std::unique_ptr<int> j = co_await std::move(h);
          }
        ();
  
        // h has been joined and an exception containing 
        // "meaning of life" will be thrown.
        };
  
      bool caught = false;
      try { sync_wait(t()); }
      catch (const async_scope_exception& error)
      {
        if (error.m_exceptions.size() != 1) throw std::runtime_error(MACORO_LOCATION);
        try { std::rethrow_exception(error.m_exceptions.front()); }
        catch (const std::runtime_error& inner)
        {
          caught = std::string(inner.what()) == "meaning of life";
        }
      }
      if (!caught) throw std::runtime_error(MACORO_LOCATION);
    }

    namespace
    {
      struct ScopeCleanupGuard
      {
        bool* destroyed;
        explicit ScopeCleanupGuard(bool& value) : destroyed(&value) {}
        ScopeCleanupGuard(ScopeCleanupGuard&& other) noexcept
          : destroyed(std::exchange(other.destroyed, nullptr)) {}
        ~ScopeCleanupGuard() { if (destroyed) *destroyed = true; }
      };

      // This guard belongs to async_scope::add's parameter, not a body local.
      struct ScopeOwnedWait
      {
        async_manual_reset_event& gate;
        ScopeCleanupGuard guard;
        bool& resultDestroyed;
        bool fail;
        auto operator co_await()
        {
          struct Awaiter : async_manual_reset_event_operation
          {
            bool& resultDestroyed;
            bool fail;
            ScopeCleanupGuard await_resume()
            {
              if (fail) throw 8;
              return ScopeCleanupGuard(resultDestroyed);
            }
          };
          return Awaiter{gate.operator co_await(), resultDestroyed, fail};
        }
      };

      task<> gatedScopeChild(async_manual_reset_event& gate, bool& finished, int error)
      {
        struct Finish
        {
          bool& finished;
          ~Finish() { finished = true; }
        } finish{finished};
        co_await gate;
        if (error) throw error;
      }
    }

    void when_all_scope_cleanup_test()
    {
      // Cover both immediate and suspended completion, success, body failure,
      // and child failure. The successful child's ignored result needs cleanup too.
      for (bool ready : {false, true})
      for (int failure : {0, 7, 8})
      {
        async_manual_reset_event gate(ready);
        bool parameterDestroyed = false, resultDestroyed = false;
        bool cleanupAtResume = false, parentFinished = false;
        int caught = 0;
        auto body = [&]() -> when_all_scope {
          co_await scoped(ScopeOwnedWait{gate, ScopeCleanupGuard(parameterDestroyed),
            resultDestroyed, failure == 8});
          if (failure == 7) throw 7;
        };
        auto observe = [&]() -> task<> {
          try { co_await body(); }
          catch (const async_scope_exception& error)
          {
            if (error.m_exceptions.size() != 1) throw std::runtime_error(MACORO_LOCATION);
            try { std::rethrow_exception(error.m_exceptions.front()); }
            catch (int value) { caught = value; }
          }
          cleanupAtResume = parameterDestroyed && (failure == 8 || resultDestroyed);
          parentFinished = true;
        };
        auto parent = make_eager(observe());
        const bool early = !ready && parentFinished;
        gate.set();
        sync_wait(parent);
        if (early || !parentFinished || !cleanupAtResume || caught != failure)
          throw std::runtime_error(MACORO_LOCATION);
      }
    }

    void when_all_scope_error_join_test()
    {
      struct Case
      {
        int bodyError, firstError, secondError;
        bool firstReady, handleFirst;
      };
      const std::array<Case, 5> cases{{
        {7, 0, 0, false, false}, // Body error, both children still suspended.
        {0, 8, 0, true, false},  // Child error before the scope begins joining.
        {0, 8, 0, false, false}, // Child error after the join has suspended.
        {7, 8, 9, false, false}, // Aggregate body and both child errors.
        {0, 8, 0, false, true}   // Explicitly handled child error is not reported twice.
      }};
      for (const auto& test : cases)
      {
        async_manual_reset_event firstGate(test.firstReady), secondGate;
        bool firstFinished = false, secondFinished = false;
        bool parentFinished = false, childrenFinishedAtCatch = false, handled = false;
        std::vector<int> errors;
        auto body = [&]() -> when_all_scope {
          auto first = co_await scoped(gatedScopeChild(firstGate, firstFinished, test.firstError));
          auto second = co_await scoped(gatedScopeChild(secondGate, secondFinished, test.secondError));
          if (test.handleFirst)
          {
            try { co_await std::move(first); }
            catch (int error) { handled = error == test.firstError; }
          }
          if (test.bodyError) throw test.bodyError;
        };
        auto observe = [&]() -> task<> {
          try { co_await body(); }
          catch (const async_scope_exception& error)
          {
            childrenFinishedAtCatch = firstFinished && secondFinished;
            for (const auto& exception : error.m_exceptions)
            {
              try { std::rethrow_exception(exception); }
              catch (int value) { errors.push_back(value); }
            }
          }
          parentFinished = true;
        };
        auto parent = make_eager(observe());
        const bool beforeChildren = parentFinished || parent.is_ready();
        firstGate.set();
        const bool beforeLastChild = parentFinished || parent.is_ready();
        secondGate.set();
        sync_wait(parent);

        std::vector<int> expected;
        if (test.bodyError) expected.push_back(test.bodyError);
        if (test.firstError && !test.handleFirst) expected.push_back(test.firstError);
        if (test.secondError) expected.push_back(test.secondError);
        std::sort(expected.begin(), expected.end());
        std::sort(errors.begin(), errors.end());
        if (beforeChildren || beforeLastChild || !parentFinished ||
            !firstFinished || !secondFinished || errors != expected ||
            (!expected.empty() && !childrenFinishedAtCatch) || handled != test.handleFirst)
          throw std::runtime_error(MACORO_LOCATION);
      }

      // The same join helper serves a plain async_scope. A second join must
      // not repeat the error consumed by the first join.
      async_scope scope;
      async_manual_reset_event gate;
      bool finished = false;
      scope.add(gatedScopeChild(gate, finished, 11));
      auto joined = make_eager(scope.join());
      const bool early = joined.is_ready();
      gate.set();
      bool caught = false;
      try { sync_wait(joined); }
      catch (const async_scope_exception& error)
      {
        if (error.m_exceptions.size() != 1) throw std::runtime_error(MACORO_LOCATION);
        try { std::rethrow_exception(error.m_exceptions.front()); }
        catch (int value) { caught = value == 11 && finished; }
      }
      sync_wait(scope.join());
      if (early || !caught) throw std::runtime_error(MACORO_LOCATION);
    }
	}
}

