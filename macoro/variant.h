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
	template<typename T>
	using variant = nonstd::variant<T>;
#else
	template<typename T>
	using variant = std::variant<T>;
#endif
}