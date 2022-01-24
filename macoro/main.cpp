//#include "Context.h"
//#include "Macros.h"
#include "coroutine_handle.h"
#include "TypeTraits.h"
#include <iostream>
#include <type_traits>
#include <chrono>
#include "Macros.h"

using namespace macoro;

struct suspend_always
{
	bool await_ready() { return false; }
	void await_suspend(coroutine_handle<>) {}
	void await_resume() {}
};
struct suspend_never
{
	bool await_ready() { return true; }
	void await_suspend(coroutine_handle<>) {}
	void await_resume() {}
};


template <typename T>
struct my_future {

	struct promise_type
	{
		void unhandled_exception() {}

		suspend_never initial_suspend() { return {}; }
		suspend_always final_suspend() { return {}; }

		my_future get_return_object() { return { this }; }


		optional<T> storage;
		coroutine_handle<> continuation;

		template<typename T2>
		void return_value(T2&& t)
		{
			storage = std::forward<T2>(t);
			if (continuation.address())
				continuation.resume();
		}

	};

	promise_type* mProm = nullptr;
	my_future(promise_type* prom)
		:mProm(prom)
	{}

	my_future() = default;
	my_future(const my_future& prom) = delete;
	my_future(my_future&& prom) = default;

	template<typename T2>
	my_future(T2&& t, enable_if_t<std::is_constructible<T, T2&&>::value, monostate> = {})
		:mProm(new promise_type)
	{
		mProm->storage = (std::forward<T2>(t));
	}


	T& get()
	{
		if (await_ready() == false)
			throw std::runtime_error("");

		return *mProm->storage;
	}

	//...
	bool await_ready() {
		return static_cast<bool>(mProm->storage);
	}
	void await_suspend(coroutine_handle<> h) { mProm->continuation = h; }
	T await_resume() { return *mProm->storage; }
};


my_future<int>::promise_type pp;


my_future<int> h() { return { &pp }; }
my_future<int> g() {

	using ReturnType = my_future<int>;
	do {
		using promise_type = ReturnType::promise_type;
		auto _macoro_frame_ = ::macoro::makeFrame<promise_type>(
			[i = int{}](::macoro::FrameBase<promise_type>* _macoro_frame_) mutable ->void {

				try {

					switch (_macoro_frame_->getSuspendPoint())
					{
					case ::macoro::SuspensionPoint::InitialSuspend:
					{
						auto& promise = _macoro_frame_->promise;
						using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.initial_suspend())>;
						_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();
						_macoro_frame_->destroyAwaiter<AwaiterFor>();
					} { do {} while (0);

					//MC_AWAIT(h());
#define SUSPEND_PTR 1
#define X h()
					using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(X)>;
					using Handle = coroutine_handle<promise_type>;
					{
						auto& promise = _macoro_frame_->promise;
						auto handle = Handle::from_promise(promise);
						auto& awaiter = _macoro_frame_->constructAwaiter(X);
						if (!awaiter.await_ready())
						{
							/*<suspend-coroutine>*/
							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)SUSPEND_PTR);

							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
							if (::macoro::await_suspend(awaiter, handle))
							{
								/*<return-to-caller-or-resumer>*/
								return;
							}
						}
					}
					/*<resume-point>*/
					MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(SUSPEND_PTR):
						_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();
						_macoro_frame_->destroyAwaiter<AwaiterFor>();
					} { do {} while (0);


					//MC_RETURN(X);



					}
					break;
					default:
						break;
					}
				}
				catch (...) {
					_macoro_frame_->promise.unhandled_exception();
				}
				/*final suspend*/
				auto& promise = _macoro_frame_->promise;
				using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.final_suspend())>;
				using Handle = coroutine_handle<promise_type>;
				auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend());
				auto handle = Handle::from_promise(promise);
				if (!awaiter.await_ready())
				{
					/*<suspend-coroutine>*/
					_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::FinalSuspend);

					/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
					if (::macoro::await_suspend(awaiter, handle))
					{
						return;
					}
				}
				_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();
				_macoro_frame_->destroyAwaiter<AwaiterFor>();
				handle.destroy();
			});

		auto _macoro_ret_ = _macoro_frame_->promise.get_return_object();

		/*initial suspend*/
		auto& promise = _macoro_frame_->promise;
		using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.initial_suspend())>;
		using Handle = coroutine_handle<promise_type>;
		auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend());
		auto handle = Handle::from_promise(promise);
		if (!awaiter.await_ready())
		{
			/*<suspend-coroutine>*/
			_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::InitialSuspend);

			/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
			if (::macoro::await_suspend(awaiter, handle))
			{
				/*<return-to-caller>*/
				return _macoro_ret_;
			}
		}
		/*begin coroutine*/
		(*_macoro_frame_)(_macoro_frame_);
		return _macoro_ret_;
	} while (0);
}

auto g2()
{
	MC_BEGIN(my_future<int>);

	MC_AWAIT_RETURN(h());

	MC_END();
}

int main(int argc, char** argv)
{
	auto fu = g2();

	pp.return_value(4);


	std::cout << fu.get() << std::endl;

	std::cout << __FUNCSIG__ << std::endl;

	return 0;
}



