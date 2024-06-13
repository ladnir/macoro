
#include "channel_spsc_tests.h"
#include "macoro/channel_spsc.h"
#include "macoro/task.h"
#include "macoro/thread_pool.h"
#include "macoro/when_all.h"
#include "macoro/sync_wait.h"
#include "macoro/result.h"
#include "macoro/transfer_to.h"
#include "macoro/inline_scheduler.h"
#include "macoro/wrap.h"

namespace macoro
{
	namespace tests
	{
		namespace
		{
			std::size_t n = 100'000;
			struct message
			{
				size_t id;
				float data;
			};

			template<typename Scheduler>
			task<void> producer(
				spsc::channel_sender<message>& chl,
				Scheduler& sched)
			{
				auto i = std::size_t{};
				auto slot = spsc::channel_sender<message>::push_wrapper{};
				for (i = 0; i < n; ++i)
				{
					// Wait until a slot is free in the buffer.
					slot  = co_await chl.push();
					co_await transfer_to(sched);

					//std::cout << "pushing " << i << std::endl;
					slot = message{ i, 123 };

					slot.publish();
				}

				// Publish a sentinel
				co_await chl.close();
				co_await transfer_to(sched);
				//std::cout << "returning producer" << std::endl;
			}

			template<typename Scheduler>
			task<void> consumer(
				spsc::channel_receiver<message>& chl,
				Scheduler& sched)
			{
				auto i = std::size_t{ 0 };
				auto msg = macoro::result<message>{};
				auto msg2 = macoro::spsc::channel<message>::pop_wrapper{};
				
				while (true)
				{
					// Wait until the next message is available
					// There may be more than one available.
					try{
						msg =  Ok(co_await chl.front());
					}
					catch (...)
					{
						msg = Err(std::current_exception());
					}

					co_await(transfer_to(sched));
					if (msg.has_error())
					{
						if (i != n)
							throw MACORO_RTE_LOC;
						//std::cout << "returning consumer" << std::endl;
						co_return;
					}
					//std::cout << "popping " << i << std::endl;

					if (msg.value().id != i++)
						throw MACORO_RTE_LOC;
					if (msg.value().data != 123)
						throw MACORO_RTE_LOC;

					msg2 = co_await(chl.pop());
					co_await(transfer_to(sched));

					if (msg.value().id != msg2->id)
						throw MACORO_RTE_LOC;
					if (msg.value().data != msg2->data)
						throw MACORO_RTE_LOC;

					msg2.publish();

				}

			}

			template<typename Scheduler>
			task<void> example(Scheduler& sched)
			{

				auto s_r = spsc::make_channel<message>(8);

				MC_BEGIN(task<void>, &sched
					, sender = std::move(std::get<0>(s_r))
					, receiver = std::move(std::get<1>(s_r))
				);

				MC_AWAIT(when_all_ready(
					producer(sender, sched),
					consumer(receiver, sched))
				);

				MC_END();
			}
		}
		void spsc_channel_test()
		{
			inline_scheduler sched;
			sync_wait(example(sched));
		}


		void spsc_channel_ex_test()
		{
			thread_pool sched;
			auto w = sched.make_work();
			sched.create_thread();
			sync_wait(example(sched));

		}
	}
}