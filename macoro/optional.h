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
	using nonstd::optional;
	using nonstd::nullopt;
#else
	using std::optional;
	using std::nullopt;
#endif
}