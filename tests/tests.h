
#pragma once
#include "task_tests.h"
#include "await_lifetime_tests.h"
namespace macoro
{
	inline void tests()
	{
		task_tests();
		await_lifetime_tests();


		std::cout << "===================  " << std::endl;
	}



}
