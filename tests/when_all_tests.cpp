#include "when_all_tests.h"
#include "macoro/when_all.h"
#include "macoro/task.h"
#include "macoro/sync_wait.h"
#include "macoro/manual_reset_event.h"
#include "macoro/async_scope.h"
#include "macoro/when_all_scope.h"

namespace macoro
{
	namespace tests
	{

		auto f = []() -> task<int> {
			//std::cout << "trace\n" << (co_await get_trace{}).str() << std::endl;
			co_return(42);
			};


		auto g = []() -> task<bool> {
			MC_BEGIN(task<bool>);
			MC_RETURN(true);
			MC_END();
			};

		bool b = true;
		auto h = []() -> task<bool&> {
			MC_BEGIN(task<bool&>);
			MC_RETURN(b);
			MC_END();
			};

		void when_all_basic_tests()
		{
			static_assert(
				is_awaitable<
				task<int>
				>::value
				, "");

			static_assert(std::is_same<
				remove_reference_and_wrapper_t<task<int>>,
				task<int>
			>::value, "");


			static_assert(
				is_awaitable<
				remove_reference_and_wrapper_t<task<int>>
				>::value
				, "");

			static_assert(
				conjunction<
				is_awaitable<
				remove_reference_and_wrapper_t<task<int>>
				>,
				is_awaitable<
				remove_reference_and_wrapper_t<task<bool>>
				>
				>::value
				, "");

			auto ff = f();
			auto gg = g();

			auto t = [&]()->task<
				std::tuple <
				detail::when_all_task<int>,
				detail::when_all_task<bool&>
				>>
			{
				co_return co_await when_all_ready(std::move(ff), h());
			}();

			auto r2 = sync_wait(std::move(t));

			detail::when_all_task<int> r0 = std::move(std::get<0>(r2));
			detail::when_all_task<bool&> r1 = std::move(std::get<1>(r2));

			if (r0.result() != 42)
				throw std::runtime_error(MACORO_LOCATION);
			if (&r1.result() != &b)
				throw std::runtime_error(MACORO_LOCATION);

		}


		void when_all_scope_test()
		{

			auto tr = []()->when_all_scope {

				scoped_task<int> r = co_await f();

				int i = co_await std::move(r);
				co_return;

				}
			();

			sync_wait(std::move(tr));
			using namespace macoro;
			using Awaitable = task<int>;
			Awaitable a = f();
			auto t = detail::make_when_all_task<Awaitable>(std::move(a));

		}
	}
}

