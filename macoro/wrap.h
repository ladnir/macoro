#pragma once

#include "task.h"
#include "result.h"
#include "macoro/macros.h"
namespace macoro
{

#ifdef MACORO_CPP_20_WRAP
	template<typename awaitable>
	enable_if_t<!std::is_void<awaitable_result_t<awaitable>>::value,
		task<
		result<
		remove_rvalue_reference_t<awaitable_result_t<awaitable>>
		>
		>
	>
		wrap(awaitable& t)
	{
		try
		{
			co_return Ok(co_await t);
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
	}

	template<typename awaitable>
	enable_if_t<!std::is_void<awaitable_result_t<awaitable>>::value,
		task<
		result<
		remove_rvalue_reference_t<awaitable_result_t<awaitable>>
		>
		>
	>
		wrap(awaitable t)
	{
		try
		{
			co_return Ok(co_await std::move(t));
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
	}


	template<typename awaitable>
	enable_if_t<std::is_void<awaitable_result_t<awaitable>>::value,
		task<
		result<
		remove_rvalue_reference_t<awaitable_result_t<awaitable>>
		>
		>
	>
		wrap(awaitable& t)
	{
		try
		{
			co_await t;
			co_return Ok();
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
	}

	template<typename awaitable>
	enable_if_t<std::is_void<awaitable_result_t<awaitable>>::value,
		task<
		result<
		remove_rvalue_reference_t<awaitable_result_t<awaitable>>
		>
		>
	>
		wrap(awaitable t)
	{
		try
		{
			co_await std::move(t);
			co_return Ok();
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
	}

#else

	template<typename awaitable>
	task<result<
		remove_rvalue_reference_t<awaitable_result_t<awaitable>>
		>>
		wrap(awaitable& t)
	{
		using T = remove_rvalue_reference_t<awaitable_result_t<awaitable>>;
		MC_BEGIN(task<result<T>>
			, &t
			, v = result<T>{});

		MC_AWAIT_TRY(v, t);
		MC_RETURN(std::move(v));
		MC_END();
	}


	template<typename awaitable>
	task<result<
		remove_rvalue_reference_t<awaitable_result_t<awaitable>>
		>>
		wrap(awaitable&& t)
	{
		using T = remove_rvalue_reference_t<awaitable_result_t<awaitable>>;
		MC_BEGIN(task<result<T>>
			, tt = std::move(t)
			, v = result<T>{});

		MC_AWAIT_TRY(v, std::move(tt));
		MC_RETURN(std::move(v));
		MC_END();
	}

#endif // MACORO_CPP_20


	struct wrap_t { };
	inline auto wrap() { return wrap_t{}; }

	template<typename awaitable>
	decltype(auto) operator|(awaitable&& a, const wrap_t&)
	{
		return wrap(std::forward<awaitable>(a));
	}

}
