
#include "macoro/sequence_spsc.h"
#include <vector>

namespace macoro
{
	namespace detail
	{
		const int tab64[64] = {
			63,  0, 58,  1, 59, 47, 53,  2,
			60, 39, 48, 27, 54, 33, 42,  3,
			61, 51, 37, 40, 49, 18, 28, 20,
			55, 30, 34, 11, 43, 14, 22,  4,
			62, 57, 46, 52, 38, 26, 32, 41,
			50, 36, 17, 19, 29, 10, 13, 21,
			56, 45, 25, 31, 35, 16,  9, 12,
			44, 24, 15,  8, 23,  7,  6,  5 };


		std::uint64_t log2floor(std::uint64_t value)
		{
			value |= value >> 1;
			value |= value >> 2;
			value |= value >> 4;
			value |= value >> 8;
			value |= value >> 16;
			value |= value >> 32;
			return tab64[((std::uint64_t)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
		}

		std::uint64_t log2ceil(std::uint64_t value)
		{
			auto floor = log2floor(value);
			return floor + (value > (1ull << floor));
		}
	}

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

		public:
			class wrapper
			{
				channel* mChl = nullptr;
				std::size_t mIndex;
			public:

				wrapper() = default;
				wrapper(const wrapper&) = delete;
				wrapper(wrapper&& o) : mChl(std::exchange(o.mChl, nullptr)), mIndex(o.mIndex) {};
				wrapper& operator=(wrapper&& o) 
				{ 
					if (mChl)
						mChl->mSequence.publish(mIndex);

					mChl = std::exchange(o.mChl, nullptr);
					mIndex = o.mIndex;
					return *this;
				}

				~wrapper()
				{
					if (mChl)
						mChl->mSequence.publish(mIndex);
				}

				wrapper(channel* c, std::size_t i)
					: mChl(c), mIndex(i)
				{}


				template<typename U>
				T& operator=(U&& u)
				{
					auto& slot = mChl->mData[mChl->mIndexMask & mIndex];
					if (slot)
						*slot = std::forward<U>(u);
					else
						slot.emplace(std::forward<U>(u));
					return *slot;
				}

				operator T&()
				{
					auto& slot = mChl->mData[mChl->mIndexMask & mIndex];
					if (!slot)
						slot.emplace();
					return slot.value();
				}
			};


			channel(std::size_t capacity)
				: mSequence(mBarrier, capacity)
				, mData(capacity)
			{
				if (capacity != 1ull << detail::log2ceil(capacity))
					throw std::runtime_error("capacity must be a power of 2");
				mIndexMask = capacity - 1;
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
						return wrapper(mChl, idx);
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
						auto available = mInner.await_resume();
						assert(available >= mChl->mFrontIndex);
						auto& slot = mChl->mData[mChl->mFrontIndex & mChl->mIndexMask];
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


					auto await_resume() {
						auto available = mInner.await_resume();
						assert(available >= mChl->mFrontIndex);
						mChl->mData[mChl->mFrontIndex & mChl->mIndexMask].reset();
						mChl->mBarrier.publish(mChl->mFrontIndex);
						++mChl->mFrontIndex;
						return;
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
			using wrapper = typename channel<T>::wrapper;

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