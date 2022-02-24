//#pragma once
//
//
//#include "macoro/config.h"
//#include "macoro/cancellation.h"
//#include "macoro/coroutine_handle.h"
//#include <chrono>
//#include <queue>
//#include <mutex>
//namespace macoro
//{
//
//
//
//	class thread_delay_schdeuler
//	{
//		using clock = std::chrono::steady_clock;
//		using time_point = std::chrono::time_point<clock>;
//		struct Item
//		{
//			coroutine_handle<> h;
//			time_point deadline;
//			bool operator<(const Item& o) const { return deadline < o.deadline; }
//		};
//
//		std::condition_variable mCondition;
//		std::mutex mMutex;
//		std::priority_queue<Item> mQueue;
//		bool mJoin = false;
//		std::thread mThrd;
//
//		struct awaiter
//		{
//			thread_delay_schdeuler* sched;
//			time_point deadline;
//			cancellation_token token;
//
//			bool await_ready() { return return false; }
//
//			coroutine_handle<> await_suspend(coroutine_handle<> h)
//			{
//				if (deadline <= clock::now())
//					return h;
//				{
//					std::unique_lock<std::mutex> lock(sched->mMutex);
//					sched->mQueue.emplace(h, deadline);
//
//					
//				}
//				sched->mCondition.notify_one();
//			}
//
//		};
//
//		template<typename Rep, typename Per>
//		awaiter schedule_after(
//			std::chrono::duration<Rep, Per> d,
//			cancellation_token cancellationToken)
//		{
//			return { this, clock::now() + d, std::move(cancellationToken) };
//		}
//
//
//		thread_delay_schdeuler()
//		{
//			mThrd = std::thread([] { run(); });
//		}
//
//		void join()
//		{
//			if (mThrd.joinable())
//			{
//				mThrd.join();
//				mThrd = {};
//			}
//		}
//
//		~thread_delay_schdeuler()
//		{
//			if(mThrd.joinable())
//				mThrd.join();
//		}
//
//		struct work
//		{
//			thread_delay_schdeuler* mEx = nullptr;
//
//			work(thread_delay_schdeuler* e)
//				: mEx(e)
//			{
//				std::lock_guard<std::mutex> lock(mEx->mMutex);
//				++mEx->mWork;
//			}
//
//			work() = default;
//			work(const work&) = delete;
//			work(work&& w) : mEx(std::exchange(w.mEx, nullptr)) {}
//			work& operator=(const work&) = delete;
//			work& operator=(work&& w) {
//				reset();
//				mEx = std::exchange(w.mEx, nullptr);
//			}
//
//
//			void reset()
//			{
//
//				if (mEx)
//				{
//
//					std::size_t v = 0;
//					{
//						std::lock_guard<std::mutex> lock(mEx->mMutex);
//						v = --mEx->mWork;
//					}
//					if (v == 0)
//					{
//						mEx->mCondition.notify_all();
//					}
//					mEx = nullptr;
//				}
//			}
//			~work()
//			{
//				reset();
//			}
//		};
//
//
//		void run()
//		{
//			// we are done when theres no more work 
//			// and the queue is empty.
//			auto done = [this]() { return mWork == 0 && mQueue.empty(); };
//
//			bool isDone;
//			{
//				std::lock_guard<std::mutex> lock(this->mMutex);
//				isDone = done();
//			}
//
//			coroutine_handle<void> fn;
//
//			while (!isDone)
//			{
//				{
//					std::unique_lock<std::mutex> lock(this->mMutex);
//
//					if (mQueue.size())
//					{
//						while(mQueue.top().deadline < clock::now())
//							this->mCondition.wait_until(lock, mQueue.top().deadline);
//					}
//					else
//					{
//						this->mCondition.wait(lock, [&] {
//							// wake up when theres something in the
//							//  queue, or this->mWork == 0
//							return !this->mQueue.empty() || this->mWork == 0;
//							});
//					}
//
//					if (!this->mQueue.empty())
//					{
//						fn = std::move(this->mQueue.top());
//						assert(fn);
//						this->mQueue.pop();
//					}
//
//					isDone = done();
//				}
//
//				if (fn)
//					fn.resume();
//
//				fn = {};
//			}
//
//		}
//	};
//
//}