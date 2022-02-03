#pragma once
#include <cstdint>
#include "type_traits.h"
#include <typeinfo>

#include "optional.h"
#include <cassert>

#include "macoro/config.h"
#include "macoro/coroutine_handle.h"
#include <iostream>
#include <array>
#include <vector>

#ifdef MACORO_CPP_20
#include <coroutine>
#endif

namespace macoro
{
	enum class SuspensionPoint : std::size_t
	{
		InitialSuspendBegin = std::numeric_limits<std::size_t>::max(),
		InitialSuspend = std::numeric_limits<std::size_t>::max() -1,
		FinalSuspend = std::numeric_limits<std::size_t>::max() - 2
	};

	template<typename PromiseType = void>
	struct FrameBase;

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
		, empty_state> = {}) noexcept(noexcept(a.await_suspend(h)))
	{
		a.await_suspend(h);
		return { true };
	}

	template<typename Awaiter, typename T>
	await_suspend_t<bool> await_suspend(Awaiter& a, coroutine_handle<T> h,
		enable_if_t<
		has_bool_await_suspend<Awaiter, coroutine_handle<T>>::value
		, empty_state> = {}) noexcept(noexcept(await_suspend_t<bool>{ a.await_suspend(h) }))
	{
		return { a.await_suspend(h) };
	}


	template<typename Awaiter, typename T>
	auto await_suspend(Awaiter& a, coroutine_handle<T> h,
		enable_if_t<
		!has_void_await_suspend<Awaiter, coroutine_handle<T>>::value &&
		!has_bool_await_suspend<Awaiter, coroutine_handle<T>>::value
		, empty_state> = {}) noexcept(noexcept(await_suspend_t<coroutine_handle<typename coroutine_handle_traits<decltype(a.await_suspend(h))>::promise_type>>{ a.await_suspend(h) }))
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
		SuspensionPoint _suspension_idx_ = SuspensionPoint::InitialSuspendBegin;

		bool done() const
		{
			return _suspension_idx_ == SuspensionPoint::FinalSuspend;
		}
#ifdef MACORO_CPP_20
		std::coroutine_handle<void>(*get_std_handle)(FrameBase<void>* ptr) = nullptr;
#endif
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



		promise_type promise;


		bool _initial_suspend_await_resumed_called_ = false;

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

		struct awaiters
		{
			size_t awaiter_idx;
#ifndef NDEBUG
			const std::type_info* _awaiter_typeid_ = nullptr;
#endif
			// The deleter for the current awaitable.
			void (*_awaiter_deleter)(void* ptr) = nullptr;
			void* _storage_ptr = nullptr;

			// The storage of the current co_await. 
			void* _awaiter_ptr = nullptr;
		};
		std::vector<awaiters> awaiters;

		
		~FrameBase()
		{
			while (awaiters.size())
			{
				awaiters.back()._awaiter_deleter(awaiters.back()._storage_ptr);
				awaiters.pop_back();
			}
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
		typename awaiter_for<promise_type, Expr&&> constructAwaiter2(Expr&& value)
		{
			return {};
		}

		template<typename Expr>
		typename awaiter_for<promise_type, Expr&&>::reference constructAwaiter(Expr&& value, size_t awaiterIdx)
		{
			//if (_awaiter_ptr)
			//{
			//	_awaiter_deleter(_storage_ptr);
			//	//std::terminate();
			//}

			//std::cout << "is_lvalue_reference_v   " << std::is_lvalue_reference_v<Expr&&> << std::endl;
			//std::cout << "is_rvalue_reference_v   " << std::is_rvalue_reference_v<Expr&&> << std::endl;


			using AwaiterFor = awaiter_for<promise_type, Expr&&>;

			for (auto& aa : awaiters)
				assert(aa.awaiter_idx != awaiterIdx);

			awaiters.emplace_back();
			auto& d = awaiters.back();
#ifndef NDEBUG
			d._awaiter_typeid_ = &typeid(AwaiterFor);
#endif
			d.awaiter_idx = awaiterIdx;

			// get the awaiter and allocate it on the heap.
			//auto ret = new Storage(this->await_transform(std::forward<Awaitable>(c)));
			auto storage_ptr = new AwaiterStorage<AwaiterFor>(promise, std::forward<Expr>(value));
			d._storage_ptr = storage_ptr;

			// construct the that is used if our destructor is called.
			d._awaiter_deleter = [](void* ptr)
			{
				auto a = (AwaiterStorage<AwaiterFor>*)(ptr);
				delete a;
			};

			d._awaiter_ptr = &storage_ptr->awaiter;
			return storage_ptr->awaiter;
		}

		template<typename AwaiterFor>
		struct DestroyAwaiterRaii
		{
			FrameBase<promise_type>* frame;
			size_t idx;

			DestroyAwaiterRaii(FrameBase<promise_type>* f, size_t i)
				: frame(f)
				, idx(i)
			{}

			DestroyAwaiterRaii(const DestroyAwaiterRaii&) = delete;


			~DestroyAwaiterRaii()
			{
#ifndef NDEBUG
				//std::cout << "is_rvalue_reference_v * " << std::is_rvalue_reference_v<AwaiterFor::expr> << std::endl;

				//auto d = frame->awaiters.end();
				//while (--d->awaiter_idx != idx)
				//	assert(d != frame->awaiters.begin());
				assert(frame->awaiters.size());
				auto d = --frame->awaiters.end();
				while (d->awaiter_idx != idx)
				{
					assert(d != frame->awaiters.begin());
					--d;
				}


				if (!d->_awaiter_ptr)
					std::terminate();

				if (d->_awaiter_typeid_ != &typeid(AwaiterFor))
					std::terminate();
#endif
				//if (!std::is_rvalue_reference_v<AwaiterFor::expr>)
				{

					auto a = (AwaiterStorage<AwaiterFor>*)(d->_storage_ptr);
					delete a;

					auto& v = frame->awaiters;
					std::swap(*d, *--v.end());
					v.pop_back();
					//v.erase(std::remove(v.begin(), v.end(), d), v.end());
				}
			}
		};

		//template<typename AwaiterFor>
		//DestroyAwaiterRaii<AwaiterFor>&& destroyAwaiterRaii(size_t idx)
		//{
		//	return DestroyAwaiterRaii<AwaiterFor>{ this, idx };
		//}

		template<typename AwaiterFor>
		void destroyAwaiter(size_t idx)
		{
			DestroyAwaiterRaii<AwaiterFor>(this, idx);
		}


		// Return the current awaiter. We assumer the caller
		// is providing the correct awaiter type. This is validated
		// in debug builds.
		template<typename AwaiterFor>
		typename AwaiterFor::reference getAwaiter(size_t idx) noexcept
		{

			assert(awaiters.size());
			auto d = --awaiters.end();
			while (d->awaiter_idx != idx)
			{
				assert(d != awaiters.begin());
				--d;
			}

#ifndef NDEBUG
			if (!d->_awaiter_ptr || d->_awaiter_typeid_ != &typeid(AwaiterFor))
				terminate();
#endif
			auto ptr = (typename AwaiterFor::pointer)d->_awaiter_ptr;
			return *ptr;
		}

#ifdef MACORO_CPP_20

		using inline_storage_adapter = std::aligned_storage_t<400, alignof(std::max_align_t)>;
		inline_storage_adapter frame_adapter_storage;

		using outter_promise_type = promise_type;
		struct std_handle_adapter
		{
			struct yield_awaiter
			{
				std::coroutine_handle<void> continuation;

				bool await_ready() noexcept { return false; }
				std::coroutine_handle<void> await_suspend(std::coroutine_handle<void> v) noexcept { return continuation; }
				void await_resume() noexcept {}

			};

			struct promise_type
			{
				suspend_always initial_suspend() noexcept { return {}; }
				yield_awaiter final_suspend() const noexcept { return { continuation }; }

				void unhandled_exception()
				{
					std::rethrow_exception(std::current_exception());
				}

				coroutine_handle<outter_promise_type> outer_handle;
				std::coroutine_handle<void> continuation;


				std_handle_adapter get_return_object() { return { this }; }

				~promise_type()
				{
					if (outer_handle)
						outer_handle.destroy();
				}

				yield_awaiter yield_value(std::coroutine_handle<void> cont)
				{
					return { cont };
				}

				void return_value(std::coroutine_handle<void> c) { continuation = c; }


				void* operator new(std::size_t size)
				{
					void* ptr = new char[size];
					if (!ptr) throw std::bad_alloc{};
					return ptr;
				}

				template<typename TT>
				void* operator new(std::size_t size, TT& base)
				{
					if (size <= sizeof(inline_storage_adapter))
					{
						return &base.frame_adapter_storage;
					}
					else
					{
						void* ptr = new char[size];
						if (!ptr) throw std::bad_alloc{};
						return ptr;
					}
				}

				void operator delete(void* ptr, std::size_t size)
				{

					if (size <= sizeof(inline_storage_adapter))
					{
						//delete base.frame_adapter_storage;
					}
					else
					{
						delete[](char*)ptr;
					}
				}


			};


			~std_handle_adapter()
			{
				std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
			}

			promise_type* promise;
		};

		std::coroutine_handle<typename std_handle_adapter::promise_type> std_handle;
		std::coroutine_handle<typename std_handle_adapter::promise_type> get_std_handle()
		{
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
		using promise_type = Promise;

		using FrameBase<Promise>::promise;
		using FrameBase<Promise>::done;

		static coroutine_handle<void> resume_impl(FrameBase<void>* ptr)
		{
			auto This = static_cast<Frame<LambdaType, Promise>*>(ptr);
			return (*This)(static_cast<FrameBase<Promise>*>(This));
		}

		static void destroy_impl(FrameBase<void>* ptr) noexcept
		{
			auto self = static_cast<Frame<LambdaType, Promise>*>(ptr);
			delete self;
		}


#ifdef MACORO_CPP_20
		using FrameBase<Promise>::std_handle;
		using std_handle_adapter = FrameBase<Promise>::std_handle_adapter;
		std_handle_adapter std_adapter;
#endif
		Frame(const Frame&) = delete;
		Frame(Frame&&) = delete;

		Frame(LambdaType&& l)
			: LambdaType(std::forward<LambdaType>(l))
#ifdef MACORO_CPP_20
			, std_adapter(adapter())
#endif
		{
			FrameBase<void>::resume = &Frame::resume_impl;
			FrameBase<void>::destroy = &Frame::destroy_impl;
#ifdef MACORO_CPP_20

			FrameBase<void>::get_std_handle = &FrameBase<promise_type>::get_std_handle_void;
			;
			auto outer_handle = coroutine_handle<promise_type>::from_promise(promise, coroutine_handle_type::mocoro);
			std_adapter.promise->outer_handle = outer_handle;
			std_handle = std::coroutine_handle<std_handle_adapter::promise_type>::from_promise(*std_adapter.promise);
#endif
		}

#ifdef MACORO_CPP_20
		~Frame()
		{
			std::cout << "~Frame " << std::endl;

			// For when std_adapter isn't the one who called our destructor,
			// prevent it from calling it recursively by setting the handle
			// back to this frame to null.
			std_adapter.promise->outer_handle = std::nullptr_t{};
		}
	private:

		std_handle_adapter adapter()
		{
			// we have to perform the symmetric transfer loop here for
			// macoro coroutines since we can only return std::coroutine_handle<>.
			while (true)
			{

				auto h = (*this)(static_cast<FrameBase<promise_type>*>(this));

				assert(h);
				bool noop = h == noop_coroutine();
				if (!noop && h.is_std() == false)
				{
					auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)h.address() ^ 1);
					auto _address = realAddr->resume(realAddr).address();
					h = coroutine_handle<void>::from_address(_address);
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
		};
#endif

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
		auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address);
		return realAddr->done();
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
			auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address);
			_address = realAddr->resume(realAddr).address();
			if (_address == noop_coroutine().address())
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
		auto realAddr = reinterpret_cast<FrameBase<void>*>((std::size_t)_address);
		return realAddr->destroy(realAddr);
#endif
	}

#ifdef MACORO_CPP_20
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

#endif
}