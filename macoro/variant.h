#pragma once

#include "macoro/config.h"
#ifdef MACORO_VARIANT_LITE_V
#include "nonstd/variant.hpp"
#else
#include <variant>
#endif

namespace macoro
{
#ifdef MACORO_VARIANT_LITE_V
	template<typename... T>
	using variant = nonstd::variant<T...>;

	template<int I, typename VARIANT>
	decltype(auto) v_get(VARIANT&& v)
	{
		return nonstd::get<I>(std::forward<VARIANT>(v));
	}
#define MACORO_VARIANT_NAMESPACE nonstd

#else
	template<typename... T>
	using variant = std::variant<T...>;

	template<int I, typename VARIANT>
	decltype(auto) v_get(VARIANT&& v)
	{
		return std::get<I>(std::forward<VARIANT>(v));
	}
#define MACORO_VARIANT_NAMESPACE std

#endif
}