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
			MC_BEGIN(task<int>, &ex, s, wait);



			// wait for cancel.
			if (wait)
			{
				MC_AWAIT(s.get_token());
				wake = steady_clock::now();

			}
			else
			{
				MC_AWAIT(ex.schedule_after(milliseconds(15)));
				wake = steady_clock::now();
				s.request_stop();
			}

			MC_RETURN(43);
			MC_END();
		}

		task<void> use(thread_pool& ex, bool wait)
		{
			MC_BEGIN(task<>, &ex, wait
				, to = timeout(ex, wait ? 10ms : 100000ms)
				, i = int{}
			);

			begin = steady_clock::now();
			MC_AWAIT_SET(i, take_until(makeTask(ex, to.get_source(), wait), std::move(to)));
			end = steady_clock::now();

			MC_END();
		}


		void timeout_test()
		{
			//io_service s;
			//auto thrd = std::thread([&] {s.process_events(); });
			auto maxLag = 30;
			thread_pool s;
			auto work = s.make_work();
			s.create_thread();

			sync_wait(use(s, true));

			auto t0 = duration_cast<milliseconds>(end - begin).count();
			auto w0 = duration_cast<milliseconds>(wake - begin).count();
			//std::cout << t0 << "ms vs 10 " << std::endl;
			//std::cout << w0 << "ms " << std::endl;
			if (t0 < 10 || t0 > 10 + maxLag)
			{
				throw MACORO_RTE_LOC;
			}

			sync_wait(use(s, false));

			auto t1 = duration_cast<milliseconds>(end - begin).count();
			auto w1 = duration_cast<milliseconds>(wake - begin).count();
			//std::cout << t1 << "ms vs 15 " << std::endl;
			//std::cout << w1 << "ms " << std::endl;
			if (t1 < 15 || t1 > 15 + maxLag)
				throw MACORO_RTE_LOC;

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

		void take_until_tests()
		{
			timeout_test();
		}

	}
}