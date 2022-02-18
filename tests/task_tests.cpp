#include "macoro/task.h"
#include <iostream>
#include "macoro/sync_wait.h"
#include "macoro/cancellation.h"

#ifdef MACORO_CPP_20
macoro::task<int> taskInt20()
{
	co_return 42;
}
#endif
macoro::task<int> taskInt14()
{
	MC_BEGIN(macoro::task<int>);
	MC_RETURN(42);
	MC_END();
}

namespace macoro
{
	void task_int_test()
	{
		std::cout << "task_int_test       ";
#ifdef MACORO_CPP_20
		{
			task<int> t = taskInt20();

			auto awaiter = t.operator co_await();
			assert(awaiter.await_ready() == false);

			auto st = awaiter.await_suspend(noop_coroutine());

			st.resume();

			auto v = awaiter.await_resume();
			assert(v == 42);
		}
#endif
		{
			task<int> t = taskInt14();

			auto awaiter = t.operator_co_await();
			assert(awaiter.await_ready() == false);

			auto st = awaiter.await_suspend(noop_coroutine());

			st.resume();

			auto v = awaiter.await_resume();
			assert(v == 42);
		}
		std::cout << "passed" << std::endl;
	}

	bool taskVoid_called = false;
#ifdef MACORO_CPP_20
	task<void> taskVoid20()
	{
		taskVoid_called = true;
		co_return;
	}
#endif
	task<void> taskVoid14()
	{
		MC_BEGIN(task<void>);
		taskVoid_called = true;
		MC_RETURN_VOID();
		MC_END();
	}
	void task_void_test()
	{
		std::cout << "task_void_test      ";
#ifdef MACORO_CPP_20
		{
			taskVoid_called = false;
			task<void> t = taskVoid20();

			auto awaiter = t.operator co_await();
			assert(awaiter.await_ready() == false);

			auto st = awaiter.await_suspend(noop_coroutine());

			st.resume();

			awaiter.await_resume();
			assert(taskVoid_called);
		}
#endif
		{
			taskVoid_called = false;
			task<void> t = taskVoid14();

			auto awaiter = t.operator_co_await();
			assert(awaiter.await_ready() == false);

			auto st = awaiter.await_suspend(noop_coroutine());

			st.resume();

			awaiter.await_resume();
			assert(taskVoid_called);
		}

		std::cout << "passed" << std::endl;
	}


	int taskRef_val = 42;
#ifdef MACORO_CPP_20
	task<int&> taskRef20()
	{
		co_return taskRef_val;
	}
#endif
	task<int&> taskRef14()
	{
		MC_BEGIN(task<int&>);
		MC_RETURN(taskRef_val);
		MC_END();
	}
	void task_ref_test()
	{
		std::cout << "task_ref_test       ";

#ifdef MACORO_CPP_20
		{
			taskRef_val = 42;
			task<int&> t = taskRef20();

			auto awaiter = t.operator co_await();
			assert(awaiter.await_ready() == false);

			auto st = awaiter.await_suspend(noop_coroutine());

			st.resume();

			auto& v = awaiter.await_resume();
			assert(v == 42);
			++taskRef_val;
			assert(v == 43);
		}
#endif
		{
			taskRef_val = 42;
			task<int&> t = taskRef14();

			auto awaiter = t.operator_co_await();
			assert(awaiter.await_ready() == false);

			auto st = awaiter.await_suspend(noop_coroutine());

			st.resume();

			auto& v = awaiter.await_resume();
			assert(v == 42);
			++taskRef_val;
			assert(v == 43);
		}


		std::cout << "passed" << std::endl;
	}

	namespace {
		struct move_only
		{
			int v;
			move_only(int vv) :v(vv) { /*std::cout << "new " << this << std::endl;*/ };
			move_only(move_only&&vv):v(vv.v) { /*std::cout << "move " << this << std::endl; */};
			move_only(const move_only&) = delete;

			~move_only() { /*std::cout << "destroy " << this << std::endl;*/ }
		};
	}

#ifdef MACORO_CPP_20
	task<move_only> taskmove20()
	{
		co_return 42;
	}
#endif
	task<move_only> taskmove14()
	{
		MC_BEGIN(task<move_only>);
		MC_RETURN(42);
		MC_END();
	}
	void task_move_test()
	{
		std::cout << "task_move_test      ";

#ifdef MACORO_CPP_20
		{
			task<move_only> t = taskmove20();
			auto awaiter = t.operator co_await();
			assert(awaiter.await_ready() == false);
			auto st = awaiter.await_suspend(noop_coroutine());
			st.resume();
			auto v = std::move(awaiter.await_resume());
			assert(v.v == 42);
		}
		{
			task<move_only> t = taskmove20();

			auto awaiter = std::move(t).operator co_await();
			assert(awaiter.await_ready() == false);
			auto st = awaiter.await_suspend(noop_coroutine());
			st.resume();
			auto v = awaiter.await_resume();
			assert(v.v == 42);
		}
#endif
		{
			task<move_only> t = taskmove14();
			auto awaiter = t.operator_co_await();
			assert(awaiter.await_ready() == false);
			auto st = awaiter.await_suspend(noop_coroutine());
			st.resume();
			auto v = std::move(awaiter.await_resume());
			assert(v.v == 42);
		}
		{
			task<move_only> t = taskmove14();
			auto awaiter = std::move(t).operator_co_await();
			assert(awaiter.await_ready() == false);
			auto st = awaiter.await_suspend(noop_coroutine());
			st.resume();
			auto v = awaiter.await_resume();
			assert(v.v == 42);
		}

		std::cout << "passed" << std::endl;
	}

#ifdef MACORO_CPP_20
	task<int> taskThrows20()
	{

		throw std::runtime_error("42");
		co_return 42;
	}
#endif

	task<int> taskThrows14()
	{
		MC_BEGIN(task<int>);
		throw std::runtime_error("42");
		MC_RETURN(42);
		MC_END();
	}


	void task_ex_test()
	{


#ifdef MACORO_CPP_20
		{
			task<int> t = taskThrows20();
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
		}
#endif
		{


			task<int> t = taskThrows14();
			auto awaiter = t.operator_co_await();
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
		}
		//assert(v.v == 42);
		

	}

	void task_blocking_int_test()
	{
		std::cout << "task_blocking_int_test  ";
#ifdef MACORO_CPP_20
		{
			task<int> t = taskInt20();
			int i = sync_wait(t);
			assert(i == 42);
		}
#endif
		{
			task<int> t = taskInt14();
			int i = sync_wait(t);
			assert(i == 42);
		}
		std::cout << "passed" << std::endl;
	}


	void task_blocking_void_test()
	{
		std::cout << "task_blocking_void_test  ";
#ifdef MACORO_CPP_20
		{
			taskVoid_called = false;
			task<void> t = taskVoid20();
			sync_wait(t);
			assert(taskVoid_called);
		}
#endif
		{
			taskVoid_called = false;
			task<void> t = taskVoid14();
			sync_wait(t);
			assert(taskVoid_called);
		}
		std::cout << "passed" << std::endl;
	}

	void task_blocking_ref_test()
	{
		std::cout << "task_blocking_ref_test  ";
#ifdef MACORO_CPP_20
		{
			taskRef_val = 42;
			task<int&> t = taskRef20();
			int& i = sync_wait(t);
			assert(i == 42);
			++taskRef_val;
			assert(i == 43);
		}
#endif
		{
			taskRef_val = 42;
			task<int&> t = taskRef14();
			int& i = sync_wait(t);
			assert(i == 42);
			++taskRef_val;
			assert(i == 43);
		}
		std::cout << "passed" << std::endl;
	}



	void task_blocking_move_test()
	{
		std::cout << "task_blocking_move_test  ";

#ifdef MACORO_CPP_20
		{
			task<move_only> t = taskmove20();
			auto v = std::move(sync_wait(t));
			assert(v.v == 42);
		}
		{
			task<move_only>&& f();
			auto v = sync_wait(taskmove20());
			assert(v.v == 42);
		}
#endif
		{
			task<move_only> t = taskmove14();
			auto v = std::move(sync_wait(t));
			assert(v.v == 42);
		}
		{
			task<move_only>&& f();
			auto v = sync_wait(taskmove14());
			assert(v.v == 42);
		}
		std::cout << "passed" << std::endl;
	}


	void task_blocking_ex_test()
	{
		std::cout << "task_blocking_ex_test  ";
#ifdef MACORO_CPP_20
		{
			task<int> t = taskThrows20();
			bool didThrow = false;
			try {
				auto v = sync_wait(t);
			}
			catch (std::runtime_error& re)
			{
				didThrow = true;
				assert(re.what() == std::string("42"));
			}
			assert(didThrow);
		}
#endif
		{
			task<int> t = taskThrows14();
			bool didThrow = false;
			try {
				auto v = sync_wait(t);
			}
			catch (std::runtime_error& re)
			{
				didThrow = true;
				assert(re.what() == std::string("42"));
			}
			assert(didThrow);
		}
		std::cout << "passed" << std::endl;
	}
	void task_blocking_cancel_test()
	{
		std::cout << "task_blocking_cancel_test  ";



		auto t = [](cancellation_token t) -> task<void>
		{
			while (true)
			{
				if (t.is_cancellation_requested())
					throw operation_cancelled();
			}

			co_return;

		};


		cancellation_source src;
		std::thread thrd = std::thread([&] {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			src.request_cancellation();
			});
		
		try {
			sync_wait(t(src.token()));
		}
		catch(operation_cancelled&e)
		{ }
		catch (...)
		{
			thrd.join();
		}
		thrd.join();

		std::cout << "passed" << std::endl;
	}

	void task_tests()
	{
		std::cout << "----------- task -----------" << std::endl;
		task_int_test();
		task_void_test();
		task_ref_test();
		task_move_test();
		task_ex_test();

		task_blocking_int_test();
		task_blocking_void_test();
		task_blocking_ref_test();
		task_blocking_move_test();
		task_blocking_ex_test();

		task_blocking_cancel_test();

	}


}
