#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <unordered_set>

#include "macoro/coroutine_handle.h"
#include "macoro/awaiter.h"

namespace macoro
{
	namespace impl
	{

		struct thread_pool_state
		{
			std::mutex              mMutex;
			std::condition_variable mCondition;

			std::size_t mWork = 0;
			std::unordered_set<std::thread::id> mThreadIds;
			std::deque<coroutine_handle<void>> mDeque;

			void post(coroutine_handle<void> fn)
			{
				assert(fn);
				{
					std::lock_guard<std::mutex> lock(mMutex);
					mDeque.push_back(std::move(fn));
				}
				mCondition.notify_one();
			}

			MACORO_NODISCARD
			bool try_dispatch(coroutine_handle<void> fn)
			{
				assert(fn);
				auto id = std::this_thread::get_id();

				{
					std::lock_guard<std::mutex> lock(mMutex);
					auto r = mThreadIds.find(id);
					if (r == mThreadIds.end())
					{
						id = {};
						mDeque.push_back(std::move(fn));
					}
				}

				if (id != std::thread::id{})
					return true;
				else
					mCondition.notify_one();
				return false;
			}


			optional<std::thread::id> get_thread_id()
			{
				auto id = std::this_thread::get_id();

				{
					std::lock_guard<std::mutex> lock(mMutex);
					auto r = mThreadIds.find(id);
					if (r == mThreadIds.end())
						return {};
				}

				return id;
			}
		};
	}


	struct thread_pool_post
	{
		impl::thread_pool_state* pool;

		bool await_ready() const noexcept { return false; }

		template<typename H>
		void await_suspend(H h) const { pool->post(coroutine_handle<void>(h)); }

		void await_resume() const noexcept {}
	};


	struct thread_pool_dispatch
	{
		impl::thread_pool_state* pool;

		bool await_ready() const noexcept { return false; }

		template<typename H>
		auto await_suspend(const H& h)const {
			using traits_C = coroutine_handle_traits<H>;
			using return_type = typename traits_C::template coroutine_handle<void>;
			if (pool->try_dispatch(coroutine_handle<void>(h)))
				return return_type(h);
			else
				return static_cast<return_type>(noop_coroutine());
		}

		void await_resume() const noexcept {}
	};


	class thread_pool
	{
	public:


		thread_pool()
			:mState(new impl::thread_pool_state)
		{}
		thread_pool(thread_pool&&) = default;
		thread_pool& operator=(thread_pool&&) = default;

		std::unique_ptr<impl::thread_pool_state> mState;

		struct work
		{
			impl::thread_pool_state* mEx = nullptr;

			work(impl::thread_pool_state* e)
				: mEx(e)
			{
				std::lock_guard<std::mutex> lock(mEx->mMutex);
				++mEx->mWork;
			}

			work() = default;
			work(const work&) = delete;
			work(work&&) = default;
			work& operator=(const work&) = delete;
			work& operator=(work&&) = default;

			~work()
			{
				if (mEx)
				{

					std::size_t v = 0;
					{
						std::lock_guard<std::mutex> lock(mEx->mMutex);
						v = --mEx->mWork;
					}
					if (v == 0)
					{
						mEx->mCondition.notify_all();
					}
				}
			}
		};

		thread_pool_post schedule()
		{
			return { mState.get() };
		}


		thread_pool_post post()
		{
			return { mState.get() };
		}


		thread_pool_dispatch dispatch()
		{
			return { mState.get() };
		}

		void post(coroutine_handle<void> fn)
		{
			mState->post(fn);
		};

		void dispatch(coroutine_handle<void> fn)
		{
			if(mState->try_dispatch(fn))
				fn.resume();
		};

		work make_work() {
			return { mState.get() };
		}

		optional<std::thread::id> get_thread_id()
		{
			return mState->get_thread_id();
		};

		void run()
		{
			auto state = mState.get();
			// we are done when theres no more work 
			// and the queue is empty.
			auto done = [state]() {return state->mWork == 0 && state->mDeque.empty(); };

			bool isDone;
			auto id = std::this_thread::get_id();
			{
				std::lock_guard<std::mutex> lock(state->mMutex);
				auto r = state->mThreadIds.insert(id);
				assert(r.second);

				isDone = done();
			}

			coroutine_handle<void> fn;

			while (!isDone)
			{
				{
					std::unique_lock<std::mutex> lock(state->mMutex);

					state->mCondition.wait(lock, [&] {
						// wake up when theres something in the
						//  queue, or state->state->mWork == 0
						return !state->mDeque.empty() || state->mWork == 0;
						});

					if (!state->mDeque.empty())
					{
						fn = std::move(state->mDeque.front());
						assert(fn);
						state->mDeque.pop_front();
					}

					isDone = done();
				}

				if (fn)
					fn.resume();

				fn = {};
			}

			{
				std::lock_guard<std::mutex> lock(state->mMutex);
				auto r = state->mThreadIds.find(id);
				assert(r != state->mThreadIds.end());
				state->mThreadIds.erase(r);
			}
		}


	};

}