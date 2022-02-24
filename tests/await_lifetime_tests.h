#pragma once

namespace macoro
{

	namespace tests
	{

		void store_as_t_lifetime_test();
		void await_lifetime_refOnly_test();
		void await_lifetime_NoCopyMove_test();
		void await_lifetime_MovOnly_test();
		void await_lifetime_fn_test();
		void yield_await_lifetime_test20();
		void yield_await_lifetime_test14();
	}



}