//#pragma once
//
//#include "task.h"
//#include "result.h"
//#include "macoro/macros.h"
//namespace macoro
//{
//	template<typename awaitable = void>
//	struct wrap_t;
//
//	template<>
//	struct wrap_t<void> { };
//
//
//	template<typename awaitable>
//	struct wrap_t
//	{
//		using traits = awaitable_traits<awaitable>;
//		using awaitable_type = awaitable;
//		using awaiter_stoage = remove_rvalue_reference_t<typename traits::awaiter>;
//		using awaitar_result = remove_rvalue_reference_t<typename traits::await_result>;
//		awaiter_stoage m_awaiter;
//
//
//		bool await_ready()
//		{
//			return m_awaiter.await_ready();
//		}
//
//		template<typename promise>
//		auto await_suspend(std::coroutine_handle<promise> h, std::source_location loc = std::source_location::current()) 
//		{
//			if constexpr (requires(awaiter_stoage m_awaiter) { { m_awaiter.await_suspend(h, loc) } -> std::convertible_to<int>; })
//			{
//				return m_awaiter.await_suspend(h, loc);
//			}
//			else
//			{
//				return m_awaiter.await_suspend(h);
//			}
//		}
//
//		template<typename promise>
//		auto await_suspend(coroutine_handle<promise> h, std::source_location loc = std::source_location::current())
//		{
//			if constexpr (requires(awaiter_stoage m_awaiter) { { m_awaiter.await_suspend(h, loc) } -> std::convertible_to<int>; })
//			{
//				return m_awaiter.await_suspend(h, loc);
//			}
//			else
//			{
//				return m_awaiter.await_suspend(h);
//			}
//		}
//
//		result<awaitar_result> await_resume() noexcept
//		{
//			try
//			{
//				if constexpr (std::is_same_v<awaitar_result, void>)
//				{
//					m_awaiter.await_resume();
//					return Ok();
//				}
//				else
//				{
//					return Ok(m_awaiter.await_resume());
//				}
//			}
//			catch (...)
//			{
//				return Err(std::current_exception());
//			}
//		}
//
//	};
//
//	inline auto wrap() { return wrap_t<>{}; }
//
//	template<typename awaitable>
//	auto wrap(awaitable&& a) {
//		return wrap_t<awaitable>{ get_awaiter(std::forward<awaitable>(a)) };
//	}
//
//	template<typename awaitable>
//	decltype(auto) operator|(awaitable&& a, const wrap_t<>&)
//	{
//		return wrap(std::forward<awaitable>(a));
//	}
//
//}
#pragma once

#include "task.h"
#include "result.h"
#include "macoro/macros.h"
namespace macoro
{
	struct wrap_t
	{
	};

	template<typename awaitable>
	struct wrapped_awaitable
	{
		using traits = awaitable_traits<awaitable>;
		using awaitable_storage = remove_rvalue_reference_t<awaitable>;
		using awaiter_storage = remove_rvalue_reference_t<typename traits::awaiter>;
		using awaitar_result = remove_rvalue_reference_t<typename traits::await_result>;

		struct awaiter
		{
			//template<typename A>
			//awaiter(A&& a)
			//	: m_awaiter(std::forward<A>(a))
			//{}

			awaiter_storage m_awaiter;

			bool await_ready()
			{
				return m_awaiter.await_ready();
			}

			template<typename promise>
			auto await_suspend(std::coroutine_handle<promise> h, std::source_location loc = std::source_location::current())
			{
				if constexpr (requires(awaiter_storage m_awaiter) { { m_awaiter.await_suspend(h, loc) } -> std::convertible_to<int>; })
				{
					return m_awaiter.await_suspend(h, loc);
				}
				else
				{
					return m_awaiter.await_suspend(h);
				}
			}

			template<typename promise>
			auto await_suspend(coroutine_handle<promise> h, std::source_location loc = std::source_location::current())
			{
				if constexpr (requires(awaiter_storage m_awaiter) { { m_awaiter.await_suspend(h, loc) } -> std::convertible_to<int>; })
				{
					return m_awaiter.await_suspend(h, loc);
				}
				else
				{
					return m_awaiter.await_suspend(h);
				}
			}

			result<awaitar_result> await_resume() noexcept
			{
				try
				{
					if constexpr (std::is_same_v<awaitar_result, void>)
					{
						m_awaiter.await_resume();
						return Ok();
					}
					else
					{
						return Ok(m_awaiter.await_resume());
					}
				}
				catch (...)
				{
					return Err(std::current_exception());
				}
			}
		};

		//wrapped_awaitable(awaitable&& a)
		//	: m_awaitable(std::forward<awaitable>(a))
		//{}

		//wrapped_awaitable() = default;
		//wrapped_awaitable(const wrapped_awaitable&) = default;
		//wrapped_awaitable(wrapped_awaitable&&) = default;
		//wrapped_awaitable& operator=(const wrapped_awaitable&) = default;
		//wrapped_awaitable& operator=(wrapped_awaitable&&) = default;


		awaitable_storage m_awaitable;

		awaiter operator co_await()&
		{
			return awaiter{ get_awaiter(m_awaitable) };
		}

		awaiter operator co_await()&&
		{
			return awaiter{ get_awaiter(std::forward<awaitable>(m_awaitable)) };
		}
	};

	inline auto wrap() { return wrap_t{}; }

	template<typename awaitable>
	wrapped_awaitable<awaitable> wrap(awaitable&& a) {
		return { std::forward<awaitable>(a) };
	}

	template<typename awaitable>
	decltype(auto) operator|(awaitable&& a, wrap_t)
	{
		return wrap(std::forward<awaitable>(a));
	}

}
