#include "macoro/async_scope.h"
#include "macoro/task.h"
#include "macoro/sync_wait.h"
#include "macoro/manual_reset_event.h"

namespace macoro
{

	namespace tests
	{

		//struct task_of
		//{
		//	struct promise_type
		//	{
		//		struct final_suspend
		//		{
		//			bool await_ready() noexcept { return false; }

		//			std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> c)noexcept
		//			{
		//				c.destroy();
		//				return std::noop_coroutine();
		//			}

		//			void await_resume() noexcept{}
		//		};

		//		std::suspend_always initial_suspend() noexcept { return {}; }
		//		std::suspend_always final_suspend() noexcept { return {}; }
		//		void unhandled_exception() noexcept {}
		//		task_of get_return_object() noexcept { return {}; }

		//		void return_void() {}
		//	};

		//};

		//task_of foo()
		//{
		//	co_return;
		//}

		void scoped_task_test()
		{

			struct scope_guard
			{
				bool* destroyed = nullptr;
				scope_guard(bool& d)
					:destroyed(&d)
				{}

				scope_guard(scope_guard&& g)
					:destroyed(std::exchange(g.destroyed, nullptr))
				{}

				~scope_guard()
				{
					if (destroyed)
						*destroyed = true;
				}
			};

			// continuation after completion
			{
				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<int> {
					co_return 42;
					};

				async_scope scope;
				scoped_task<int> t = scope.add(f({ destroyed }));

				if (t.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);

				//auto a = sync_wait(std::move(f({ destroyed })));
				auto v = sync_wait(std::move(t));

				if (v != 42)
					throw std::runtime_error(MACORO_LOCATION);

				if (!destroyed)
					throw std::runtime_error(MACORO_LOCATION);
			}

			// continuation before completion.
			{
				async_manual_reset_event e;


				auto f = [&](scope_guard g) -> task<int> {
					co_await e;
					co_return 42;
					};
				async_scope scope;
				bool destroyed = false;
				scoped_task<int> t = scope.add(f({ destroyed }));

				if (t.is_ready() == true)
					throw std::runtime_error(MACORO_LOCATION);

				if (destroyed)
					throw std::runtime_error(MACORO_LOCATION);

				auto eager = [&]() -> eager_task<int> {
					auto v = co_await std::move(t);
					co_return v;
					}
				();


				if (destroyed)
					throw std::runtime_error(MACORO_LOCATION);

				if (eager.is_ready() == true)
					throw std::runtime_error(MACORO_LOCATION);

				e.set();

				if (eager.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);

				if (destroyed == false)
					throw std::runtime_error(MACORO_LOCATION);

				auto v = sync_wait(eager);

				if (v != 42)
					throw std::runtime_error(MACORO_LOCATION);
			}


			// drops before completion
			{
				async_manual_reset_event e;

				bool called = false, destroyed = false;
				auto f = [&](scope_guard g) -> task<int> {
					co_await e;
					called = true;
					co_return 42;
					};
				async_scope scope;
				scope.add(f({ destroyed }));

				if (called)
					throw std::runtime_error(MACORO_LOCATION);

				e.set();

				if (!called)
					throw std::runtime_error(MACORO_LOCATION);

				if (destroyed == false)
					throw std::runtime_error(MACORO_LOCATION);

				sync_wait(scope);
			}


			// joins before completion
			{
				async_manual_reset_event e;

				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<int> {
					co_await e;
					co_return 42;
					};
				async_scope scope;
				scoped_task<int> t = scope.add(f({ destroyed }));

				if (t.is_ready() == true)
					throw std::runtime_error(MACORO_LOCATION);

				auto eager = [&]() -> eager_task<> {
					co_await scope;
					}
				();

				if (eager.is_ready() == true)
					throw std::runtime_error(MACORO_LOCATION);

				e.set();

				if (t.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);
				if (eager.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);

				sync_wait(eager);


				if (destroyed)
					throw std::runtime_error(MACORO_LOCATION);

				auto v = sync_wait(std::move(t));

				if(v != 42)
					throw std::runtime_error(MACORO_LOCATION);

				if (!destroyed)
					throw std::runtime_error(MACORO_LOCATION);
			}


			// joins before completion and then discard
			{
				async_manual_reset_event e;

				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<int> {
					co_await e;
					co_return 42;
					};
				async_scope scope;
				scoped_task<int> t = scope.add(f({ destroyed }));

				if (t.is_ready() == true)
					throw std::runtime_error(MACORO_LOCATION);

				auto eager = [&]() -> eager_task<> {
					co_await scope;
					}
				();

				if (eager.is_ready() == true)
					throw std::runtime_error(MACORO_LOCATION);

				e.set();

				if (t.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);
				if (eager.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);

				sync_wait(eager);

				if (destroyed)
					throw std::runtime_error(MACORO_LOCATION);

				t = {};

				if (!destroyed)
					throw std::runtime_error(MACORO_LOCATION);
			}


			struct test_exception : std::exception
			{

			};

			// get exception
			{
				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<int> {
					throw test_exception{};
					co_return 42;
					};
				async_scope scope;

				{
					scoped_task<int> t = scope.add(f({ destroyed }));

					if (t.is_ready() == false)
						throw std::runtime_error(MACORO_LOCATION);

					if (destroyed == true)
						throw std::runtime_error(MACORO_LOCATION);

					try
					{
						auto v = sync_wait(std::move(t));
						throw std::runtime_error("failed");
					}
					catch (test_exception& e)
					{
					}
				}


				if (destroyed == false)
					throw std::runtime_error(MACORO_LOCATION);

			}


			// get exception join
			{
				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<int> {
					throw test_exception{};
					co_return 42;
					};
				async_scope scope;

				{
					scoped_task<int> t = scope.add(f({ destroyed }));

					if (t.is_ready() == false)
						throw std::runtime_error(MACORO_LOCATION);

					if (destroyed == true)
						throw std::runtime_error(MACORO_LOCATION);
				}

				if (destroyed == false)
					throw std::runtime_error(MACORO_LOCATION);

				try
				{
					sync_wait(scope);
					throw std::runtime_error("failed");
				}
				catch (async_scope_exception& e)
				{
					if(e.m_exceptions.size() != 1)
						throw std::runtime_error(MACORO_LOCATION);

					try
					{
						std::rethrow_exception(e.m_exceptions[0]);
					}
					catch (test_exception& e)
					{
					}
				}
			}

			// reference type
			{
				int x = 0;
				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<int&> {
					co_return x;
					};

				async_scope scope;
				scoped_task<int&> t = scope.add(f({ destroyed }));

				if (t.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);

				auto& v = sync_wait(std::move(t));

				if (&v != &x)
					throw std::runtime_error(MACORO_LOCATION);

				if (!destroyed)
					throw std::runtime_error(MACORO_LOCATION);
			}

			// move only type
			{
				std::unique_ptr<int> x(new int{ 42 });
				auto addr = x.get();
				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<std::unique_ptr<int>> {
					co_return std::move(x);
					};

				async_scope scope;
				scoped_task<std::unique_ptr<int>> t = scope.add(f({ destroyed }));

				if (t.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);

				auto v = sync_wait(std::move(t));

				if (v.get() != addr)
					throw std::runtime_error(MACORO_LOCATION);

				if (!destroyed)
					throw std::runtime_error(MACORO_LOCATION);
			}


			// pointer type
			{
				int X = 42;
				int* x = &X;
				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<int*> {
					co_return x;
					};

				async_scope scope;
				scoped_task<int*> t = scope.add(f({ destroyed }));

				if (t.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);

				auto v = sync_wait(std::move(t));

				if (v != x)
					throw std::runtime_error(MACORO_LOCATION);

				if (!destroyed)
					throw std::runtime_error(MACORO_LOCATION);
			}



			// void type
			{
				bool destroyed = false;
				auto f = [&](scope_guard g) -> task<> {
					co_return;
					};

				async_scope scope;
				scoped_task<> t = scope.add(f({ destroyed }));

				if (t.is_ready() == false)
					throw std::runtime_error(MACORO_LOCATION);

				sync_wait(std::move(t));

				if (!destroyed)
					throw std::runtime_error(MACORO_LOCATION);
			}
		}


		void async_scope_test()
		{


		}
	}
}