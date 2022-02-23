#include "eager_task_tests.h"
#include "macoro/start_on.h"
#include "macoro/sync_wait.h"
#include "macoro/thread_pool.h"
#include "macoro/when_all.h"
namespace macoro
{
	void make_eager_test()
	{
		std::cout << "make_eager_test         ";
		//assert(0);
		auto foo = []() -> task<int>
		{
			MC_BEGIN(task<int>);
			MC_RETURN(42);
			MC_END();
		};

		auto t = foo();

		auto e = make_eager_task(std::move(t));
		auto i = sync_wait(e);
		assert(i == 42);

		std::cout << "   passed" << std::endl;
	}

	void thread_pool_post_test()
	{
		std::cout << "thread_pool_post_test         ";

		thread_pool tp;

		auto foo = [scheduler = tp.schedule()]()->task<int>
		{
			MC_BEGIN(task<int>, scheduler);
			MC_AWAIT(scheduler);
			MC_RETURN(42);
			MC_END();
		};

		auto t = foo();

		auto e = make_eager_task(std::move(t));

		tp.run();

		auto i = sync_wait(e);
		assert(i == 42);

		std::cout << "   passed" << std::endl;
	}
	void thread_pool_dispatch_test()
	{
		std::cout << "thread_pool_dispatch_test         ";

		thread_pool tp;

		auto foo = [scheduler = tp.dispatch()]()->task<int>
		{
			MC_BEGIN(task<int>, scheduler);
			MC_AWAIT(scheduler);
			MC_RETURN(42);
			MC_END();
		};

		auto t = foo();

		auto e = make_eager_task(std::move(t));

		tp.run();

		auto i = sync_wait(e);
		assert(i == 42);

		std::cout << "   passed" << std::endl;
	}

	void thread_pool_start_on_test()
	{
		std::cout << "thread_pool_start_on_test         ";
		//assert(0);

		thread_pool tp;

		auto foo = []()->task<int>
		{
#ifdef MACORO_CPP_20
			co_return 42;
#else

			MC_BEGIN(task<int>);
			MC_RETURN(42);
			MC_END();
#endif
		};

		auto t = foo()
			| start_on(tp);

		auto e = make_eager_task(std::move(t));

		tp.run();

		auto i = sync_wait(e);
		assert(i == 42);

		std::cout << "   passed" << std::endl;
	}

	namespace
	{

#ifdef MACORO_CPP_20

		eager_task<int> eager_taskInt20()
		{
			//assert(0);
			//throw std::runtime_error("");
			co_return 42;
		}
#endif
		eager_task<int> eager_taskInt14()
		{
			MC_BEGIN(eager_task<int>);
			MC_RETURN(42);
			MC_END();
		}
		bool taskVoid_called = false;
#ifdef MACORO_CPP_20
		eager_task<void> eager_taskVoid20()
		{
			taskVoid_called = true;
			co_return;
		}
#endif
		eager_task<void> eager_taskVoid14()
		{
			MC_BEGIN(eager_task<void>);
			taskVoid_called = true;
			MC_RETURN_VOID();
			MC_END();
		}
		int taskRef_val = 42;
#ifdef MACORO_CPP_20
		eager_task<int&> eager_taskRef20()
		{
			//assert(0);
			//throw std::runtime_error("");
			co_return taskRef_val;
		}
#endif
		eager_task<int&> eager_taskRef14()
		{
			MC_BEGIN(eager_task<int&>);
			MC_RETURN(taskRef_val);
			MC_END();
		}
		struct move_only
		{
			int v;
			move_only(int vv) :v(vv) { /*std::cout << "new " << this << std::endl;*/ };
			move_only(move_only&& vv) :v(vv.v) { /*std::cout << "move " << this << std::endl; */ };
			move_only(const move_only&) = delete;

			~move_only() { /*std::cout << "destroy " << this << std::endl;*/ }
		};


#ifdef MACORO_CPP_20
		eager_task<move_only> eager_taskmove20()
		{
			co_return 42;
		}
#endif
		eager_task<move_only> eager_taskmove14()
		{
			MC_BEGIN(eager_task<move_only>);
			MC_RETURN(42);
			MC_END();
		}
#ifdef MACORO_CPP_20
		eager_task<int> eager_taskThrows20()
		{

			throw std::runtime_error("42");
			co_return 42;
		}
#endif

		eager_task<int> eager_taskThrows14()
		{
			MC_BEGIN(eager_task<int>);
			throw std::runtime_error("42");
			MC_RETURN(42);
			MC_END();
		}


	}

	void eager_task_int_test()
	{
		std::cout << "eager_task_int_test  ";
#ifdef MACORO_CPP_20
		{
			eager_task<int> t = eager_taskInt20();
			int i = sync_wait(t);
			assert(i == 42);
		}
#endif
		{
			eager_task<int> t = eager_taskInt14();
			int i = sync_wait(t);
			assert(i == 42);
		}
		std::cout << "passed" << std::endl;
	}


	void eager_task_void_test()
	{
		std::cout << "eager_task_void_test  ";
#ifdef MACORO_CPP_20
		{
			taskVoid_called = false;
			eager_task<void> t = eager_taskVoid20();
			sync_wait(t);
			assert(taskVoid_called);
		}
#endif
		{
			taskVoid_called = false;
			eager_task<void> t = eager_taskVoid14();
			sync_wait(t);
			assert(taskVoid_called);
		}
		std::cout << "passed" << std::endl;
	}

	void eager_task_ref_test()
	{
		std::cout << "eager_task_ref_test  ";
#ifdef MACORO_CPP_20
		{
			taskRef_val = 42;
			eager_task<int&> t = eager_taskRef20();
			int& i = sync_wait(t);
			assert(i == 42);
			++taskRef_val;
			assert(i == 43);
		}
#endif
		{
			taskRef_val = 42;
			eager_task<int&> t = eager_taskRef14();
			int& i = sync_wait(t);
			assert(i == 42);
			++taskRef_val;
			assert(i == 43);
		}
		std::cout << "passed" << std::endl;
	}



	void eager_task_move_test()
	{
		std::cout << "eager_task_move_test  ";

#ifdef MACORO_CPP_20
		{
			eager_task<move_only> t = eager_taskmove20();
			auto v = std::move(sync_wait(t));
			assert(v.v == 42);
		}
		{
			eager_task<move_only>&& f();
			auto v = sync_wait(eager_taskmove20());
			assert(v.v == 42);
		}
#endif
		{
			eager_task<move_only> t = eager_taskmove14();
			auto v = std::move(sync_wait(t));
			assert(v.v == 42);
		}
		{
			eager_task<move_only>&& f();
			auto v = sync_wait(eager_taskmove14());
			assert(v.v == 42);
		}
		std::cout << "passed" << std::endl;
	}


	void eager_task_ex_test()
	{
		std::cout << "eager_task_ex_test  ";
#ifdef MACORO_CPP_20
		{
			eager_task<int> t = eager_taskThrows20();
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
			eager_task<int> t = eager_taskThrows14();
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

	void eager_task_stress_test()
	{
		std::cout << "eager_task_stress_test  ";
		int trials = 10000;

		thread_pool pool;
		auto w = pool.make_work();
		std::vector<std::thread> thrds(2); std::thread::hardware_concurrency();
		for (int i = 0; i < thrds.size(); ++i)
			thrds[i] = std::thread([&, i] {pool.run(); });


		std::vector<eager_task<>> ts; ts.reserve(trials);

		for (int i = 0; i < trials; ++i)
		{
			std::promise<void> p, p0,p1;
			auto f = p.get_future().share();
			auto f0 = p0.get_future();
			auto f1 = p1.get_future();
			auto t = [](std::promise<void> p0, std::shared_future<void> f) -> task<>
			{
				MC_BEGIN(task<>, p0 = std::move(p0), f);
				p0.set_value();
				f.get();
				//co_return;
				MC_RETURN_VOID();

				MC_END();
			};

			auto a = [](eager_task<>& tt, std::promise<void> p1, std::shared_future<void> f) -> task<>
			{
				MC_BEGIN(task<>,&tt, p1 = std::move(p1), f);
				p1.set_value();
				f.get();
				MC_AWAIT(tt);

				MC_END();
			};

			ts.push_back(t(std::move(p0), f) | start_on(pool));
			auto aa = a(ts.back(), std::move(p1), f) | start_on(pool);

			f0.get();
			f1.get();
			p.set_value();

			sync_wait(aa);
		}

		w.reset();
		for (int i = 0; i < thrds.size(); ++i)
			thrds[i].join();
		std::cout << "passed" << std::endl;
	}



	void eager_task_slow_test()
	{
		std::cout << "eager_task_slow_test  ";

		// the task t is slow to complete. 
		// a will set its continuation.

		auto t = []() -> eager_task<>
		{
			MC_BEGIN(eager_task<>);
			MC_AWAIT( suspend_always{});
			MC_RETURN_VOID();
			MC_END();
		};

		bool done = false;
		auto a = [&](eager_task<>& tt) -> eager_task<>
		{
			MC_BEGIN(eager_task<>, &tt, &done);
			MC_AWAIT(tt);
			done = true;
			MC_RETURN_VOID();
			MC_END();
		};

		auto tt = t();
		auto aa = a(tt) ;

		assert(!done);

		tt.handle().resume();
		assert(done);


		std::cout << "passed" << std::endl;
	}

	void eager_task_fast_test()
	{
		std::cout << "eager_task_fast_test  ";
		
		// the task t is fast to complete. 
		// a will set its continuation.

		auto t = []() -> eager_task<>
		{
			MC_BEGIN(eager_task<>);
			MC_RETURN_VOID();
			MC_END();
		};

		bool done = false;
		auto a = [&](eager_task<>& tt) -> eager_task<>
		{
			MC_BEGIN(eager_task<>, &tt, &done);
			MC_AWAIT(tt);
			done = true;
			MC_RETURN_VOID();
			MC_END();
		};

		auto tt = t();
		auto aa = a(tt);

		assert(done);

		std::cout << "passed" << std::endl;
	}


	void eager_task_tests()
	{
		std::cout << "-------- eager_task ----------" << std::endl;
		make_eager_test();
		thread_pool_post_test();
		thread_pool_dispatch_test();
		thread_pool_start_on_test();
		eager_task_int_test();
		eager_task_void_test();
		eager_task_ref_test();
		eager_task_move_test();
		eager_task_ex_test();
		eager_task_slow_test();
		eager_task_fast_test();
		eager_task_stress_test();

	}

}