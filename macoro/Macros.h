#pragma once

//#include "macoro/Context.h"
#include "coro_frame.h"


#define MACORO_CAT(a, b)   MACORO_PP_CAT_I(a, b)
#define MACORO_PP_CAT_I(a, b) MACORO_PP_CAT_II(~, a ## b)
#define MACORO_PP_CAT_II(p, res) res
#define MACORO_EXPAND(x) MACORO_PP_EXPAND_I(x)
#define MACORO_PP_EXPAND_I(x) x
#



// Begin a lambda based coroutine which returns the given type.
#define MC_BEGIN(ReturnType, ...)																				\
do { auto _macoro_frame_ = ::macoro::makeFrame<typename coroutine_traits<ReturnType>::promise_type>(			\
	[__VA_ARGS__](::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>* _macoro_frame_) mutable ->::macoro::coroutine_handle<void>\
	{	using _macoro_frame_t_ = ::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>;		\
		try {																									\
																												\
			switch (_macoro_frame_->getSuspendPoint())															\
			{																									\
			case ::macoro::SuspensionPoint::InitialSuspendBegin:												\
				{																								\
					/*initial suspend*/																			\
					using promise_type = decltype(_macoro_frame_->promise);										\
					using AwaiterFor = decltype(_macoro_frame_->constructAwaiter2(_macoro_frame_->promise.initial_suspend()));		\
					/*using AwaiterFor = typename ::macoro::awaiter_for<promise_type, */							\
						/*decltype(_macoro_frame_->promise.initial_suspend())>;*/									\
					using Handle = ::macoro::coroutine_handle<promise_type>;											\
					{																							\
						promise_type& promise = _macoro_frame_->promise;										\
						auto& awaiter = _macoro_frame_->constructAwaiter(promise.initial_suspend(), (size_t)::macoro::SuspensionPoint::InitialSuspend);			\
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
					typename _macoro_frame_t_::template DestroyAwaiterRaii<AwaiterFor>							\
						raii	(_macoro_frame_, (size_t)::macoro::SuspensionPoint::InitialSuspend);			\
					_macoro_frame_->getAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::InitialSuspend).await_resume();									\
				} do {} while(0)

//              User code goes here
//              User code goes here
//              User code goes here
//              User code goes here

#define IMPL_MC_AWAIT(EXPRESSION, RETURN_SLOT, OPTIONAL_BREAK, SUSPEND_IDX)													\
				{																								\
					using promise_type = decltype(_macoro_frame_->promise);										\
					using  MACORO_CAT(AwaiterFor,SUSPEND_IDX) = decltype(_macoro_frame_->constructAwaiter2(EXPRESSION));\
					/*using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(EXPRESSION)>;*/	\
					using Handle = ::macoro::coroutine_handle<promise_type>;									\
					{																							\
						auto& promise = _macoro_frame_->promise;												\
						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);				\
						auto& awaiter = _macoro_frame_->constructAwaiter(EXPRESSION,SUSPEND_IDX);				\
						if (!awaiter.await_ready())																\
						{																						\
							/*<suspend-coroutine>*/																\
							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)SUSPEND_IDX);			\
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
			MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(SUSPEND_IDX):									\
					typename _macoro_frame_t_::template DestroyAwaiterRaii<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>	\
						raii(_macoro_frame_, SUSPEND_IDX);														\
					RETURN_SLOT _macoro_frame_->getAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX).await_resume();						\
					OPTIONAL_BREAK;																				\
				} do{}while(0)


#define MC_AWAIT_FN(FN_SLOT, EXPRESSION)														\
				{																								\
					using promise_type = decltype(_macoro_frame_->promise);										\
					using AwaiterFor = decltype(_macoro_frame_->constructAwaiter2(EXPRESSION));		\
					using Handle = ::macoro::coroutine_handle<promise_type>;												\
					{																							\
						auto& promise = _macoro_frame_->promise;												\
						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);				\
						auto& awaiter = _macoro_frame_->constructAwaiter(EXPRESSION, __LINE__);							\
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
					typename _macoro_frame_t_::template DestroyAwaiterRaii<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>	\
						raii(_macoro_frame_, __LINE__);															\
					FN_SLOT(_macoro_frame_->getAwaiter<AwaiterFor>(__LINE__).await_resume());					\
				} do{}while(0)



#define IMPL_MC_YIELD_AWAIT(EXPRESSION, SUSPEND_IDX)														\
				{																								\
					using promise_type = decltype(_macoro_frame_->promise);										\
					using MACORO_CAT(AwaiterFor,SUSPEND_IDX) = decltype(_macoro_frame_->constructAwaiter2(EXPRESSION));		\
					using Handle = ::macoro::coroutine_handle<promise_type>;									\
					{																							\
						auto& promise = _macoro_frame_->promise;												\
						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);				\
						auto& awaiter = _macoro_frame_->constructAwaiter(EXPRESSION, SUSPEND_IDX);				\
						if (!awaiter.await_ready())																\
						{																						\
							/*<suspend-coroutine>*/																\
							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)SUSPEND_IDX);			\
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
			MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(SUSPEND_IDX):										\
					/*_macoro_frame_t_::DestroyAwaiterRaii<MACORO_CAT(AwaiterFor,SUSPEND_IDX)> raii(_macoro_frame_, SUSPEND_IDX);*/	\
					IMPL_MC_AWAIT(_macoro_frame_->promise.yield_value(_macoro_frame_->getAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX).await_resume()), ,,__COUNTER__);\
				} do{}while(0)

#define MC_YIELD_AWAIT(EXPRESSION) IMPL_MC_YIELD_AWAIT(EXPRESSION, __COUNTER__)

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



#define MC_AWAIT(X)						IMPL_MC_AWAIT(X, , , __COUNTER__)
#define MC_AWAIT_SET(RETURN_SLOT, X)	IMPL_MC_AWAIT(X, RETURN_SLOT = , ,__COUNTER__)
#define MC_RETURN_AWAIT(X)				IMPL_MC_AWAIT(X, auto&& temp = , MC_RETURN(static_cast<decltype(temp)>(temp)), __COUNTER__)

#define MC_YIELD(X)						MC_AWAIT(_macoro_frame_->promise.yield_value(X));
#define MC_YIELD_SET(RETURN_SLOT, X)	MC_AWAIT_SET(RETURN_SLOT , _macoro_frame_->promise.yield_value(X));
#define MC_RETURN_YIELD(X)				MC_RETURN_AWAIT(_macoro_frame_->promise.yield_value(X));

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
		using AwaiterFor = decltype(_macoro_frame_->constructAwaiter2(_macoro_frame_->promise.final_suspend()));		\
		/*using AwaiterFor = typename ::macoro::awaiter_for<promise_type, decltype(_macoro_frame_->promise.final_suspend())>;*/\
		using Handle = ::macoro::coroutine_handle<promise_type>;															\
		{ 																										\
			promise_type& promise = _macoro_frame_->promise;													\
			auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend(),(size_t)::macoro::SuspensionPoint::FinalSuspend);							\
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
		_macoro_frame_->getAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend).await_resume();												\
		_macoro_frame_->destroyAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend);															\
		_macoro_frame_->destroy(_macoro_frame_);																\
		return noop_coroutine();																				\
	});																											\
																												\
	auto _macoro_ret_ = _macoro_frame_->promise.macoro_get_return_object();										\
	using promise_type = decltype(_macoro_frame_->promise);														\
	using Handle = ::macoro::coroutine_handle<promise_type>;																\
	promise_type& promise = _macoro_frame_->promise;															\
	auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);									\
	handle.resume();																							\
	return _macoro_ret_;																						\
} while (0)
