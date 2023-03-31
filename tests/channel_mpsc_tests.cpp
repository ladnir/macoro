#include "channel_mpsc_tests.h"
#include "macoro/channel.h"
#include "macoro/task.h"
#include "macoro/thread_pool.h"
#include "macoro/when_all.h"
#include "macoro/sync_wait.h"
#include "macoro/result.h"
#include "macoro/transfer_to.h"
#include "macoro/inline_scheduler.h"
#include "macoro/start_on.h"

namespace macoro
{
    namespace tests
    {
        namespace
        {
            std::size_t n = 1000;
            struct message
            {
                int tIdx;
                int id;
                float data;
            };

            template<typename Scheduler>
            task<void> producer2(
                mpsc::channel_sender<message>& chl,
                Scheduler& sched,
                int tIdx)
            {
                ;
                MC_BEGIN(task<>, &chl, &sched, tIdx
                    , i = int{}
                    , slot = std::move(mpsc::channel_sender<message>::push_wrapper{})
                );
                for (i = 0; i < n; ++i)
                {
                    // Wait until a slot is free in the buffer.
                    MC_AWAIT_SET(slot, chl.push());
                    MC_AWAIT(transfer_to(sched));

                    //std::cout << "pushing "<< tIdx << " " << i << std::endl;
                    slot = message{ tIdx, i, 123 };

                    slot.publish();
                }

                MC_END();
            }

            template<typename Scheduler>
            task<void> producer(
                int numThreads,
                mpsc::channel_sender<message>& chl,
                Scheduler& sched)
            {
                ;
                MC_BEGIN(task<>, &chl, &sched, numThreads
                    , i = int{}
                    , tasks = std::vector<eager_task<>>{}
                );
                tasks.resize(numThreads);
                for (i = 0; i < numThreads; ++i)
                {
                    tasks[i] = producer2(chl, sched, i)
                        | start_on(sched)
                        | make_eager();
                }
                for (i = 0; i < numThreads; ++i)
                {
                    MC_AWAIT(tasks[i]);
                }

                // Publish a sentinel
                MC_AWAIT(chl.close());
                MC_AWAIT(transfer_to(sched));
                //std::cout << "returning producer" << std::endl;


                MC_END();
            }

            template<typename Scheduler>
            task<void> consumer(
                int numProducers,
                mpsc::channel_receiver<message>& chl,
                Scheduler& sched)
            {
                MC_BEGIN(task<>, &chl, &sched, numProducers
                    , i = std::vector<int>( numProducers )
                    , msg = macoro::result<message>{}
                    , msgPtr = (message*)nullptr
                    , msg2 = std::move(macoro::mpsc::channel<message>::pop_wrapper{})
                );
                while (true)
                {
                    // Wait until the next message is available
                    // There may be more than one available.
                    MC_AWAIT_TRY(msg, chl.front());
                    MC_AWAIT(transfer_to(sched));
                    if (msg.has_error())
                    {
                        for (int j = 0; j < numProducers; ++j)
                        {

                            if (i[j] != n)
                            {
                                std::cout << "failed at " << j << " " << i[j] << std::endl;
                                throw MACORO_RTE_LOC;
                            }
                        }
                        //std::cout << "returning consumer" << std::endl;
                        MC_RETURN_VOID();
                    }
                    //std::cout << "popping " << i << std::endl;

                    msgPtr = &msg.value();
                    if (msg.value().tIdx > numProducers)
                        throw MACORO_RTE_LOC;
                    if (msg.value().id != i[msg.value().tIdx])
                        throw MACORO_RTE_LOC;
                    if (msg.value().data != 123)
                        throw MACORO_RTE_LOC;

                    MC_AWAIT_SET(msg2, chl.pop());
                    MC_AWAIT(transfer_to(sched));

                    if (msg.value().id != msg2->id)
                        throw MACORO_RTE_LOC;
                    if (msg.value().data != msg2->data)
                        throw MACORO_RTE_LOC;

                    msg2.publish();

                    //std::cout << "popped " << i << std::endl;
                    ++i[msg.value().tIdx];

                }

                MC_END();
            }

            template<typename Scheduler>
            task<void> example(Scheduler& sched, int numThreads)
            {

                auto s_r = mpsc::make_channel<message>(8);

                MC_BEGIN(task<void>, &sched, numThreads
                    , sender = std::move(std::get<0>(s_r))
                    , receiver = std::move(std::get<1>(s_r))
                );

                MC_AWAIT(when_all_ready(
                    producer(numThreads, sender, sched),
                    consumer(numThreads, receiver, sched))
                );

                MC_END();
            }
        }
        void mpsc_channel_test()
        {
            int numThreads = 10;
            inline_scheduler sched;
            sync_wait(example(sched, numThreads));
        }


        void mpsc_channel_ex_test()
        {
            int numThreads = 10;
            thread_pool sched;
            auto w = sched.make_work();
            sched.create_threads(numThreads);
            sync_wait(example(sched, numThreads));

        }
    }
}