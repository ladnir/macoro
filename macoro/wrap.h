#pragma once

#include "task.h"
#include "result.h"
#include "macoro/macros.h"
namespace macoro
{

#pragma push_macro("MC_UNHANDLED_EXCEPTION")
#undef MC_UNHANDLED_EXCEPTION
#define MC_UNHANDLED_EXCEPTION MC_RETURN(Err(std::current_exception())); 

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
#ifdef MACORO_CPP_20
		try
		{
			co_return Ok(co_await t);
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
#else
		using T = remove_rvalue_reference_t<awaitable_result_t<awaitable>>;
		MC_BEGIN(task<result<T>>
			, &t
			, v = optional<T>{});

		MC_AWAIT_SET(v, t);

		MC_RETURN(Ok(std::move(v.value())));

		MC_END();
#endif
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
#ifdef MACORO_CPP_20
		try
		{
			co_return Ok(co_await t);
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
#else

		using T = remove_rvalue_reference_t<awaitable_result_t<awaitable>>;
		MC_BEGIN(task<result<T>>
			, tt = std::move(t)
			, v = optional<T>{});

		MC_AWAIT_SET(v, tt);

		MC_RETURN(Ok(std::move(v.value())));

		MC_END();
#endif // MACORO_CPP_20
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
#ifdef MACORO_CPP_20
		try
		{
			co_await t;
			co_return Ok();
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
#else
		MC_BEGIN(task<result<void>>, &t);

		MC_AWAIT(t);
		MC_RETURN(Ok());
		MC_END();

#endif
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
#ifdef MACORO_CPP_20
		try
		{
			co_await t;
			co_return Ok();
		}
		catch (...)
		{
			co_return Err(std::current_exception());
		}
#else
		MC_BEGIN(task<result<void>>
			, tt = std::move(t));

		MC_AWAIT(tt);
		MC_RETURN(Ok());
		MC_END();

#endif
	}
	
#pragma pop_macro("MC_UNHANDLED_EXCEPTION")

	struct wrap_t { };
	inline auto wrap() { return wrap_t{}; }

	template<typename awaitable>
	decltype(auto) operator|(awaitable&& a, const wrap_t&)
	{
		return wrap(std::forward<awaitable>(a));
	}

}
