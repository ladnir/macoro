#include "yield_await_tests.h"

#include <cassert>
#include "macoro/Macros.h"
#include "macoro/task.h"
#include <iostream>
namespace macoro
{

	//	namespace tests
	//	{
	//
	//		template<typename T>
	//		struct test_task;
	//
	//		template<typename T>
	//		struct test_promise_base
	//		{
	//			std::exception_ptr exception;
	//			bool is_set = false;
	//
	//			void wait()
	//			{
	//				assert(is_set);
	//			}
	//
	//			void set()
	//			{
	//				assert(is_set == false);
	//			}
	//
	//
	//			suspend_always initial_suspend() noexcept { return{}; }
	//			suspend_always final_suspend() noexcept {
	//				set();
	//				return { };
	//			}
	//
	//			void return_void() {}
	//
	//
	//			void unhandled_exception() noexcept
	//			{
	//				exception = std::current_exception();
	//			}
	//		};
	//
	//		template<typename T>
	//		struct test_promise : public test_promise_base<T>
	//		{
	//			typename std::remove_reference<T>::type* mVal = nullptr;
	//
	//			test_task<T> get_return_object() noexcept;
	//			test_task<T> macoro_get_return_object() noexcept;
	//
	//			using reference_type = T&&;
	//			suspend_always yield_value(reference_type v) noexcept
	//			{
	//				mVal = std::addressof(v);
	//				this->set();
	//				return { };
	//			}
	//
	//			reference_type value()
	//			{
	//				if (this->exception)
	//					std::rethrow_exception(this->exception);
	//				return static_cast<reference_type>(*mVal);
	//			}
	//
	//
	//
	//			template<typename TT>
	//			decltype(auto) await_transform(TT&& t)
	//			{
	//				return static_cast<TT&&>(t);
	//			}
	//
	//			struct get_promise
	//			{
	//				test_promise<T>* promise = nullptr;
	//				bool await_ready() { return true; }
	//				template<typename TT>
	//				void await_suspend(TT&&) {}
	//				test_promise<T>* await_resume()
	//				{
	//					return promise;
	//				}
	//			};
	//
	//			get_promise await_transform(get_promise)
	//			{
	//				return { this };
	//			}
	//		};
	//
	//		template<>
	//		struct test_promise<void> : public test_promise_base<void>
	//		{
	//			test_task<void> get_return_object() noexcept;
	//			test_task<void> macoro_get_return_object() noexcept;
	//
	//			void value()
	//			{
	//				if (this->exception)
	//					std::rethrow_exception(this->exception);
	//			}
	//		};
	//
	//
	//		template<typename T>
	//		struct test_task
	//		{
	//			using promise_type = test_promise<T>;
	//			coroutine_handle<promise_type> handle;
	//			test_task(coroutine_handle<promise_type> h)
	//				: handle(h)
	//			{}
	//
	//			test_task() = delete;
	//			test_task(test_task&& h) noexcept :handle(std::exchange(h.handle, std::nullptr_t{})) {}
	//			test_task& operator=(test_task&& h) { handle = std::exchange(h.handle, std::nullptr_t{}); }
	//
	//			~test_task()
	//			{
	//				if (handle)
	//					handle.destroy();
	//			}
	//
	//			void start()
	//			{
	//				handle.resume();
	//			}
	//
	//			decltype(auto) get()
	//			{
	//				handle.promise().wait();
	//				return handle.promise().value();
	//			}
	//		};
	//
	//
	//
	//		template<typename T>
	//		inline test_task<T> test_promise<T>::get_return_object() noexcept { return { coroutine_handle<test_promise<T>>::from_promise(*this, coroutine_handle_type::std) }; }
	//		template<typename T>
	//		inline test_task<T> test_promise<T>::macoro_get_return_object() noexcept { return { coroutine_handle<test_promise<T>>::from_promise(*this, coroutine_handle_type::mocoro) }; }
	//		inline test_task<void> test_promise<void>::get_return_object() noexcept { return { coroutine_handle<test_promise<void>>::from_promise(*this, coroutine_handle_type::std) }; }
	//		inline test_task<void> test_promise<void>::macoro_get_return_object() noexcept { return { coroutine_handle<test_promise<void>>::from_promise(*this, coroutine_handle_type::mocoro) }; }
	//
	//		template<
	//			typename Awaitable,
	//			typename ResultType = typename awaitable_traits<Awaitable&&>::await_result>
	//			enable_if_t<!std::is_void<ResultType>::value,
	//			test_task<ResultType>
	//			>
	//			make_test_task14(Awaitable&& awaitable)
	//		{
	//			MC_BEGIN(test_task<ResultType>, &awaitable);
	//			MC_YIELD_AWAIT(static_cast<Awaitable&&>(awaitable));
	//			MC_END();
	//		}
	//
	//		template<
	//			typename Awaitable,
	//			typename ResultType = typename awaitable_traits<Awaitable&&>::await_result>
	//			enable_if_t<!std::is_void<ResultType>::value,
	//			test_task<ResultType>
	//			>
	//			make_test_task20(Awaitable&& awaitable)
	//		{
	//			co_yield co_await std::forward<Awaitable>(awaitable);
	//			//auto promise = co_await typename test_promise<ResultType>::get_promise{};
	//			//auto awaiter = promise->yield_value(co_await std::forward<Awaitable>(a));
	//			//co_await awaiter;
	//		}
	//
	////
	////		template<
	////			typename Awaitable,
	////			typename ResultType = typename awaitable_traits<Awaitable&&>::await_result>
	////			enable_if_t<std::is_void<ResultType>::value,
	////			test_task<ResultType>
	////			>
	////			make_test_task(Awaitable&& awaitable)
	////		{
	////#if 0
	////			co_await std::forward<Awaitable>(awaitable);
	////#else
	////			MC_BEGIN(test_task<ResultType>, &awaitable);
	////			MC_AWAIT(static_cast<Awaitable&&>(awaitable));
	////			MC_END();
	////#endif
	////		}
	//
	//
	//	}

	namespace
	{
		struct test_value
		{
			test_value() {
				std::cout << "new test_value " << this << std::endl;
			};
			test_value(test_value&& o)
			{
				std::cout << "mov test_value " << this << " " << &o << std::endl;
			}

			~test_value()
			{
				std::cout << "del test_value " << this << std::endl;
			}

		};

		struct yield_awaitable
		{

			yield_awaitable() {
				std::cout << "new yield_awaitable " << this << std::endl;
			}

			yield_awaitable(yield_awaitable&& o)
			{
				std::cout << "new yield_awaitable " << this << " < " << &o << std::endl;
			}

			~yield_awaitable()
			{
				std::cout << "del yield_awaitable " << this << std::endl;
			}


			bool await_ready() { return false; }

			template<typename T>
			std::coroutine_handle<void> await_suspend(const std::coroutine_handle<T>& c)
			{
				return std::noop_coroutine();
			}
			coroutine_handle<> await_suspend(coroutine_handle<> c)
			{
				return noop_coroutine();
			}

			void await_resume() {
				throw std::runtime_error("");
			}
		};


		template<typename T>
		struct test_task
		{


			struct promise_type
			{
				promise_type()
				{
					std::cout << "new promise_type " << this << std::endl;
				}

				~promise_type()
				{
					std::cout << "del promise_type " << this << std::endl;
				}

				suspend_always initial_suspend() { return {}; }
				impl::continuation_awaiter<> final_suspend() noexcept { return { cont ? cont : std::noop_coroutine() }; }


				yield_awaitable yield_value(T&& v)
				{
					return {};
				}

				test_task<T> get_return_object() { return { coroutine_handle<promise_type>::from_promise(*this, coroutine_handle_type::std) }; }
				test_task<T> macoro_get_return_object() { return { coroutine_handle<promise_type>::from_promise(*this, coroutine_handle_type::mocoro) }; }

				void unhandled_exception()
				{
					e = std::current_exception();
				}

				std::exception_ptr e;
				coroutine_handle<> cont;
				optional<test_value> s;
				void return_value(test_value&& v) {
					s.emplace(std::forward<test_value>(v));
				}
			};


			coroutine_handle<promise_type> h;

			struct value_awaitable
			{

				value_awaitable() = delete;
				value_awaitable(coroutine_handle<promise_type> hh) : h(hh) {
					std::cout << "new value_awaitable " << this << std::endl;
				}

				~value_awaitable()
				{
					std::cout << "del value_awaitable " << this << std::endl;

				}

				coroutine_handle<promise_type> h;

				bool await_ready()
				{
					return h.promise().s.has_value() || h.promise().e;
				}

				template<typename T>
				std::coroutine_handle<void> await_suspend(const std::coroutine_handle<T>& c)
				{
					h.promise().cont = c;
					return h.std_cast();
				}


				coroutine_handle<promise_type> await_suspend(coroutine_handle<> c)
				{
					h.promise().cont = c;
					return h;
				}

				test_value&& await_resume()
				{
					if (h.promise().e)
						std::rethrow_exception(h.promise().e);
					return std::move(h.promise().s.value());
				}

			};

			value_awaitable operator co_await()&&
			{
				return { h };
			}
		};


	}

	bool throws = false;
	void yield_await_lifetime_test20()
	{
		std::cout << "yield_await_lifetime_test20n============================ " << std::endl;;
		//using namespace tests;

		auto f = [](test_task<test_value>&& awaitable) -> test_task<test_value> {
			std::cout << "outter start" << std::endl;

			co_yield co_await std::forward<test_task<test_value>>(awaitable);

			std::cout << "outter end" << std::endl;
			co_return{};
		};

		auto t = []()->test_task<test_value> {

			if (throws)
			{
				std::cout << "inner throw" << std::endl;
				throw std::runtime_error("");
			}
			std::cout << "inner return" << std::endl;
			co_return test_value{};
		}();
		test_task<test_value> r = f(std::move(t));

		r.h.resume();

		std::cout << "mid" << std::endl;
		if (!throws)
			r.h.resume();

		std::cout << "n------------------- passed " << std::endl;;
	}



	void yield_await_lifetime_test14()
	{
		std::cout << "yield_await_lifetime_test14n============================ " << std::endl;;
		//using namespace tests;

		auto f = [](test_task<test_value>&& awaitable) -> test_task<test_value> {
			MC_BEGIN(test_task<test_value>, &awaitable);
			std::cout << "outter start" << std::endl;

			//co_yield co_await std::forward<test_task<test_value>>(awaitable);
			MC_YIELD_AWAIT(std::forward<test_task<test_value>>(awaitable));

			//{
			//	using promise_type = decltype(_macoro_frame_->promise);
			//	using AwaiterFor222 = decltype(_macoro_frame_->constructAwaiter2(std::forward<test_task<test_value>>(awaitable)));
			//	using Handle = ::macoro::coroutine_handle<promise_type>;
			//	{
			//		auto& promise = _macoro_frame_->promise;
			//		auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
			//		auto& awaiter = _macoro_frame_->constructAwaiter(std::forward<test_task<test_value>>(awaitable), 222);
			//		if (!awaiter.await_ready())
			//		{
			//			/*<suspend-coroutine>*/
			//			_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)222);
			//			/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
			//			auto s = ::macoro::await_suspend(awaiter, handle);
			//			if (s)
			//			{
			//				/*perform symmetric transfer or return to caller (for noop_coroutine) */
			//				return s.get_handle();
			//			}
			//		}
			//	}
			//	/*<resume-point>*/
			//MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(222):
			//	/*_macoro_frame_t_::DestroyAwaiterRaii<MACORO_CAT(AwaiterFor,SUSPEND_IDX)> raii(_macoro_frame_, SUSPEND_IDX);*/
			//	//IMPL_MC_AWAIT(_macoro_frame_->promise.yield_value(_macoro_frame_->getAwaiter<AwaiterFor222>(222).await_resume()), , , 444);
			//	{
			//		using promise_type = decltype(_macoro_frame_->promise);
			//		using  AwaiterFor444 = decltype(_macoro_frame_->constructAwaiter2(_macoro_frame_->promise.yield_value(_macoro_frame_->getAwaiter<AwaiterFor222>(222).await_resume())));
			//		using Handle = ::macoro::coroutine_handle<promise_type>;
			//		{
			//			auto& promise = _macoro_frame_->promise;
			//			auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
			//			auto& awaiter = _macoro_frame_->constructAwaiter(_macoro_frame_->promise.yield_value(_macoro_frame_->getAwaiter<AwaiterFor222>(222).await_resume()), 444);
			//			if (!awaiter.await_ready())
			//			{
			//				/*<suspend-coroutine>*/
			//				_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)444);
			//				/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
			//				auto s = ::macoro::await_suspend(awaiter, handle);
			//				if (s)
			//				{
			//					/*perform symmetric transfer or return to caller (for noop_coroutine) */
			//					return s.get_handle();
			//				}
			//			}
			//		}
			//		/*<resume-point>*/
			//	MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(444):
			//		_macoro_frame_->getAwaiter<AwaiterFor444>(444).await_resume();
			//		_macoro_frame_.destroyAwaiter<AwaiterFor444>(444);					
			//	
			//	} do {} while (0);
			//	_macoro_frame_.destroyAwaiter<AwaiterFor222>(222);
			//}

			std::cout << "outter end" << std::endl;
			MC_END();
		};

		auto t = []()->test_task<test_value> {
			MC_BEGIN(test_task<test_value>);

			if (throws)
			{
				std::cout << "inner throw" << std::endl;
				throw std::runtime_error("");
			}
			std::cout << "inner return" << std::endl;
			MC_RETURN(test_value{});
			//co_return test_value{};

			MC_END();
		}();
		test_task<test_value> r = f(std::move(t));

		r.h.resume();

		std::cout << "mid" << std::endl;
		if (!throws)
			r.h.resume();

		std::cout << "n------------------- passed " << std::endl;;
	}


	void yield_await_tests()
	{

		yield_await_lifetime_test20();
		yield_await_lifetime_test14();

	}
}