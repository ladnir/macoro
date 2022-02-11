#pragma once
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include "macoro/type_traits.h"
#include <tuple>
#include <atomic>
#include "macoro/coroutine_handle.h"
#include "macoro/impl/when_all_awaitable.h"
#include "macoro/impl/when_all_task.h"

namespace macoro
{

	template<typename... Awaitables,
		enable_if_t<
			std::conjunction<
				is_awaitable<
					remove_reference_and_wrapper_t<Awaitables>
				>...
			>::value, int> = 0
		>
	MACORO_NODISCARD
	auto when_all_ready(Awaitables&&... awaitables)
	{
		return impl::when_all_ready_awaitable<
				std::tuple<
					impl::when_all_task<
						awaitable_result_t<
							remove_reference_and_wrapper_t<Awaitables>
						>
					>...
				>
			>
			(std::make_tuple(
				impl::make_when_all_task(
					std::forward<Awaitables>(awaitables)
				)...
			)
		);
	}



	// TODO: Generalise this from vector<AWAITABLE> to arbitrary sequence of awaitable.

	template<
		typename AWAITABLE,
		typename RESULT = awaitable_result_t<remove_reference_wrapper_t<AWAITABLE>>>
	MACORO_NODISCARD
	auto when_all_ready(std::vector<AWAITABLE> awaitables)
	{
		std::vector<impl::when_all_task<RESULT>> tasks;

		tasks.reserve(awaitables.size());

		for (auto& awaitable : awaitables)
		{
			tasks.emplace_back(impl::make_when_all_task(std::move(awaitable)));
		}

		return impl::when_all_ready_awaitable<std::vector<impl::when_all_task<RESULT>>>(
			std::move(tasks));
	}
}