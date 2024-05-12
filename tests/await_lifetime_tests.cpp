#include "await_lifetime_tests.h"

#include <cassert>
#include "macoro/macros.h"
#include "macoro/task.h"
#include <iostream>
#include <unordered_map>
#include <cstring>
#include "macoro/awaiter.h"
#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#endif

#define test_assert(X) if(!(X)) throw std::runtime_error(MACORO_LOCATION)


namespace macoro
{

	namespace tests
	{
		namespace
		{

			bool printRec = false;

			int counter = 0;
			std::unordered_map<void*, int> map;
			int fetch(void* p)
			{
				auto iter = map.find(p);
				if (iter == map.end())
				{
					iter = map.insert({ p, counter++ }).first;
				}

				return iter->second;
			}
			struct Rec
			{
				std::string str;
				int i, o = 0;

				Rec(const char* ss, int ii, int oo = 0)
					: str(ss)
					, i(ii)
					, o(oo)
				{
				}


				Rec(const char* ss, void* ii, void* oo)
					: str(ss)
					, i(fetch(ii))
					, o(fetch(oo))
				{
					if (printRec)
						std::cout << "{\"" << str << "\", " << i << ", " << o << "}," << std::endl;
				}


				Rec(const char* ss, void* ii)
					: str(ss)
					, i(fetch(ii))
				{
					if (printRec)
						std::cout << "{\"" << str << "\", " << i << ", " << o << "}," << std::endl;
				}


				bool operator==(const Rec& other) const
				{
					return
						str == other.str &&
						i == other.i &&
						o == other.o;
				}
			};

			std::vector<Rec> log;

			std::vector<Rec> getLog()
			{
				counter = 0;
				map.clear();
				return std::move(log);
			}


			void print(std::vector<Rec>& l)
			{
				for (size_t i = 0; i < l.size(); ++i)
					std::cout << "{\"" << l[i].str << "\", " << l[i].i << ", " << l[i].o << "}," << std::endl;
			}

			void print(std::vector<Rec>& l, std::vector<Rec>& r)
			{
				auto lMax = 0ull, rMax = 0ull;
				for (size_t i = 0; i < l.size(); ++i)
					lMax = std::max<size_t>(lMax, l[i].str.size());
				for (size_t i = 0; i < r.size(); ++i)
					rMax = std::max<size_t>(rMax, r[i].str.size());
				for (size_t i = 0; i < std::max(l.size(), r.size()); ++i)
				{
					if (l.size() > i && r.size() > i && !(l[i] == r[i]))
						std::cout << "> ";
					else
						std::cout << "  ";

					auto ls = i < l.size();
					auto rs = i < r.size();
					auto lsr = (ls ? l[i].str : std::string(lMax, ' '));
					auto rsr = (rs ? r[i].str : std::string(rMax, ' '));

					if (ls) lsr.resize(lMax);
					if (rs) rsr.resize(rMax);
					std::cout
						<< lsr << " " << (ls ? l[i].i : 0) << " ~~ "
						<< rsr << " " << (rs ? r[i].i : 0) << std::endl;
				}
			}

			struct test_value
			{
				test_value() {
					log.emplace_back("new test_value", this);
				};
				test_value(test_value&& o)
				{
					log.emplace_back("mov test_value", this, &o);
				}

				~test_value()
				{
					log.emplace_back("del test_value", this);
				}

			};

			struct yield_awaitable
			{

				yield_awaitable() {
					log.emplace_back("new yield_awaitable", this);
				}

				yield_awaitable(yield_awaitable&& o)
				{
					log.emplace_back("new yield_awaitable", this, &o);
				}

				~yield_awaitable()
				{
					log.emplace_back("del yield_awaitable", this);
				}


				bool await_ready() { return false; }
#ifdef MACORO_CPP_20
				template<typename T>
				std::coroutine_handle<void> await_suspend(const std::coroutine_handle<T>& c)
				{
					return std::noop_coroutine();
				}
#endif

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

#ifdef MACORO_CPP_20
				template<typename T>
				void await_suspend(const std::coroutine_handle<T>& c) { }
#endif
				template<typename T>
				void await_suspend(const coroutine_handle<T>& c) { }

				void await_resume() { }
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

#ifdef MACORO_CPP_20
				RefOnlyAwaiter& operator co_await()
#else
				RefOnlyAwaiter& operator_co_await()
#endif
				{
					return r;
				}


			};


			struct RefOnly
			{
				RefOnlyAwaitable r;

				RefOnly()
				{
					log.emplace_back("new RefOnly", this);
				}
				RefOnly(const RefOnly&) = delete;
				RefOnly(RefOnly&&) = delete;

				~RefOnly()
				{
					log.emplace_back("del RefOnly", this);
				}
			};



			struct MovOnlyAwaiter : public Awaiter
			{
				MovOnlyAwaiter()
				{
					log.emplace_back("new MovOnlyAwaiter", this);
				}
				MovOnlyAwaiter(const MovOnlyAwaiter&) = delete;
				MovOnlyAwaiter(MovOnlyAwaiter&& o) {
					log.emplace_back("mov MovOnlyAwaiter", this, &o);
				};

				~MovOnlyAwaiter()
				{
					log.emplace_back("del MovOnlyAwaiter", this);
				}

			};

			struct MovOnlyAwaitable
			{
				MovOnlyAwaitable()
				{
					log.emplace_back("new MovOnlyAwaitable", this);

				}
				MovOnlyAwaitable(const MovOnlyAwaitable&) = delete;
				MovOnlyAwaitable(MovOnlyAwaitable&& o)
				{
					log.emplace_back("mov MovOnlyAwaitable", this, &o);

				}

				~MovOnlyAwaitable()
				{
					log.emplace_back("del MovOnlyAwaitable", this);
				}

				MovOnlyAwaiter r;
#ifdef MACORO_CPP_20
				MovOnlyAwaiter&& operator co_await()
#else
				MovOnlyAwaiter&& operator_co_await()
#endif
				{
					return std::move(r);
				}
			};


			struct MovOnly
			{
				MovOnlyAwaitable r;

				MovOnly()
				{
					log.emplace_back("new MovOnly", this);
				}
				MovOnly(const MovOnly&) = delete;
				MovOnly(MovOnly&& o)
				{
					log.emplace_back("mov MovOnly", this, &o);;
				}

				~MovOnly()
				{
					log.emplace_back("del MovOnly", this);
				}
			};



			struct NoCopyMove
			{
				NoCopyMove()
				{
					log.emplace_back("new NoCopyMove", this);
				}
				NoCopyMove(const NoCopyMove&) = delete;
				NoCopyMove(NoCopyMove&&) = delete;
				~NoCopyMove()
				{
					log.emplace_back("del NoCopyMove", this);
				}
			};
			struct NoCopyMoveAwaiter : public Awaiter
			{
				NoCopyMoveAwaiter(const NoCopyMoveAwaiter&) = delete;
				//NoCopyMoveAwaiter(NoCopyMoveAwaiter&& o) {
				//	log.emplace_back("mov NoCopyMoveAwaiter " , this ,1 );;

				//}
				NoCopyMoveAwaiter() {
					log.emplace_back("new NoCopyMoveAwaiter", this);
				}

				~NoCopyMoveAwaiter()
				{
					log.emplace_back("del NoCopyMoveAwaiter", this);
				}
			};

			struct NoCopyMoveAwaitable
			{
				NoCopyMoveAwaitable(const NoCopyMoveAwaitable&) = delete;
				//NoCopyMoveAwaitable(NoCopyMoveAwaitable&& o) {
				//	log.emplace_back("mov NoCopyMoveAwaitable " , this ,1 );;

				//}
				NoCopyMoveAwaitable() {
					log.emplace_back("new NoCopyMoveAwaitable", this);
				}

				~NoCopyMoveAwaitable()
				{
					log.emplace_back("del NoCopyMoveAwaitable", this);
				}


#ifdef MACORO_CPP_20
				NoCopyMoveAwaiter operator co_await()
#else
				NoCopyMoveAwaiter operator_co_await()
#endif
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
						log.emplace_back("new promise_type", this);
					}

					~promise_type()
					{
						log.emplace_back("del promise_type", this);
					}

					suspend_always initial_suspend() { return {}; }

					detail::continuation_awaiter<> final_suspend() noexcept { return { cont ? cont : noop_coroutine() }; }


					yield_awaitable yield_value(T&& v)
					{
						return {};
					}

					test_task<T> get_return_object() { return { coroutine_handle<promise_type>::from_promise(*this, coroutine_handle_type::std) }; }
					test_task<T> macoro_get_return_object() { return { coroutine_handle<promise_type>::from_promise(*this, coroutine_handle_type::macoro) }; }

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

				test_task() = default;
				test_task(const test_task&) = delete;
				test_task(test_task&& o) : h(std::exchange(o.h, std::nullptr_t{})) {}
				test_task(coroutine_handle<promise_type>hh) :h(hh) {}

				~test_task()
				{
					if (h)
						h.destroy();
				}

				coroutine_handle<promise_type> h;


				struct task_awaitable
				{

					task_awaitable() = delete;
					task_awaitable(const task_awaitable&) = delete;
					task_awaitable(task_awaitable&& o) {
						log.emplace_back("mov task_awaitable", this, &o);;

					}
					task_awaitable(coroutine_handle<promise_type> hh) : h(hh) {
						log.emplace_back("new task_awaitable", this);
					}

					~task_awaitable()
					{
						log.emplace_back("del task_awaitable", this);

					}

					coroutine_handle<promise_type> h;

					bool await_ready()
					{
						return h.promise().s.has_value() || h.promise().e;
					}

#ifdef MACORO_CPP_20
					template<typename TT>
					std::coroutine_handle<void> await_suspend(const std::coroutine_handle<TT>& c)
					{
						h.promise().cont = coroutine_handle<TT>(c);
						return h.std_cast();
					}
#endif


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

#ifdef MACORO_CPP_20
				task_awaitable operator co_await() &&
#else
				task_awaitable operator_co_await() &&
#endif
				{
					return { h };
				}
			};

		}

		//template<typename T>
		//std::true_type is_lvalue2(T&) { return {}; }
		//template<typename T>
		//std::false_type is_lvalue2(T&&) { return {}; }


		template<typename T>
		struct AsRef
		{
			using type = T;
		};

		template<typename DeclType, typename RefType>
		decltype(auto) as_ref2(RefType&&)
		{

			//using isl1 = decltype(is_lvalue(std::forward<test_task<test_value>>(awaitable)));
			using T = std::conditional_t<std::is_lvalue_reference<RefType&&>::value, DeclType&, DeclType>;
			return AsRef<T>{};
		}

		// gcc has a bug that the awaiter is moved when it shouldnt be. This makes some tests fail.
#ifdef GCC_VERSION
#ifndef __llvm__
#define HAS_GCC_MOVE_AWAITER_BUG 0
//#define HAS_GCC_MOVE_AWAITER_BUG (GCC_VERSION < 12 * 10000)
#else
#define HAS_GCC_MOVE_AWAITER_BUG 0
#endif
#else
#define HAS_GCC_MOVE_AWAITER_BUG 0
#endif

#ifdef MACORO_CPP_20
#define ENABLE_NO_MOVE_TEST (!HAS_GCC_MOVE_AWAITER_BUG)
#else
#define ENABLE_NO_MOVE_TEST 0 
#endif

#if ENABLE_NO_MOVE_TEST
		test_task<test_value> yield_await20(test_task<test_value>&& awaitable) {
			log.emplace_back("outter start", nullptr);

			co_yield co_await std::forward<test_task<test_value>>(awaitable);

			log.emplace_back("outter end", nullptr);
			co_return{};
		};


		test_task<test_value> base_task20(bool throws)
		{
			if (throws)
			{
				log.emplace_back("inner throw", nullptr);
				throw std::runtime_error("");
			}
			log.emplace_back("inner return", nullptr);
			co_return test_value{};
		};

		void yield_await_lifetime_test20()
		{
			//std::cout << "yield_await_lifetime_test20 ";
			//using namespace tests;
			getLog();

			for (auto throws : { true, false })
			{
				{

					auto t = base_task20(throws);
					test_task<test_value> r = yield_await20(std::move(t));

					r.h.resume();

					log.emplace_back("mid", nullptr);
					if (!throws)
						r.h.resume();
				}

				auto log = getLog();

				if (!throws)
				{

					std::vector<Rec> exp{
						{"new promise_type", 0, 0},
						{"new promise_type", 1, 0},
						{"outter start", 2, 0},
						{"new task_awaitable", 3, 0},
						{"inner return", 2, 0},
						{"new test_value", 4, 0},
						{"mov test_value", 5, 4},
						{"del test_value", 4, 0},
						{"new yield_awaitable", 6, 0},
						{"mid", 2, 0},
						{"del yield_awaitable", 6, 0},
						{"del task_awaitable", 3, 0},
						{"del promise_type", 1, 0},
						{"del promise_type", 0, 0},
						{"del test_value", 5, 0}
					};
					//print(log);
					if (!(log == exp))
					{
						print(log, exp);
					}
					test_assert(log == exp);
				}
				else
				{
					std::vector<Rec> exp{
						{"new promise_type", 0, 0},
						{"new promise_type", 1, 0},
						{"outter start", 2, 0},
						{"new task_awaitable", 3, 0},
						{"inner throw", 2, 0},
						{"del task_awaitable", 3, 0},
						{"mid", 2, 0},
						{"del promise_type", 1, 0},
						{"del promise_type", 0, 0}
					};
					//std::cout << "n";
					//print(log);
					test_assert(log == exp);
				}
			}

			//std::cout << "      passed " << std::endl;;
		}
#else
		void yield_await_lifetime_test20()
		{}
#endif

		void store_as_t_lifetime_test()
		{
			int test_lvalue;
			static_assert(std::is_same<store_as_t<decltype(as_reference(test_lvalue)), decltype(test_lvalue)>, int&>::value, "");
			int& test_lvalue_ref = test_lvalue;
			static_assert(std::is_same<store_as_t<decltype(as_reference(test_lvalue_ref)), decltype(test_lvalue_ref)>, int&>::value, "");

			static_assert(std::is_same < store_as_t<decltype(as_reference(int{})), decltype(int{}) > , int > ::value, "");

			auto foo = []() -> int {return {}; };
			static_assert(std::is_same < store_as_t<decltype(as_reference(foo())), decltype(foo()) >, int> ::value, "");

			auto bar = [&]() -> int&& {return std::move(test_lvalue); };
			static_assert(std::is_same < store_as_t<decltype(as_reference(bar())), decltype(bar()) >, int&&> ::value, "");


			auto tar = [&]() -> int& {return (test_lvalue); };
			static_assert(std::is_same < store_as_t<decltype(as_reference(tar())), decltype(tar()) >, int&> ::value, "");
		}

		test_task<test_value> yield_await14(test_task<test_value> a) {
			MC_BEGIN(test_task<test_value>, awaitable = std::move(a));
			log.emplace_back("outter start", nullptr);

			//co_yield co_await std::forward<test_task<test_value>>(awaitable);
			//MC_AWAIT(std::forward<test_task<test_value>>(awaitable));
			MC_YIELD_AWAIT(std::forward<test_task<test_value>>(awaitable));

			log.emplace_back("outter end", nullptr);
			MC_END();
		};

		test_task<test_value> base_task14(bool throws)
		{
			MC_BEGIN(test_task<test_value>, throws);

			if (throws)
			{
				log.emplace_back("inner throw", nullptr);
				throw std::runtime_error("");
			}
			log.emplace_back("inner return", nullptr);
			MC_RETURN(test_value{});
			//co_return test_value{};

			MC_END();
		};



		void yield_await_lifetime_test14()
		{
			//std::cout << "yield_await_lifetime_test14    ";
			getLog();


			for (auto throws : { true, false })
			{
				{

					auto t = base_task14(throws);
					test_task<test_value> r = yield_await14(std::move(t));

					r.h.resume();

					log.emplace_back("mid", nullptr);
					if (!throws)
						r.h.resume();

				}
				auto log = getLog();

				if (!throws)
				{

					std::vector<Rec> exp{
						{"new promise_type", 0, 0},
						{"new promise_type", 1, 0},
						{"outter start", 2, 0},
						{"new task_awaitable", 3, 0},
						{"inner return", 2, 0},
						{"new test_value", 4, 0},
						{"mov test_value", 5, 4},
						{"del test_value", 4, 0},
						{"new yield_awaitable", 6, 0},
						{"mid", 2, 0},
						{"del yield_awaitable", 6, 0},
						{"del task_awaitable", 3, 0},
						{"del promise_type", 1, 0},
						{"del promise_type", 0, 0},
						{"del test_value", 5, 0}
					};
					test_assert(log == exp);
				}
				else
				{

					std::vector<Rec> exp{
						{"new promise_type", 0, 0},
						{"new promise_type", 1, 0},
						{"outter start", 2, 0},
						{"new task_awaitable", 3, 0},
						{"inner throw", 2, 0},
						{"del task_awaitable", 3, 0},
						{"mid", 2, 0},
						{"del promise_type", 1, 0},
						{"del promise_type", 0, 0}
					};
					test_assert(log == exp);
				}
			}
			//std::cout << "      passed " << std::endl;;
		}


		void await_lifetime_refOnly_test()
		{

			//std::cout << "await_lifetime_refOnly_test    ";

			RefOnly ref;
			//#define EXPRESSION NoCopyMove{}
			//#define EXPRESSION MovOnly{}
			//#define EXPRESSION mov()
			bool done = false;
			auto f = [&]() -> test_task<test_value> {
				MC_BEGIN(test_task<test_value>, &);


				MC_AWAIT(ref);

				done = true;
				MC_RETURN(test_value{});

				MC_END();
			};



			std::vector<Rec> exp{
				{"new promise_type",  0 } ,
				{"new test_value",	  1	},
				{"mov test_value",	  2, 1	},
				{"del test_value",	  1	},
				{"del promise_type" , 0	},
				{"del test_value",	  2}
			};

			getLog();
			done = false;
#if ENABLE_NO_MOVE_TEST
			auto g = [&]() ->test_task<test_value>
			{
				co_await ref;
				done = true;
				co_return test_value{};
			};
			{
				auto gg = g();
				gg.h.resume();

			}
			test_assert(done);

			auto log1 = getLog();
			if (!(log1 == exp))
			{
				print(log1, exp);
			}

			test_assert(log1 == exp);
#endif
			counter = 0;
			done = false;

			{
				auto r = f();
				r.h.resume();
			}
			test_assert(done);
			auto log2 = getLog();



			if (!(log2 == exp))
			{
				print(log2, exp);
			}
			test_assert(log2 == exp);
			//std::cout << "  passed " << std::endl;;

		}



		void await_lifetime_NoCopyMove_test()
		{
			// requires mandatory elision 
#if MACORO_CPP_20

			//std::cout << "await_lifetime_NoCopyMove_test    ";



			//#define EXPRESSION MovOnly{}
			//#define EXPRESSION mov()
			bool done = false;
			auto f = [&]() -> test_task<test_value> {
				MC_BEGIN(test_task<test_value>, &);


				MC_AWAIT(NoCopyMove{});

				done = true;
				MC_RETURN(test_value{});
				MC_END();
			};

			std::vector<Rec> exp{
				{"new promise_type", 0, 0},
				{"new NoCopyMove", 1, 0},
				{"new NoCopyMoveAwaitable", 2, 0},
				{"new NoCopyMoveAwaiter", 3, 0},
				{"del NoCopyMoveAwaiter", 3, 0},
				{"del NoCopyMoveAwaitable", 2, 0},
				{"del NoCopyMove", 1, 0},
				{"new test_value", 4, 0},
				{"mov test_value", 5, 4},
				{"del test_value", 4, 0},
				{"del promise_type", 0, 0},
				{"del test_value", 5, 0}
			};

			getLog();
			done = false;

#ifdef MACORO_CPP_20
			auto g = [&]() ->test_task<test_value>
			{
				co_await NoCopyMove{};
				done = true;
				co_return test_value{};
			};

			{
				auto gg = g();
				gg.h.resume();

			}
			test_assert(done);

			auto log1 = getLog();

			if (!(log1 == exp))
			{
				print(log1, exp);
			}

			test_assert(log1 == exp);
#endif
			done = false;
			{
				auto r = f();
				r.h.resume();
			}
			test_assert(done);
			auto log2 = getLog();


			if (!(log2 == exp))
			{
				print(log2, exp);
			}
			test_assert(log2 == exp);
			//std::cout << "  passed " << std::endl;;

#endif
		}



		void await_lifetime_MovOnly_test()
		{
			//std::cout << "await_lifetime_MovOnly_test    ";

			//#define EXPRESSION NoCopyMove{}
			//#define EXPRESSION MovOnly{}
			//#define EXPRESSION mov()
			bool done = false;
			auto f = [&]() -> test_task<test_value> {
				MC_BEGIN(test_task<test_value>, &);
				MC_AWAIT(MovOnly{});
				done = true;
				MC_RETURN(test_value{});
				MC_END();
			};
			std::vector<Rec> exp{
				{"new promise_type", 0, 0},
				{"new MovOnlyAwaiter", 1, 0},
				{"new MovOnlyAwaitable", 1, 0},
				{"new MovOnly", 1, 0},
				{"del MovOnly", 1, 0},
				{"del MovOnlyAwaitable", 1, 0},
				{"del MovOnlyAwaiter", 1, 0},
				{"new test_value", 2, 0},
				{"mov test_value", 3, 2},
				{"del test_value", 2, 0},
				{"del promise_type", 0, 0},
				{"del test_value", 3, 0}
			};
			getLog();
			done = false;

#ifdef MACORO_CPP_20

#if HAS_GCC_MOVE_AWAITER_BUG

			std::vector<Rec> exp20{
				{"new promise_type", 0, 0},
				{"new MovOnlyAwaiter", 1, 0},
				{"new MovOnlyAwaitable", 1, 0},
				{"new MovOnly", 1, 0},
				{"mov MovOnlyAwaiter", 2, 1},
				{"del MovOnlyAwaiter", 2, 0},
				{"del MovOnly", 1, 0},
				{"del MovOnlyAwaitable", 1, 0},
				{"del MovOnlyAwaiter", 1, 0},
				{"new test_value", 3, 0},
				{"mov test_value", 4, 3},
				{"del test_value", 3, 0},
				{"del promise_type", 0, 0},
				{"del test_value", 4, 0},
			};
#else
			auto exp20 = exp;
#endif
			auto g = [&]() ->test_task<test_value>
			{
				co_await MovOnly{};
				done = true;
				co_return test_value{};
			};

			{
				auto gg = g();
				gg.h.resume();

			}
			test_assert(done);

			auto log1 = getLog();
			//std::cout << "\n\n";
			//print(log1);

			if (!(log1 == exp20))
			{
				print(log1, exp20);
			}
			test_assert(log1 == exp20);

			//print(log1);
#endif
			done = false;
			{
				auto r = f();
				r.h.resume();
			}
			test_assert(done);
			auto log2 = getLog();


			if (!(log2 == exp))
			{
				print(log2, exp);
			}
			test_assert(log2 == exp);
			//std::cout << "  passed " << std::endl;;
		}



		void await_lifetime_fn_test()
		{
			//std::cout << "await_lifetime_fn_test    ";


			auto fn = []() -> MovOnly {return{}; };

			bool done = false;
			auto f = [&]() -> test_task<test_value> {
				MC_BEGIN(test_task<test_value>, &);
				MC_AWAIT(fn());
				done = true;
				MC_RETURN(test_value{});
				MC_END();
			};


			std::vector<Rec> exp{
				{"new promise_type", 0, 0},
				{"new MovOnlyAwaiter", 1, 0},
				{"new MovOnlyAwaitable", 1, 0},
				{"new MovOnly", 1, 0},
				{"del MovOnly", 1, 0},
				{"del MovOnlyAwaitable", 1, 0},
				{"del MovOnlyAwaiter", 1, 0},
				{"new test_value", 2, 0},
				{"mov test_value", 3, 2},
				{"del test_value", 2, 0},
				{"del promise_type", 0, 0},
				{"del test_value", 3, 0}
			};

			getLog();
			done = false;

#ifdef MACORO_CPP_20

#if HAS_GCC_MOVE_AWAITER_BUG
			std::vector<Rec> exp20{
				{"new promise_type", 0, 0},
				{ "new MovOnlyAwaiter", 1, 0 },
				{ "new MovOnlyAwaitable", 1, 0 },
				{ "new MovOnly", 1, 0 },
				{ "mov MovOnlyAwaiter", 2, 1 },
				{ "del MovOnlyAwaiter", 2, 0 },
				{ "del MovOnly", 1, 0 },
				{ "del MovOnlyAwaitable", 1, 0 },
				{ "del MovOnlyAwaiter", 1, 0 },
				{ "new test_value", 3, 0 },
				{ "mov test_value", 4, 3 },
				{ "del test_value", 3, 0 },
				{ "del promise_type", 0, 0 },
				{ "del test_value", 4, 0 }
			};
#else
			auto exp20 = exp;
#endif
			auto g = [&]() ->test_task<test_value>
			{
				co_await fn();
				done = true;
				co_return test_value{};
			};
			{
				auto gg = g();
				gg.h.resume();

			}
			test_assert(done);

			auto log1 = getLog();
			//print(log1);


			if (!(log1 == exp20))
			{
				print(log1, exp20);
			}

			test_assert(log1 == exp20);
#endif
			done = false;
			{
				auto r = f();
				r.h.resume();
			}
			test_assert(done);
			auto log2 = getLog();



			if (!(log2 == exp))
			{
				print(log2, exp);
			}
			test_assert(log2 == exp);
			//std::cout << "  passed " << std::endl;;
		}
	}
}