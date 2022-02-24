#include "macoro/take_until.h"
#include "macoro/io_service.h"
#include "macoro/sync_wait.h"
using namespace std::chrono_literals;
namespace macoro
{
	namespace tests
	{
		task<int> makeTask(cancellation_token t)
		{
			// wait for cancel.
			co_await t;

			co_return 43;
		}

		task<void> use(io_service &s)
		{
			;
			//std::chrono::duration<std::chrono::milliseconds> d;
			
			timeout<io_service> to(s, 10ms);

			int i = co_await take_until(makeTask(to.token()), to);

			co_return;
		}
		

		void timeout_test()
		{
			io_service s;
			auto thrd = std::thread([&] {s.process_events(); });

			sync_wait(use(s));
			std::cout << "success" << std::endl;

			s.stop();
			thrd.join();
		}



		void take_until_tests()
		{
			timeout_test();
		}

	}
}