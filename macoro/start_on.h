#pragma once
#include "macoro/eager_task.h"
#include "macoro/type_traits.h"
#include "macoro/transfer_to.h"

namespace macoro
{
	template<typename scheduler, typename awaitable,
		enable_if_t<std::is_void<awaitable_result_t<awaitable>>::value == false, int> = 0>
		auto start_on(scheduler& s, awaitable& a)
		-> eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>
	{
		MC_BEGIN(eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>,
			&s, &a);

		MC_AWAIT(transfer_to(s));
		MC_RETURN_AWAIT(a);
		//co_return co_await a;

		MC_END();
	}
	template<typename scheduler, typename awaitable,
		enable_if_t<std::is_void<awaitable_result_t<awaitable>>::value == false, int> = 0>
		auto start_on(scheduler& s, awaitable aa)
		-> eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>
	{
		//co_await transfer_to(s);
		//co_return co_await static_cast<awaitable&&>(a);

		MC_BEGIN(eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>,
			&s, a = std::move(aa));

		MC_AWAIT(transfer_to(s));
		MC_RETURN_AWAIT(a);

		MC_END();
	}

	template<typename scheduler, typename awaitable,
		enable_if_t<std::is_void<awaitable_result_t<awaitable>>::value == true, int> = 0>
		auto start_on(scheduler& s, awaitable& a)
		-> eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>
	{
		MC_BEGIN(eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>,
			&s, &a);

		MC_AWAIT(transfer_to(s));
		MC_AWAIT(a);
		MC_END();
	}
	template<typename scheduler, typename awaitable,
		enable_if_t<std::is_void<awaitable_result_t<awaitable>>::value == true, int> = 0>
		auto start_on(scheduler& s, awaitable aa)
		-> eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>
	{

		MC_BEGIN(eager_task<remove_rvalue_reference_t<awaitable_result_t<awaitable>>>,
			&s, a = std::move(aa));

		MC_AWAIT(transfer_to(s));
		MC_AWAIT(a);

		MC_END();
	}

	template<typename Scheduler>
	struct start_on_t
	{
		template<typename S>
		start_on_t(S&& s)
			: scheduler(std::forward<S>(s))
		{}

		Scheduler scheduler;
	};

	template<typename scheduler>
	start_on_t<scheduler&> start_on(scheduler& s)
	{
		return start_on_t<scheduler&>{ s };
	}

	template<typename scheduler>
	start_on_t<scheduler> start_on(scheduler&& s)
	{
		return start_on_t<scheduler>{ s };
	}

	template<typename awaitable, typename scheduler>
	decltype(auto) operator|(awaitable&& a, start_on_t<scheduler> s)
	{
		return start_on(s.scheduler, std::forward<awaitable>(a));
	}

}


