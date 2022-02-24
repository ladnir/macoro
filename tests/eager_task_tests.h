#pragma once



namespace macoro
{
	namespace tests
	{

		void make_eager_test();
		void thread_pool_post_test();
		void thread_pool_dispatch_test();
		void thread_pool_start_on_test();
		void eager_task_int_test();
		void eager_task_void_test();
		void eager_task_ref_test();
		void eager_task_move_test();
		void eager_task_ex_test();
		void eager_task_slow_test();
		void eager_task_fast_test();
		void eager_task_stress_test();
	}
}