#pragma once
#include "cancellation_token.h"
#include "cancellation_registration.h"
#include "macoro/coroutine_handle.h"

namespace macoro
{


	class cancellation_awaiter
	{
	public:
		cancellation_awaiter(cancellation_token&& t)
			:mtoken(std::move(t))
		{}
		cancellation_token mtoken;
		optional<cancellation_registration> mReg;
		bool await_ready() { return false; }
		std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
		{
			return await_suspend(coroutine_handle<>(h)).std_cast();
		}

		coroutine_handle<> await_suspend(coroutine_handle<> h)
		{
			if (mtoken.can_be_cancelled() && ! mtoken.is_cancellation_requested())
			{
				mReg.emplace(mtoken, [h] {
					h.resume();
					});
				return noop_coroutine();
			}
			return h;
		}
		void await_resume() {}
	};


	inline cancellation_awaiter operator co_await(cancellation_token t)
	{
		return { std::move(t) };
	}
}