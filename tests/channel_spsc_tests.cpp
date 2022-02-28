#include "channel_spsc_tests.h"
#include "macoro/channel_spsc.h"
#include "macoro/task.h"
#include "macoro/thread_pool.h"
#include "macoro/when_all.h"
#include "macoro/sync_wait.h"
#include "macoro/result.h"

namespace macoro
{
	namespace tests
	{
		namespace
		{

			struct message
			{
				int id;
				float data;
			};

			task<void> producer(
				thread_pool& ioSvc,
				spsc::channel_sender<message>& chl)
			{
				;
				MC_BEGIN(task<>, &ioSvc, &chl
					, i = int{}
					, slot = std::move(spsc::channel_sender<message>::wrapper{})
				);
				for (i = 0; i < 16; ++i)
				{
					// Wait until a slot is free in the buffer.
					MC_AWAIT_SET(slot, chl.push(ioSvc));

					std::cout << "pushing " << i << std::endl;
					slot = message{ i, 123 };
				}

				// Publish a sentinel
				MC_AWAIT(chl.close(ioSvc));
				std::cout << "returning producer" << std::endl;

				MC_END();
			}

			task<void> consumer(
				thread_pool& threadPool,
				spsc::channel_receiver<message>& chl)
			{
				MC_BEGIN(task<>, &threadPool, &chl
					, i = int{ 0 }
					, msg = macoro::result<message>{}
				);
				while (true)
				{
					// Wait until the next message is available
					// There may be more than one available.
					MC_AWAIT_TRY(msg, chl.front(threadPool));
					if (msg.has_error())
					{
						if (i != 16)
							throw MACORO_RTE_LOC;
						std::cout << "returning consumer" << std::endl;
						MC_RETURN_VOID();
					}
					std::cout << "popping " << i << std::endl;

					if (msg.value().id != i++)
						throw MACORO_RTE_LOC;
					if (msg.value().data != 123)
						throw MACORO_RTE_LOC;

					MC_AWAIT(chl.pop(threadPool));
				}
				
				MC_END();
			}

			task<void> example(thread_pool& tp)
			{
		
				auto s_r = spsc::make_channel<message>(8);

				MC_BEGIN(task<void>, &tp
					, sender = std::move(std::get<0>(s_r))
					, receiver = std::move(std::get<1>(s_r))
				);

				MC_AWAIT(when_all_ready(
					producer(tp, sender),
					consumer(tp, receiver))
				);

				MC_END();
			}
		}
		void spsc_channel_test()
		{
			thread_pool::work w;
			thread_pool tp(1, w);
			sync_wait(when_all_ready(example(tp)));

			w = {};

		}
	}
}