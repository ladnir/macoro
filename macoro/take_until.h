#pragma once

#include "stop.h"
#include "task.h"
#include <chrono>
#include "type_traits.h"
#include "result.h"
#include "macoro/macros.h"
#include "macoro/wrap.h"

namespace macoro
{

	template<typename T>
	void request_stop(T&& t)
	{
		t.request_stop();
	}

	template<typename AWAITABLE, typename UNTIL>
		task<remove_rvalue_reference_t<awaitable_result_t<AWAITABLE>>> take_until(AWAITABLE awaitable, UNTIL until)
	{
		auto v = co_await( std::move(awaitable) | wrap());

		request_stop(until);

		co_await(std::move(until) | wrap());

		if (v.has_error())
			std::rethrow_exception(v.error());

		if constexpr(std::is_void<awaitable_result_t<AWAITABLE>>::value == false)
			co_return static_cast<awaitable_result_t<AWAITABLE>>(v.value());
	}
}
