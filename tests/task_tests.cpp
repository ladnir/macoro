#include "macoro/task.h"
#include <iostream>
#include "macoro/sync_wait.h"
#include "macoro/stop.h"
#include <chrono>
#include <thread>
#include <memory>
#include "macoro/detail/operation_cancelled.h"

namespace
{
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
}

#define TEST(X) if(!(X)) throw MACORO_RTE_LOC

namespace macoro
{
	namespace tests
	{

		void task_int_test()
		{
#ifdef MACORO_CPP_20
			{
				task<int> t = taskInt20();

				auto awaiter = t.operator co_await();
				TEST(awaiter.await_ready() == false);

				auto st = awaiter.await_suspend(noop_coroutine());

				st.resume();

				auto v = awaiter.await_resume();
				TEST(v == 42);
			}
#endif
			{
				task<int> t = taskInt14();

				auto awaiter = t.MACORO_OPERATOR_COAWAIT();
				TEST(awaiter.await_ready() == false);

				auto st = awaiter.await_suspend(noop_coroutine());

				st.resume();

				auto v = awaiter.await_resume();
				TEST(v == 42);
			}
			//std::cout << "passed" << std::endl;
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
			//std::cout << "task_void_test      ";
#ifdef MACORO_CPP_20
			{
				taskVoid_called = false;
				task<void> t = taskVoid20();

				auto awaiter = t.operator co_await();
				TEST(awaiter.await_ready() == false);

				auto st = awaiter.await_suspend(noop_coroutine());

				st.resume();

				awaiter.await_resume();
				TEST(taskVoid_called);
			}
#endif
			{
				taskVoid_called = false;
				task<void> t = taskVoid14();

				auto awaiter = t.MACORO_OPERATOR_COAWAIT();
				TEST(awaiter.await_ready() == false);

				auto st = awaiter.await_suspend(noop_coroutine());

				st.resume();

				awaiter.await_resume();
				TEST(taskVoid_called);
			}

			//std::cout << "passed" << std::endl;
		}


		int taskRef_val = 42;
#ifdef MACORO_CPP_20
		task<int&> taskRef20()
		{
			//TEST(0);
			//throw std::runtime_error("");
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
			//std::cout << "task_ref_test       ";

#ifdef MACORO_CPP_20
			{
				taskRef_val = 42;
				task<int&> t = taskRef20();

				auto awaiter = t.operator co_await();
				TEST(awaiter.await_ready() == false);

				auto st = awaiter.await_suspend(noop_coroutine());

				st.resume();

				auto& v = awaiter.await_resume();
				TEST(v == 42);
				++taskRef_val;
				TEST(v == 43);
			}
#endif
			{
				taskRef_val = 42;
				task<int&> t = taskRef14();

				auto awaiter = t.MACORO_OPERATOR_COAWAIT();
				TEST(awaiter.await_ready() == false);

				auto st = awaiter.await_suspend(noop_coroutine());

				st.resume();

				auto& v = awaiter.await_resume();
				TEST(v == 42);
				++taskRef_val;
				TEST(v == 43);
			}
		}

		namespace {
			struct move_only
			{
				int v;
				move_only(int vv) :v(vv) { /*std::cout << "new " << this << std::endl;*/ };
				move_only(move_only&& vv) :v(vv.v) { /*std::cout << "move " << this << std::endl; */ };
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
			//std::cout << "task_move_test      ";

#ifdef MACORO_CPP_20
			{
				task<move_only> t = taskmove20();
				auto awaiter = t.operator co_await();
				TEST(awaiter.await_ready() == false);
				auto st = awaiter.await_suspend(noop_coroutine());
				st.resume();
				auto v = std::move(awaiter.await_resume());
				TEST(v.v == 42);
			}
			{
				task<move_only> t = taskmove20();

				auto awaiter = std::move(t).operator co_await();
				TEST(awaiter.await_ready() == false);
				auto st = awaiter.await_suspend(noop_coroutine());
				st.resume();
				auto v = awaiter.await_resume();
				TEST(v.v == 42);
			}
#endif
			{
				task<move_only> t = taskmove14();
				auto awaiter = t.MACORO_OPERATOR_COAWAIT();
				TEST(awaiter.await_ready() == false);
				auto st = awaiter.await_suspend(noop_coroutine());
				st.resume();
				auto v = std::move(awaiter.await_resume());
				TEST(v.v == 42);
			}
			{
				task<move_only> t = taskmove14();
				auto awaiter = std::move(t).MACORO_OPERATOR_COAWAIT();
				TEST(awaiter.await_ready() == false);
				auto st = awaiter.await_suspend(noop_coroutine());
				st.resume();
				auto v = awaiter.await_resume();
				TEST(v.v == 42);
			}

			//std::cout << "passed" << std::endl;
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
				TEST(awaiter.await_ready() == false);

				auto st = awaiter.await_suspend(noop_coroutine());
				st.resume();
				auto didThrow = false;
				try {
					awaiter.await_resume();
				}
				catch (std::runtime_error& re)
				{
					didThrow = true;
					TEST(re.what() == std::string("42"));
				}
				TEST(didThrow);
			}
#endif
			{


				task<int> t = taskThrows14();
				auto awaiter = t.MACORO_OPERATOR_COAWAIT();
				TEST(awaiter.await_ready() == false);

				auto st = awaiter.await_suspend(noop_coroutine());
				st.resume();
				auto didThrow = false;
				try {
					awaiter.await_resume();
				}
				catch (std::runtime_error& re)
				{
					didThrow = true;
					TEST(re.what() == std::string("42"));
				}
				TEST(didThrow);
			}
			//TEST(v.v == 42);


		}

		void task_blocking_int_test()
		{
			//std::cout << "task_blocking_int_test  ";
#ifdef MACORO_CPP_20
			{

				task<int> t = taskInt20();
				int i = sync_wait(t);
				TEST(i == 42);
			}
#endif
			{
				task<int> t = taskInt14();
				int i = sync_wait(t);
				TEST(i == 42);
			}
			//std::cout << "passed" << std::endl;
		}


		void task_blocking_void_test()
		{
			//std::cout << "task_blocking_void_test  ";
#ifdef MACORO_CPP_20
			{
				taskVoid_called = false;
				task<void> t = taskVoid20();
				sync_wait(t);
				TEST(taskVoid_called);
			}
#endif
			{
				taskVoid_called = false;
				task<void> t = taskVoid14();
				sync_wait(t);
				TEST(taskVoid_called);
			}
			//std::cout << "passed" << std::endl;
		}

		void task_blocking_ref_test()
		{
			//std::cout << "task_blocking_ref_test  ";
#ifdef MACORO_CPP_20
			{
				taskRef_val = 42;
				task<int&> t = taskRef20();
				int& i = sync_wait(t);
				TEST(i == 42);
				++taskRef_val;
				TEST(i == 43);
			}
#endif
			{
				taskRef_val = 42;
				task<int&> t = taskRef14();
				int& i = sync_wait(t);
				TEST(i == 42);
				++taskRef_val;
				TEST(i == 43);
			}
			//std::cout << "passed" << std::endl;
		}



		void task_blocking_move_test()
		{
			//std::cout << "task_blocking_move_test  ";

#ifdef MACORO_CPP_20
			{
				task<move_only> t = taskmove20();
				auto v = std::move(sync_wait(t));
				TEST(v.v == 42);
			}
			{
				auto v = sync_wait(taskmove20());
				TEST(v.v == 42);
			}
#endif
			{
				task<move_only> t = taskmove14();
				auto v = std::move(sync_wait(t));
				TEST(v.v == 42);
			}
			{
				// task<move_only>&& f();
				auto v = sync_wait(taskmove14());
				TEST(v.v == 42);
			}
			//std::cout << "passed" << std::endl;
		}


		void task_blocking_ex_test()
		{
			//std::cout << "task_blocking_ex_test  ";
#ifdef MACORO_CPP_20
			{
				task<int> t = taskThrows20();
				bool didThrow = false;
				try {
					sync_wait(t);
				}
				catch (std::runtime_error& re)
				{
					didThrow = true;
					TEST(re.what() == std::string("42"));
				}
				TEST(didThrow);
			}
#endif
			{
				task<int> t = taskThrows14();
				bool didThrow = false;
				try {
					sync_wait(t);
				}
				catch (std::runtime_error& re)
				{
					didThrow = true;
					TEST(re.what() == std::string("42"));
				}
				TEST(didThrow);
			}
			//std::cout << "passed" << std::endl;
		}
		void task_blocking_cancel_test()
		{
			//std::cout << "task_blocking_cancel_test  ";



			auto t = [](stop_token t) -> task<void>
				{
					MC_BEGIN(task<>, t);
					while (true)
					{
						if (t.stop_requested())
							throw operation_cancelled();
					}

					MC_RETURN_VOID();
					MC_END();
				};


			stop_source mSrc;
			std::thread thrd = std::thread([&] {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				mSrc.request_stop();
				});

			try {
				sync_wait(t(mSrc.get_token()));
			}
			catch (operation_cancelled& e)
			{
			}
			catch (...)
			{
				thrd.join();
			}
			thrd.join();

			//std::cout << "passed" << std::endl;
		}
	}

}