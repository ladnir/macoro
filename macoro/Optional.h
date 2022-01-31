#pragma once

#include "macoro/config.h"
#ifdef MACORO_OPTIONAL_LITE_V
#include "nonstd/optional.hpp"
#else
#include <optional>
#endif

namespace macoro
{
#ifdef MACORO_OPTIONAL_LITE_V
	template<typename T>
	using optional = nonstd::optional<T>;
#else
	template<typename T>
	using optional = std::optional<T>;
#endif
}