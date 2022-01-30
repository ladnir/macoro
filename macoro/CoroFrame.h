#pragma once
#include <cstdint>
#include <type_traits>
#include <typeinfo>

#include "Optional.h"
#include <cassert>

#include "config.h"
#include "coroutine_handle.h"

#ifdef MACORO_CPP_20
#include <coroutine>
#include <variant>
#endif

namespace macoro
{
	enum class SuspensionPoint : std::size_t
	{
		Init = 0,
		InitialSuspend = std::numeric_limits<std::size_t>::max(),
		FinalSuspend = std::numeric_limits<std::size_t>::max() - 1
	};

	template<typename PromiseType = void>
	struct FrameBase;

	template<typename nested_promise>
	struct std_handle_adapter
	{
		struct final_awaiter
		{
			std::coroutine_handle<void> continuation;

			bool await_ready() noexcept { return false; }
			std::coroutine_handle<void> await_suspend(std::coroutine_handle<void> v) noexcept { return continuation; }
			void await_resume() noexcept {}

		};

		struct promise_type
		{
			suspend_always initial_suspend() noexcept { return {}; }
			final_awaiter final_suspend() const noexcept { return { continuation }; }

			void unhandled_exception()
			{
				std::cout << "not impl. " << __FILE__ << ":" << __LINE__ << std::endl;
				std::terminate();
			}

			coroutine_handle<nested_promise> handle;
			std::coroutine_handle<void> continuation;


			std_handle_adapter get_return_object() { return { this }; }

			~promise_type()
			{
				if (handle)
					handle.destroy();
			}

			suspend_always yield_value(std::coroutine_handle<void> cont)
			{
				continuation = cont;
				return {};
			}

			void return_value(std::coroutine_handle<void> cont) {
				continuation = cont;
			}
		};

		using nested_promise_type = nested_promise;

		promise_type* promise;
	};


	template<typename T>
	struct coroutine_handle_traits
	{
	};

	template<typename T>
	struct coroutine_handle_traits<std::coroutine_handle<T>>
	{
		using promise_type = T;
	};

	template<typename T>
	struct coroutine_handle_traits<coroutine_handle<T>>
	{
		using promise_type = T;
	};

	template<typename handle>
	struct await_suspend_t
	{
		handle value;

		explicit operator bool() const
		{
			return true;
		}

		auto get_handle()
		{
			return value;
		}
	};

	template<>
	struct await_suspend_t<bool>
	{
		bool value;

		explicit operator bool() const
		{
			return value;
		}
		coroutine_handle<> get_handle()
		{
			return noop_coroutine();
		}
	};

	template<typename Awaiter, typename T>
	await_suspend_t<bool> await_suspend(Awaiter& a, coroutine_handle<T> h,
		enable_if_t<
		has_void_await_suspend<Awaiter, coroutine_handle<T>>::value
		, monostate> = {})
	{
		a.await_suspend(h);
		return { true };
	}

	template<typename Awaiter, typename T>
	await_suspend_t<bool> await_suspend(Awaiter& a, coroutine_handle<T> h,
		enable_if_t<
		has_bool_await_suspend<Awaiter, coroutine_handle<T>>::value
		, monostate> = {})
	{
		return { a.await_suspend(h) };
	}


	template<typename Awaiter, typename T>
	auto await_suspend(Awaiter& a, coroutine_handle<T> h,
		enable_if_t<
		!has_void_await_suspend<Awaiter, coroutine_handle<T>>::value &&
		!has_bool_await_suspend<Awaiter, coroutine_handle<T>>::value
		, monostate> = {})
	{

		static_assert(
			std::is_same<decltype(a.await_suspend(h)), bool>::value ||
			std::is_convertible<decltype(a.await_suspend(h)), macoro::coroutine_handle<>>::value,
			"await_suspend() must return 'void', 'bool', or convertible to macoro::coroutine_handle<>.");

		using promise_type = typename coroutine_handle_traits<decltype(a.await_suspend(h))>::promise_type;

		return await_suspend_t<coroutine_handle<promise_type>>{ a.await_suspend(h) };
	}

	template<>
	struct FrameBase<void>
	{
		FrameBase() = default;
		FrameBase(const FrameBase<void>&) = delete;
		FrameBase(FrameBase<void>&&) = delete;

		// The current suspend location of the coroutines
		SuspensionPoint _suspension_idx_ = SuspensionPoint::Init;

		bool done() const
		{
			return _suspension_idx_ == SuspensionPoint::FinalSuspend;
		}

		std::coroutine_handle<void>(*get_std_handle)(FrameBase<void>* ptr) = nullptr;
		coroutine_handle<void>(*resume)(FrameBase<void>* ptr) = nullptr;
		void (*destroy)(FrameBase<void>* ptr) noexcept = nullptr;
	};


	// The object which allows lambda based coroutines
	// to set their current suspend point, and return a
	// value. This base object has to be independent from
	// the lambda function so we can pass it in as a parameter.
	template<typename PromiseType>
	struct FrameBase
		: public FrameBase<void>
	{
		using promise_type = PromiseType;

#ifndef NDEBUG
		const std::type_info* _awaiter_typeid_ = nullptr;
#endif

		promise_type promise;

		// a flag to make sure the suspend index is set.
		bool _suspension_idx_set_ = false;

		// Return the current suspend location of the coroutines.
		SuspensionPoint getSuspendPoint() const {
			return _suspension_idx_;
		}

		// set the current suspend location of the coroutines. 
		// Must be called if the coroutines does not complete.
		void setSuspendPoint(SuspensionPoint idx) {
			_suspension_idx_ = idx;
			_suspension_idx_set_ = true;
		}

		// The deleter for the current awaitable.
		void (*_awaiter_deleter)(void* ptr) = nullptr;

		// The storage of the current co_await. 
		void* _awaiter_ptr = nullptr;
		void* _storage_ptr = nullptr;


		FrameBase()
		{
			FrameBase<void>::get_std_handle = &FrameBase::get_std_handle_void;
		}

		~FrameBase()
		{
			if (_storage_ptr)
				_awaiter_deleter(_storage_ptr);
		}

		template<typename AwaiterFor>
		struct AwaiterStorage
		{

			using Expr = typename  AwaiterFor::expr;
			using Awaitable = typename  AwaiterFor::awaitable;
			using value_type = typename AwaiterFor::value_type;

			Expr value;
			Awaitable awaitable;
			value_type awaiter;

			AwaiterStorage(promise_type& promise, Expr&& val)
				: value(std::forward<Expr>(val))
				, awaitable(get_awaitable(promise, std::forward<Expr>(value)))
				, awaiter(get_awaiter(static_cast<Awaitable>(awaitable)))
			{}
		};

		template<typename Expr>
		typename awaiter_for<promise_type, Expr>::reference constructAwaiter(Expr&& value)
		{
			if (_awaiter_ptr)
				std::terminate();

			using AwaiterFor = awaiter_for<promise_type, Expr>;
#ifndef NDEBUG
			_awaiter_typeid_ = &typeid(AwaiterFor);
#endif
			// get the awaiter and allocate it on the heap.
			//auto ret = new Storage(this->await_transform(std::forward<Awaitable>(c)));
			auto storage_ptr = new AwaiterStorage<AwaiterFor>(promise, std::forward<Expr>(value));
			_storage_ptr = storage_ptr;

			// construct the that is used if our destructor is called.
			_awaiter_deleter = [](void* ptr)
			{
				auto a = (AwaiterStorage<AwaiterFor>*)(ptr);
				delete a;
			};

			_awaiter_ptr = &storage_ptr->awaiter;
			return storage_ptr->awaiter;
		}

		template<typename AwaiterFor>
		void destroyAwaiter()
		{
#ifndef NDEBUG
			if (!_awaiter_ptr)
				std::terminate();

			if (_awaiter_typeid_ != &typeid(AwaiterFor))
				terminate();
#endif
			auto a = (AwaiterStorage<AwaiterFor>*)(_storage_ptr);
			delete a;

			_awaiter_ptr = nullptr;
			_storage_ptr = nullptr;
		}

		// Return the current awaiter. We assumer the caller
		// is providing the correct awaiter type. This is validated
		// in debug builds.
		template<typename AwaiterFor>
		typename AwaiterFor::reference getAwaiter()
		{
			assert(_awaiter_ptr);
			auto ptr = (typename AwaiterFor::pointer)_awaiter_ptr;
#ifndef NDEBUG
			if (_awaiter_typeid_ != &typeid(AwaiterFor))
				terminate();
#endif
			return *ptr;
		}

#ifdef MACORO_CPP_20

		using adapter = std_handle_adapter<promise_type>;
		adapter make_adapter()
		{
			auto h = resume(this);
			while (true)
			{
				assert(h);
				bool noop = h == noop_coroutine();
				if (!noop && h.is_std() == false)
				{
					auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)h.address() ^ 1);
					auto _address = realAddr->resume(realAddr).address();
					h = coroutine_handle<void>::from_address(_address);
					//h.resume();
				}
				else
				{
					std::coroutine_handle<void> r;
					r = h.std_cast();

					if (done())
					{
						co_return r;
					}
					else
					{
						co_yield r;
					}
				}
			}
		}

		adapter std_adapter;
		std::coroutine_handle<typename adapter::promise_type> std_handle;

		std::coroutine_handle<typename adapter::promise_type> get_std_handle()
		{
			if (!std_handle)
			{
				auto handle = coroutine_handle<promise_type>::from_promise(promise, coroutine_handle_type::mocoro);
				std_adapter = make_adapter();
				std_adapter.promise->handle = handle;
				std_handle = std::coroutine_handle<typename adapter::promise_type>::from_promise(*std_adapter.promise);
			}

			return std_handle;
		}

		static std::coroutine_handle<void> get_std_handle_void(FrameBase<void>* basev)
		{
			auto base = reinterpret_cast<FrameBase<promise_type>*>(basev);
			return base->get_std_handle();
		}
#endif

	};

	// Frame is a Resumable which invokes a lambda each
	// time it is resumed. This lambda takes as input a  
	// FrameBase<ReturnType>* which allows the lambda
	// to tell the runtime/Resumable if the protocol is done 
	// or should be suspended, etc. The return value is also set
	// via FrameBase<ReturnType>. The Resumable::resume_
	// function is implemented by ProtoResumeCRTP. resume_ will 
	// cal resumeHandle and we will in turn vall the lambda function
	// and pass in ourself (FrameBase<ReturnType>) as the parameter.
	template<typename LambdaType, typename Promise>
	struct Frame :
		public LambdaType,
		public FrameBase<Promise>
	{
		using LambdaType::operator();
		using lambda_type = LambdaType;

		static coroutine_handle<void> resume_impl(FrameBase<void>* ptr)
		{
			auto self = static_cast<Frame<LambdaType, Promise>*>(ptr);
			return (*self)(static_cast<FrameBase<Promise>*>(self));
		}

		static void destroy_impl(FrameBase<void>* ptr) noexcept
		{
			auto self = static_cast<Frame<LambdaType, Promise>*>(ptr);
			delete self;
		}

		Frame(LambdaType&& l)
			: LambdaType(std::forward<LambdaType>(l))
		{
			FrameBase<void>::resume = &Frame::resume_impl;
			FrameBase<void>::destroy = &Frame::destroy_impl;
		}
	};

	// makes a Proto from the given lambda. The lambda
	// should take as input a FrameBase<ReturnType>* 
	template<typename Promise, typename LambdaType>
	inline Frame<LambdaType, Promise>* makeFrame(LambdaType&& l)
	{
		return new Frame<LambdaType, Promise>(std::forward<LambdaType>(l));
	}

#ifdef MACORO_CPP_20
	inline bool _macoro_coro_is_std(void* _address) noexcept
	{
		return !bool((std::size_t)_address & 1);
	}
#endif
	template<typename Promise>
	Promise* _macoro_coro_promise(void* _address) noexcept
	{
#ifdef MACORO_CPP_20
		if (!_macoro_coro_is_std(_address))
		{
			auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address ^ 1);
			auto ptrV = reinterpret_cast<FrameBase<void>*>(realAddr);
			auto ptr = static_cast<FrameBase<Promise>*>(ptrV);
			return &ptr->promise;
		}
		else
		{
			return &std::coroutine_handle<Promise>::from_address(_address).promise();
		}
#else
		auto ptrV = reinterpret_cast<FrameBase<void>*>(_address);
		auto ptr = static_cast<FrameBase<Promise>*>(ptrV);
		return &ptr->promise;
#endif
	}

	template<typename Promise>
	void* _macoro_coro_frame(Promise* promise, coroutine_handle_type te)noexcept
	{
#ifdef MACORO_CPP_20
		if (te == coroutine_handle_type::mocoro)
		{
			std::size_t offset = ((std::size_t) & reinterpret_cast<char const volatile&>((((FrameBase<Promise>*)0)->promise)));
			auto frame = (FrameBase<Promise>*)((char*)promise - offset);
			auto base = static_cast<FrameBase<void>*>(frame);
			auto tag = (std::size_t)base;
			assert((tag & 1) == 0);
			tag ^= 1;
			base = reinterpret_cast<FrameBase<void>*>(tag);
			return base;
		}
		else
		{
			auto ret = std::coroutine_handle<Promise>::from_promise(*promise).address();
			assert(((std::size_t)ret & 1) == 0);
			return ret;
		}
#else
		std::size_t offset = ((std::size_t) & reinterpret_cast<char const volatile&>((((FrameBase<Promise>*)0)->promise)));
		auto frame = (FrameBase<Promise>*)((char*)promise - offset);
		auto base = static_cast<FrameBase<void>*>(frame);
		return base;
#endif
	}

	inline bool _macoro_coro_done(void* _address)noexcept
	{
#ifdef MACORO_CPP_20
		if (!_macoro_coro_is_std(_address))
		{
			auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address ^ 1);
			return realAddr->done();
		}
		return std::coroutine_handle<void>::from_address(_address).done();
#else
		return _address->done();
#endif
	}

	inline void _macoro_coro_resume(void* _address)
	{
		while (true)
		{
#ifdef MACORO_CPP_20

			if (!_macoro_coro_is_std(_address))
			{
				auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address ^ 1);
				_address = realAddr->resume(realAddr).address();
				assert(_address);
				if (_address == noop_coroutine().address())
					return;
			}
			else
			{
				std::coroutine_handle<void>::from_address(_address).resume();
				return;
			}
#else
			auto symTransfer = _address->resume(_address);
			if (_address == nullptr)
				return;
#endif
		}
	}

	inline void _macoro_coro_destroy(void* _address)noexcept
	{
#ifdef MACORO_CPP_20
		if (!_macoro_coro_is_std(_address))
		{
			auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address ^ 1);
			return realAddr->destroy(realAddr);
		}
		else
		{
			std::coroutine_handle<void>::from_address(_address).destroy();
		}
#else
		_address->destroy(_address);
#endif
	}

	inline std::coroutine_handle<void> coroutine_handle<void>::std_cast() const
	{
		if (*this == noop_coroutine())
		{
			return std::noop_coroutine();
		}
		if (!is_std())
		{
			auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_Ptr ^ 1);
			return realAddr->get_std_handle(realAddr);
		}
		else
		{
			return std::coroutine_handle<void>::from_address(_Ptr);
		}
	}

	template<class _Promise>
	inline std::coroutine_handle<void> coroutine_handle<_Promise>::std_cast() const
	{

		if (*this == noop_coroutine())
		{
			return std::noop_coroutine();
		}
		if (!_macoro_coro_is_std(_Ptr))
		{
			auto realAddr = reinterpret_cast<FrameBase<_Promise>*>((std::size_t)_Ptr ^ 1);
			return realAddr->get_std_handle();
		}
		else
		{
			return std::coroutine_handle<_Promise>::from_address(_Ptr);
		}
	}



	//inline bool coroutine_handle<void>::done() const noexcept {

	//}
	//inline void coroutine_handle<void>::resume() {

	//}
	//inline void coroutine_handle<void>::destroy() { 

	//}
	//inline void* coroutine_handle<void>::address() const noexcept { return _address; }
	//inline coroutine_handle<void> coroutine_handle<void>::from_address(void* address)noexcept { return {  address }; }

	//template<typename Promise>
	//inline Promise& coroutine_handle<Promise>::promise() const
	//{

	//}
	//template<typename Promise>
	//inline coroutine_handle<Promise> coroutine_handle<Promise>::from_promise(Promise& promise, coroutine_handle_type te) noexcept
	//{

	//}

	//template<typename Promise>
	//inline coroutine_handle<Promise> coroutine_handle<Promise>::from_address(void* address) noexcept
	//{
	//	return { address };
	//}
}