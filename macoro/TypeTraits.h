#pragma once
#include <type_traits>

namespace macoro
{

	template< class T >
	struct remove_cvref {
		using type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
	};


	template< class T >
	using remove_cvref_t = typename remove_cvref<T>::type;


	template <bool Cond, typename T = void>
	using enable_if_t = typename std::enable_if<Cond, T>::type;

	template <typename ...>
	struct _void_t_impl
	{
		typedef void type;
	};

	template <typename ... T>
	using void_t = typename _void_t_impl<T...>::type;

	using false_type = std::false_type;
	using true_type = std::true_type;


	template<typename T, typename = void>
	struct has_any_await_transform_member : false_type
	{};

	template <typename T>
	struct has_any_await_transform_member <T, void_t<

		// must have a await_transform(...) member fn
		decltype(T::await_transform)

		>>
		: true_type{};


	template<typename T, typename = void>
	struct has_member_operator_co_await : false_type
	{};

	template <typename T>
	struct has_member_operator_co_await <T, void_t<

		// must have a operator_co_await() member fn
		decltype(std::declval<T>().operator_co_await)

		>>
		: true_type{};


	template<typename T, typename = void>
	struct has_free_operator_co_await : false_type
	{};


	template <typename T>
	struct has_free_operator_co_await <T, void_t<

		// must have a operator_co_await() member fn
		decltype(operator_co_await(std::declval<T>()))

		>>
		: true_type{};



	template<typename Awaiter, typename T, typename = void>
	struct has_void_await_suspend : false_type
	{};

	template <typename Awaiter, typename T>
	struct has_void_await_suspend <Awaiter, T, void_t<

		// must have a await_suspend(...) member fn
		decltype(std::declval<Awaiter>().await_suspend(std::declval<T>())),

		// must return void
		enable_if_t<std::is_same<
			decltype(std::declval<Awaiter>().await_suspend(std::declval<T>())),
			void
		>::value, void>

		>>
		: true_type{};



	//template<typename P, typename T>
	//decltype(auto) get_awaitable(P& promise, T&& expr)
	//{
	//	if constexpr (has_any_await_transform_member_v<P>)
	//		return promise.await_transform(static_cast<T&&>(expr));
	//	else
	//		return static_cast<T&&>(expr);
	//}

	struct monostate {};

	template<typename P, typename T>
	inline decltype(auto) get_awaitable(
		P& promise, T&& expr, typename enable_if_t<has_any_await_transform_member<P>::value, monostate> = {})
	{
		return promise.await_transform(static_cast<T&&>(expr));
	}

	template<typename P, typename T>
	inline decltype(auto) get_awaitable(
		P& promise, T&& expr, typename enable_if_t<!has_any_await_transform_member<P>::value, monostate> = {})
	{
		return static_cast<T&&>(expr);
	}

	//template<typename Awaitable>
	//decltype(auto) get_awaiter(Awaitable&& awaitable)
	//{
	//	if constexpr (has_member_operator_co_await_v<Awaitable>)
	//		return static_cast<Awaitable&&>(awaitable).operator co_await();
	//	else if constexpr (has_non_member_operator_co_await_v<Awaitable&&>)
	//		return operator co_await(static_cast<Awaitable&&>(awaitable));
	//	else
	//		return static_cast<Awaitable&&>(awaitable);
	//}

	template<typename Awaitable>
	inline decltype(auto) get_awaiter(
		Awaitable&& awaitable, 
		typename enable_if_t<
			has_member_operator_co_await<Awaitable>::value, monostate> = {})
	{
		return static_cast<Awaitable&&>(awaitable).operator_co_await();
	}

	template<typename Awaitable>
	inline decltype(auto) get_awaiter(
		Awaitable&& awaitable, 
		typename enable_if_t<
			!has_member_operator_co_await<Awaitable>::value&&
			has_free_operator_co_await<Awaitable>::value, monostate> = {})
	{
		return operator_co_await(static_cast<Awaitable&&>(awaitable));
	}

	template<typename Awaitable>
	inline decltype(auto) get_awaiter(
		Awaitable&& awaitable,
		typename enable_if_t<
			!has_member_operator_co_await<Awaitable>::value &&
			!has_free_operator_co_await<Awaitable>::value, monostate> = {})
	{
		return static_cast<Awaitable&&>(awaitable);
	}

	template<typename promise_type, typename Expr>
	struct awaiter_for
	{
		using expr = Expr;
		using awaitable = decltype(get_awaitable(std::declval<promise_type&>(), std::declval<Expr&&>()));
		using awaiter = decltype(get_awaiter(std::declval<awaitable>()));
		using type = awaiter;

		using value_type = typename std::remove_reference<awaiter>::type;
		using pointer = value_type*;
		using reference = value_type&;
	};

	//template<typename Expr>
	//using awaitable_from_expr_t = decltype(get_awaitable(std::declval<promise_type>(), std::declval<Expr&&>()));

	//template<typename Awaitable>
	//using awaiter_from_awaitable_t = decltype(get_awaiter(std::declval<Awaitable&&>()));

	//template<typename Expr>
	//using awaiter_from_expr_t = awaiter_from_awaitable<awaitable_from_expr<Expr>>;

	// for a Promise and an expression <expr> is an Awaitable if
	// 
	// 	   Awaiter = Promise::await_transform(<expr>);
	// 
	// 	   Awaiter::await_ready() -> bool;
	// 	   Awaiter::await_suspend() -> bool;
	// 	   Awaiter::await_resume() -> bool;
	//
}