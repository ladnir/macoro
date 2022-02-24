///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include "macoro/detail/cancellation_registration.h"

#include "macoro/detail/cancellation_state.h"

#include <cassert>

macoro::cancellation_registration::~cancellation_registration()
{
	if (m_state != nullptr)
	{
		m_state->deregister_callback(this);
		m_state->release_token_ref();
	}
}

void macoro::cancellation_registration::register_callback(cancellation_token&& token)
{
	auto* state = token.m_state;
	if (state != nullptr && state->can_be_cancelled())
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
