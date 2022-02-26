#pragma once

#include "cancellation.h"
#include "task.h"
#include <chrono>
#include "type_traits.h"
#include "result.h"
#include "macoro/macros.h"

namespace macoro
{

	template<typename T>
	void request_cancellation(T&& t)
	{
		t.request_cancellation();
	}

	template<typename AWAITABLE, typename UNTIL,
		enable_if_t<std::is_void<awaitable_result_t<AWAITABLE>>::value == false, int> = 0>
	task<awaitable_result_t<AWAITABLE>> take_until(AWAITABLE a, UNTIL t)
	{
		using return_type = awaitable_result_t<AWAITABLE>;
		MC_BEGIN(task<return_type>,
			awaitable = std::move(a),
			until = std::move(t),
			v = result<return_type>{},
			w = result<void>{});

		MC_AWAIT_TRY(v, std::move(awaitable));
		
		request_cancellation(until);

		MC_AWAIT_TRY(w, std::move(until));

		if (v.has_error())
			std::rethrow_exception(v.error());
		MC_RETURN(v.value());
		MC_END();
	}

	template<typename AWAITABLE, typename UNTIL,
		enable_if_t<std::is_void<awaitable_result_t<AWAITABLE>>::value == true, int> = 0>
		task<void> take_until(AWAITABLE a, UNTIL t)
	{
		MC_BEGIN(task<>,
			awaitable = std::move(a),
			until = std::move(t),
			v = result<void>{},
			w = result<void>{});

		MC_AWAIT_TRY(v, awaitable);
		request_cancellation(until);
		MC_AWAIT_TRY(w, until);

		if (v.has_error())
			std::rethrow_exception(v.error());
		MC_RETURN_VOID();
		MC_END();
	}

}
