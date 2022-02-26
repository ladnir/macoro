#include "macoro/take_until.h"
#include "macoro/timeout.h"
#include "macoro/io_service.h"
#include "macoro/sync_wait.h"
#include "macoro/thread_pool.h"

using namespace std::chrono_literals;
namespace macoro
{
	namespace tests
	{
		task<int> makeTask(cancellation_source s, bool wait)
		{
			MC_BEGIN(task<int>, s, wait);

			

			// wait for cancel.
			if (wait)
				MC_AWAIT(s.token());
			else
				s.request_cancellation();

			MC_RETURN(43);
			MC_END();
		}

		task<void> use(thread_pool&s, bool wait)
		{
			MC_BEGIN(task<>, &s , wait
				, to = timeout(s, wait ? 10ms : 100000ms)
				, i = int{}
				);

			MC_AWAIT_SET(i, take_until(makeTask(to.source(), wait), std::move(to)));

			MC_END();
		}
		

		void timeout_test()
		{
			//io_service s;
			//auto thrd = std::thread([&] {s.process_events(); });
			thread_pool s;
			auto work = s.make_work();
			s.create_thread();

			sync_wait(use(s, true));

			sync_wait(use(s, false));


			work = {};
			s.join();
		}



		void take_until_tests()
		{
			timeout_test();
		}

	}
}