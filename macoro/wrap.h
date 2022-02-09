#pragma once

#include "task.h"
#include "result.h"

namespace macoro
{
	template<typename T>
	task<result<T>> wrap(task<T>& t)
	{
		try
		{
			co_return Ok(co_await t);
		}
		catch (std::exception& e)
		{
			co_return Err(e);
		}
	}
	template<typename T>
	task<result<T>> wrap(task<T> t)
	{
		try
		{
			co_return Ok(co_await t);
		}
		catch (std::exception& e)
		{
			co_return Err(e);
		}
	}
}
