
#include <chrono>
#include "macoro/task.h"
#include "macoro/thread_pool.h"
#include "macoro/when_all.h"
#include "macoro/sync_wait.h"
#include "macoro/sequence_mpsc.h"
#include "macoro/sequence_spsc.h"
#include "macoro/macros.h"
using namespace std::chrono;

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
				steady_clock::time_point timestamp;
				float data;
			};

			constexpr size_t bufferSize = 8; // Must be power-of-two
			constexpr size_t indexMask = bufferSize - 1;
			message buffer[bufferSize];

			task<void> producer(
				sequence_spsc<size_t>& sequencer)
			{
				MC_BEGIN(task<>, &sequencer
					, i = size_t{}
					, seq = size_t{}
					, msg = (message*)nullptr
				);
				for (i = 0; i < n; ++i)
				{
					// Wait until a slot is free in the buffer.
					MC_AWAIT_SET(seq, sequencer.claim_one());

					// Populate the message.
					msg = &buffer[seq & indexMask];
					msg->id = i;
					msg->timestamp = steady_clock::now();
					msg->data = 123;

					//std::cout << "push " << i << std::endl;
					// Publish the message.
					sequencer.publish(seq);

				}

				// Publish a sentinel
				MC_AWAIT_SET(seq, sequencer.claim_one());
				msg = &buffer[seq & indexMask];
				msg->id = -1;
				//std::cout << "push end " << i << std::endl;
				sequencer.publish(seq);

				MC_END();
			}

			task<void> consumer(
				const sequence_spsc<size_t>& sequencer,
				sequence_barrier<size_t>& consumerBarrier)
			{
				MC_BEGIN(task<>, &sequencer, &consumerBarrier
					, nextToRead = size_t{ 0 }
					, msg = (message*)nullptr
					, available = size_t{}
				);
				while (true)
				{
					// Wait until the next message is available
					// There may be more than one available.
					MC_AWAIT_SET(available, sequencer.wait_until_published(nextToRead));
					do {
						msg = &buffer[nextToRead & indexMask];
						if (msg->id == -1)
						{
							consumerBarrier.publish(nextToRead);
							//std::cout << "pop done " << nextToRead << std::endl;
							MC_RETURN_VOID();
						}
						//std::cout << "pop " << nextToRead << std::endl;

						if (msg->id != nextToRead)
							throw MACORO_RTE_LOC;
						if (msg->data != 123)
							throw MACORO_RTE_LOC;

					} while (nextToRead++ != available);

					// Notify the producer that we've finished processing
					// up to 'nextToRead - 1'.
					consumerBarrier.publish(available);
				}

				MC_END();
			}

			task<void> example()
			{
				struct State
				{
					sequence_barrier<size_t> barrier;
					sequence_spsc<size_t> sequencer;
					State()
						: sequencer(barrier, bufferSize)
					{}
				};

				MC_BEGIN(task<void>
					, state = new State{}
				);

				MC_AWAIT(when_all_ready(
					producer(state->sequencer),
					consumer(state->sequencer, state->barrier))
				);

				delete state;

				MC_END();
			}

		}

		void sequence_spsc_test()
		{
			sync_wait(example());
		}
	}

}
