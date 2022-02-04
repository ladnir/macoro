#pragma once

//#include "macoro/Context.h"
#include "coro_frame.h"


#define MACORO_CAT(a, b)   MACORO_PP_CAT_I(a, b)
#define MACORO_PP_CAT_I(a, b) MACORO_PP_CAT_II(~, a ## b)
#define MACORO_PP_CAT_II(p, res) res
#define MACORO_EXPAND(x) MACORO_PP_EXPAND_I(x)
#define MACORO_PP_EXPAND_I(x) x



// This macro expands out to generated the await_ready and await_suspend calls.
// It defines a suspension point but does not call await_resume. This should be
// used with other macros which perform the await_resume call.
#define IMPL_MC_AWAIT_CORE(EXPRESSION, SUSPEND_IDX)										\
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
					}	}	}																					\
			MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(SUSPEND_IDX):		
						

// a helper macro that defines a basic co_await expression. 
// RETURN_SLOT can be used to set a variable to the result of the co_await expression.
// SUSPEND_IDX should be a unique integer literal. Typically obtained using __COUNTER__.
#define IMPL_MC_AWAIT(EXPRESSION, RETURN_SLOT, SUSPEND_IDX)														\
				{																								\
					IMPL_MC_AWAIT_CORE(EXPRESSION, SUSPEND_IDX)													\
					/*<resume-point>*/																			\
					RETURN_SLOT _macoro_frame_->getAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX).await_resume();		\
					_macoro_frame_->destroyAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX);			\
				} do{}while(0)

// A helper macro that implements the MC_YIELD_AWAIT. We first perform a normal co_await
// and then we call another co_await on the result of promise.yield_value(awaiter.await_resume()).
#define IMPL_MC_YIELD_AWAIT(EXPRESSION, SUSPEND_IDX)															\
				{	IMPL_MC_AWAIT_CORE(EXPRESSION, SUSPEND_IDX)													\
					/*<resume-point>*/																			\
					IMPL_MC_AWAIT(_macoro_frame_->promise.yield_value(_macoro_frame_->getAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX).await_resume()), ,__COUNTER__);\
					_macoro_frame_->destroyAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX);			\
				} do{}while(0)


// A helper macro that implements the MC_AWAIT_AWAIT. We first perform a normal co_await
// and then we call another co_await on the result of awaiter.await_resume().
#define IMPL_MC_AWAIT_AWAIT(EXPRESSION, SUSPEND_IDX)															\
				{	IMPL_MC_AWAIT_CORE(EXPRESSION, SUSPEND_IDX)													\
					/*<resume-point>*/																			\
					IMPL_MC_AWAIT(_macoro_frame_->getAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX).await_resume(), ,__COUNTER__);\
					_macoro_frame_->destroyAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX);			\
				} do{}while(0)


// A helper macro that implements the MC_AWAIT_FN. We first perform a normal co_await
// and then we call the function with the await_resume result as the parameter.
#define IMPL_MC_AWAIT_FN(FN_SLOT, EXPRESSION, OPTIONAL_GOTO, SUSPEND_IDX)										\
				{																								\
					IMPL_MC_AWAIT_CORE(EXPRESSION, SUSPEND_IDX)													\
					/*<resume-point>*/																			\
					FN_SLOT(_macoro_frame_->getAwaiter<AwaiterFor>(SUSPEND_IDX).await_resume());				\
					_macoro_frame_->destroyAwaiter<MACORO_CAT(AwaiterFor,SUSPEND_IDX)>(SUSPEND_IDX);			\
					OPTIONAL_GOTO																				\
				} do{}while(0)


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
					IMPL_MC_AWAIT_CORE(_macoro_frame_->promise.initial_suspend(), MACORO_INITIAL_SUSPEND_IDX)	\
					_macoro_frame_->_initial_suspend_await_resumed_called_ = true;								\
					_macoro_frame_->getAwaiter<MACORO_CAT(AwaiterFor,MACORO_INITIAL_SUSPEND_IDX)>(MACORO_INITIAL_SUSPEND_IDX).await_resume();			\
					_macoro_frame_->destroyAwaiter<MACORO_CAT(AwaiterFor,MACORO_INITIAL_SUSPEND_IDX)>(MACORO_INITIAL_SUSPEND_IDX);						\
				} do {} while(0)


// Perform a co_yield co_await expression. This is equivalent to
// `co_yield (co_await EXPRESSION)`
#define MC_YIELD_AWAIT(EXPRESSION) IMPL_MC_YIELD_AWAIT(EXPRESSION, __COUNTER__)


// Perform a co_await co_await expression. This is equivalent to
// `co_await (co_await EXPRESSION)`
#define MC_AWAIT_AWAIT(EXPRESSION) IMPL_MC_AWAIT_AWAIT(EXPRESSION, __COUNTER__)


// Calls the given function on the result of the co_await expression. This is equivalent to
// `f(co_await EXPRESSION)` where f is the function name provided.
#define MC_AWAIT_FN(FN_SLOT, EXPRESSION)	IMPL_MC_AWAIT_FN(FN_SLOT,EXPRESSION, , __COUNTER__)					

// Returns void. Equivalent to `co_return;`
#define MC_RETURN_VOID()																						\
				_macoro_frame_->promise.return_void();															\
				goto MACORO_FINAL_SUSPEND_BEGIN;

// Returns a value. Equivalent to `co_return X;`
#define MC_RETURN(X)																							\
				_macoro_frame_->promise.return_value(X);														\
				goto MACORO_FINAL_SUSPEND_BEGIN;


// perform a basic co_await expression. This is equivalent to
// `co_await EXPRESSION`
#define MC_AWAIT(EXPRESSION)				IMPL_MC_AWAIT(EXPRESSION, , __COUNTER__)


// perform a basic co_await expression and set VARIABLE to the result. This is equivalent to
// `VARIABLE = co_await EXPRESSION`
#define MC_AWAIT_SET(VARIABLE, EXPRESSION)	IMPL_MC_AWAIT(EXPRESSION, VARIABLE = , __COUNTER__)

// perform a basic co_await expression and return to the result. This is equivalent to
// `co_return co_await EXPRESSION`
#define MC_RETURN_AWAIT(EXPRESSION)			IMPL_MC_AWAIT_FN(_macoro_frame_->promise.return_value, EXPRESSION, goto MACORO_FINAL_SUSPEND_BEGIN;,__COUNTER__)


// perform a basic co_yeild expression. This is equivalent to
// `co_yeild EXPRESSION`
#define MC_YIELD(EXPRESSION)				MC_AWAIT(_macoro_frame_->promise.yield_value(EXPRESSION));


// perform a basic co_yeild expression and set VARIABLE to the result. This is equivalent to
// `VARIABLE = co_yeild EXPRESSION`
#define MC_YIELD_SET(VARIABLE, EXPRESSION)	MC_AWAIT_SET(VARIABLE , _macoro_frame_->promise.yield_value(EXPRESSION));

// perform a basic co_yeild expression and return to the result. This is equivalent to
// `co_return co_yeild EXPRESSION`
#define MC_RETURN_YIELD(EXPRESSION)			MC_RETURN_AWAIT(_macoro_frame_->promise.yield_value(EXPRESSION));

// End the coroutine body.
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
			_macoro_frame_->destroyAwaiters();																	\
			if (!_macoro_frame_->_initial_suspend_await_resumed_called_)										\
				throw; 																							\
			_macoro_frame_->promise.unhandled_exception();														\
		}																										\
		/*final suspend*/																						\
MACORO_FINAL_SUSPEND_BEGIN:																						\
		using promise_type = decltype(_macoro_frame_->promise);													\
		using AwaiterFor = decltype(_macoro_frame_->constructAwaiter2(_macoro_frame_->promise.final_suspend()));\
		using Handle = ::macoro::coroutine_handle<promise_type>;												\
		{ 																										\
			promise_type& promise = _macoro_frame_->promise;													\
			auto& awaiter = _macoro_frame_->constructAwaiter(promise.final_suspend(),(size_t)::macoro::SuspensionPoint::FinalSuspend);	\
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
		_macoro_frame_->getAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend).await_resume();	\
		_macoro_frame_->destroyAwaiter<AwaiterFor>((size_t)::macoro::SuspensionPoint::FinalSuspend);			\
		_macoro_frame_->destroy(_macoro_frame_);																\
		return noop_coroutine();																				\
	});																											\
																												\
	auto _macoro_ret_ = _macoro_frame_->promise.macoro_get_return_object();										\
	using promise_type = decltype(_macoro_frame_->promise);														\
	using Handle = ::macoro::coroutine_handle<promise_type>;													\
	promise_type& promise = _macoro_frame_->promise;															\
	auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);									\
	handle.resume();																							\
	return _macoro_ret_;																						\
} while (0)
