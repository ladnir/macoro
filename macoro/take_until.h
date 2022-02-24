#pragma once

#include "cancellation.h"
#include "task.h"
#include <chrono>
#include "type_traits.h"
#include "result.h"

namespace macoro
{
	template<typename AWAITABLE, typename UNTIL>
	task<awaitable_result_t<AWAITABLE>> take_until(AWAITABLE&& a, UNTIL&& t)
	{
		auto v = co_await a;
		t.request_cancellation();
		try { co_await t; }
		catch (...) {}

		co_return v;
	}


	//task<int> makeTask(cancellation_source s)
	//{
	//	co_await suspend_always{};
	//	s.request_cancellation();
	//	co_return 42;
	//}

	template<typename Scheduler>
	struct timeout
	{
		cancellation_source src;
		eager_task<> t;


		template<typename Rep, typename Per>
		timeout(Scheduler& s,
			std::chrono::duration<Rep, Per> d)
		{
			t = [](
				Scheduler& s, 
				cancellation_source src, 
				std::chrono::duration<Rep, Per> d) 
				-> eager_task<>
			{
				try
				{
					co_await s.schedule_after(d);
					src.request_cancellation();
				}
				catch (...) {}
			}(s, src, d);
		}

		void request_cancellation() {
			src.request_cancellation();
		} 

		cancellation_token token() { return src.token(); }


		auto operator co_await()
		{
			return t.operator co_await();
		}
	};


}
