
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
	return macoro::testCollection.runIf(cmd) == macoro::TestCollection::Result::failed ? 1 : 0;
}

