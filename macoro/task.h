#include "macoro/coroutine_handle.h"
#include "macoro/coro_frame.h"
#include "macoro/awaiter.h"
#include "macoro/variant.h"
#include "macoro/type_traits.h"
namespace macoro
{
	template<typename T>
	class task;

	namespace impl
	{

		template<typename T>
		struct task_promise 
		{
			using value_type = T;
			using reference_type = T&;
			using rvalue_type = T&&;
			using exception_type = std::exception_ptr;
			using handle_type = coroutine_handle<task_promise<value_type>>;

			coroutine_handle<> cont;
			template<typename C>
			void set_continuation(C&& c) { cont = c; }
			suspend_always initial_suspend() const noexcept { return {}; }
			continuation_awaiter<> final_suspend() const noexcept { return { cont }; }

			variant<empty_state, value_type, exception_type> storage;

			task<T> get_return_object() noexcept;
			task<T> macoro_get_return_object() noexcept;

			void unhandled_exception() noexcept
			{
				storage = std::current_exception();
			}

			template<typename TT>
			void return_value(TT&& val) noexcept(std::is_nothrow_constructible<T, TT&&>::value)
			{
				static_assert(std::is_convertible<TT&&, T>::value, "the task value_type can not be constructed from the co_return expression.");
				storage.emplace<1>(std::forward<TT>(val));
			}

			reference_type get()&
			{
				if (storage.index() == 2)
					std::rethrow_exception(v_get<2>(storage));
				assert(storage.index() == 1);
				return v_get<1>(storage);
			}

			rvalue_type get()&&
			{
				if (storage.index() == 2)
					std::rethrow_exception(v_get<2>(storage));
				assert(storage.index() == 1);
				return std::move(v_get<1>(storage));
			}
		};


		template<typename T>
		struct task_promise<T&>
		{

			using value_type = T&;
			using reference_type = T&;
			using pointer_type = T*;
			using exception_type = std::exception_ptr;
			using handle_type = coroutine_handle<task_promise<value_type>>;

			coroutine_handle<> cont;
			template<typename C>
			void set_continuation(C&& c) { cont = c; }
			suspend_always initial_suspend() const noexcept { return {}; }
			continuation_awaiter<> final_suspend() const noexcept { return { cont }; }

			task<value_type> get_return_object() noexcept;
			task<value_type> macoro_get_return_object() noexcept;

			variant<empty_state, pointer_type, exception_type> storage;

			void unhandled_exception() noexcept
			{
				storage = std::current_exception();
			}

			void return_value(reference_type val) noexcept
			{
				storage.emplace<1>(&val);
			}

			reference_type get()
			{
				if (storage.index() == 2)
					std::rethrow_exception(v_get<2>(storage));
				assert(storage.index() == 1);
				return *v_get<1>(storage);
			}
		};

		template<>
		struct task_promise<void> 
		{
			using value_type = void;
			using reference_type = void;
			using exception_type = std::exception_ptr;
			using handle_type = coroutine_handle<task_promise<value_type>>;

			coroutine_handle<> cont;

			template<typename C>
			void set_continuation(C&&c) { cont = c; }

			suspend_always initial_suspend() const noexcept { return {}; }
			continuation_awaiter<> final_suspend() const noexcept { return { cont }; }

			task<value_type> get_return_object() noexcept;
			task<value_type> macoro_get_return_object() noexcept;

			exception_type storage;

			void unhandled_exception() noexcept
			{
				storage = std::current_exception();
			}

			void return_void() noexcept { }

			void get()
			{
				if (storage)
					std::rethrow_exception(storage);
			}
		};

		template<typename T>
		decltype(auto) optMove(std::true_type, T&& t)
		{
			return std::move(t.get());
		}

		template<typename T>
		decltype(auto) optMove(std::false_type, T&& t)
		{
			return t.get();
		}

	}

	template<typename T>
	class task
	{
	public:
		using promise_type = impl::task_promise<T>;
		using value_type = typename impl::task_promise<T>::value_type;
		using reference_type = typename impl::task_promise<T>::reference_type;
		using exception_type = typename impl::task_promise<T>::exception_type;
		using handle_type = typename impl::task_promise<T>::handle_type;

#ifdef MACORO_CPP_20
		static_assert(has_set_coninuation_member<handle_type, std::coroutine_handle<>>::value, "pomise_type must has a set_continuation method");
#endif
		static_assert(has_set_coninuation_member<handle_type, coroutine_handle<>>::value, "pomise_type must has a set_continuation method");
		static_assert(has_set_coninuation_member<handle_type, coroutine_handle<noop_coroutine_handle>>::value, "pomise_type must has a set_continuation method");

		handle_type handle;

		task()
		{
			//std::cout << "new t  " << this  << std::endl;
		}
		task(const task&) = delete;
		task(task&& t) : handle(std::exchange(t.handle, std::nullptr_t{})) {
		
			//std::cout << "move t " << this  << std::endl;
		}
		task& operator=(const task&) = delete;
		task& operator=(task&& t) { 
			//std::cout << "move= t " << this  << std::endl;
			~task();
			handle = std::exchange(t.handle, std::nullptr_t{});
		};
		~task()
		{
			//std::cout << "destroy t " << this  << std::endl;
			if (handle)
				handle.destroy();
		}

		bool ready() const { return !handle || handle.done(); }

		explicit operator bool() const { return ready(); }

		template<typename move>
		struct awaitable : impl::continuation_awaiter<handle_type, true>
		{
			awaitable() { 
				//std::cout << "new a  " << this << " " << move::value << std::endl;
			}

			awaitable(handle_type h) : impl::continuation_awaiter<handle_type, true>{ h } {
				//std::cout << "new' a " << this << " " << move::value << std::endl;
			}
			awaitable(awaitable&& h) : impl::continuation_awaiter<handle_type, true>(h.cont) {
				//std::cout << "move a " << this << " " << move::value << std::endl;
			}

			~awaitable()  {
				//std::cout << "destroy a " << this << " " << move::value << std::endl;
			}


			using impl::continuation_awaiter<handle_type, true>::await_ready;
			using impl::continuation_awaiter<handle_type, true>::await_suspend;
			decltype(auto) await_resume() {
				if (!this->cont)
					throw broken_promise{ MACORO_LOCATION };

				using move2 = typename std::conditional<std::is_same<decltype(this->cont.promise()), void>::value, std::false_type, move>::type;

				return impl::optMove(move2{}, this->cont.promise());
			}
		};

		auto operator_co_await() const& noexcept
		{
			return awaitable<std::false_type>{ handle };
		}

		auto operator_co_await() const&& noexcept
		{
			return awaitable<std::true_type>{ handle };
		}
#ifdef MACORO_CPP_20
		auto operator co_await() const& noexcept
		{
			return awaitable<std::false_type>{ handle };
		}

		auto operator co_await() const&& noexcept
		{
			return awaitable<std::true_type>{ handle };
		}
#endif
	private:
		friend class promise_type;

		task(handle_type h)
			:handle(h)
		{
			//std::cout << "new' t  " << this << std::endl;

		}


	};



	namespace impl
	{

		template<typename T>
		task<T> task_promise<T>::get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::std) }; }
		template<typename T>
		task<T> task_promise<T>::macoro_get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::mocoro) }; }

		template<typename T>
		task<T&> task_promise<T&>::get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::std) }; }
		template<typename T>
		task<T&> task_promise<T&>::macoro_get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::mocoro) }; }

		inline task<void> task_promise<void>::get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::std) }; }
		inline task<void> task_promise<void>::macoro_get_return_object() noexcept { return { handle_type::from_promise(*this, coroutine_handle_type::mocoro) }; }



	}
}
