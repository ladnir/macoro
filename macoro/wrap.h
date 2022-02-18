#pragma once

#include "task.h"
#include "result.h"

namespace macoro
{
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
			co_return Ok(co_await t);
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
			co_await t;
			co_return Ok();
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
	}
	

	struct wrap_t { };
	inline auto wrap() { return wrap_t{}; }

	template<typename awaitable>
	decltype(auto) operator|(awaitable&& a, const wrap_t&)
	{
		return wrap(std::forward<awaitable>(a));
	}

}
