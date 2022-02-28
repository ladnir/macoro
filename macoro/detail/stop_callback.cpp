///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include "macoro/detail/stop_callback.h"

#include "macoro/detail/stop_state.h"

#include <cassert>

macoro::stop_callback::~stop_callback()
{
	if (m_state != nullptr)
	{
		m_state->deregister_callback(this);
		m_state->release_token_ref();
	}
}

void macoro::stop_callback::register_callback(stop_token&& token)
{
	auto* state = token.m_state;
	if (state != nullptr && state->stop_possible())
	{
		m_state = state;
		if (state->try_register_callback(this))
		{
			token.m_state = nullptr;
		}
		else
		{
			m_state = nullptr;
			m_callback();
		}
	}
	else
	{
		m_state = nullptr;
	}
}
