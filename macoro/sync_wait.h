#pragma once

#include "macoro/type_traits.h"
#include "macoro/coro_frame.h"
#include "macoro/optional.h"
#include "macoro/coroutine_handle.h"
#include <future>
#include "Macros.h"
namespace macoro
{

	namespace detail
	{
		template<typename T>
		struct blocking_task;


		template<typename T>
		struct blocking_promise_base
		{
			struct final_awaiter
			{
				bool await_ready() { return false; }
#ifdef MACORO_CPP_20
				template<typename T>
				void await_suspend(std::coroutine_handle<T> h) { h.promise().set(); }
#endif
				template<typename T>
				void await_suspend(coroutine_handle<T> h) { h.promise().set(); }
				void await_resume() {}
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
			final_awaiter final_suspend() noexcept {
				return { };
			}

			void return_void() {}


			void unhandled_exception() noexcept
			{
				exception = std::current_exception();
			}
		};

		template<typename T>
		struct blocking_promise : public blocking_promise_base<T>
		{
			typename std::remove_reference<T>::type* mVal = nullptr;

			blocking_task<T> get_return_object() noexcept;
			blocking_task<T> macoro_get_return_object() noexcept;

			using reference_type = T&&;
			suspend_always yield_value(reference_type v) noexcept
			{
				mVal = std::addressof(v);
				this->set();
				return { };
			}

			reference_type value()
			{
				if (this->exception)
					std::rethrow_exception(this->exception);
				return static_cast<reference_type>(*mVal);
			}
		};

		template<>
		struct blocking_promise<void> : public blocking_promise_base<void>
		{
			blocking_task<void> get_return_object() noexcept;
			blocking_task<void> macoro_get_return_object() noexcept;

			void value()
			{
				if (this->exception)
					std::rethrow_exception(this->exception);
			}
		};


		template<typename T>
		struct blocking_task
		{
			using promise_type = blocking_promise<T>;
			coroutine_handle<promise_type> handle;
			blocking_task(coroutine_handle<promise_type> h)
				: handle(h)
			{}

			blocking_task() = delete;
			blocking_task(blocking_task&& h) noexcept :handle(std::exchange(h.handle, std::nullptr_t{})) {}
			blocking_task& operator=(blocking_task&& h) { handle = std::exchange(h.handle, std::nullptr_t{}); }

			~blocking_task()
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



		template<typename T>
		inline blocking_task<T> blocking_promise<T>::get_return_object() noexcept { return { coroutine_handle<blocking_promise<T>>::from_promise(*this, coroutine_handle_type::std) }; }
		template<typename T>
		inline blocking_task<T> blocking_promise<T>::macoro_get_return_object() noexcept { return { coroutine_handle<blocking_promise<T>>::from_promise(*this, coroutine_handle_type::macoro) }; }
		inline blocking_task<void> blocking_promise<void>::get_return_object() noexcept { return { coroutine_handle<blocking_promise<void>>::from_promise(*this, coroutine_handle_type::std) }; }
		inline blocking_task<void> blocking_promise<void>::macoro_get_return_object() noexcept { return { coroutine_handle<blocking_promise<void>>::from_promise(*this, coroutine_handle_type::macoro) }; }

		template<
			typename Awaitable,
			typename ResultType = typename awaitable_traits<Awaitable&&>::await_result>
			enable_if_t<!std::is_void<ResultType>::value,
			blocking_task<ResultType>
			>
			make_blocking_task(Awaitable&& awaitable)
		{
#if MACORO_MAKE_BLOCKING_20
			auto promise = co_await typename blocking_promise<ResultType>::get_promise{};
			auto awaiter = promise->yield_value(co_await std::forward<Awaitable>(awaitable));
			co_await awaiter;
#else
			MC_BEGIN(blocking_task<ResultType>, &awaitable);

			// co_yield co_await static_cast<Awaitable&&>(awaitable)
			MC_YIELD_AWAIT(static_cast<Awaitable&&>(awaitable));
			MC_END();

#endif
		}

		template<
			typename Awaitable,
			typename ResultType = typename awaitable_traits<Awaitable&&>::await_result>
			enable_if_t<std::is_void<ResultType>::value,
			blocking_task<ResultType>
			>
			make_blocking_task(Awaitable&& awaitable)
		{
#if MACORO_MAKE_BLOCKING_20
			co_await std::forward<Awaitable>(awaitable);
#else
			MC_BEGIN(blocking_task<ResultType>, &awaitable);
			MC_AWAIT(static_cast<Awaitable&&>(awaitable));
			MC_END();
#endif
		}
	}


	template<typename Awaitable>
	typename awaitable_traits<Awaitable&&>::await_result
		sync_wait(Awaitable&& awaitable)
	{
		auto task = detail::make_blocking_task<Awaitable&&>(std::forward<Awaitable>(awaitable));
		task.start();
		return task.get();
	}


}