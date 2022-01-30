//#include "Context.h"
//#include "Macros.h"
#include "TypeTraits.h"
#include <iostream>
#include <type_traits>
#include <chrono>
#include "Macros.h"
#include <coroutine>
#include <numeric>
#include <fstream>
using namespace macoro;

template <typename T>
struct my_future {

	struct promise_type;
	struct finial_awaiter
	{
		constexpr bool await_ready() const noexcept { return false; }

		constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) const noexcept;
		constexpr coroutine_handle<> await_suspend(coroutine_handle<promise_type> h) const noexcept;

		constexpr void await_resume() const noexcept {}
	};


	struct promise_type
	{
		void unhandled_exception() {}

		suspend_always initial_suspend() noexcept { return {}; }
		finial_awaiter final_suspend() const noexcept { return {}; }

		my_future macoro_get_return_object() noexcept { return { this, coroutine_handle_type::mocoro }; }
		my_future get_return_object() noexcept { return { this, coroutine_handle_type::std }; }


		optional<T> storage;
		coroutine_handle<> continuation = noop_coroutine();

		template<typename T2>
		void return_value(T2&& t)
		{
			storage = std::forward<T2>(t);
		}

	};

	coroutine_handle<promise_type> handle;
	my_future(promise_type* h, macoro::coroutine_handle_type t)
		:handle(coroutine_handle<promise_type>::from_promise(*h, t))
	{}

	my_future() = default;
	my_future(const my_future& prom) = delete;
	my_future(my_future&& prom) = default;

	T& get()
	{
		if (handle.promise().storage.has_value() == false)
			throw std::runtime_error("");
		return *handle.promise().storage;
	}

	bool await_ready() {
		return false;
	}

	template<typename T>
	std::coroutine_handle<void> await_suspend(std::coroutine_handle<T> h) {
		handle.promise().continuation = h;
		return handle.std_cast();
	}
	coroutine_handle<promise_type> await_suspend(coroutine_handle<> h) {
		handle.promise().continuation = h;
		return handle;
	}
	T await_resume() { return *handle.promise().storage; }
};

template<typename T>
inline constexpr coroutine_handle<> my_future<T>::finial_awaiter::await_suspend(coroutine_handle<promise_type> h) const noexcept
{
	return h.promise().continuation;
}

template<typename T>
inline constexpr std::coroutine_handle<> my_future<T>::finial_awaiter::await_suspend(std::coroutine_handle<promise_type> h) const noexcept
{
	auto& p = h.promise();
}



my_future<int>::promise_type pp;


my_future<int> h() {
	MC_BEGIN(my_future<int>);


	MC_RETURN(42);


	MC_END();
}

// C++20 version
my_future<int> g1() {
	int i = co_await h();
	if (i == 42)
		i = 0;
	co_return i;
}

// C++14 version
my_future<int> g2() {
	// Specify the return type and lambda capture and local variables.
	//MC_BEGIN(my_future<int>, i = int{});
	using ReturnType = my_future<int>;
#define __VA_ARGS_ i = int{}

	auto _macoro_frame_ = ::macoro::makeFrame<typename ReturnType::promise_type>(
		[__VA_ARGS_](::macoro::FrameBase<typename ReturnType::promise_type>* _macoro_frame_) mutable -> ::macoro::coroutine_handle<void>
		{
			try {


				switch (_macoro_frame_->getSuspendPoint())
				{
				case ::macoro::SuspensionPoint::Init:
				{
					/*initial suspend*/
					using promise_type = decltype(_macoro_frame_->promise);
					using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(_macoro_frame_->promise.initial_suspend())>;
					using Handle = coroutine_handle<promise_type>;
					{
						promise_type& promise = _macoro_frame_->promise;
						auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend());
						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
						if (!awaiter.await_ready())
						{
							/*<suspend-coroutine>*/
							_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::InitialSuspend);

							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
							auto s = ::macoro::await_suspend(awaiter, handle);
							if (s)
							{
								/*<return-to-caller-or-resumer>*/
								return s.get_handle();
							}
						}
					}
				MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint::InitialSuspend:
					promise_type& promise = _macoro_frame_->promise;
					_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();
					_macoro_frame_->destroyAwaiter<AwaiterFor>();
				}

				// assign the result of h() to i
				//MC_AWAIT_VAL(i, h());

			//#define IMPL_MC_AWAIT(, RETURN_SLOT, OPTIONAL_BREAK)	
#define EXPRESSION h()
#define RETURN_SLOT i =
#define OPTIONAL_BREAK
				{
					using promise_type = decltype(_macoro_frame_->promise);
					using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(EXPRESSION)>;
					using Handle = coroutine_handle<promise_type>;
					{
						auto& promise = _macoro_frame_->promise;
						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
						auto& awaiter = _macoro_frame_->constructAwaiter(EXPRESSION);
						if (!awaiter.await_ready())
						{
							/*<suspend-coroutine>*/
							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)241);

							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
							auto s = ::macoro::await_suspend(awaiter, handle);
							if (s)
							{
								/*<return-to-caller-or-resumer>*/
								return s.get_handle();
							}
						}
					}
					/*<resume-point>*/
				MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(241):
					try {
						RETURN_SLOT _macoro_frame_->getAwaiter<AwaiterFor>().await_resume();
						_macoro_frame_->destroyAwaiter<AwaiterFor>();
						OPTIONAL_BREAK;
					}
					catch (...) {
						_macoro_frame_->destroyAwaiter<AwaiterFor>();
						std::rethrow_exception(std::current_exception());
					}
				}

				if (i == 42)
					i = 0;

				MC_RETURN(i);

				//MC_END();
				break;
				default:
					std::terminate();
					break;
				}
			}
			catch (...) {
				_macoro_frame_->promise.unhandled_exception();
			}
			/*final suspend*/
		MACORO_FINAL_SUSPEND_BEGIN:
			using promise_type = decltype(_macoro_frame_->promise);
			promise_type& promise = _macoro_frame_->promise;
			using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.final_suspend())>;
			using Handle = coroutine_handle<promise_type>;
			auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend());
			auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
			if (!awaiter.await_ready())
			{
				/*<suspend-coroutine>*/
				_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::FinalSuspend);

				/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
				auto s = ::macoro::await_suspend(awaiter, handle);
				if (s)
				{
					return s.get_handle();
				}
			}
			_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();
			_macoro_frame_->destroyAwaiter<AwaiterFor>();
			handle.destroy();

			return {};
		});

	auto _macoro_ret_ = _macoro_frame_->promise.macoro_get_return_object();
	using promise_type = decltype(_macoro_frame_->promise);
	using Handle = coroutine_handle<promise_type>;
	promise_type& promise = _macoro_frame_->promise;
	auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);

	/*initial suspend*/
	//using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.initial_suspend())>;
	//auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend());
	//if (!awaiter.await_ready())
	//{
	//	/*<suspend-coroutine>*/
	//	_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::InitialSuspend);

	//	/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
	//	auto s = ::macoro::await_suspend(awaiter, handle);
	//	if (s)
	//	{
	//		if (s.get_handle().address() == nullptr)
	//			throw std::runtime_error("not sure what to do here.");
	//		//throw std::runtime_error("");
	//		/*<return-to-caller>*/
	//		return _macoro_ret_;
	//	}
	//}
	/*begin coroutine*/
	//auto s = (*_macoro_frame_)(_macoro_frame_);

	handle.resume();

	return _macoro_ret_;

}

//
//void exp()
//{
//	//void* 
//	std::vector<void*> mm(1000, nullptr);
//
//	std::vector<void*> resume(mm.size());
//	std::vector<void*> resume2(mm.size());
//	//std::iota(mm.begin(), mm.end(), 0);
//	for (size_t i = 0; i < mm.size(); ++i)
//	{
//		mm[i] = (void*)i;
//		resume[i] = (void*)((1ull << 32) + i);
//		resume2[i] = (void*)((2ull << 32) + i);
//	}
//	mm[0] = resume.data();
//	resume[5] = resume2.data();
//
//	auto hh = std::coroutine_handle<void>::from_address(mm.data());
//
//
//	hh.resume();
//
//}

int main(int argc, char** argv)
{
	auto fu = g1();
	fu.handle.resume();
	//pp.return_value(4);

	//auto f2 = g2();

	std::cout << fu.get() << std::endl;

	return 0;
}

