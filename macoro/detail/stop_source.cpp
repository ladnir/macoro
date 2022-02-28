///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include "macoro/detail/stop_source.h"
#include "macoro/detail/stop_state.h"

#include <cassert>

macoro::stop_source::stop_source()
	: m_state(detail::stop_state::create())
{
}

macoro::stop_source::stop_source(const stop_source& other) noexcept
	: m_state(other.m_state)
{
	if (m_state != nullptr)
	{
		m_state->add_source_ref();
	}
}

macoro::stop_source::stop_source(stop_source&& other) noexcept
	: m_state(other.m_state)
{
	other.m_state = nullptr;
}

macoro::stop_source::~stop_source()
{
	if (m_state != nullptr)
	{
		m_state->release_source_ref();
	}
}

macoro::stop_source& macoro::stop_source::operator=(const stop_source& other) noexcept
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

macoro::stop_source& macoro::stop_source::operator=(stop_source&& other) noexcept
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

bool macoro::stop_source::stop_possible() const noexcept
{
	return m_state != nullptr;
}

macoro::stop_token macoro::stop_source::get_token() const noexcept
{
	return stop_token(m_state);
}

void macoro::stop_source::request_stop()
{
	if (m_state != nullptr)
	{
		m_state->request_stop();
	}
}

bool macoro::stop_source::stop_requested() const noexcept
{
	return m_state != nullptr && m_state->stop_requested();
}
