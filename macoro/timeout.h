#pragma once

#include "cancellation.h"
#include "task.h"
#include <chrono>
#include "type_traits.h"
#include "result.h"
#include "macoro/macros.h"

namespace macoro
{

	struct timeout
	{
		optional<cancellation_source> src;
		eager_task<> t;

		timeout() = default;
		timeout(const timeout&) = delete;
		timeout(timeout&& o) : src(o.src), t(std::move(o.t)) {}
		timeout& operator=(const timeout&) = delete;
		timeout& operator=(timeout&& o) { src = o.src; t = std::move(o.t); return *this; };


		template<typename Scheduler, typename Rep, typename Per>
		timeout(Scheduler& s,
			std::chrono::duration<Rep, Per> d)
		{
			reset(s, d);
		}


		template<typename Scheduler, typename Rep, typename Per>
		void reset(Scheduler& s,
			std::chrono::duration<Rep, Per> d)
		{
			src.emplace();
			t = make_task(s, src.value(), d);
		}

		void request_cancellation() {
			src.value().request_cancellation();
		}

		cancellation_token token() { return src.value().token(); }

		cancellation_source source() { return src.value(); }

		auto MACORO_OPERATOR_COAWAIT()
		{
			return t.MACORO_OPERATOR_COAWAIT();
		}

	private:

		template<typename Scheduler, typename Rep, typename Per>
		static eager_task<> make_task(Scheduler& s,
			cancellation_source src,
			std::chrono::duration<Rep, Per> d)
		{
			MC_BEGIN(eager_task<>, &s, src = std::move(src), d,
				r = result<void>{});

			MC_AWAIT_TRY(r, s.schedule_after(d, src.token()));
			src.request_cancellation();
			MC_END();

		}
	};


}
