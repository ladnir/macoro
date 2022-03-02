///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See github.com/lewissbaker/cppcoro LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "macoro/detail/stop_token.h"

#include <functional>
#include <utility>
#include "macoro/type_traits.h"
#include "macoro/optional.h"
#include <atomic>
#include <cstdint>
#include <memory>

namespace macoro
{
	namespace detail
	{
		class stop_state;
		struct stop_callback_list_chunk;
		struct stop_callback_state;
	}

	class stop_callback
	{
	public:

		/// Registers the callback to be executed when cancellation is requested
		/// on the stop_token.
		///
		/// The callback will be executed if cancellation is requested for the
		/// specified cancellation token. If cancellation has already been requested
		/// then the callback will be executed immediately, before the constructor
		/// returns. If cancellation has not yet been requested then the callback
		/// will be executed on the first thread to request cancellation inside
		/// the call to stop_source::request_stop().
		///
		/// \param token
		/// The cancellation token to register the callback with.
		///
		/// \param callback
		/// The callback to be executed when cancellation is requested on the
		/// the stop_token. Note that callback must not throw an exception
		/// if called when cancellation is requested otherwise std::terminate()
		/// will be called.
		///
		/// \throw std::bad_alloc
		/// If registration failed due to insufficient memory available.
		template<
			typename FUNC,
			typename = enable_if_t<std::is_constructible<std::function<void()>, FUNC&&>::value>>
		stop_callback(stop_token token, FUNC&& callback)
			: m_callback(std::forward<FUNC>(callback))
		{
			register_callback(std::move(token));
		}

		stop_callback(const stop_callback& other) = delete;
		stop_callback& operator=(const stop_callback& other) = delete;

		/// Deregisters the callback.
		///
		/// After the destructor returns it is guaranteed that the callback
		/// will not be subsequently called during a call to request_stop()
		/// on the stop_source.
		///
		/// This may block if cancellation has been requested on another thread
		/// is it will need to wait until this callback has finished executing
		/// before the callback can be destroyed.
		~stop_callback();

	private:

		friend class detail::stop_state;
		friend struct detail::stop_callback_state;

		void register_callback(stop_token&& token);

		detail::stop_state* m_state;
		std::function<void()> m_callback;
		detail::stop_callback_list_chunk* m_chunk;
		std::uint32_t m_entryIndex;
	};

	namespace detail
	{

		//struct RVO_TEST
		//{
		//	RVO_TEST(int) {};
		//	RVO_TEST(const RVO_TEST&) = delete;
		//	RVO_TEST(RVO_TEST&&) = delete;
		//};


		//inline stop_callback CALL_RVO_TEST2()
		//{
		//	return {  };
		//}


		inline void CALL_RVO_TEST1()
		{
			optional<stop_callback> t;
			t.emplace(stop_token{}, []() {});
		}
	}

#ifndef MACORO_RVO
	#if defined(MACORO_CPP20) || defined(_MSC_VER)
		#define MACORO_RVO 1
	#else 
		#define MACORO_RVO 0
	#endif
#endif // !MACORO_RVO

#if MACORO_RVO
	using optional_stop_callback = optional<stop_callback>;
#else
	struct optional_stop_callback : std::unique_ptr<stop_callback>
	{
		operator bool() const
		{
			return get();
		}

		template<typename... Args>
		stop_callback& emplace(Args&&... args)
		{
			reset(new stop_callback(std::forward<Args>(args)...));
			return value();
		}

		bool has_value() const
		{
			return *this;
		}

		stop_callback& value() { return *get(); }
		const stop_callback& value() const { return *get(); }
	};

#endif
}

