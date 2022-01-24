#pragma once
#include <cstdint>
#include <type_traits>
#include <typeinfo>

#include "Optional.h"
#include <cassert>

#include <cstddef>

namespace macoro
{




	// A location which can store a return value
	template<typename T>
	struct ReturnStorage;

	// void specializzation.
	template<>
	struct ReturnStorage<void>
	{
		// required by coroutine.
		void return_void() {}
		void* getValue() { return nullptr; }
	};

	// Stores a T inside and will give
	// back a void* to it.
	template<typename T>
	struct ReturnStorage
	{
		optional<T> mVal;

		// set/store the return value.
		void return_value(T&& t)
		{
			mVal = std::forward<T>(t);
		}

		// set/store the return value.
		void return_value(const T& t)
		{
			mVal = t;
		}

		// get a pointer the return value if it has one. 
		// Otherwise return null.
		void* getValue()
		{
			if (mVal)
				return &mVal.value();
			else
				return nullptr;
		}
	};

	enum class SuspensionPoint : std::size_t
	{
		// the initial suspend point
		InitialSuspend = std::numeric_limits<std::size_t>::max(),
		FinalSuspend = std::numeric_limits<std::size_t>::max() - 1
	};



	template<typename Promise = void>
	struct coroutine_handle;

	template<typename PromiseType = void>
	struct FrameBase;

	template<>
	struct coroutine_handle<void>
	{
		FrameBase<void>* _address = nullptr;

		bool done() const;

		void resume();

		void destroy();

		void* address() const;
		static coroutine_handle from_address(void* address);;
	};

	template<typename Promise>
	struct coroutine_handle : coroutine_handle<void>
	{
		coroutine_handle() = default;
		coroutine_handle(const coroutine_handle&) = default;
		coroutine_handle(FrameBase<void>* p) : coroutine_handle<void>({ p }) {};

		Promise& promise() const;
		static coroutine_handle<Promise> from_promise(Promise& promise);

		static coroutine_handle<Promise> from_address(void* address);
	};

	template<typename Awaiter, typename T>
	bool await_suspend(Awaiter& a, coroutine_handle<T> h,
		enable_if_t<
		has_void_await_suspend<Awaiter, coroutine_handle<T>>::value
		, monostate> = {})
	{
		a.await_suspend(h);
		return true;
	}

	template<typename Awaiter, typename T>
	auto await_suspend(Awaiter& a, coroutine_handle<T> h,
		enable_if_t<
		!has_void_await_suspend<Awaiter, coroutine_handle<T>>::value
		, monostate> = {})
	{
		static_assert(
			std::is_same_v<decltype(a.await_suspend(h)), bool>,
			"await_suspend() must return 'void' or 'bool'.");

		return a.await_suspend(h);
	}


	template<>
	struct FrameBase<void>
	{
		FrameBase() = default;
		FrameBase(const FrameBase<void>&) = delete;
		FrameBase(FrameBase<void>&&) = delete;

		// The current suspend location of the coroutines
		SuspensionPoint _suspension_idx_ = SuspensionPoint::InitialSuspend;

		bool done() const
		{
			return _suspension_idx_ == SuspensionPoint::FinalSuspend;
		}

		void (*resume)(FrameBase<void>* ptr) = nullptr;
		void (*destroy)(FrameBase<void>* ptr) = nullptr;
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
		SuspensionPoint getSuspendPoint() const { return _suspension_idx_; }

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

		static void resume_impl(FrameBase<void>* ptr)
		{
			auto self = static_cast<Frame<LambdaType,Promise>*>(ptr);
			(*self)(static_cast<FrameBase<Promise>*>(self));
		}

		static void destroy_impl(FrameBase<void>* ptr)
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


	inline bool coroutine_handle<void>::done() const { return _address->done(); }
	inline void coroutine_handle<void>::resume() { _address->resume(_address); }
	inline void coroutine_handle<void>::destroy() { _address->destroy(_address); }
	inline void* coroutine_handle<void>::address() const { return _address; }
	inline coroutine_handle<void> coroutine_handle<void>::from_address(void* address) { return { (FrameBase<void>*)address }; }
	template<typename Promise>
	inline Promise& coroutine_handle<Promise>::promise() const
	{
		auto ptr = reinterpret_cast<FrameBase<Promise>*>(_address);
		return ptr->promise;
	}
	template<typename Promise>
	inline coroutine_handle<Promise> coroutine_handle<Promise>::from_promise(Promise& promise)
	{
		std::size_t offset = ((std::size_t) & reinterpret_cast<char const volatile&>((((FrameBase<Promise>*)0)->promise)));
		auto frame = (FrameBase<Promise>*)((char*)&promise - offset);
		auto base = static_cast<FrameBase<void>*>(frame);
		return from_address(base);
	}

	template<typename Promise>
	inline coroutine_handle<Promise> coroutine_handle<Promise>::from_address(void* address)
	{
		return { (FrameBase<void>*)address };
	}
}