#pragma once

//#include "macoro/Context.h"
#include "CoroFrame.h"


// Begin a lambda based coroutine which returns the given type.
#define MC_BEGIN(ReturnType, ...)																				\
do { auto _macoro_frame_ = ::macoro::makeFrame<typename ReturnType::promise_type>(								\
	[__VA_ARGS__](::macoro::FrameBase<typename ReturnType::promise_type>* _macoro_frame_) mutable ->coroutine_handle<void>		\
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
			} do{}while(0)

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
						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);											\
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
								return std::nullptr_t{};																			\
							}																					\
						}																						\
					}																							\
					/*<resume-point>*/																			\
			MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(__LINE__):										\
					{ RETURN_SLOT _macoro_frame_->getAwaiter<AwaiterFor>().await_resume();						\
					_macoro_frame_->destroyAwaiter<AwaiterFor>();												\
					OPTIONAL_BREAK;	}																			\
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



#define MC_AWAIT(X)                IMPL_MC_AWAIT(X, , )
#define MC_AWAIT_VAL(RETURN_SLOT, X)   IMPL_MC_AWAIT(X, RETURN_SLOT = , )
#define MC_AWAIT_RETURN(X)         IMPL_MC_AWAIT(X, auto&& temp = , MC_RETURN(static_cast<decltype(temp)>(temp)))

#define MC_END()																								\
			break; 																							\
			default:																							\
				if(_macoro_frame_->getSuspendPoint() == ::macoro::SuspensionPoint::FinalSuspend)					\
					std::cout << "coroutine was resumed after it finished. " <<__FILE__ <<":" <<__LINE__ << std::endl;			\
				else																							\
					std::cout << "coroutine frame corrupted. " <<__FILE__ <<":" <<__LINE__ << std::endl;			\
				std::terminate();																				\
				break;																							\
			}																									\
		}																										\
		catch (...) {																							\
			_macoro_frame_->promise.unhandled_exception();														\
		}																										\
		/*final suspend*/																						\
MACORO_FINAL_SUSPEND_BEGIN:	\
		using promise_type = decltype(_macoro_frame_->promise);													\
		promise_type& promise = _macoro_frame_->promise;														\
		using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.final_suspend())>;		\
		using Handle = coroutine_handle<promise_type>;															\
		auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend());								\
		auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);															\
		if (!awaiter.await_ready())																				\
		{																										\
			/*<suspend-coroutine>*/																				\
			_macoro_frame_->setSuspendPoint(::macoro::SuspensionPoint::FinalSuspend);							\
																												\
			/*<call awaiter.await_suspend(). If it's void return, then return true.>*/							\
			auto s = ::macoro::await_suspend(awaiter, handle);													\
			if (s)																								\
			{																									\
				return s.get_handle();																							\
			}																									\
		}																										\
		_macoro_frame_->getAwaiter<AwaiterFor>().await_resume();												\
		_macoro_frame_->destroyAwaiter<AwaiterFor>();															\
		handle.destroy();																						\
		return {};																								\
	});																											\
																												\
	auto _macoro_ret_ = _macoro_frame_->promise.macoro_get_return_object();											\
																												\
	/*initial suspend*/																							\
	using promise_type = decltype(_macoro_frame_->promise);														\
	promise_type& promise = _macoro_frame_->promise;															\
	using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(promise.initial_suspend())>;		\
	using Handle = coroutine_handle<promise_type>;																\
	auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend());								\
	auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);																\
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
