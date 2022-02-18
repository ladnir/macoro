///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include "cancellation_token.h"
#include "operation_cancelled.h"

#include "cancellation_state.h"

#include <utility>
#include <cassert>

macoro::cancellation_token::cancellation_token() noexcept
	: m_state(nullptr)
{
}

macoro::cancellation_token::cancellation_token(const cancellation_token& other) noexcept
	: m_state(other.m_state)
{
	if (m_state != nullptr)
	{
		m_state->add_token_ref();
	}
}

macoro::cancellation_token::cancellation_token(cancellation_token&& other) noexcept
	: m_state(other.m_state)
{
	other.m_state = nullptr;
}

macoro::cancellation_token::~cancellation_token()
{
	if (m_state != nullptr)
	{
		m_state->release_token_ref();
	}
}

macoro::cancellation_token& macoro::cancellation_token::operator=(const cancellation_token& other) noexcept
{
	if (other.m_state != m_state)
	{
		if (m_state != nullptr)
		{
			m_state->release_token_ref();
		}

		m_state = other.m_state;

		if (m_state != nullptr)
		{
			m_state->add_token_ref();
		}
	}

	return *this;
}

macoro::cancellation_token& macoro::cancellation_token::operator=(cancellation_token&& other) noexcept
{
	if (this != &other)
	{
		if (m_state != nullptr)
		{
			m_state->release_token_ref();
		}

		m_state = other.m_state;
		other.m_state = nullptr;
	}

	return *this;
}

void macoro::cancellation_token::swap(cancellation_token& other) noexcept
{
	std::swap(m_state, other.m_state);
}

bool macoro::cancellation_token::can_be_cancelled() const noexcept
{
	return m_state != nullptr && m_state->can_be_cancelled();
}

bool macoro::cancellation_token::is_cancellation_requested() const noexcept
{
	return m_state != nullptr && m_state->is_cancellation_requested();
}

void macoro::cancellation_token::throw_if_cancellation_requested() const
{
	if (is_cancellation_requested())
	{
		throw operation_cancelled{};
	}
}

macoro::cancellation_token::cancellation_token(impl::cancellation_state* state) noexcept
	: m_state(state)
{
	if (m_state != nullptr)
	{
		m_state->add_token_ref();
	}
}
