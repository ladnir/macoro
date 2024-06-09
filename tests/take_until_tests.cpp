#include "macoro/take_until.h"
#include "macoro/timeout.h"
#include "macoro/sync_wait.h"
#include "macoro/thread_pool.h"
#include "take_until_tests.h"

using namespace std::chrono;
using namespace std::chrono_literals;
namespace macoro
{
	namespace tests
	{
		namespace
		{
			time_point<steady_clock> begin, end, wake;
		}

		task<int> makeTask(thread_pool& ex, stop_source s, bool wait)
		{


			// wait for cancel.
			if (wait)
			{
				co_await (s.get_token());
				wake = steady_clock::now();

			}
			else
			{
				co_await(ex.schedule_after(milliseconds(15)));
				wake = steady_clock::now();
				s.request_stop();
			}

			co_return(43);
		}

		task<void> use(thread_pool& ex, bool wait)
		{
			auto to = timeout(ex, wait ? 10ms : 100000ms);

			begin = steady_clock::now();
			auto i = co_await take_until(makeTask(ex, to.get_source(), wait), std::move(to));
			end = steady_clock::now();
		}


		void timeout_test()
		{
			//io_service s;
			//auto thrd = std::thread([&] {s.process_events(); });
			auto maxLag = 40;
			thread_pool s;
			auto work = s.make_work();
			s.create_thread();

			sync_wait(use(s, true));

			auto t0 = duration_cast<milliseconds>(end - begin).count();
			auto w0 = duration_cast<milliseconds>(wake - begin).count();
			if (t0 < 10 || t0 > 10 + maxLag)
			{
				std::cout << t0 << "ms vs 10  ~ max " << 10 + maxLag << std::endl;
			//std::cout << w0 << "ms " << std::endl;
				throw MACORO_RTE_LOC;
			}

			sync_wait(use(s, false));

			auto t1 = duration_cast<milliseconds>(end - begin).count();
			auto w1 = duration_cast<milliseconds>(wake - begin).count();
			//std::cout << t1 << "ms vs 15 " << std::endl;
			//std::cout << w1 << "ms " << std::endl;
			if (t1 < 15 || t1 > 15 + maxLag)
			{
				std::cout << t1 << "ms vs 15  ~ max " << 15 + maxLag << std::endl;
				throw MACORO_RTE_LOC;
			}

			work = {};
			s.join();
		}



		void schedule_after()
		{
			auto start = steady_clock::now();
			auto maxLag = 30;
			thread_pool s;
			auto work = s.make_work();
			s.create_thread();

			for (int i = 0; i < 10; ++i)
			{

				std::promise<void> p;

				auto init = [](std::promise<void>& p, thread_pool& s) -> task<>
				{
					MC_BEGIN(task<>, &p, &s);
					begin = steady_clock::now();
					MC_AWAIT(s.schedule_after(milliseconds(15)));
					end = steady_clock::now();
					p.set_value();
					MC_END();
				}(p, s);

				sync_wait(init);

				auto t1 = duration_cast<milliseconds>(end - begin).count();
				if (t1 > 15 + maxLag)
					throw MACORO_RTE_LOC;
				//std::cout << t1 << "ms vs 5 " << std::endl;
			}
			//std::cout << s.print_log(start) << std::endl;
			work = {};
			s.join();
		}

		void schedule_after_cancaled()
		{
			
			macoro::thread_pool s;
			auto w = s.make_work();
			s.create_thread();
			stop_source src;
			auto token = src.get_token();
			src.request_stop();
			auto t = [](thread_pool& s, stop_token token) 
			{
				MC_BEGIN(task<>, &s, token);

				MC_AWAIT(s.schedule_after(std::chrono::milliseconds(1), token));

				MC_END();
			}(s, token);

			sync_wait(t);

		}



		void take_until_tests()
		{
			timeout_test();
		}

	}
}