///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <atomic>
#include <exception>
#include <utility>
#include <type_traits>
#include <cstdint>
#include <cassert>

#include <coroutine>
#include "macoro/coroutine_handle.h"
#include "macoro/awaiter.h"
#include "macoro/type_traits.h"
#include "macoro/macros.h"

namespace macoro
{
	template<typename T = void, bool lazy = true> class task;

	namespace impl
	{
		template<bool lazy>
		class task_promise_base;

		template<typename T, bool lazy>
		struct task_awaitable_base;

		template<>
		class task_promise_base<true>
		{
			friend struct final_awaitable;

			struct final_awaitable
			{
				bool await_ready() const noexcept { return false; }
#ifdef MACORO_CPP_20
				template<typename PROMISE>
				std::coroutine_handle<> await_suspend(
					std::coroutine_handle<PROMISE> coro) noexcept
				{
					auto& prom = coro.promise();
					assert(prom.m_continuation && "the lazy task type `task<T>` completed without a continuation.");
					return prom.m_continuation.std_cast();
				}
#endif

				template<typename PROMISE>
				coroutine_handle<> await_suspend(
					coroutine_handle<PROMISE> coro) noexcept
				{
					auto& prom = coro.promise();
					assert(prom.m_continuation && "the lazy task type `task<T>` completed without a continuation.");
					return prom.m_continuation;
				}
				void await_resume() noexcept {}
			};

		public:

			task_promise_base() noexcept
			{}

			auto initial_suspend() noexcept
			{
				return suspend_always{};
			}

			auto final_suspend() noexcept
			{
				return final_awaitable{};
			}

#ifdef MACORO_CPP_20
			void set_continuation(std::coroutine_handle<> continuation) noexcept
			{
				assert(!m_continuation);
				m_continuation = coroutine_handle<>(continuation);
			}
#endif
			void set_continuation(coroutine_handle<> continuation) noexcept
			{
				assert(!m_continuation);
				m_continuation = continuation;
			}

		private:
			coroutine_handle<> m_continuation;
		};

		template<>
		class task_promise_base<false>
		{
			friend struct final_awaitable;

			struct final_awaitable
			{
				bool await_ready() const noexcept { return false; }

#ifdef MACORO_CPP_20
				template<typename PROMISE>
				std::coroutine_handle<> await_suspend(
					std::coroutine_handle<PROMISE> coro) noexcept
				{
					return await_suspend(coroutine_handle<PROMISE>(coro)).std_cast();
				}
#endif

				template<typename PROMISE>
				coroutine_handle<> await_suspend(
					coroutine_handle<PROMISE> coro) noexcept
				{
					auto& promise = coro.promise();
					// release our result. acquire their continuation (if they beat us).
					auto b = promise.has_completed_or_continutation.exchange(true, std::memory_order_acq_rel);

					// if b, then we have finished after the continuation was set. 
					if (b)
					{
						assert(promise.m_continuation);
						return promise.m_continuation;
					}
					else
						return noop_coroutine();
				}
				void await_resume() noexcept {}
			};

		public:

			task_promise_base() noexcept
				: has_completed_or_continutation(false)
			{}

			auto initial_suspend() noexcept
			{
				return suspend_never{};
			}

			auto final_suspend() noexcept
			{
				return final_awaitable{};
			}
#ifdef MACORO_CPP_20
			bool try_set_continuation(std::coroutine_handle<> continuation) noexcept
			{
				return try_set_continuation(coroutine_handle<>(continuation));
			}
#endif
			bool try_set_continuation(coroutine_handle<> continuation) noexcept
			{
				assert(!m_continuation);
				m_continuation = continuation;
				return has_completed_or_continutation.exchange(true, std::memory_order_acq_rel);
			}

		private:
			std::atomic<bool> has_completed_or_continutation;

			coroutine_handle<> m_continuation;
		};

		template<typename T, bool lazy>
		class task_promise final : public task_promise_base<lazy>
		{
		public:

			task_promise() noexcept {}

			~task_promise()
			{
				switch (m_resultType)
				{
				case result_type::value:
					m_value.~T();
					break;
				case result_type::exception:
					m_exception.~exception_ptr();
					break;
				default:
					break;
				}
			}

			task<T, lazy> get_return_object() noexcept;
			task<T, lazy> macoro_get_return_object() noexcept;

			void unhandled_exception() noexcept
			{
				::new (static_cast<void*>(std::addressof(m_exception))) std::exception_ptr(
					std::current_exception());
				m_resultType = result_type::exception;
			}

			template<
				typename VALUE,
				typename = enable_if_t<std::is_convertible<VALUE&&, T>::value>>
				void return_value(VALUE&& value)
				noexcept(std::is_nothrow_constructible<T, VALUE&&>::value)
			{
				::new (static_cast<void*>(std::addressof(m_value))) T(std::forward<VALUE>(value));
				m_resultType = result_type::value;
			}

			T& result()&
			{
				if (m_resultType == result_type::exception)
				{
					std::rethrow_exception(m_exception);
				}

				assert(m_resultType == result_type::value);

				return m_value;
			}

			// HACK: Need to have co_await of task<int> return prvalue rather than
			// rvalue-reference to work around an issue with MSVC where returning
			// rvalue reference of a fundamental type from await_resume() will
			// cause the value to be copied to a temporary. This breaks the
			// sync_wait() implementation.
			// See https://github.com/lewissbaker/cppcoro/issues/40#issuecomment-326864107
			using rvalue_type = typename std::conditional<
				std::is_arithmetic<T>::value || std::is_pointer<T>::value,
				T,
				T&&>::type;

			rvalue_type result()&&
			{
				if (m_resultType == result_type::exception)
				{
					std::rethrow_exception(m_exception);
				}

				assert(m_resultType == result_type::value);

				return std::move(m_value);
			}

		private:

			enum class result_type { empty, value, exception };

			result_type m_resultType = result_type::empty;

			union
			{
				T m_value;
				std::exception_ptr m_exception;
			};

		};

		template<bool lazy>
		class task_promise<void, lazy> : public task_promise_base<lazy>
		{
		public:

			task_promise() noexcept = default;

			task<void, lazy> get_return_object() noexcept;
			task<void, lazy> macoro_get_return_object() noexcept;

			void return_void() noexcept
			{}

			void unhandled_exception() noexcept
			{
				m_exception = std::current_exception();
			}

			void result()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

		private:

			std::exception_ptr m_exception;

		};

		template<typename T, bool lazy>
		class task_promise<T&, lazy> : public task_promise_base<lazy>
		{
		public:

			task_promise() noexcept = default;

			task<T&, lazy> get_return_object() noexcept;
			task<T&, lazy> macoro_get_return_object() noexcept;

			void unhandled_exception() noexcept
			{
				m_exception = std::current_exception();
			}

			void return_value(T& value) noexcept
			{
				m_value = std::addressof(value);
			}

			T& result()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}

				return *m_value;
			}

		private:

			T* m_value = nullptr;
			std::exception_ptr m_exception;

		};



		template<typename T>
		struct task_awaitable_base<T,true>
		{
			using promise_type = task_promise<T, true>;
			coroutine_handle<promise_type> m_coroutine;
#ifdef MACORO_CPP_20
			task_awaitable_base(std::coroutine_handle<promise_type> coroutine) noexcept
				: m_coroutine(coroutine_handle<promise_type>(coroutine))
			{}
#endif
			task_awaitable_base(coroutine_handle<promise_type> coroutine) noexcept
				: m_coroutine(coroutine)
			{}

			bool await_ready() const noexcept
			{
				return !m_coroutine || m_coroutine.done();
			}

#ifdef MACORO_CPP_20
			std::coroutine_handle<> await_suspend(
				std::coroutine_handle<> awaitingCoroutine) noexcept
			{
				m_coroutine.promise().set_continuation(awaitingCoroutine);
				return m_coroutine.std_cast();
			}
#endif

			coroutine_handle<> await_suspend(
				coroutine_handle<> awaitingCoroutine) noexcept
			{
				m_coroutine.promise().set_continuation(awaitingCoroutine);
				return m_coroutine;
			}
		};

		template<typename T>
		struct task_awaitable_base<T, false>
		{
			using promise_type = task_promise<T, false>;
			coroutine_handle<promise_type> m_coroutine;
#ifdef MACORO_CPP_20
			task_awaitable_base(std::coroutine_handle<promise_type> coroutine) noexcept
				: m_coroutine(coroutine_handle<promise_type>(coroutine))
			{}
#endif

			task_awaitable_base(coroutine_handle<promise_type> coroutine) noexcept
				: m_coroutine(coroutine)
			{}

			bool await_ready() const noexcept
			{
				return !m_coroutine || m_coroutine.done();
			}

#ifdef MACORO_CPP_20
			std::coroutine_handle<> await_suspend(
				std::coroutine_handle<> awaitingCoroutine) noexcept
			{
				return await_suspend(
					coroutine_handle<>(awaitingCoroutine)).std_cast();
			}
#endif

			coroutine_handle<> await_suspend(
				coroutine_handle<> awaitingCoroutine) noexcept
			{
				// release our continuation. acquire their result (if they beat us).
				auto b = m_coroutine.promise().try_set_continuation(awaitingCoroutine);

				// if b, then  the result is ready and we should just resume the
				// awaiting coroutine. 
				if (b)
				{
					return awaitingCoroutine;
				}
				else
				{
					// otherwise we have finished first and our continuation will be resumed
					// when the coroutine finishes.
					return noop_coroutine();
				}
			}
		};
	}

	/// \brief
	/// A task represents an operation that produces a result both lazily
	/// and asynchronously.
	///
	/// When you call a coroutine that returns a task, the coroutine
	/// simply captures any passed parameters and returns exeuction to the
	/// caller. Execution of the coroutine body does not start until the
	/// coroutine is first co_await'ed.
	template<typename T, bool lazy>
	class [[nodiscard]] task
	{
	public:

		using promise_type = impl::task_promise<T, lazy>;

		using value_type = T;

	private:


	public:

		task() noexcept
			: m_coroutine(nullptr)
		{}
#ifdef MACORO_CPP_20
		explicit task(std::coroutine_handle<promise_type> coroutine)
			: m_coroutine(coroutine_handle<promise_type>(coroutine))
		{}
#endif
		explicit task(coroutine_handle<promise_type> coroutine)
			: m_coroutine(coroutine)
		{}

		task(task&& t) noexcept
			: m_coroutine(t.m_coroutine)
		{
			t.m_coroutine = nullptr;
		}

		/// Disable copy construction/assignment.
		task(const task&) = delete;
		task& operator=(const task&) = delete;

		/// Frees resources used by this task.
		~task()
		{
			if (m_coroutine)
			{
				m_coroutine.destroy();
			}
		}

		task& operator=(task&& other) noexcept
		{
			if (std::addressof(other) != this)
			{
				if (m_coroutine)
				{
					m_coroutine.destroy();
				}

				m_coroutine = other.m_coroutine;
				other.m_coroutine = nullptr;
			}

			return *this;
		}

		/// \brief
		/// Query if the task result is complete.
		///
		/// Awaiting a task that is ready is guaranteed not to block/suspend.
		bool is_ready() const noexcept
		{
			return !m_coroutine || m_coroutine.done();
		}

		struct ref_awaitable : impl::task_awaitable_base<T,lazy>
		{
			using impl::task_awaitable_base<T, lazy>::task_awaitable_base;

			decltype(auto) await_resume()
			{
				if (!this->m_coroutine)
				{
					throw broken_promise{};
				}

				return this->m_coroutine.promise().result();
			}
		};
		auto MACORO_OPERATOR_COAWAIT() const& noexcept
		{

			return ref_awaitable{ m_coroutine };
		}

		struct mov_awaitable : impl::task_awaitable_base<T, lazy>
		{
			using impl::task_awaitable_base<T, lazy>::task_awaitable_base;

			decltype(auto) await_resume()
			{
				if (!this->m_coroutine)
				{
					throw broken_promise{};
				}

				return std::move(this->m_coroutine.promise()).result();
			}
		};

		auto MACORO_OPERATOR_COAWAIT() const&& noexcept
		{
			return mov_awaitable{ m_coroutine };
		}

		struct ready_awaitable : impl::task_awaitable_base<T, lazy>
		{
			using impl::task_awaitable_base<T, lazy>::task_awaitable_base;

			void await_resume() const noexcept {}
		};
		/// \brief
		/// Returns an awaitable that will await completion of the task without
		/// attempting to retrieve the result.
		auto when_ready() const noexcept
		{
			return ready_awaitable{ m_coroutine };
		}

		coroutine_handle<promise_type> handle() const
		{
			return m_coroutine;
		}

	private:

		coroutine_handle<promise_type> m_coroutine;

	};


	template<typename T>
	using eager_task = task<T, false>;

	namespace impl
	{
		template<typename T, bool lazy>
		task<T, lazy> task_promise<T, lazy>::get_return_object() noexcept
		{
			return task<T, lazy>{ coroutine_handle<task_promise>::from_promise(*this, coroutine_handle_type::std) };
		}

		template<bool lazy>
		inline task<void, lazy> task_promise<void, lazy>::get_return_object() noexcept
		{
			return task<void, lazy>{ coroutine_handle<task_promise>::from_promise(*this, coroutine_handle_type::std) };
		}

		template<typename T, bool lazy>
		task<T&, lazy> task_promise<T&, lazy>::get_return_object() noexcept
		{
			return task<T&, lazy>{ coroutine_handle<task_promise>::from_promise(*this, coroutine_handle_type::std) };
		}


		template<typename T, bool lazy>
		task<T, lazy> task_promise<T, lazy>::macoro_get_return_object() noexcept
		{
			return task<T, lazy>{ coroutine_handle<task_promise>::from_promise(*this, coroutine_handle_type::macoro) };
		}

		template<bool lazy>
		inline task<void, lazy> task_promise<void, lazy>::macoro_get_return_object() noexcept
		{
			return task<void, lazy>{ coroutine_handle<task_promise>::from_promise(*this, coroutine_handle_type::macoro) };
		}

		template<typename T, bool lazy>
		task<T&, lazy> task_promise<T&, lazy>::macoro_get_return_object() noexcept
		{
			return task<T&, lazy>{ coroutine_handle<task_promise>::from_promise(*this, coroutine_handle_type::macoro) };
		}
	}

#ifdef MACORO_CPP_20
	template<typename AWAITABLE>
	auto make_task(AWAITABLE awaitable)
		-> task<remove_rvalue_reference_t<awaitable_result_t<AWAITABLE>>>
	{
		co_return co_await static_cast<AWAITABLE&&>(awaitable);
	}
	template<typename AWAITABLE>
	auto make_eager_task(AWAITABLE awaitable)
		-> eager_task<remove_rvalue_reference_t<awaitable_result_t<AWAITABLE>>>
	{
		co_return co_await static_cast<AWAITABLE&&>(awaitable);
	}
#else	
	template<typename AWAITABLE>
	auto make_task(AWAITABLE a)
		-> task<remove_rvalue_reference_t<awaitable_result_t<AWAITABLE>>>
	{
		MC_BEGIN(task<remove_rvalue_reference_t<awaitable_result_t<AWAITABLE>>>, awaitable = std::move(a));
		MC_RETURN_AWAIT(static_cast<AWAITABLE&&>(awaitable));
		MC_END();
	}
	template<typename AWAITABLE>
	auto make_eager_task(AWAITABLE a)
		-> eager_task<remove_rvalue_reference_t<awaitable_result_t<AWAITABLE>>>
	{
		MC_BEGIN(eager_task<remove_rvalue_reference_t<awaitable_result_t<AWAITABLE>>>, awaitable = std::move(a));
		MC_RETURN_AWAIT(static_cast<AWAITABLE&&>(awaitable));
		MC_END();
		//co_return co_await static_cast<AWAITABLE&&>(awaitable);
	}

#endif
}

