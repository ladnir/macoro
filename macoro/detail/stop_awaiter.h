#pragma once
#include "stop_token.h"
#include "stop_callback.h"
#include "macoro/coroutine_handle.h"

namespace macoro
{


	class stop_awaiter
	{
	public:
		stop_awaiter(stop_token&& t)
			:mtoken(std::move(t))
		{}
		stop_awaiter() = default;
		stop_awaiter(const stop_awaiter&) = delete;
#ifdef MACORO_CPP20
		stop_awaiter(stop_awaiter&&) = delete;
#else
		stop_awaiter(stop_awaiter&&) = default;
#endif

		stop_token mtoken;
		optional_stop_callback mReg;
		bool await_ready() { return false; }

#ifdef MACORO_CPP20
		std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
		{
			return await_suspend(coroutine_handle<>(h)).std_cast();
		}
#endif
		coroutine_handle<> await_suspend(coroutine_handle<> h)
		{
			if (mtoken.stop_possible() && ! mtoken.stop_requested())
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


	inline stop_awaiter MACORO_OPERATOR_COAWAIT(stop_token t)
	{
		return std::move(t);
	}
}