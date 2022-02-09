#pragma once
#include "macoro/coroutine_handle.h"
#include "macoro/coro_frame.h"
#include "macoro/awaiter.h"
#include "macoro/variant.h"
#include "macoro/type_traits.h"
#include "task.h"
#include <atomic>
namespace macoro
{
	template<typename T>
	struct eager_task;


	namespace impl
	{
		template<typename T>
		struct eager_task_promise;

		struct eager_task_final_awaiter
		{
			bool await_ready() const noexcept { return false; }

			template<typename T>
			auto await_suspend(const std::coroutine_handle<eager_task_promise<T>>& t) noexcept
			{
				return await_suspend_impl(t.promise()).std_cast();
			}
			template<typename T>
			auto await_suspend(const coroutine_handle<eager_task_promise<T>>& t) noexcept
			{
				return await_suspend_impl(t.promise());
			}

			template<typename T>
			coroutine_handle<void> await_suspend_impl(eager_task_promise<T>& promise) noexcept
			{
				// release our result. acquire their continuation (if they beat us).
				auto b = promise.has_completed_or_continutation.exchange(true, std::memory_order::memory_order_acq_rel);

				// if b, then we have finished after the continuation was set. 
				if (b)
				{
					assert(promise.cont);
					return promise.cont;
				}
				else
					return noop_coroutine();
			}


			void await_resume() const noexcept {}
		};

		template<typename T>
		struct eager_task_promise
		{
			eager_task_promise()
				: has_completed_or_continutation(false)
			{}


			using value_type = T;
			using reference_type = T&;
			using rvalue_type = T&&;
			using exception_type = std::exception_ptr;
			using handle_type = coroutine_handle<eager_task_promise<value_type>>;

			std::atomic<bool> has_completed_or_continutation;
			coroutine_handle<> cont;
			variant<empty_state, value_type, exception_type> storage;

			suspend_never initial_suspend() const noexcept { return {}; }
			eager_task_final_awaiter final_suspend() const noexcept { return { }; }


			eager_task<T> get_return_object() noexcept;
			eager_task<T> macoro_get_return_object() noexcept;

			void unhandled_exception() noexcept
			{
				storage = std::current_exception();
			}

			template<typename TT>
			void return_value(TT&& val) noexcept(std::is_nothrow_constructible<T, TT&&>::value)
			{
				static_assert(std::is_convertible<TT&&, T>::value, "the eager_task value_type can not be constructed from the co_return expression.");
				storage.template emplace<1>(std::forward<TT>(val));
			}

			reference_type get()&
			{
				if (storage.index() == 2)
					std::rethrow_exception(v_get<2>(storage));
				assert(storage.index() == 1);
				return v_get<1>(storage);
			}

			rvalue_type get()&&
			{
				if (storage.index() == 2)
					std::rethrow_exception(v_get<2>(storage));
				assert(storage.index() == 1);
				return std::move(v_get<1>(storage));
			}
		};


		template<typename T>
		struct eager_task_promise<T&>
		{
			eager_task_promise()
				: has_completed_or_continutation(false)
			{}


			using value_type = T&;
			using reference_type = T&;
			using pointer_type = T*;
			using exception_type = std::exception_ptr;
			using handle_type = coroutine_handle<eager_task_promise<value_type>>;

			std::atomic<bool> has_completed_or_continutation;
			coroutine_handle<> cont;
			variant<empty_state, pointer_type, exception_type> storage;


			suspend_never initial_suspend() const noexcept { return {}; }
			eager_task_final_awaiter final_suspend() const noexcept { return {}; }

			eager_task<value_type> get_return_object() noexcept;
			eager_task<value_type> macoro_get_return_object() noexcept;


			void unhandled_exception() noexcept
			{
				storage = std::current_exception();
			}

			void return_value(reference_type val) noexcept
			{
				storage.template emplace<1>(&val);
			}

			reference_type get()
			{
				if (storage.index() == 2)
					std::rethrow_exception(v_get<2>(storage));
				assert(storage.index() == 1);
				return *v_get<1>(storage);
			}
		};

		template<>
		struct eager_task_promise<void>
		{
			eager_task_promise()
				: has_completed_or_continutation(false)
			{}

			using value_type = void;
			using reference_type = void;
			using exception_type = std::exception_ptr;
			using handle_type = coroutine_handle<eager_task_promise<value_type>>;

			std::atomic<bool> has_completed_or_continutation;
			coroutine_handle<> cont;

			suspend_never initial_suspend() const noexcept { return {}; }
			eager_task_final_awaiter final_suspend() const noexcept { return {}; }

			eager_task<value_type> get_return_object() noexcept;
			eager_task<value_type> macoro_get_return_object() noexcept;

			exception_type storage;

			void unhandled_exception() noexcept
			{
				storage = std::current_exception();
			}

			void return_void() noexcept { }

			void get()
			{
				if (storage)
					std::rethrow_exception(storage);
			}
		};


	}

	template<typename T>
	class eager_task
	{
	public:
		using promise_type = impl::eager_task_promise<T>;
		using value_type = typename impl::eager_task_promise<T>::value_type;
		using reference_type = typename impl::eager_task_promise<T>::reference_type;
		using exception_type = typename impl::eager_task_promise<T>::exception_type;
		using handle_type = typename impl::eager_task_promise<T>::handle_type;

		handle_type handle;

		eager_task() { }
		eager_task(const eager_task&) = delete;
		eager_task(eager_task&& t) : handle(std::exchange(t.handle, std::nullptr_t{})) {
		}
		eager_task& operator=(const eager_task&) = delete;
		eager_task& operator=(eager_task&& t) {
			~eager_task();
			handle = std::exchange(t.handle, std::nullptr_t{});
		};
		~eager_task()
		{
			if (handle)
				handle.destroy();
		}

		//bool ready() const { return !handle || handle.done(); }
		//explicit operator bool() const { return ready(); }

		template<typename move>
		struct awaitable
		{
			awaitable() { }

			awaitable(handle_type h) : handle(h) { }
			awaitable(awaitable&& h) : handle(h.handle) { }

			~awaitable() { }

			handle_type handle;

			bool await_ready()
			{
				return !handle;
			}

			template<typename P>
			auto await_suspend(const std::coroutine_handle<P>& t) noexcept
			{
				return await_suspend_impl(t).std_cast();
			}

			template<typename P>
			auto await_suspend(const coroutine_handle<P>& t) noexcept
			{
				return await_suspend_impl(t);
			}

			coroutine_handle<void> await_suspend_impl(coroutine_handle<void> continuation) noexcept
			{
				auto& promise = handle.promise();
				assert(!promise.cont);
				promise.cont = continuation;

				// release our continuation. acquire their result (if they beat us).
				auto b = promise.has_completed_or_continutation.exchange(true, std::memory_order::memory_order_acq_rel);

				// if b, then  the result is ready and we should just resume the
				// awaiting coroutine. 
				if (b)
				{
					return continuation;
				}
				else
				{
					// otherwise we have finished first and our continuation will be resumed
					// when the coroutine finishes.
					return noop_coroutine();
				}
			}

			decltype(auto) await_resume() {
				if (!handle)
					throw broken_promise{ MACORO_LOCATION };

				// true_type if we should move the result out. 
				using move2 = typename std::conditional<std::is_same<decltype(handle.promise().get()), void>::value, std::false_type, move>::type;

				// conditional move.
				return impl::optMove(move2{}, handle.promise());
			}
		};

		auto operator_co_await() const& noexcept
		{
			return awaitable<std::false_type>{ handle };
		}

		auto operator_co_await() const&& noexcept
		{
			return awaitable<std::true_type>{ handle };
		}
#ifdef MACORO_CPP_20
		auto operator co_await() const& noexcept
		{
			return awaitable<std::false_type>{ handle };
		}

		auto operator co_await() const&& noexcept
		{
			return awaitable<std::true_type>{ handle };
		}
#endif
	private:
		friend promise_type;

		eager_task(handle_type h)
			:handle(h)
		{}
	};


	template<typename awaitable>
	auto make_eager_task(awaitable a)
		-> eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>
	{
		co_return co_await static_cast<awaitable&&>(a);
	}

	namespace impl
	{

		template<typename T>
		eager_task<T> eager_task_promise<T>::get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::std) }; }
		template<typename T>
		eager_task<T> eager_task_promise<T>::macoro_get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::mocoro) }; }

		template<typename T>
		eager_task<T&> eager_task_promise<T&>::get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::std) }; }
		template<typename T>
		eager_task<T&> eager_task_promise<T&>::macoro_get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::mocoro) }; }

		inline eager_task<void> eager_task_promise<void>::get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::std) }; }
		inline eager_task<void> eager_task_promise<void>::macoro_get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::mocoro) }; }



	}

}


