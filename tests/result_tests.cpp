#include "result_tests.h"

#include "macoro/result.h"
#include "macoro/wrap.h"
#include "macoro/sync_wait.h"
namespace macoro 
{

	void result_basic_store_test()
	{
		std::cout << "result_basic_store_test     " << std::endl;
		result<int> r;
		r = Ok(42);

		assert(r.has_value());
		assert(r.value() == 42);
		try {
			r.error();
		}
		catch (std::exception& e)
		{
			r = Err(e);
		}

		assert(r.has_error());
		std::cout << "   passed" << std::endl;
	}


	void result_co_await_test()
	{
		std::cout << "result_co_await_test     " << std::endl;
		result<int> r;

		auto foo = []()->result<int>
		{
			result<bool> b;

			bool bb = co_await b;

			co_return bb ? 1 : 42;
		};

		r = foo();

		assert(r.has_value());
		assert(r.value() == 42);
		try {
			r.error();
		}
		catch (std::exception& e)
		{
			r = Err(e);
		}

		assert(r.has_error());
		std::cout << "   passed" << std::endl;
	}

	void result_task_wrap_test()
	{
		std::cout << "result_task_wrap_test     " << std::endl;

		auto foo = []()->task<int>
		{
			result<bool> b;
			std::exception bb = b.error();
			co_return 42;
		};

		;
		result<int> r = sync_wait(wrap(foo()));
		assert(r.has_error());
		std::cout << "   passed" << std::endl;
	}

	void result_tests()
	{
		std::cout << "-------- result ----------" << std::endl;
		result_basic_store_test();
		result_co_await_test();
		result_task_wrap_test();
	}

}
