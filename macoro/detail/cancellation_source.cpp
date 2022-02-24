///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include "macoro/detail/cancellation_source.h"
#include "macoro/detail/cancellation_state.h"

#include <cassert>

macoro::cancellation_source::cancellation_source()
	: m_state(detail::cancellation_state::create())
{
}

macoro::cancellation_source::cancellation_source(const cancellation_source& other) noexcept
	: m_state(other.m_state)
{
	if (m_state != nullptr)
	{
		m_state->add_source_ref();
	}
}

macoro::cancellation_source::cancellation_source(cancellation_source&& other) noexcept
	: m_state(other.m_state)
{
	other.m_state = nullptr;
}

macoro::cancellation_source::~cancellation_source()
{
	if (m_state != nullptr)
	{
		m_state->release_source_ref();
	}
}

macoro::cancellation_source& macoro::cancellation_source::operator=(const cancellation_source& other) noexcept
{
	if (m_state != other.m_state)
	{
		if (m_state != nullptr)
		{
			m_state->release_source_ref();
		}

		m_state = other.m_state;

		if (m_state != nullptr)
		{
			m_state->add_source_ref();
		}
	}

	return *this;
}

macoro::cancellation_source& macoro::cancellation_source::operator=(cancellation_source&& other) noexcept
{
	if (this != &other)
	{
		if (m_state != nullptr)
		{
			m_state->release_source_ref();
		}

		m_state = other.m_state;
		other.m_state = nullptr;
	}

	return *this;
}

bool macoro::cancellation_source::can_be_cancelled() const noexcept
{
	return m_state != nullptr;
}

macoro::cancellation_token macoro::cancellation_source::token() const noexcept
{
	return cancellation_token(m_state);
}

void macoro::cancellation_source::request_cancellation()
{
	if (m_state != nullptr)
	{
		m_state->request_cancellation();
	}
}

bool macoro::cancellation_source::is_cancellation_requested() const noexcept
{
	return m_state != nullptr && m_state->is_cancellation_requested();
}
