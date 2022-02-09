#pragma once
#include "type_traits.h"
#include "variant.h"
#include "error_code.h"
#include "coroutine_handle.h"
#include "awaiter.h"
#include "variant.h"

namespace macoro
{

	template<typename T, typename Error>
	class result;
	namespace impl
	{


		template<typename T>
		struct OkTag {
			const T& mV;

			OkTag(T& e)
				:mV(e)
			{}

			operator const T& ()
			{
				return mV;
			}
		};

		template<typename T>
		struct OkMvTag {
			T& mV;


			OkMvTag(T& e)
				:mV(e)
			{}


			operator T && ()
			{
				return mV;
			}
		};

		template<typename E>
		struct ErrorTag {
			const E& mE;

			ErrorTag(E& e)
				:mE(e)
			{}

			operator const E& ()
			{
				return mE;
			}
		};

		template<typename E>
		struct ErrorMvTag {
			E& mE;

			ErrorMvTag(E& e)
				:mE(e)
			{}

			operator E && ()
			{
				return std::move(mE);
			}
		};

		template<typename T, typename Error>
		enable_if_t<std::is_default_constructible<T>::value, variant<T, Error>> resultDefaultConstruct()
		{
			return variant<T, Error>{ MACORO_VARIANT_NAMESPACE::in_place_index<0> };
		}

		template<typename T, typename Error>
		enable_if_t<!std::is_default_constructible<T>::value, variant<T, Error>> resultDefaultConstruct()
		{
			static_assert(std::is_default_constructible<Error>::value, "either T or Error must be default constructible to default construct a result.");
			return variant<T, Error>{ MACORO_VARIANT_NAMESPACE::in_place_index<1> };
		}
	}


	template<typename T>
	impl::ErrorTag<remove_cvref_t<T>> Err(const T& t)
	{
		return impl::ErrorTag<remove_cvref_t<T>>(t);
	}

	template<typename T>
	impl::ErrorMvTag<remove_cvref_t<T>> Err(T&& t)
	{
		return impl::ErrorMvTag<remove_cvref_t<T>>(t);
	}


	template<typename T>
	impl::OkTag<remove_cvref_t<T>> Ok(const T& t)
	{
		return impl::OkTag<remove_cvref_t<T>>(t);
	}

	template<typename T>
	impl::OkMvTag<remove_cvref_t<T>> Ok(T&& t)
	{
		return impl::OkMvTag<remove_cvref_t<T>>(t);
	}

	
	namespace impl
	{
		template<typename E>
		void result_rethrow(E&& e)
		{
			throw e;
		}


		template<typename T, typename E>
		std::exception& result_unhandled_exception()
		{
			static_assert(std::is_base_of<std::exception, E>::value,
				"for custom result error types with coroutines, the user must implement result_unhandled_exception. This could be to just be rethrow.");
			try {
				std::rethrow_exception(std::current_exception());
			}
			catch (std::exception& e)
			{
				return e;
			}
		}

		template<typename T, typename E>
		struct result_awaiter
		{
			result<T, E>* result;

			bool await_ready() noexcept { return true; }
			template<typename P>
			void await_suspend(const std::coroutine_handle<P>&) noexcept {}
			template<typename P>
			void await_suspend(const coroutine_handle<P>&) noexcept {}
			T await_resume()
			{
				if (result->has_error())
					result_rethrow(result->error());

				return std::move(result->value());
			}
		};


		template<typename T, typename E>
		struct result_promise
		{
			variant<empty_state,
				typename std::remove_reference<T>::type*,
				typename std::remove_reference<E>::type*
			> mVar;

			suspend_never initial_suspend() const noexcept { return {}; }
			suspend_always final_suspend() const noexcept { return {}; }

			struct Ret
			{
				coroutine_handle<result_promise> handle;

				operator result<T, E>() const
				{

					auto& promise = handle.promise();
					auto idx = promise.mVar.index();
					if (idx == 1)
					{
						result<T, E> r(Ok(std::move(*MACORO_VARIANT_NAMESPACE::get<1>(promise.mVar))));
						handle.destroy();
						return r;
					}
					else if (idx == 2)
					{
						result<T, E> r(Err(std::move(*MACORO_VARIANT_NAMESPACE::get<2>(promise.mVar))));
						handle.destroy();
						return r;
					}

					handle.destroy();
					throw broken_promise();
				}
			};

			Ret get_return_object()
			{
				return { coroutine_handle<result_promise>::from_promise(*this, coroutine_handle_type::std) };
			}
			Ret macoro_get_return_object()
			{
				return { coroutine_handle<result_promise>::from_promise(*this, coroutine_handle_type::std) };
			}

			template<typename TT>
			suspend_never await_transform(TT&&) = delete;

			template<typename TT>
			decltype(auto) await_transform(result<TT, E>&& r)
			{
				return std::forward<result<TT, E>>(r);
			}

			template<typename TT>
			decltype(auto) await_transform(result<TT, E>& r)
			{
				return std::forward<result<TT, E>>(r);
			}

			void unhandled_exception()
			{
				mVar.template emplace<2>(&result_unhandled_exception<T, E>());
			}

			void return_value(T&& t)
			{
				mVar.template emplace<1>(&t);
			}
			void return_value(const T& t)
			{
				mVar.template emplace<1>(&t);
			}
		};

	}


	template<typename T, typename Error = std::exception>
	class result
	{
	public:
		using promise_type = impl::result_promise<T, Error>;
		using value_type = remove_cvref_t<T>;
		using error_type = remove_cvref_t<Error>;


		result()
			: mVar(impl::resultDefaultConstruct<T,Error>())
		{
		}

		result(impl::OkTag<value_type>&& v) :mVar(v.mV) {}
		result(impl::OkMvTag<value_type>&& v) : mVar(std::move(v.mV)) {}
		result(impl::ErrorTag<error_type>&& e) : mVar(e.mE) {}
		result(impl::ErrorMvTag<error_type>&& e) :mVar(std::move(e.mE)) {}


		variant<value_type, error_type> mVar;
		variant<value_type, error_type>& var() {
			return mVar;
		};
		const variant<value_type, error_type>& var() const {
			return mVar;
		};


		bool has_value() const {
			return var().index() == 0;
		}

		bool has_error() const {
			return !has_value();
		}

		explicit operator bool() {
			return has_value();
		}

		value_type& value()
		{
			if (has_error())
				impl::result_rethrow(error());

			return MACORO_VARIANT_NAMESPACE::get<0>(var());
		}

		const value_type& value() const
		{
			if (has_error())
				impl::result_rethrow(error());

			return value();
		}

		const value_type& operator*() const { return value(); }

		value_type& operator*() { return value(); }

		error_type& error()
		{
			if (has_value())
				throw std::runtime_error("error() was called on a Result<T,E> which stores an value_type");

			return MACORO_VARIANT_NAMESPACE::get<1>(var());
		}

		const error_type& error() const
		{
			if (has_value())
				throw std::runtime_error("error() was called on a Result<T,E> which stores an value_type");

			return MACORO_VARIANT_NAMESPACE::get<1>(var());
		}


		value_type value_or(value_type&& alt)
		{
			if (has_error())
				return alt;
			return value();
		}


		value_type& value_or(value_type& alt)
		{
			if (has_error())
				return alt;
			return value();
		}

		const value_type& value_or(const value_type& alt) const
		{
			if (has_error())
				return alt;
			return value();
		}


		error_type error_or(error_type&& alt)
		{
			if (has_error())
				return error();
			return alt;
		}

		error_type& error_or(error_type& alt)
		{
			if (has_error())
				return error();
			return alt;
		}

		const error_type& error_or(const error_type& alt) const
		{
			if (has_error())
				return error();
			return alt;
		}



		void operator=(impl::OkMvTag<value_type>&& v)
		{
			var() = variant<value_type, error_type>{ MACORO_VARIANT_NAMESPACE::in_place_index<0>, std::move(v.mV) };
		}
		void operator=(impl::OkTag<value_type>&& v)
		{
			var() = variant<value_type, error_type>{ MACORO_VARIANT_NAMESPACE::in_place_index<0>, v.mV };
		}

		void operator=(impl::ErrorMvTag<error_type>&& v)
		{
			var() = variant<value_type, error_type>{ MACORO_VARIANT_NAMESPACE::in_place_index<1>, std::move(v.mE) };
		}
		void operator=(impl::ErrorTag<error_type>&& v)
		{
			var() = variant<value_type, error_type>{ MACORO_VARIANT_NAMESPACE::in_place_index<1>, v.mE };
		}


		template<typename T2>
		bool operator==(impl::OkTag<T2>&& v) const
		{
			return (has_value() && value() == v.mV);
		}

		template<typename T2>
		bool operator==(impl::OkMvTag<T2>&& v)const
		{
			return (has_value() && value() == v.mV);
		}

		template<typename E2>
		bool operator==(impl::ErrorTag<E2>&& v)const
		{
			return (has_error() && error() == v.mE);
		}
		template<typename E2>
		bool operator==(impl::ErrorMvTag<E2>&& v)const
		{
			return (has_error() && error() == v.mE);
		}

		template<typename T2>
		bool operator!=(impl::OkTag<T2>&& v)const
		{
			return !(*this == v);
		}

		template<typename T2>
		bool operator!=(impl::OkMvTag<T2>&& v)const
		{
			return !(*this == v);
		}

		template<typename E2>
		bool operator!=(impl::ErrorTag<E2>&& v)const
		{
			return !(*this == v);
		}
		template<typename E2>
		bool operator!=(impl::ErrorMvTag<E2>&& v)const
		{
			return !(*this == v);
		}


		impl::result_awaiter<T, Error> operator co_await()
		{
			return { this };
		}
	};



}