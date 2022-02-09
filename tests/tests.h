
#pragma once
#include "task_tests.h"
#include "eager_task_tests.h"
#include "await_lifetime_tests.h"
#include "result_tests.h"
namespace macoro
{
	inline void tests()
	{
		result_tests();
		task_tests();
		eager_task_tests();
		await_lifetime_tests();

		std::cout << "===================  " << std::endl;
	}



}
