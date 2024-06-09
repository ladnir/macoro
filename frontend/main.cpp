//#include "Context.h"
//#include "macros.h"
#include "macoro/type_traits.h"
#include "macoro/macros.h"
#include "macoro/optional.h"
#include <iostream>
#include <type_traits>
#include <chrono>
#include <numeric>
#include <fstream>
#include "tests/tests.h"
#include <mutex>

using namespace macoro;


int main(int argc, char** argv)
{
	std::mutex m_mutex;
	{
		m_mutex.lock();
		std::cout << "pre " << std::endl;
		m_mutex.unlock();

	}
	macoro::CLP cmd(argc, argv);
	macoro::testCollection.runIf(cmd);
	return 0;
}

