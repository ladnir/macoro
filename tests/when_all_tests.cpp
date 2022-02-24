#include "when_all_tests.h"
#include "macoro/when_all.h"
#include "macoro/task.h"
#include "macoro/sync_wait.h"

namespace macoro
{
	namespace tests
	{
		void when_all_basic_tests()
		{

			auto f = []() -> task<int> {
				MC_BEGIN(task<int>);
				MC_RETURN(42);
				MC_END();
			};


			auto g = []() -> task<bool> {
				MC_BEGIN(task<bool>);
				MC_RETURN(true);
				MC_END();
			};

			bool b;
			auto h = [&]() -> task<bool&> {
				MC_BEGIN(task<bool&>, &b);
				MC_RETURN(b);
				MC_END();
			};

			static_assert(
				is_awaitable<
				task<int>
				>::value
				, "");

			static_assert(std::is_same_v<
				remove_reference_and_wrapper_t<task<int>>,
				task<int>
			>, "");


			static_assert(
				is_awaitable<
				remove_reference_and_wrapper_t<task<int>>
				>::value
				, "");

			static_assert(
				std::conjunction<
				is_awaitable<
				remove_reference_and_wrapper_t<task<int>>
				>,
				is_awaitable<
				remove_reference_and_wrapper_t<task<bool>>
				>
				>::value
				, "");

			std::tuple <
				detail::when_all_task<int>,
				detail::when_all_task<bool>
			>
				r = sync_wait(when_all_ready(f(), g()));

			auto ff = f();
			auto gg = g();

			std::tuple <
				detail::when_all_task<int>,
				detail::when_all_task<bool&>
			>
				r2 = sync_wait(when_all_ready(std::move(ff), h()));
			//auto r = sync_wait(w);
			detail::when_all_task<int> r0 = std::move(std::get<0>(r));
			assert(std::get<0>(r).result() == 42);
			assert(std::get<1>(r).result() == true);
		}

	}
}