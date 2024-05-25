#include "result_tests.h"

#include "macoro/result.h"
#include "macoro/wrap.h"
#include "macoro/sync_wait.h"
namespace macoro
{
	namespace tests
	{
		void result_basic_store_test()
		{
			result<int> r = Ok(42);

			if (!r.has_value())
				throw MACORO_RTE_LOC;
			if(r.value() != 42)
				throw MACORO_RTE_LOC;

			try {
				r.error();
			}
			catch (...)
			{
				r = Err(std::current_exception());
			}

			if(!r.has_error())
				throw MACORO_RTE_LOC;
		}


		void result_co_await_test()
		{
			auto foo = []()->result<int>
			{
				auto b = result<bool>{};
				bool bb = co_await b;
				co_return Ok(bb ? 1 : 42);
			};

			auto r = foo();

			if(!r.has_value())
				throw MACORO_RTE_LOC;
			if(r.value() != 42)
				throw MACORO_RTE_LOC;
			try {
				r.error();
			}
			catch (...)
			{
				r = Err(std::current_exception());
			}

			if(!r.has_error())
				throw MACORO_RTE_LOC;
		}

		void result_task_wrap_test()
		{

			auto foo = []()->task<int>
			{
				auto b = result<bool>{};
				auto bb = b.error();
				co_return 42;
			};

			result<int> r = sync_wait(wrap(foo()));
			if(!r.has_error())
				throw MACORO_RTE_LOC;
		}


		void result_task_wrap_pipe_test()
		{
			auto foo = []()->task<int>
			{
				auto b = result<bool>{};
				auto bb = b.error();
				co_return 42;
			};

			result<int> r = sync_wait(foo() | wrap());
			if (!r.has_error())
				throw MACORO_RTE_LOC;
		}


		void result_void_test()
		{
			auto foo = []()->result<void>
			{
				co_return Ok();
			};

			result<void> r = sync_wait(wrap(foo()));
			if(r.has_error())
				throw MACORO_RTE_LOC;
		}

		void result_unique_awaiter_test()
		{
			struct awaitable
			{
				struct awaiter
				{
					awaiter() = delete;
					awaiter(const awaiter&) = delete;
					awaiter(awaiter&&) = delete;

					awaiter(int) {}
					
					bool await_ready() { return true; }
					void await_suspend(std::coroutine_handle<> h) {}
					void await_resume() {}
				};

				awaitable() = delete;
				awaitable(const awaitable&) = delete;
				awaitable(awaitable&&) = default;
				awaitable(int) {}

				awaiter operator co_await()&&
				{
					return { 0 };
				}
			};

			auto foo = []()->task<void>{
				//co_await wrapped_awaitable<awaitable>{awaitable(0)};
				//co_await wrap(awaitable(0));
				co_await(awaitable(0) | wrap());
				};

			sync_wait(foo());
		}


		void result_ref_awaiter_test()
		{
			struct awaitable
			{
				struct awaiter
				{
					awaiter() = delete;
					awaiter(const awaiter&) = delete;
					awaiter(awaiter&&) = delete;

					int m_val; 
					awaiter(int i) : m_val(i) {}

					bool await_ready() { return true; }
					void await_suspend(std::coroutine_handle<> h) {}
					auto await_resume() { return m_val; }
				};

				awaitable() = delete;
				awaitable(const awaitable&) = delete;
				awaitable(awaitable&&) = delete;
				awaitable(int i) : a(i) {}

				awaiter a;
				awaiter& operator co_await()&
				{
					return a;
				}
			};

			auto foo = []()->task<void> {
				auto a = awaitable(42);
				auto r = co_await(a | wrap());

				if (r.value() != 42)
					throw MACORO_RTE_LOC;
				};
			sync_wait(foo());
		}


		template<typename awaitable>
		decltype(auto) wrap2(awaitable&& a) {
			return wrapped_awaitable<awaitable>(std::forward<awaitable>(a));
		}


		void result_suspend_test()
		{
			struct awaitable
			{
				struct awaiter
				{
					awaiter() = delete;
					awaiter(const awaiter&) = delete;
					awaiter(awaiter&&) = delete;

					std::unique_ptr<int> m_d;
					std::coroutine_handle<> m_h;
					awaiter(std::unique_ptr<int>d) : m_d(std::move(d)) {}
					~awaiter()
					{
						*m_d = 0;
					}


					bool await_ready() { return false; }
					void await_suspend(std::coroutine_handle<> h) {
						m_h = h;
					}
					auto await_resume() { return m_d.get(); }
				};

				awaitable() = delete;
				awaitable(const awaitable&) = delete;
				awaitable(awaitable&&) = default;
				std::unique_ptr<int> m_d;
				awaitable(std::unique_ptr<int> i) : m_d(std::move(i)) {}

				awaiter operator co_await()
				{
					return {std::move(m_d)};
				}
			};

			//auto a = ;
			auto foo = [&]()->task<void> {
				//auto tt = wrapped_awaitable<awaitable>(awaitable(std::unique_ptr<int>{ new int(42) }));
				//auto bb = wrapped_awaitable<awaitable>(std::forward<awaitable&&>(awaitable(std::unique_ptr<int>{ new int(42) })));
				//auto dd = wrap2(awaitable(std::unique_ptr<int>{ new int(42) }));
				auto r = co_await(awaitable(std::unique_ptr<int>{ new int(42) }) | wrap());
				if (*r.value() != 42)
					throw MACORO_RTE_LOC;
				co_return;
				};
			auto task = foo();
			decltype(auto) awaiter = task.operator co_await();
			awaiter.await_ready();
			awaiter.await_suspend(std::noop_coroutine());
			awaiter.m_coroutine.resume();
		}


	}
}
