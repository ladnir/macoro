#pragma once

//#include "macoro/Context.h"
#include "coro_frame.h"


#ifndef MACORO_HANDLE_TYPE
#define MACORO_HANDLE_TYPE ::macoro::coroutine_handle
#endif


// Begin a lambda based coroutine which returns the given type.
#define MC_BEGIN(ReturnType, ...)																				\
do { auto _macoro_frame_ = ::macoro::makeFrame<typename coroutine_traits<ReturnType>::promise_type>(			\
	[__VA_ARGS__](::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>* _macoro_frame_) mutable ->MACORO_HANDLE_TYPE<void>\
	{																											\
		try {																									\
																												\
			switch (_macoro_frame_->getSuspendPoint())															\
			{																									\
			case ::macoro::SuspensionPoint::InitialSuspendBegin:												\
				{																								\
					/*initial suspend*/																			\
					using promise_type = decltype(_macoro_frame_->promise);										\
					using AwaiterFor = typename ::macoro::awaiter_for<promise_type, 							\
						decltype(_macoro_frame_->promise.initial_suspend())>;									\
					using Handle = MACORO_HANDLE_TYPE<promise_type>;											\
					{																							\
						promise_type& promise = _macoro_frame_->promise;										\
						auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend());			\
						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);				\
						if (!awaiter.await_ready())																\
						{																						\
							/*<suspend-coroutine>*/																\
							_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::InitialSuspend);			\
																												\
							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/			\
							auto s = ::macoro::await_suspend(awaiter, handle);									\
							if (s)																				\
							{																					\
								/*perform symmetric transfer or return to caller (for noop_coroutine) */		\
								return s.get_handle();															\
							}																					\
						}																						\
					}																							\
			MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint::InitialSuspend:									\
					_macoro_frame_->_initial_suspend_await_resumed_called_ = true;								\
					auto raii = _macoro_frame_->destroyAwaiterRaii<AwaiterFor>();								\
					_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();									\
				} do {} while(0)

//              User code goes here
//              User code goes here
//              User code goes here
//              User code goes here

#define IMPL_MC_AWAIT(EXPRESSION, RETURN_SLOT, OPTIONAL_BREAK)													\
				{																								\
					using promise_type = decltype(_macoro_frame_->promise);										\
					using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(EXPRESSION)>;		\
					using Handle = MACORO_HANDLE_TYPE<promise_type>;												\
					{																							\
						auto& promise = _macoro_frame_->promise;												\
						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);				\
						auto& awaiter = _macoro_frame_->constructAwaiter(EXPRESSION);							\
						if (!awaiter.await_ready())																\
						{																						\
							/*<suspend-coroutine>*/																\
							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)__LINE__);				\
																												\
							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/			\
							auto s = ::macoro::await_suspend(awaiter, handle);									\
							if (s)																				\
							{																					\
								/*perform symmetric transfer or return to caller (for noop_coroutine) */		\
								return s.get_handle();															\
							}																					\
						}																						\
					}																							\
					/*<resume-point>*/																			\
			MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(__LINE__):										\
					auto raii = _macoro_frame_->destroyAwaiterRaii<AwaiterFor>();								\
					RETURN_SLOT _macoro_frame_->getAwaiter<AwaiterFor>().await_resume();						\
					OPTIONAL_BREAK;																				\
				} do{}while(0)

//              User code goes here
//              User code goes here
//              User code goes here
//              User code goes here

#define MC_RETURN_VOID()																						\
				_macoro_frame_->promise.return_void();															\
				goto MACORO_FINAL_SUSPEND_BEGIN;

#define MC_RETURN(X)																							\
				_macoro_frame_->promise.return_value(X);														\
				goto MACORO_FINAL_SUSPEND_BEGIN;



#define MC_AWAIT(X)						IMPL_MC_AWAIT(X, , )
#define MC_AWAIT_SET(RETURN_SLOT, X)	IMPL_MC_AWAIT(X, RETURN_SLOT = , )
#define MC_AWAIT_RETURN(X)				IMPL_MC_AWAIT(X, auto&& temp = , MC_RETURN(static_cast<decltype(temp)>(temp)))

#define MC_YIELD(X)						MC_AWAIT(_macoro_frame_->yield_value(X));
#define MC_YIELD_SET(RETURN_SLOT, X)	MC_AWAIT_SET(RETURN_SLOT , _macoro_frame_->yield_value(X));
#define MC_YIELD_RETURN(X)				MC_AWAIT_RETURN(_macoro_frame_->yield_value(X));

#define MC_END()																								\
				break; 																							\
			case ::macoro::SuspensionPoint::FinalSuspend:														\
				goto MACORO_FINAL_SUSPEND_RESUME;																\
			default:																							\
				std::cout << "coroutine frame corrupted. " <<__FILE__ <<":" <<__LINE__ << std::endl;			\
				std::terminate();																				\
				break;																							\
			}																									\
		}																										\
		catch (...) {																							\
			if (!_macoro_frame_->_initial_suspend_await_resumed_called_)										\
				throw; 																							\
			_macoro_frame_->promise.unhandled_exception();														\
		}																										\
		/*final suspend*/																						\
MACORO_FINAL_SUSPEND_BEGIN:																						\
		using promise_type = decltype(_macoro_frame_->promise);													\
		using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(_macoro_frame_->promise.final_suspend())>;\
		using Handle = MACORO_HANDLE_TYPE<promise_type>;															\
		{ 																										\
			promise_type& promise = _macoro_frame_->promise;													\
			auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend());							\
			auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);							\
			if (!awaiter.await_ready())																			\
			{																									\
				/*<suspend-coroutine>*/																			\
				_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::FinalSuspend);						\
																												\
				/*<call awaiter.await_suspend(). If it's void return, then return true.>*/						\
				auto s = ::macoro::await_suspend(awaiter, handle);												\
				if (s)																							\
				{																								\
					/*perform symmetric transfer or return to caller (for noop_coroutine) */					\
					return s.get_handle();																		\
				}																								\
			}																									\
		}																										\
MACORO_FINAL_SUSPEND_RESUME:																					\
		_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();												\
		_macoro_frame_->destroyAwaiter<AwaiterFor>();															\
		_macoro_frame_->destroy(_macoro_frame_);																\
		return noop_coroutine();																				\
	});																											\
																												\
	auto _macoro_ret_ = _macoro_frame_->promise.macoro_get_return_object();										\
	using promise_type = decltype(_macoro_frame_->promise);														\
	using Handle = MACORO_HANDLE_TYPE<promise_type>;																\
	promise_type& promise = _macoro_frame_->promise;															\
	auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);									\
	handle.resume();																							\
	return _macoro_ret_;																						\
} while (0)
