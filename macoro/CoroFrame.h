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
		InitialSuspend = std::numeric_limits<std::size_t>::max(),
		FinalSuspend = std::numeric_limits<std::size_t>::max() - 1
	};

	//std::coroutine_handle<void>;

	//template<typename Promise = void>
	//struct coroutine_handle;

	template<typename PromiseType = void>
	struct FrameBase;


//
//	template <>
//	struct coroutine_handle<void> {
//		constexpr coroutine_handle() noexcept = default;
//		constexpr coroutine_handle(nullptr_t) noexcept {}
//
//#ifdef MACORO_CPP_20
//		template<typename T>
//		coroutine_handle(const std::coroutine_handle<T>& h) noexcept {
//			_Ptr = h.address();
//			assert(((std::size_t)_Ptr & 1) == 0);
//		};
//		template<typename T>
//		coroutine_handle(std::coroutine_handle<T>&& h) noexcept {
//			_Ptr = h.address();
//			assert(((std::size_t)_Ptr & 1) == 0);
//		};
//#endif // MACORO_CPP_20
//
//		coroutine_handle& operator=(nullptr_t) noexcept {
//			_Ptr = nullptr;
//			return *this;
//		}
//
//		MACORO_NODISCARD constexpr void* address() const noexcept {
//			return _Ptr;
//		}
//
//		MACORO_NODISCARD static constexpr coroutine_handle from_address(void* const _Addr) noexcept { // strengthened
//			coroutine_handle _Result;
//			_Result._Ptr = _Addr;
//			return _Result;
//		}
//
//		constexpr explicit operator bool() const noexcept {
//			return _Ptr != nullptr;
//		}
//
//		MACORO_NODISCARD bool done() const noexcept;
//
//		void operator()() const;
//
//		void resume() const;
//
//		void destroy() const noexcept;
//
//	private:
//		void* _Ptr = nullptr;
//	};


	//template<>
	//struct coroutine_handle<void>
	//{
	//	void* _address = nullptr;

	//	constexpr coroutine_handle() noexcept = default;
	//	constexpr coroutine_handle(coroutine_handle&) noexcept = default;
	//	constexpr coroutine_handle(coroutine_handle&&) noexcept = default;
	//	constexpr coroutine_handle(coroutine_handle&&) noexcept = default;



	//	bool done() const noexcept;

	//	void resume();

	//	void destroy();

	//	void* address() const noexcept;
	//	static coroutine_handle from_address(void* address) noexcept;

	//protected: 
	//	coroutine_handle(void* p) noexcept { _address = p; }
	//};

//	template<typename Promise>
//	struct coroutine_handle : coroutine_handle<void>
//	{
//		coroutine_handle() = default;
//		coroutine_handle(const coroutine_handle&) = default;
//
//#ifdef MACORO_CPP_20
//		coroutine_handle(const std::coroutine_handle<Promise>& h) noexcept : coroutine_handle<void>({ h }) {};
//#endif // MACORO_CPP_20
//
//
//		Promise& promise() const ;
//		static coroutine_handle<Promise> from_promise(Promise& promise, coroutine_handle_type t = coroutine_handle_type::mocoro) noexcept;
//
//		static coroutine_handle<Promise> from_address(void* address) noexcept;
//
//
//	private:
//		coroutine_handle(void* p) :coroutine_handle<void>(p) { }
//	};

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


	
	template<typename Promise>
	Promise* _macoro_coro_promise(void* _address) noexcept
	{
#ifdef MACORO_CPP_20
		if ((std::size_t)_address & 1)
		{
			auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address ^ 1);
			auto ptrV = reinterpret_cast<FrameBase<void>*>(realAddr);
			auto ptr = static_cast<FrameBase<Promise>*>(ptrV);
			return ptr->promise;
		}
		else
		{
			return &std::coroutine_handle<Promise>::from_address(_address).promise();
		}
#else
		auto ptrV = reinterpret_cast<FrameBase<void>*>(_address);
		auto ptr = static_cast<FrameBase<Promise>*>(ptrV);
		return ptr->promise;
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
		if ((std::size_t)_address & 1)
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
#ifdef MACORO_CPP_20
		if ((std::size_t)_address & 1)
		{
			auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address ^ 1);
			return realAddr->resume(realAddr);
		}
		else
		{
			std::coroutine_handle<void>::from_address(_address).resume();
		}
#else
		_address->resume(_address);
#endif
	}

	inline void _macoro_coro_destroy(void* _address)noexcept
	{
#ifdef MACORO_CPP_20
		if ((std::size_t)_address & 1)
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