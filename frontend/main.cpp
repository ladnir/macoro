
#include "macoro/type_traits.h"
#include "macoro/macros.h"
#include "macoro/optional.h"
#include <iostream>
#include <type_traits>
#include <chrono>
#include <numeric>
#include <fstream>
#include "tests/tests.h"

using namespace macoro;


int main(int argc, char** argv)
{
	macoro::CLP cmd(argc, argv);
	macoro::testCollection.runIf(cmd);
	return 0;
}

