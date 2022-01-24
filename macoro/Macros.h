#pragma once

//#include "macoro/Context.h"
#include "CoroFrame.h"

#ifdef MACORO_CPP20
#define MACORO_FALLTHROUGH [[fallthrough]]
#else
#define MACORO_FALLTHROUGH
#endif

#define MACORO_LOCATION __FILE__


// Begin a lambda based coroutine which returns the given type.
#define MC_BEGIN(ReturnType, ...)																				\
do { auto _macoro_frame_ = ::macoro::makeFrame<typename ReturnType::promise_type>(								\
	[__VA_ARGS__](::macoro::FrameBase<typename ReturnType::promise_type>* _macoro_frame_) mutable ->void		\
	{																											\
		try {																									\
																												\
			switch (_macoro_frame_->getSuspendPoint())															\
			{																									\
			case ::macoro::SuspensionPoint::InitialSuspend:														\
			{																									\
				using promise_type = decltype(_macoro_frame_->promise);											\
				promise_type& promise = _macoro_frame_->promise;												\
				using AwaiterFor = typename ::macoro::awaiter_for<promise_type,									\
					decltype(promise.initial_suspend())>;														\
				_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();										\
				_macoro_frame_->destroyAwaiter<AwaiterFor>();													\
			} { do{}while(0)

//              User code goes here
//              User code goes here
//              User code goes here
//              User code goes here

#define IMPL_MC_AWAIT(EXPRESSION, RETURN_SLOT, OPTIONAL_BREAK)													\
				{																								\
					using promise_type = decltype(_macoro_frame_->promise);										\
					using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(EXPRESSION)>;		\
					using Handle = coroutine_handle<promise_type>;												\
					{																							\
						auto& promise = _macoro_frame_->promise;												\
						auto handle = Handle::from_promise(promise);											\
						auto& awaiter = _macoro_frame_->constructAwaiter(EXPRESSION);							\
						if (!awaiter.await_ready())																\
						{																						\
							/*<suspend-coroutine>*/																\
							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)__LINE__);				\
																												\
							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/			\
							if (::macoro::await_suspend(awaiter, handle))										\
							{																					\
								/*<return-to-caller-or-resumer>*/												\
								return;																			\
							}																					\
						}																						\
					}																							\
					/*<resume-point>*/																			\
			MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(__LINE__):										\
					{ RETURN_SLOT _macoro_frame_->getAwaiter<AwaiterFor>().await_resume();						\
					_macoro_frame_->destroyAwaiter<AwaiterFor>();												\
					OPTIONAL_BREAK;	}																			\
				} 																								\
			} { do{}while(0)

//              User code goes here
//              User code goes here
//              User code goes here
//              User code goes here

#define MC_RETURN_VOID()																						\
				_macoro_frame_->promise.return_void();															\
				break;

#define MC_RETURN(X)																							\
				_macoro_frame_->promise.return_value(X);														\
				break;



#define MC_AWAIT(X)                  IMPL_MC_AWAIT(X, , )
#define MC_AWAIT_VAL(RETURN_SLOT, X) IMPL_MC_AWAIT(X, RETURN_SLOT = , )
#define MC_AWAIT_RETURN(X)           IMPL_MC_AWAIT(X, auto&& temp = , MC_RETURN(static_cast<decltype(temp)>(temp)))
#define MC_AWAIT_RETURN_VOID(X)      IMPL_MC_AWAIT(X, , MC_RETURN_VOID())



#define MC_END()																								\
			} break; 																							\
			default:																							\
				std::terminate();																				\
				break;																							\
			}																									\
		}																										\
		catch (...) {																							\
			_macoro_frame_->promise.unhandled_exception();														\
		}																										\
		/*final suspend*/																						\
		using promise_type = decltype(_macoro_frame_->promise);													\
		promise_type& promise = _macoro_frame_->promise;														\
		using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.final_suspend())>;		\
		using Handle = coroutine_handle<promise_type>;															\
		auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend());								\
		auto handle = Handle::from_promise(promise);															\
		if (!awaiter.await_ready())																				\
		{																										\
			/*<suspend-coroutine>*/																				\
			_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::FinalSuspend);							\
																												\
			/*<call awaiter.await_suspend(). If it's void return, then return true.>*/							\
			if (::macoro::await_suspend(awaiter, handle))														\
			{																									\
				return;																							\
			}																									\
		}																										\
		_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();												\
		_macoro_frame_->destroyAwaiter<AwaiterFor>();															\
		handle.destroy();																						\
	});																											\
																												\
	auto _macoro_ret_ = _macoro_frame_->promise.get_return_object();											\
																												\
	/*initial suspend*/																							\
	using promise_type = decltype(_macoro_frame_->promise);														\
	promise_type& promise = _macoro_frame_->promise;															\
	using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.initial_suspend())>;		\
	using Handle = coroutine_handle<promise_type>;																\
	auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend());								\
	auto handle = Handle::from_promise(promise);																\
	if (!awaiter.await_ready())																					\
	{																											\
		/*<suspend-coroutine>*/																					\
		_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::InitialSuspend);								\
																												\
		/*<call awaiter.await_suspend(). If it's void return, then return true.>*/								\
		if (::macoro::await_suspend(awaiter, handle))															\
		{																										\
			/*<return-to-caller>*/																				\
			return _macoro_ret_;																				\
		}																										\
	}																											\
	/*begin coroutine*/																							\
	(*_macoro_frame_)(_macoro_frame_);																			\
	return _macoro_ret_;																						\
} while (0)
