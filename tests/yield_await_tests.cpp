#include "yield_await_tests.h"

#include <cassert>
#include "macoro/Macros.h"
#include "macoro/task.h"
#include <iostream>
namespace macoro
{

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

		struct Awaiter
		{
			bool await_ready() { return true; }
			template<typename T>
			void await_suspend(const std::coroutine_handle<T>& c) { }
			void await_suspend(coroutine_handle<> c) { }
			void await_resume() { }
		};


		struct NoCopyMove
		{
			NoCopyMove()
			{
				std::cout << "new NoCopyMove " << this << std::endl;
			}
			NoCopyMove(const NoCopyMove&) = delete;
			NoCopyMove(NoCopyMove&&) = delete;
			~NoCopyMove()
			{
				std::cout << "del NoCopyMove " << this << std::endl;
			}
		};

		struct RefOnlyAwaiter : public Awaiter
		{
			RefOnlyAwaiter() = default;
			RefOnlyAwaiter(const RefOnlyAwaiter&) = delete;
			RefOnlyAwaiter(RefOnlyAwaiter&&) = delete;

		};

		struct RefOnlyAwaitable
		{
			RefOnlyAwaitable() = default;
			RefOnlyAwaitable(const RefOnlyAwaitable&) = delete;
			RefOnlyAwaitable(RefOnlyAwaitable&&) = delete;


			RefOnlyAwaiter r;

			RefOnlyAwaiter& operator co_await()
			{
				return r;
			}
		};


		struct RefOnly
		{
			RefOnlyAwaitable r;

			RefOnly()
			{
				std::cout << "new RefOnly " << this << std::endl;
			}
			RefOnly(const RefOnly&) = delete;
			RefOnly(RefOnly&&) = delete;

			~RefOnly()
			{
				std::cout << "del RefOnly " << this << std::endl;
			}
		};



		struct MovOnlyAwaiter : public Awaiter
		{
			MovOnlyAwaiter()
			{
				std::cout << "new MovOnlyAwaiter " << this << std::endl;
			}
			MovOnlyAwaiter(const MovOnlyAwaiter&) = delete;
			MovOnlyAwaiter(MovOnlyAwaiter&&) {
				std::cout << "mov MovOnlyAwaiter " << this << std::endl;
			};

			~MovOnlyAwaiter()
			{
				std::cout << "del MovOnlyAwaiter " << this << std::endl;
			}

		};

		struct MovOnlyAwaitable
		{
			MovOnlyAwaitable()
			{
				std::cout << "new MovOnlyAwaitable " << this << std::endl;

			}
			MovOnlyAwaitable(const MovOnlyAwaitable&) = delete;
			MovOnlyAwaitable(MovOnlyAwaitable&&)
			{
				std::cout << "mov MovOnlyAwaitable " << this << std::endl;

			}

			~MovOnlyAwaitable()
			{
				std::cout << "del MovOnlyAwaitable " << this << std::endl;
			}

			MovOnlyAwaiter r;

			MovOnlyAwaiter&& operator co_await()
			{
				return std::move(r);
			}
		};


		struct MovOnly
		{
			MovOnlyAwaitable r;

			MovOnly()
			{
				std::cout << "new MovOnly " << this << std::endl;
			}
			MovOnly(const MovOnly&) = delete;
			MovOnly(MovOnly&& o)
			{
				std::cout << "mov MovOnly " << this << " " << &o << std::endl;
			}

			~MovOnly()
			{
				std::cout << "del MovOnly " << this << std::endl;
			}
		};



		struct NoCopyMoveAwaiter : public Awaiter
		{
			NoCopyMoveAwaiter(const NoCopyMoveAwaiter&) = delete;
			//NoCopyMoveAwaiter(NoCopyMoveAwaiter&& o) {
			//	std::cout << "mov NoCopyMoveAwaiter " << this << " " << &o << std::endl;

			//}
			NoCopyMoveAwaiter() {
				std::cout << "new NoCopyMoveAwaiter " << this << std::endl;
			}

			~NoCopyMoveAwaiter()
			{
				std::cout << "del NoCopyMoveAwaiter " << this << std::endl;
			}
		};

		struct NoCopyMoveAwaitable
		{
			NoCopyMoveAwaitable(const NoCopyMoveAwaitable&) = delete;
			//NoCopyMoveAwaitable(NoCopyMoveAwaitable&& o) {
			//	std::cout << "mov NoCopyMoveAwaitable " << this << " " << &o << std::endl;

			//}
			NoCopyMoveAwaitable() {
				std::cout << "new NoCopyMoveAwaitable " << this << std::endl;
			}

			~NoCopyMoveAwaitable()
			{
				std::cout << "del NoCopyMoveAwaitable " << this << std::endl;
			}

			NoCopyMoveAwaiter operator co_await()
			{
				return {};
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

				template<typename TT>
				decltype(auto) await_transform(TT&& t)
				{
					return static_cast<TT&&>(t);
				}

				NoCopyMoveAwaitable await_transform(NoCopyMove&& t)
				{
					return {};
				}

				RefOnlyAwaitable& await_transform(RefOnly& t)
				{
					return t.r;
				}

				MovOnlyAwaitable&& await_transform(MovOnly&& t)
				{
					return std::move(t.r);
				}


			};


			coroutine_handle<promise_type> h;


			struct task_awaitable
			{

				task_awaitable() = delete;
				task_awaitable(const task_awaitable&) = delete;
				task_awaitable(task_awaitable&& o) {
					std::cout << "mov task_awaitable " << this << " " << &o << std::endl;

				}
				task_awaitable(coroutine_handle<promise_type> hh) : h(hh) {
					std::cout << "new task_awaitable " << this << std::endl;
				}

				~task_awaitable()
				{
					std::cout << "del task_awaitable " << this << std::endl;

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
			task_awaitable operator co_await()&&
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




//	auto f2 = [](test_task<test_value>&& awaitable) -> test_task<test_value> {
//		//MC_BEGIN(test_task<test_value>, &awaitable);
//		do {
//			using ReturnType = test_task<test_value>;
//			auto _macoro_frame_ = ::macoro::makeFrame<typename coroutine_traits<ReturnType>::promise_type>(
//				[&awaitable](::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>* _macoro_frame_) mutable ->::macoro::coroutine_handle<void>
//				{	using _macoro_frame_t_ = ::macoro::FrameBase<typename coroutine_traits<ReturnType>::promise_type>;
//			try {
//
//
//				switch (_macoro_frame_->getSuspendPoint())
//				{
//				case ::macoro::SuspensionPoint::InitialSuspendBegin:
//				{
//#define SUSPEND_IDX MACORO_INITIAL_SUSPEND_IDX
//#define EXPRESSION _macoro_frame_->promise.initial_suspend()
//					//IMPL_MC_AWAIT_CORE(_macoro_frame_->promise.initial_suspend(), MACORO_INITIAL_SUSPEND_IDX)	
//					using promise_type = decltype(_macoro_frame_->promise);
//					using isl = decltype(is_lvalue(EXPRESSION));
//					using AwaitContext222 = AwaitContext<promise_type, SUSPEND_IDX, std::conditional_t<isl::value, decltype(EXPRESSION)&, decltype(EXPRESSION)>>;
//					using Handle = ::macoro::coroutine_handle<promise_type>;
//					{
//						auto& promise = _macoro_frame_->promise;
//						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//						auto& ctx = _macoro_frame_->makeAwaitContext<AwaitContext222>();
//						ctx.expr.ptr = new (ctx.expr.v()) decltype(ctx.expr)::Constructor(EXPRESSION);
//						ctx.awaitable.ptr = new (ctx.awaitable.v()) decltype(ctx.awaitable)::Constructor(get_awaitable(promise, ctx.expr.get()));
//						ctx.awaiter.ptr = new (ctx.awaiter.v()) decltype(ctx.awaiter)::Constructor(get_awaiter(ctx.awaitable.get()));
//						auto& awaiter = ctx.awaiter.getRef();
//						*ctx.awaiter_ptr = &awaiter;
//						if (!awaiter.await_ready())
//						{
//							/*<suspend-coroutine>*/
//							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)SUSPEND_IDX);
//
//							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
//							auto s = ::macoro::await_suspend(awaiter, handle);
//							if (s)
//							{
//								/*perform symmetric transfer or return to caller (for noop_coroutine) */
//								return s.get_handle();
//							}
//						}	}
//					_macoro_frame_->_initial_suspend_await_resumed_called_ = true;
//					_macoro_frame_->getAwaiter<AwaitContext222>().await_resume();
//					_macoro_frame_->destroyAwaiter<AwaitContext222>();
//				} do {} while (0);
//
//				std::cout << "outter start" << std::endl;
//
//				//co_yield co_await std::forward<test_task<test_value>>(awaitable);
//				//MC_YIELD_AWAIT(std::forward<test_task<test_value>>(awaitable));
//#define EXPRESSION std::forward<test_task<test_value>>(awaitable)
//#define SUSPEND_IDX2 2
//				{
//					//IMPL_MC_AWAIT_CORE(EXPRESSION, SUSPEND_IDX)													
//					using promise_type = decltype(_macoro_frame_->promise);
//					using isl = decltype(is_lvalue(EXPRESSION));
//					using AwaitContext2 = AwaitContext<promise_type, SUSPEND_IDX2,
//						std::conditional_t<isl::value, decltype(EXPRESSION)&, decltype(EXPRESSION)>>;
//					using Handle = ::macoro::coroutine_handle<promise_type>;
//					{
//						auto& promise = _macoro_frame_->promise;
//						auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//						auto& ctx = _macoro_frame_->makeAwaitContext<AwaitContext2>();
//						ctx.expr.ptr = new (ctx.expr.v()) decltype(ctx.expr)::Constructor(EXPRESSION);
//						ctx.awaitable.ptr = new (ctx.awaitable.v()) decltype(ctx.awaitable)::Constructor(get_awaitable(promise, ctx.expr.get()));
//						ctx.awaiter.ptr = new (ctx.awaiter.v()) decltype(ctx.awaiter)::Constructor(get_awaiter(ctx.awaitable.get()));
//						auto& awaiter = ctx.awaiter.getRef();
//						*ctx.awaiter_ptr = &awaiter;
//						if (!awaiter.await_ready())
//						{
//							/*<suspend-coroutine>*/
//							_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)SUSPEND_IDX2);
//
//							/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
//							auto s = ::macoro::await_suspend(awaiter, handle);
//							if (s)
//							{
//								/*perform symmetric transfer or return to caller (for noop_coroutine) */
//								return s.get_handle();
//							}
//						}
//					}
//				MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(SUSPEND_IDX2):
//
//
//#define EXPRESSION _macoro_frame_->promise.yield_value(_macoro_frame_->getAwaiter<AwaitContext2>().await_resume())
//#define SUSPEND_IDX3 3
//					/*<resume-point>*/
//					//IMPL_MC_AWAIT(_macoro_frame_->promise.yield_value(_macoro_frame_->getAwaiter<MACORO_CAT(AwaitContext, SUSPEND_IDX)>(SUSPEND_IDX).await_resume()), , __COUNTER__);
//					{
//						//IMPL_MC_AWAIT_CORE(EXPRESSION, SUSPEND_IDX3)													
//						using promise_type = decltype(_macoro_frame_->promise);
//						using isl = decltype(is_lvalue(EXPRESSION));
//						using AwaitContext3 = AwaitContext<promise_type, SUSPEND_IDX3,
//							std::conditional_t<isl::value, decltype(EXPRESSION)&, decltype(EXPRESSION)>>;
//						using Handle = ::macoro::coroutine_handle<promise_type>;
//						{
//							auto& promise = _macoro_frame_->promise;
//							auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//							auto& ctx = _macoro_frame_->makeAwaitContext<AwaitContext3>();
//							ctx.expr.ptr = new (ctx.expr.v()) decltype(ctx.expr)::Constructor(EXPRESSION);
//							ctx.awaitable.ptr = new (ctx.awaitable.v()) decltype(ctx.awaitable)::Constructor(get_awaitable(promise, ctx.expr.get()));
//							ctx.awaiter.ptr = new (ctx.awaiter.v()) decltype(ctx.awaiter)::Constructor(get_awaiter(ctx.awaitable.get()));
//							auto& awaiter = ctx.awaiter.getRef();
//							*ctx.awaiter_ptr = &awaiter;
//							if (!awaiter.await_ready())
//							{
//								/*<suspend-coroutine>*/
//								_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)SUSPEND_IDX3);
//
//								/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
//								auto s = ::macoro::await_suspend(awaiter, handle);
//								if (s)
//								{
//									/*perform symmetric transfer or return to caller (for noop_coroutine) */
//									return s.get_handle();
//								}
//							}
//						}
//					MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(SUSPEND_IDX3):
//						/*<resume-point>*/
//						_macoro_frame_->getAwaiter<AwaitContext3>().await_resume();
//						_macoro_frame_->destroyAwaiter<AwaitContext3>();
//					} do {} while (0);
//					_macoro_frame_->destroyAwaiter<AwaitContext2>();
//				} do {} while (0);
//
//				std::cout << "outter end" << std::endl;
//				//MC_END();
//
//				break;
//				case ::macoro::SuspensionPoint::FinalSuspend:
//					goto MACORO_FINAL_SUSPEND_RESUME;
//				default:
//					std::cout << "coroutine frame corrupted. " << __FILE__ << ":" << __LINE__ << std::endl;
//					std::terminate();
//					break;
//				}
//			}
//			catch (...) {
//				_macoro_frame_->destroyAwaiters();
//				if (!_macoro_frame_->_initial_suspend_await_resumed_called_)
//					throw;
//				_macoro_frame_->promise.unhandled_exception();
//			}
//			/*final suspend*/
//		MACORO_FINAL_SUSPEND_BEGIN:
//			IMPL_MC_AWAIT_CORE_NS(_macoro_frame_->promise.final_suspend(), MACORO_FINAL_SUSPEND_IDX)
//				MACORO_FINAL_SUSPEND_RESUME :
//				_macoro_frame_->getAwaiter<MACORO_CAT(AwaitContext, MACORO_FINAL_SUSPEND_IDX)>().await_resume();
//			_macoro_frame_->destroyAwaiter<MACORO_CAT(AwaitContext, MACORO_FINAL_SUSPEND_IDX)>();
//			_macoro_frame_->destroy(_macoro_frame_);
//			return noop_coroutine();
//				});
//
//			auto _macoro_ret_ = _macoro_frame_->promise.macoro_get_return_object();
//			using promise_type = decltype(_macoro_frame_->promise);
//			using Handle = ::macoro::coroutine_handle<promise_type>;
//			promise_type& promise = _macoro_frame_->promise;
//			auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
//			handle.resume();
//			return _macoro_ret_;
//		} while (0);
//	};



	void yield_await_lifetime_test14()
	{
		std::cout << "yield_await_lifetime_test14n============================ " << std::endl;;
		//using namespace tests;





		auto f = [](test_task<test_value>&& awaitable) -> test_task<test_value> {
			MC_BEGIN(test_task<test_value>, &awaitable);
			std::cout << "outter start" << std::endl;

			//co_yield co_await std::forward<test_task<test_value>>(awaitable);
			MC_YIELD_AWAIT(std::forward<test_task<test_value>>(awaitable));

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

	//	template<typename T>
	//	struct rvalue {
	//		static std::string name() { return typeid(T).name() + std::string("&&"); }
	//	};
	//
	//	template<typename T>
	//	rvalue<T> type(T&&) { return {}; }
	//
	//	template<typename T>
	//	struct clvalue {
	//		static std::string name() { return std::string("const ") + typeid(T).name() + std::string("&"); }
	//	};
	//
	//	template<typename T>
	//	clvalue<T> type(const T&) { return {}; }
	//
	//	template<typename T>
	//	struct lvalue {
	//		static std::string name() { return typeid(T).name() + std::string("&"); }
	//	};
	//
	//	template<typename T>
	//	lvalue<T> type(T&) { return {}; }
	//
	//#define TYPE(T) decltype(type(std::declval<T>()))::name()
	//#define VAR_TYPE(v) decltype(type(v))::name()


	template<typename T>
	struct type;

	template<typename T>
	struct type<T&>
	{
		static std::string name() { return typeid(T).name() + std::string("&"); }
	};
	template<typename T>
	struct type<const T&>
	{
		static std::string name() { return  std::string("const ") + typeid(T).name() + std::string("&"); }
	};
	template<typename T>
	struct type<T&&>
	{
		static std::string name() { return typeid(T).name() + std::string("&&"); }
	};
	template<typename T>
	struct type<const T&&>
	{
		static std::string name() { return std::string("const ") + typeid(T).name() + std::string("&&"); }
	};
	template<typename T>
	struct type<const T>
	{
		static std::string name() { return std::string("const ") + typeid(T).name(); }
	};
	template<typename T>
	struct type
	{
		static std::string name() { return typeid(T).name(); }
	};

#define TYPE(T) type<T>::name()
#define VAR_TYPE(v) type<decltype(v)>::name()

	template<typename TT>
	decltype(auto) as_reference(TT&& s) { return static_cast<decltype(s)>(s); }


	void* ctx;
	template<typename CTX>
	auto& makeAwaitContext()
	{
		auto p = new CTX;
		ctx = p;
		return *p;
	}

	template<typename CTX>
	auto& getAwaiter()
	{
		auto p = (CTX*)ctx;
		return p->awaiter.getRef();
	}
	template<typename CTX>
	auto destroyAwaiter()
	{
		auto p = (CTX*)ctx;
		p->awaiter.destroy();
		p->awaitable.destroy();
		p->expr.destroy();
		delete p;
	}

	// T& -> ref
	// T&& -> ref
	auto mov = []() -> MovOnly&& { return {}; };

	RefOnly ref;
	const RefOnly cref;
	void awaiterStorage_test()
	{


#define EXPRESSION ref
		//#define EXPRESSION NoCopyMove{}
		//#define EXPRESSION MovOnly{}
		//#define EXPRESSION mov()

		{

			auto foo = []() -> RefOnly { return {}; };
			using TT0 = decltype(is_lvalue(ref));
			using TT1 = decltype(is_lvalue(cref));
			using TT2 = decltype(is_lvalue(RefOnly{}));
			using TT3 = decltype(is_lvalue(foo()));

			using EE = std::conditional_t<decltype(is_lvalue(EXPRESSION))::value, decltype(EXPRESSION)&, decltype(EXPRESSION)>;
			//using promise_type = test_task<test_value>::promise_type ;
			//promise_type promise;
			//using Xtc = AwaitContext<promise_type, 1, EE>;

			//auto GG = decltype(get_awaitable(promise, ctx.expr.get()));
		}

		auto f = []() -> test_task<test_value> {
			MC_BEGIN(test_task<test_value>);


			MC_AWAIT(EXPRESSION);
//#define SUSPEND_IDX 222
			//{
			//	//IMPL_MC_AWAIT_CORE(EXPRESSION, SUSPEND_IDX)									
			//	using promise_type = decltype(_macoro_frame_->promise);
			//	using MACORO_CAT(AwaitContext, SUSPEND_IDX) = AwaitContext<promise_type, SUSPEND_IDX, std::conditional_t<decltype(is_lvalue(EXPRESSION))::value, decltype(EXPRESSION)&, decltype(EXPRESSION)>>;
			//	using Handle = ::macoro::coroutine_handle<promise_type>;
			//	{
			//		auto& promise = _macoro_frame_->promise;
			//		auto& ctx = /*_macoro_frame_->*/makeAwaitContext<MACORO_CAT(AwaitContext, SUSPEND_IDX)>();
			//		ctx.expr.pt = new (ctx.expr.v()) decltype(ctx.expr)::Constructor(EXPRESSION);
			//		ctx.awaitable.pt = new (ctx.awaitable.v()) decltype(ctx.awaitable)::Constructor(get_awaitable(promise, ctx.expr.get()));
			//		ctx.awaiter.pt = new (ctx.awaiter.v()) decltype(ctx.awaiter)::Constructor(get_awaiter(ctx.awaitable.get()));
			//		auto& awaiter = /*_macoro_frame_->*/getAwaiter<MACORO_CAT(AwaitContext, SUSPEND_IDX)>();

			//		if (!awaiter.await_ready())
			//		{
			//			/*<suspend-coroutine>*/
			//			_macoro_frame_->setSuspendPoint((::macoro::SuspensionPoint)SUSPEND_IDX);

			//			/*<call awaiter.await_suspend(). If it's void return, then return true.>*/
			//			auto handle = Handle::from_promise(promise, coroutine_handle_type::mocoro);
			//			auto s = ::macoro::await_suspend(awaiter, handle);
			//			if (s)
			//			{
			//				/*perform symmetric transfer or return to caller (for noop_coroutine) */
			//				return s.get_handle();
			//			}
			//		}
			//	}
			//		MACORO_FALLTHROUGH; case ::macoro::SuspensionPoint(SUSPEND_IDX):
			//			/*<resume-point>*/
			//			/*_macoro_frame_->*/ getAwaiter<MACORO_CAT(AwaitContext, SUSPEND_IDX)>().await_resume();
			//			/*_macoro_frame_->*/ destroyAwaiter<MACORO_CAT(AwaitContext, SUSPEND_IDX)>();
			//} do {} while (0);

			MC_END();
		};

		auto g = []() ->test_task<test_value>
		{
			co_await EXPRESSION;
		};


		std::cout << "--------- NoCopyMove -------" << std::endl;
		auto gg = g();
		gg.h.resume();
		std::cout << "---------  -------" << std::endl;

		auto r = f();
		r.h.resume();
		std::cout << "---------  -------" << std::endl;
		//AF::expr;
		//AF::value_type;

		//auto ff = [] {return NoCopyMove{}; };
		//auto cc = ff();
		//std::aligned_storage_t<sizeof(NoCopyMove), alignof(NoCopyMove)> ss;

		//auto ptr = (void*)&ss;
		//new (ptr) NoCopyMove(ff());


		//{
		//	std::cout << "--------- NoCopyMove -------" << std::endl;
		//	using AF = awaiter_for<test_task<test_value>::promise_type, NoCopyMove>;
		//	std::cout <<
		//		"Exprnt" << TYPE(ExpressionStorage2<AF>::Expr) << std::endl;
		//	std::cout <<
		//		"Awaitablent" << TYPE(ExpressionStorage2<AF>::Awaitable) << std::endl;
		//	std::cout <<
		//		"Awaiternt" << TYPE(ExpressionStorage2<AF>::Awaiter) << std::endl;

		//	using Expr = ExpressionStorage2<AF>::Expr;
		//	using Awaitable = ExpressionStorage2<AF>::Awaitable;
		//	using Awaiter = ExpressionStorage2<AF>::Awaiter;

		//	ExpressionStorage2<AF>a;

		//	Storage<Expr> se;
		//	se.pt = new (se.v()) typename Storage<Expr>::C(NoCopyMove{});

		//	auto& promise = r.h.promise();
		//	Storage<Awaitable> sa;
		//	sa.pt = new (sa.v()) typename Storage<Awaitable>::C(get_awaitable(promise, se.get()));

		//	Storage<Awaiter> saa;
		//	saa.pt = new (saa.v()) typename Storage<Awaiter>::C(get_awaiter(sa.get()));



		//	//return static_cast<Expr>(*pe);
		//	//ExpressionStorage2<AF>::mExpr(&a, NoCopyMove{});
		//	//ExpressionStorage2<AF>a(r.h.promise(), NoCopyMove{ });
		//	std::cout << "mid" << std::endl;
		//}

		//{
		//	RefOnly ref{ };
		//	std::cout << "------- RefOnly ---------" << std::endl;
		//	using AF = awaiter_for<test_task<test_value>::promise_type, RefOnly&>;
		//	std::cout <<
		//		"Exprnt" << TYPE(ExpressionStorage2<AF>::Expr) << std::endl;
		//	std::cout <<
		//		"Awaitablent" << TYPE(ExpressionStorage2<AF>::Awaitable) << std::endl;
		//	std::cout <<
		//		"Awaiternt" << TYPE(ExpressionStorage2<AF>::Awaiter) << std::endl;


		//	using Expr = ExpressionStorage2<AF>::Expr;
		//	using Awaitable = ExpressionStorage2<AF>::Awaitable;
		//	using Awaiter = ExpressionStorage2<AF>::Awaiter;
		//	ExpressionStorage2<AF>a;
		//	//a.vp = new (a.vs()) Expr(ref);

		//	Storage<Expr> se;
		//	se.pt = new (se.v()) typename Storage<Expr>::Constructor(ref);


		//	Storage<Awaitable> sa;
		//	sa.pt = new (sa.v()) typename Storage<Awaitable>::Constructor(se.get());

		//	Storage<Awaitable> saa;
		//	saa.pt = new (saa.v()) typename Storage<Awaitable>::Constructor(sa.get());





		//	//ExpressionStorage2<AF>a(r.h.promise(), ref);
		//	std::cout << "mid" << std::endl;
		//}


	}


	void yield_await_tests()
	{
		awaiterStorage_test();
		//yield_await_lifetime_test20();
		//yield_await_lifetime_test14();

	}
}