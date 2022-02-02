#include "macoro/task.h"
#include <iostream>
#include "macoro/blocking.h"

namespace macoro
{
	task<int> taskInt()
	{
		co_return 42;
	}
	void task_int_test()
	{
		std::cout << "task_int_test       ";

		task<int> t = taskInt();

		auto awaiter = t.operator co_await();
		assert(awaiter.await_ready() == false);

		auto st = awaiter.await_suspend(noop_coroutine());

		st.resume();

		auto v = awaiter.await_resume();
		assert(v == 42);


		std::cout << "passed" << std::endl;
	}

	bool taskVoid_called = false;
	task<void> taskVoid()
	{
		taskVoid_called = true;
		co_return;
	}
	void task_void_test()
	{
		std::cout << "task_void_test      ";
		taskVoid_called = false;
		task<void> t = taskVoid();

		auto awaiter = t.operator co_await();
		assert(awaiter.await_ready() == false);

		auto st = awaiter.await_suspend(noop_coroutine());

		st.resume();

		awaiter.await_resume();
		assert(taskVoid_called);

		std::cout << "passed" << std::endl;
	}
	int taskRef_val = 42;
	task<int&> taskRef()
	{
		co_return taskRef_val;
	}
	void task_ref_test()
	{
		std::cout << "task_ref_test       ";
		taskRef_val = 42;
		task<int&> t = taskRef();

		auto awaiter = t.operator co_await();
		assert(awaiter.await_ready() == false);

		auto st = awaiter.await_suspend(noop_coroutine());

		st.resume();

		auto& v = awaiter.await_resume();
		assert(v == 42);
		++taskRef_val;
		assert(v == 43);


		std::cout << "passed" << std::endl;
	}
	namespace {
		struct move_only
		{
			int v;
			move_only(int vv) :v(vv) { std::cout << "new " << this << std::endl; };
			move_only(move_only&&vv):v(vv.v) { std::cout << "move " << this << std::endl; };
			move_only(const move_only&) = delete;

			~move_only() { std::cout << "destroy " << this << std::endl; }
		};
	}

	task<move_only> taskmove()
	{
		co_return 42;
	}
	void task_move_test()
	{
		std::cout << "task_move_test      ";

		{
			task<move_only> t = taskmove();
			auto awaiter = t.operator co_await();
			assert(awaiter.await_ready() == false);

			auto st = awaiter.await_suspend(noop_coroutine());
			st.resume();
			auto v = std::move(awaiter.await_resume());
			assert(v.v == 42);
		}
		{
			task<move_only> t = taskmove();

			auto awaiter = std::move(t).operator co_await();
			assert(awaiter.await_ready() == false);

			auto st = awaiter.await_suspend(noop_coroutine());
			st.resume();
			auto v = awaiter.await_resume();
			assert(v.v == 42);
		}

		std::cout << "passed" << std::endl;
	}

	task<int> taskThrows()
	{

		throw std::runtime_error("42");
		co_return 42;
	}


	void task_ex_test()
	{

		task<int> t = taskThrows();
		auto awaiter = t.operator co_await();
		assert(awaiter.await_ready() == false);

		auto st = awaiter.await_suspend(noop_coroutine());
		st.resume();
		auto didThrow = false;
		try {
			auto v = awaiter.await_resume();

		}
		catch (std::runtime_error& re)
		{
			didThrow = true;
			assert(re.what() == std::string("42"));
		}
		assert(didThrow);
		//assert(v.v == 42);
		

	}

	void task_blocking_int_test()
	{
		std::cout << "task_blocking_int_test  ";
		task<int> t = taskInt();
		int i = blocking_get(t);
		assert(i == 42);
		std::cout << "passed" << std::endl;
	}


	void task_blocking_void_test()
	{
		std::cout << "task_blocking_void_test  ";
		taskVoid_called = false;
		task<void> t = taskVoid();
		blocking_get(t);
		assert(taskVoid_called);
		std::cout << "passed" << std::endl;
	}

	void task_blocking_ref_test()
	{
		std::cout << "task_blocking_ref_test  ";
		taskRef_val = 42;
		task<int&> t = taskRef();
		int& i = blocking_get(t);
		assert(i == 42);
		++taskRef_val;
		assert(i == 43);
		std::cout << "passed" << std::endl;
	}



	void task_blocking_move_test()
	{
		std::cout << "task_blocking_move_test  ";

		{
			task<move_only> t = taskmove();
			auto v = std::move(blocking_get(t));
			assert(v.v == 42);
		}
		{
			task<move_only>&& f();
			//using tt = decltype(f());

			auto v = blocking_get(taskmove());
			assert(v.v == 42);
		}
		std::cout << "passed" << std::endl;
	}


	void task_blocking_ex_test()
	{
		std::cout << "task_blocking_ex_test  ";
		task<int> t = taskThrows();
		bool didThrow = false;
		try{
			auto v = blocking_get(t);
		}
		catch (std::runtime_error& re)
		{
			didThrow = true;
			assert(re.what() == std::string("42"));
		}
		assert(didThrow);
		std::cout << "passed" << std::endl;
	}


	void task_tests()
	{
		std::cout << "----------- task -----------" << std::endl;
		//task_int_test();
		//task_void_test();
		//task_ref_test();
		//task_move_test();
		//task_ex_test();

		//task_blocking_int_test();
		//task_blocking_void_test();
		//task_blocking_ref_test();
		task_blocking_move_test();
		//task_blocking_ex_test();

	}


}
