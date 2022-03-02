
#include "macoro/sequence_spsc.h"
#include <vector>

namespace macoro
{


	struct channel_closed_exception : public std::exception
	{

		channel_closed_exception() noexcept
			: std::exception()
		{}

		const char* what() const noexcept override { return "channel was closed by sender"; }
	};

	namespace spsc
	{
		template<typename T>
		class channel
		{

			sequence_barrier<> mBarrier;
			sequence_spsc<> mSequence;

			std::size_t mIndexMask = 0, mFrontIndex = 0;
			std::vector<optional<T>> mData;

			class wrapper_base
			{
			protected:
				channel* mChl = nullptr;
				std::size_t mIndex;
				bool mPop;
			public:

				wrapper_base() = default;
				wrapper_base(const wrapper_base&) = delete;
				wrapper_base(wrapper_base&& o) : mChl(std::exchange(o.mChl, nullptr)), mIndex(o.mIndex), mPop(o.mPop) {};
				wrapper_base& operator=(wrapper_base&& o)
				{
					publish();
					mChl = std::exchange(o.mChl, nullptr);
					mIndex = o.mIndex;
					mPop = o.mPop;
					return *this;
				}

				wrapper_base(channel* c, std::size_t i, bool pop)
					: mChl(c), mIndex(i), mPop(pop)
				{}

				~wrapper_base()
				{
					publish();
				}

				void publish()
				{

					if (mChl)
					{
						if (mPop)
						{
							assert(mIndex == mChl->mFrontIndex);
							mChl->mData[mChl->mFrontIndex & mChl->mIndexMask].reset();
							mChl->mBarrier.publish(mChl->mFrontIndex);
							++mChl->mFrontIndex;
						}
						else
						{
							mChl->mSequence.publish(mIndex);
						}
						mChl = nullptr;
					}
				}

			};
		public:


			struct pop_wrapper : public wrapper_base
			{
				pop_wrapper() = default;
				pop_wrapper(const pop_wrapper&) = delete;
				pop_wrapper(pop_wrapper&& o) = default;
				pop_wrapper& operator=(pop_wrapper&& o) = default;

				pop_wrapper(channel* c, std::size_t i)
					: wrapper_base(c, i, true)
				{}



				operator T && ()
				{
					assert(this->mChl);
					auto& slot = this->mChl->mData[this->mChl->mIndexMask & this->mIndex];
					if (!slot)
						slot.emplace();
					return std::move(slot.value());
				}

				T&& operator*()
				{
					return operator T && ();
				}

				T* operator->()
				{
					auto&& t = this->operator T && ();
					return &t;
				}
			};

			struct push_wrapper : public wrapper_base
			{
				push_wrapper() = default;
				push_wrapper(const push_wrapper&) = delete;
				push_wrapper(push_wrapper&& o) = default;
				push_wrapper& operator=(push_wrapper&& o) = default;
				push_wrapper(channel* c, std::size_t i)
					: wrapper_base(c, i, false)
				{}

				template<typename U>
				T& operator=(U&& u)
				{
					auto& slot = this->mChl->mData[this->mChl->mIndexMask & this->mIndex];
					if (slot)
						*slot = std::forward<U>(u);
					else
						slot.emplace(std::forward<U>(u));
					return *slot;
				}

				operator T& ()
				{
					auto& slot = this->mChl->mData[this->mChl->mIndexMask & this->mIndex];
					if (!slot)
						slot.emplace();
					return slot.value();
				}
			};

			channel(std::size_t capacity)
				: mSequence(mBarrier, capacity)
				, mData(capacity)
			{
				auto powOf2 = (capacity > 0 && (capacity & (capacity - 1)) == 0);
				if (!powOf2)
					throw std::runtime_error("capacity must be a power of 2");
				mIndexMask = capacity - 1;
			}

			auto push(T&& t)
			{
				struct push_awaitable
				{
					using inner = decltype(mSequence.claim_one());
					inner mInner;
					channel* mChl;
					T mT;
					push_awaitable(inner&& in, channel* chl, T&& t)
						: mInner(std::move(in))
						, mChl(chl)
						, mT(std::forward<T>(t))
					{}

					bool await_ready() { return mInner.await_ready(); }

					auto await_suspend(coroutine_handle<> h) {
						return mInner.await_suspend(h);
					}
					auto await_resume() {
						auto idx = mInner.await_resume();
						mChl->mData[idx & mChl->mIndexMask].emplace(std::forward<T>(mT));
						mChl->mSequence.publish(idx);
					}
				};
				return push_awaitable(mSequence.claim_one(), this, std::forward<T>(t));
			}

			auto push()
			{
				struct push_awaitable
				{
					using inner = decltype(mSequence.claim_one());
					inner mInner;
					channel* mChl;

					push_awaitable(inner&& in, channel* chl)
						: mInner(std::move(in))
						, mChl(chl)
					{}

					bool await_ready() { return mInner.await_ready(); }

					auto await_suspend(coroutine_handle<> h) {
						return mInner.await_suspend(h);
					}
					auto await_resume() {
						auto idx = mInner.await_resume();
						return push_wrapper(mChl, idx);
					}
				};

				return push_awaitable(mSequence.claim_one(), this);
			}


			auto close()
			{
				struct close_awaitable
				{
					using inner = decltype(mSequence.claim_one());
					inner mInner;
					channel* mChl;

					close_awaitable(inner&& in, channel* chl)
						: mInner(std::move(in))
						, mChl(chl)
					{}

					bool await_ready() { return mInner.await_ready(); }

					auto await_suspend(coroutine_handle<> h) {
						return mInner.await_suspend(h);
					}
					auto await_resume() {
						auto idx = mInner.await_resume();
						mChl->mSequence.publish(idx);
					}
				};

				return close_awaitable(mSequence.claim_one(), this);
			}

			struct front_awaitable_base
			{
				using inner = decltype(mSequence.wait_until_published(mFrontIndex));
				inner mInner;
				channel* mChl;
				front_awaitable_base(inner&& in, channel* c)
					: mInner(std::move(in))
					, mChl(c)
				{}

				bool await_ready() { return mInner.await_ready(); }

				auto await_suspend(coroutine_handle<> h) {
					return mInner.await_suspend(h);
				}

			};
			auto front()
			{
				struct front_awaitable : public front_awaitable_base
				{
					using inner = typename front_awaitable_base::inner;
					front_awaitable(inner&& in, channel* c)
						:front_awaitable_base(std::forward<inner>(in), c)
					{}

					auto& await_resume() {
						auto available = this->mInner.await_resume();
						assert(available >= this->mChl->mFrontIndex);
						auto& slot = this->mChl->mData[this->mChl->mFrontIndex & this->mChl->mIndexMask];
						if (!slot)
							throw channel_closed_exception{};
						return slot.value();
					}
				};

				return front_awaitable(mSequence.wait_until_published(mFrontIndex), this);
			}

			auto pop()
			{
				struct pop_awaitable : public front_awaitable_base
				{
					using inner = typename front_awaitable_base::inner;
					pop_awaitable(inner&& in, channel* c)
						:front_awaitable_base(std::forward<inner>(in), c)
					{}


					pop_wrapper await_resume() {
						auto available = this->mInner.await_resume();
						assert(available >= this->mChl->mFrontIndex);
						return pop_wrapper(this->mChl, this->mChl->mFrontIndex);
					}
				};

				return pop_awaitable(mSequence.wait_until_published(mFrontIndex), this);
			}
		public:
		};

		template<typename T>
		class channel_sender
		{
			std::shared_ptr<channel<T>> mBase;
		public:
			using push_wrapper = typename channel<T>::push_wrapper;

			channel_sender(std::shared_ptr<channel<T>> b)
				:mBase(std::move(b))
			{}
			channel_sender() = default;
			channel_sender(const channel_sender&) = delete;
			channel_sender(channel_sender&&) = default;
			channel_sender& operator=(const channel_sender&) = delete;
			channel_sender& operator=(channel_sender&&) = default;

			auto push()
			{
				return mBase->push();
			}

			auto close()
			{
				return mBase->close();
			}

		};

		template<typename T>
		class channel_receiver
		{
			std::shared_ptr<channel<T>> mBase;
		public:
			channel_receiver(std::shared_ptr<channel<T>> b)
				:mBase(std::move(b))
			{}
			channel_receiver() = default;
			channel_receiver(const channel_receiver&) = delete;
			channel_receiver(channel_receiver&&) = default;
			channel_receiver& operator=(const channel_receiver&) = delete;
			channel_receiver& operator=(channel_receiver&&) = default;


			auto front()
			{
				return mBase->front();
			}

			auto pop()
			{
				return mBase->pop();
			}
		};


		template<typename T>
		auto make_channel(std::size_t capcaity)
		{
			std::shared_ptr<channel<T>> ptr = std::make_shared<channel<T>>(capcaity);
			return std::make_pair<channel_sender<T>, channel_receiver<T>>(ptr, ptr);
		}
	}

}