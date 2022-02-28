///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include "stop_token.h"
#include "operation_cancelled.h"

#include "stop_state.h"

#include <utility>
#include <cassert>

macoro::stop_token::stop_token() noexcept
	: m_state(nullptr)
{
}

macoro::stop_token::stop_token(const stop_token& other) noexcept
	: m_state(other.m_state)
{
	if (m_state != nullptr)
	{
		m_state->add_token_ref();
	}
}

macoro::stop_token::stop_token(stop_token&& other) noexcept
	: m_state(other.m_state)
{
	other.m_state = nullptr;
}

macoro::stop_token::~stop_token()
{
	if (m_state != nullptr)
	{
		m_state->release_token_ref();
	}
}

macoro::stop_token& macoro::stop_token::operator=(const stop_token& other) noexcept
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

macoro::stop_token& macoro::stop_token::operator=(stop_token&& other) noexcept
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

void macoro::stop_token::swap(stop_token& other) noexcept
{
	std::swap(m_state, other.m_state);
}

bool macoro::stop_token::stop_possible() const noexcept
{
	return m_state != nullptr && m_state->stop_possible();
}

bool macoro::stop_token::stop_requested() const noexcept
{
	return m_state != nullptr && m_state->stop_requested();
}

void macoro::stop_token::throw_if_stop_requested() const
{
	if (stop_requested())
	{
		throw operation_cancelled{};
	}
}

macoro::stop_token::stop_token(detail::stop_state* state) noexcept
	: m_state(state)
{
	if (m_state != nullptr)
	{
		m_state->add_token_ref();
	}
}

