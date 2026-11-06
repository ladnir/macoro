
#pragma once
#include "macoro/work_item.h"

namespace macoro
{
	struct inline_scheduler
	{
		void post(work_item item) noexcept { item(); }
		void post_batch(const work_item* items, std::size_t count) noexcept
		{
			for (std::size_t i = 0; i < count; ++i) items[i]();
		}

		suspend_never schedule()
		{
			return { };
		}
	};
}
