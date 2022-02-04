//#include "Context.h"
//#include "Macros.h"
#include "macoro/type_traits.h"
#include "macoro/macros.h"
#include "macoro/optional.h"
#include <iostream>
#include <type_traits>
#include <chrono>
#include <coroutine>
#include <numeric>
#include <fstream>
#include "tests/tests.h"

using namespace macoro;

//template <typename T>
//struct my_future {
//
//	struct promise_type;
//	struct finial_awaiter
//	{
//		constexpr bool await_ready() const noexcept { return false; }
//
//
//#ifdef MACORO_CPP_20
//		constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) const noexcept;
//#endif
//		constexpr coroutine_handle<> await_suspend(coroutine_handle<promise_type> h) const noexcept;
//
//		constexpr void await_resume() const noexcept {}
//	};
//
//
//	struct promise_type
//	{
//		void unhandled_exception() {}
//
//		suspend_always initial_suspend() noexcept { return {}; }
//		finial_awaiter final_suspend() const noexcept { return {}; }
//
//		my_future macoro_get_return_object() noexcept { return { this, coroutine_handle_type::mocoro }; }
//		my_future get_return_object() noexcept { return { this, coroutine_handle_type::std }; }
//
//
//		optional<T> storage;
//		coroutine_handle<> continuation = noop_coroutine();
//
//		template<typename T2>
//		void return_value(T2&& t)
//		{
//			storage = std::forward<T2>(t);
//		}
//
//	};
//
//	coroutine_handle<promise_type> handle;
//	my_future(promise_type* h, macoro::coroutine_handle_type t)
//		:handle(coroutine_handle<promise_type>::from_promise(*h, t))
//	{}
//
//	my_future() = default;
//	my_future(const my_future& prom) = delete;
//	my_future(my_future&& prom)
//		: handle(std::exchange(prom.handle, std::nullptr_t{}))
//	{}
//
//	~my_future()
//	{
//		if (handle)
//			handle.destroy();
//	}
//
//	T& get()
//	{
//		if (handle.promise().storage.has_value() == false)
//			throw std::runtime_error("");
//		return *handle.promise().storage;
//	}
//
//	bool await_ready() {
//		return false;
//	}
//
//#ifdef MACORO_CPP_20
//	template<typename T>
//	std::coroutine_handle<void> await_suspend(std::coroutine_handle<T> h) {
//		handle.promise().continuation = h;
//		auto hh = handle.std_cast();
//		std::cout << "std " << hh.address() << std::endl;
//		//hh.resume();
//		return hh;
//	}
//#endif // MACORO_CPP_20
//	coroutine_handle<promise_type> await_suspend(coroutine_handle<> h) {
//		handle.promise().continuation = h;
//		return handle;
//	}
//	T await_resume() { return *handle.promise().storage; }
//};
//
//template<typename T>
//inline constexpr coroutine_handle<> my_future<T>::finial_awaiter::await_suspend(coroutine_handle<promise_type> h) const noexcept
//{
//	return h.promise().continuation;
//}
//
//#ifdef MACORO_CPP_20
//template<typename T>
//inline constexpr std::coroutine_handle<> my_future<T>::finial_awaiter::await_suspend(std::coroutine_handle<promise_type> h) const noexcept
//{
//	return h.promise().continuation.std_cast();
//}
//#endif
//
//
//my_future<int>::promise_type pp;
//
//
//my_future<int> h() {
//	//MC_BEGIN(my_future<int>);
//#define __VA_ARGS_
//	using ReturnType = my_future<int>;
//	do {
//		auto _macoro_frame_ = ::macoro::makeFrame<typename coroutine_traits<ReturnType>::promise_type>(
//			[__VA_ARGS_](::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>* _macoro_frame_) mutable->::macoro::coroutine_handle<void>
//			{using _macoro_frame_t_ = ::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>;
//				try {
//
//
//					switch (_macoro_frame_->getSuspendPoint())
//					{
//					case ::macoro::SuspensionPoint::InitialSuspendBegin:
//					{
//						/*initial suspend*/
//						using promise_type = decltype(_macoro_frame_->promise);
//						using AwaiterFor = typename ::macoro::awaiter_for<promise_type,
//							decltype(_macoro_frame_->promise.initial_suspend())>;
//						using Handle = ::macoro::coroutine_handle<promise_type>;
//						{
//							promise_type& promise = _macoro_frame_->promise;
//							auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend(), (size_t)::macoro::SuspensionPoint::InitialSuspend);
//							auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//							if (!awaiter.await_ready())
//							{
//								/*<suspend-coroutine>*/
//								_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::InitialSuspend);
//
//								/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
//								auto s = ::macoro::await_suspend(awaiter, handle);
//								if (s)
//								{
//									/*perform symmetric transfer or return to caller (for noop_coroutine) */
//									return s.get_handle();
//								}
//							}
//						}
//					MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint::InitialSuspend:
//						_macoro_frame_->_initial_suspend_await_resumed_called_ = true;
//						_macoro_frame_t_::DestroyAwaiterRaii<AwaiterFor> raii(_macoro_frame_, (size_t)::macoro::SuspensionPoint::InitialSuspend);	\
//						_macoro_frame_->getAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::InitialSuspend).await_resume();
//					} do {} while (0);
//
//						MC_RETURN(42);
//
//
//					//MC_END();
//
//					break;
//					case ::macoro::SuspensionPoint::FinalSuspend:
//						goto MACORO_FINAL_SUSPEND_RESUME;
//					default:
//						std::cout << "coroutine frame corrupted. " << __FILE__ << ":" << __LINE__ << std::endl;
//						std::terminate();
//						break;
//					}
//				}
//				catch (...) {
//
//					if (!_macoro_frame_->_initial_suspend_await_resumed_called_)
//						throw;
//					_macoro_frame_->promise.unhandled_exception();
//				}
//				/*final suspend*/
//			MACORO_FINAL_SUSPEND_BEGIN:
//				using promise_type = decltype(_macoro_frame_->promise);
//				using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(_macoro_frame_->promise.final_suspend())>;
//				using Handle = ::macoro::coroutine_handle<promise_type>;
//				{
//					promise_type& promise = _macoro_frame_->promise;
//					auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend(), (size_t)::macoro::SuspensionPoint::FinalSuspend);
//					auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//					if (!awaiter.await_ready())
//					{
//						/*<suspend-coroutine>*/
//						_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::FinalSuspend);
//
//						/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
//						auto s = ::macoro::await_suspend(awaiter, handle);
//						if (s)
//						{
//							/*perform symmetric transfer or return to caller (for noop_coroutine) */
//							return s.get_handle();
//						}
//					}
//				}
//			MACORO_FINAL_SUSPEND_RESUME:
//				_macoro_frame_->getAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend).await_resume();
//				_macoro_frame_->destroyAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend);
//				_macoro_frame_->destroy(_macoro_frame_);
//				return noop_coroutine();
//			});
//
//		auto _macoro_ret_ = _macoro_frame_->promise.macoro_get_return_object();
//		using promise_type = decltype(_macoro_frame_->promise);
//		using Handle = ::macoro::coroutine_handle<promise_type>;
//		promise_type& promise = _macoro_frame_->promise;
//		auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//		handle.resume();
//		return _macoro_ret_;
//	} while (0);
//}
//
//#ifdef MACORO_CPP_20
//// C++20 version
//my_future<int> g1() {
//	int i = co_await h();
//	if (i == 42)
//		i = 0;
//	co_return i;
//}
//#endif
//
//// C++14 version
//my_future<int> g2() {
//	// Specify the return type and lambda capture and local variables.
//	//MC_BEGIN(my_future<int>, i = int{});
//	using ReturnType = my_future<int>;
//#define __VA_ARGS_ i = int{}
//	//std::coroutine_traits
//	auto _macoro_frame_ = ::macoro::makeFrame<typename coroutine_traits<ReturnType>::promise_type>(
//		[__VA_ARGS_](::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>* _macoro_frame_) mutable -> ::macoro::coroutine_handle<void>
//		{using _macoro_frame_t_ = ::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>;
//			try {
//				switch (_macoro_frame_->getSuspendPoint())
//				{
//				case ::macoro::SuspensionPoint::InitialSuspendBegin:
//				{
//					/*initial suspend*/
//					using promise_type = decltype(_macoro_frame_->promise);
//					using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(_macoro_frame_->promise.initial_suspend())>;
//					using Handle = coroutine_handle<promise_type>;
//					{
//						promise_type& promise = _macoro_frame_->promise;
//						auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend(),(size_t)::macoro::SuspensionPoint::InitialSuspend);
//						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//						if (!awaiter.await_ready())
//						{
//							/*<suspend-coroutine>*/
//							_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::InitialSuspend);
//
//							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
//							auto s = ::macoro::await_suspend(awaiter, handle);
//							if (s)
//							{
//								/*perform symmetric transfer or return to caller (for noop_coroutine) */
//								return s.get_handle();
//							}
//						}
//					}
//				MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint::InitialSuspend:
//					_macoro_frame_->_initial_suspend_await_resumed_called_ = true;
//					_macoro_frame_t_::DestroyAwaiterRaii<AwaiterFor> raii(_macoro_frame_, (size_t)::macoro::SuspensionPoint::InitialSuspend);	
//					_macoro_frame_->getAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::InitialSuspend).await_resume();
//				}
//
//				// assign the result of h() to i
//				//MC_AWAIT_VAL(i, h());
//
//			//#define IMPL_MC_AWAIT(, RETURN_SLOT, OPTIONAL_BREAK)	
//#define EXPRESSION h()
//#define RETURN_SLOT i =
//#define OPTIONAL_BREAK
//				{
//					using promise_type = decltype(_macoro_frame_->promise);
//					using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(EXPRESSION)>;
//					using Handle = coroutine_handle<promise_type>;
//					{
//						auto& promise = _macoro_frame_->promise;
//						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//						auto& awaiter = _macoro_frame_->constructAwaiter(EXPRESSION, 241);
//						if (!awaiter.await_ready())
//						{
//							/*<suspend-coroutine>*/
//							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)241);
//
//							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
//							auto s = ::macoro::await_suspend(awaiter, handle);
//							if (s)
//							{
//								/*<return-to-caller-or-resumer>*/
//								return s.get_handle();
//							}
//						}
//					}
//					/*<resume-point>*/
//				MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(241):
//				{
//					_macoro_frame_t_::DestroyAwaiterRaii<AwaiterFor> raii(_macoro_frame_, 241);
//					//auto raii = _macoro_frame_->destroyAwaiterRaii<AwaiterFor>(241);
//					RETURN_SLOT _macoro_frame_->getAwaiter<AwaiterFor>(241).await_resume();
//					OPTIONAL_BREAK;
//				}
//				}
//
//				if (i == 42)
//					i = 0;
//
//				MC_RETURN(i);
//
//				//MC_END();
//				break;
//				case ::macoro::SuspensionPoint::FinalSuspend:
//					goto MACORO_FINAL_SUSPEND_RESUME;
//				default:
//					std::terminate();
//					break;
//				}
//			}
//			catch (...) {
//				if (!_macoro_frame_->_initial_suspend_await_resumed_called_)
//					throw;
//				_macoro_frame_->promise.unhandled_exception();
//			}
//			/*final suspend*/
//		MACORO_FINAL_SUSPEND_BEGIN:
//			using promise_type = decltype(_macoro_frame_->promise);
//			using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(_macoro_frame_->promise.final_suspend())>;
//			using Handle = coroutine_handle<promise_type>;
//			{
//				promise_type& promise = _macoro_frame_->promise;
//				auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend(), (size_t)::macoro::SuspensionPoint::FinalSuspend);
//				static_assert(noexcept(awaiter.await_ready()), "final suspend (await_ready) must be noexcept");
//				if (!awaiter.await_ready())
//				{
//					/*<suspend-coroutine>*/
//					_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::FinalSuspend);
//
//					/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
//					static_assert(noexcept(::macoro::await_suspend(awaiter, Handle::from_promise(promise, coroutine_handle_type::mocoro))), "final suspend (await_suspend) must be noexcept");
//					auto s = ::macoro::await_suspend(awaiter, Handle::from_promise(promise, coroutine_handle_type::mocoro));
//					if (s)
//					{
//						return s.get_handle();
//					}
//				}
//			}
//		MACORO_FINAL_SUSPEND_RESUME:
//			static_assert(noexcept(_macoro_frame_->getAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend).await_resume()), "final suspend (await_resume) must be noexcept");
//			_macoro_frame_->getAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend).await_resume();
//			_macoro_frame_->destroyAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend);
//			_macoro_frame_->destroy(_macoro_frame_);
//			return noop_coroutine();
//		});
//
//	auto _macoro_ret_ = _macoro_frame_->promise.macoro_get_return_object();
//	using promise_type = decltype(_macoro_frame_->promise);
//	using Handle = coroutine_handle<promise_type>;
//	promise_type& promise = _macoro_frame_->promise;
//	auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//	handle.resume();
//
//	return _macoro_ret_;
//
//}


int main(int argc, char** argv)
{
	macoro::tests();
	//auto fu = g1();
	//fu.handle.resume();
	////pp.return_value(4);

	////auto f2 = g2();

	//std::cout << fu.get() << std::endl;

	return 0;
}

