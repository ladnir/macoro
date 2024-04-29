#include "when_all_tests.h"
#include "macoro/when_all.h"
#include "macoro/task.h"
#include "macoro/sync_wait.h"

namespace macoro
{
	namespace tests
	{
		void when_all_basic_tests()
		{

			auto f = []() -> task<int> {
				MC_BEGIN(task<int>);
				MC_RETURN(42);
				MC_END();
			};


			auto g = []() -> task<bool> {
				MC_BEGIN(task<bool>);
				MC_RETURN(true);
				MC_END();
			};

			bool b;
			auto h = [&]() -> task<bool&> {
				MC_BEGIN(task<bool&>, &b);
				MC_RETURN(b);
				MC_END();
			};

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

			std::tuple <
				detail::when_all_task<int>,
				detail::when_all_task<bool>
			>
				r = sync_wait(when_all_ready(f(), g()));

			auto ff = f();
			auto gg = g();

			std::tuple <
				detail::when_all_task<int>,
				detail::when_all_task<bool&>
			>
				r2 = sync_wait(when_all_ready(std::move(ff), h()));
			//auto r = sync_wait(w);
			detail::when_all_task<int> r0 = std::move(std::get<0>(r));
			assert(std::get<0>(r).result() == 42);
			assert(std::get<1>(r).result() == true);
		}

	}


	#ifdef MACORO_CPP_20
	#define MACORO_MAKE_blocking2_20 1
	#endif


	namespace detail
	{
		struct blocking2_task;


		struct blocking2_promise 
		{
			int* mVal = nullptr;

			blocking2_task get_return_object() noexcept;

			using reference_type = int&;
			void return_value(reference_type v) noexcept
			{
				mVal = std::addressof(v);
			}

			reference_type value()
			{
				if (this->exception)
					std::rethrow_exception(this->exception);
				return static_cast<reference_type>(*mVal);
			}

			struct final_awaiter
			{
				final_awaiter() noexcept{};
				final_awaiter(const final_awaiter&) noexcept {};
				final_awaiter(final_awaiter&&) noexcept {};

				bool await_ready() noexcept { return false; }
#ifdef MACORO_CPP_20
				template<typename P>
				void await_suspend(std::coroutine_handle<P> h) noexcept { h.promise().set(); }
#endif
				template<typename P>
				void await_suspend(coroutine_handle<P> h) noexcept { h.promise().set(); }
				void await_resume() noexcept {}
			};

			std::exception_ptr exception;
			std::mutex mutex;
			std::condition_variable cv;
			bool is_set = false;

			void wait()
			{
				std::unique_lock<std::mutex> lock(mutex);
				cv.wait(lock, [this] { return is_set; });
			}

			void set()
			{
				assert(is_set == false);
				std::lock_guard<std::mutex> lock(this->mutex);
				this->is_set = true;
				this->cv.notify_all();
			}


			suspend_always initial_suspend() noexcept { return{}; }

			final_awaiter final_suspend() noexcept(true) {
				return { };
			}

			void unhandled_exception() noexcept
			{
				exception = std::current_exception();
			}
		};

		//static_assert(std::is_nothrow_invocable<blocking2_promise::final_suspend, blocking2_promise&>::value, "");
		struct blocking2_task
		{
			using promise_type = blocking2_promise;
			coroutine_handle<promise_type> handle;
			blocking2_task(coroutine_handle<promise_type> h)
				: handle(h)
			{}

			blocking2_task() = delete;
			blocking2_task(blocking2_task&& h) noexcept :handle(std::exchange(h.handle, std::nullptr_t{})) {}
			blocking2_task& operator=(blocking2_task&& h) noexcept { handle = std::exchange(h.handle, std::nullptr_t{}); }

			~blocking2_task()
			{
				if (handle)
					handle.destroy();
			}

			void start()
			{
				handle.resume();
			}

			decltype(auto) get()
			{
				handle.promise().wait();
				return handle.promise().value();
			}
		};

		blocking2_task  blocking2_promise::get_return_object() noexcept
		{
			return { coroutine_handle<blocking2_promise>::from_promise(*this, coroutine_handle_type::std) };
		}


//		template<
//			typename Awaitable
//		>
//		blocking2_task
//			make_blocking2_task(Awaitable&& awaitable)
//		{
//#if MACORO_MAKE_blocking2_20
//			//auto promise = co_await typename blocking2_promise<ResultType>::get_promise;
//			//auto awaiter = promise->yield_value(co_await std::forward<Awaitable>(awaitable));
//			//co_await awaiter;
//			co_return co_await static_cast<Awaitable&&>(awaitable);
//#else
//			MC_BEGIN(blocking2_task<ResultType>, &awaitable);
//			MC_RETURN_AWAIT(static_cast<Awaitable&&>(awaitable));
//			MC_END();
//
//#endif
//		}

//		template<
//			typename Awaitable,
//			typename ResultType = typename awaitable_traits<Awaitable&&>::await_result>
//		enable_if_t<std::is_void<ResultType>::value,
//			blocking2_task<ResultType>
//		>
//			make_blocking2_task(Awaitable&& awaitable)
//		{
//#if MACORO_MAKE_blocking2_20
//			co_await std::forward<Awaitable>(awaitable);
//#else
//			MC_BEGIN(blocking2_task<ResultType>, &awaitable);
//			MC_AWAIT(static_cast<Awaitable&&>(awaitable));
//			MC_END();
//#endif
//		}
	}


	//template<typename Awaitable>
	//typename awaitable_traits<Awaitable&&>::await_result
	//	sync_wait2(Awaitable&& awaitable)
	//{
	//	auto task = detail::make_blocking2_task<Awaitable&&>(std::forward<Awaitable>(awaitable));
	//	task.start();
	//	return task.get();
	//}


	//struct sync_wait2_t
	//{
	//};

	//inline sync_wait2_t sync_wait2()
	//{
	//	return {};
	//}


	//template<typename awaitable>
	//decltype(auto) operator|(awaitable&& a, sync_wait2_t)
	//{
	//	return sync_wait2(std::forward<awaitable>(a));
	//}


	void when_all_basic2_tests()
	{

		auto f = []() -> task<int> {
			MC_BEGIN(task<int>);
			MC_RETURN(42);
			MC_END();
			};

		auto g = []() -> task<bool> {
			MC_BEGIN(task<bool>);
			MC_RETURN(true);
			MC_END();
			};

		using F = decltype(f());
		using Awaitable = F;
		using ResultType = int;

		auto task = [&]() -> detail::blocking2_task {
			 auto i = co_await f();
			 co_return i;
			}();
		task.start();
		auto i = task.get();

	}

}

