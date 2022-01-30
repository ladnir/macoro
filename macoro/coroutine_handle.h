#include "config.h"

#include <functional>
#include <cassert>
#ifdef MACORO_CPP_20
#include <coroutine>
#endif // MACORO_CPP_20

namespace macoro
{

    template<typename Promise = void>
    struct coroutine_handle;

    template<typename nested_promise>
    struct std_handle_adapter;

    enum class coroutine_handle_type
    {
        std,
        mocoro
    };

#ifdef MACORO_CPP_20
    inline bool _macoro_coro_is_std(void* _address) noexcept;
#endif

    template<typename Promise>
    Promise* _macoro_coro_promise(void* address)noexcept;

    template<typename Promise>
    void* _macoro_coro_frame(void* address, coroutine_handle_type te) noexcept;

    bool _macoro_coro_done(void* address)noexcept;
    void _macoro_coro_resume(void* address);
    void _macoro_coro_destroy(void* address)noexcept;


    template <>
    struct coroutine_handle<void> {
        constexpr coroutine_handle() noexcept = default;
        constexpr coroutine_handle(nullptr_t) noexcept {}

#ifdef MACORO_CPP_20
        template<typename T>
        coroutine_handle(const std::coroutine_handle<T>& h) noexcept {
            _Ptr = h.address();
            assert(((std::size_t)_Ptr & 1) == 0);
        };
        template<typename T>
        coroutine_handle(std::coroutine_handle<T>&& h) noexcept {
            _Ptr = h.address();
            assert(((std::size_t)_Ptr & 1) == 0);
        };
#endif // MACORO_CPP_20

        coroutine_handle& operator=(nullptr_t) noexcept {
            _Ptr = nullptr;
            return *this;
        }

        MACORO_NODISCARD constexpr void* address() const noexcept {
            return _Ptr;
        }

        MACORO_NODISCARD static constexpr coroutine_handle from_address(void* const _Addr) noexcept { // strengthened
            coroutine_handle _Result;
            _Result._Ptr = _Addr;
            return _Result;
        }

        constexpr explicit operator bool() const noexcept {
            return _Ptr != nullptr;
        }

        MACORO_NODISCARD bool done() const noexcept { // strengthened
            return _macoro_coro_done(_Ptr);
        }

        void operator()() const {
            _macoro_coro_resume(_Ptr);
        }

        void resume() const {
            _macoro_coro_resume(_Ptr);
        }

        void destroy() const noexcept { // strengthened
            _macoro_coro_destroy(_Ptr);
        }

#ifdef MACORO_CPP_20
        bool is_std() const
        {
            return _macoro_coro_is_std(_Ptr);
        }

        std::coroutine_handle<void> std_cast() const;
#endif // MACORO_CPP_20

    private:
        void* _Ptr = nullptr;
    };

    template <class _Promise>
    struct coroutine_handle {
        constexpr coroutine_handle() noexcept = default;
        constexpr coroutine_handle(nullptr_t) noexcept {}

        coroutine_handle& operator=(nullptr_t) noexcept {
            _Ptr = nullptr;
            return *this;
        }

        MACORO_NODISCARD constexpr void* address() const noexcept {
            return _Ptr;
        }

        MACORO_NODISCARD static constexpr coroutine_handle from_address(void* const _Addr) noexcept { // strengthened
            coroutine_handle _Result;
            _Result._Ptr = _Addr;
            return _Result;
        }

        constexpr operator coroutine_handle<>() const noexcept {
            return coroutine_handle<>::from_address(_Ptr);
        }

        constexpr explicit operator bool() const noexcept {
            return _Ptr != nullptr;
        }

        MACORO_NODISCARD bool done() const noexcept { // strengthened
            return _macoro_coro_done(_Ptr);
        }

        void operator()() const {
            _macoro_coro_resume(_Ptr);
        }

        void resume() const {
            _macoro_coro_resume(_Ptr);
        }

        void destroy() const noexcept { // strengthened
            _macoro_coro_destroy(_Ptr);
        }

        MACORO_NODISCARD _Promise& promise() const noexcept { // strengthened
            return *_macoro_coro_promise<_Promise>(_Ptr);
        }

#ifdef MACORO_CPP_20
        std::coroutine_handle<void> std_cast() const;
        //MACORO_NODISCARD static coroutine_handle from_promise(_Promise& _Prom)
        //{
        //    return from_promise(_Prom, coroutine_handle_type::std);
        //}
#endif // MACORO_CPP_20


        MACORO_NODISCARD static coroutine_handle from_promise(_Promise& _Prom, coroutine_handle_type te) noexcept { // strengthened
            const auto _Frame_ptr = _macoro_coro_frame<_Promise>(std::addressof(_Prom), te);
            coroutine_handle _Result;
            _Result._Ptr = _Frame_ptr;
            return _Result;
        }

    private:
        void* _Ptr = nullptr;
    };

    MACORO_NODISCARD constexpr bool operator==(const coroutine_handle<> _Left, const coroutine_handle<> _Right) noexcept {
        return _Left.address() == _Right.address();
    }


    MACORO_NODISCARD constexpr bool operator!=(const coroutine_handle<> _Left, const coroutine_handle<> _Right) noexcept {
        return !(_Left == _Right);
    }

    MACORO_NODISCARD constexpr bool operator<(const coroutine_handle<> _Left, const coroutine_handle<> _Right) noexcept {
        return std::less<void*>{}(_Left.address(), _Right.address());
    }

    MACORO_NODISCARD constexpr bool operator>(const coroutine_handle<> _Left, const coroutine_handle<> _Right) noexcept {
        return _Right < _Left;
    }

    MACORO_NODISCARD constexpr bool operator<=(const coroutine_handle<> _Left, const coroutine_handle<> _Right) noexcept {
        return !(_Left > _Right);
    }

    MACORO_NODISCARD constexpr bool operator>=(const coroutine_handle<> _Left, const coroutine_handle<> _Right) noexcept {
        return !(_Left < _Right);
    }

    //template <class _Promise>
    //struct hash<coroutine_handle<_Promise>> {
    //    MACORO_NODISCARD size_t operator()(const coroutine_handle<_Promise>& _Coro) const noexcept {
    //        return _Hash_representation(_Coro.address());
    //    }
    //};

    // STRUCT noop_coroutine_promise
    struct noop_coroutine_promise {};
    extern noop_coroutine_promise _noop_coroutine_promise;

    // STRUCT coroutine_handle<noop_coroutine_promise>
    template <>
    struct coroutine_handle<noop_coroutine_promise> {
        friend coroutine_handle noop_coroutine() noexcept;

        constexpr operator coroutine_handle<>() const noexcept {
            //throw std::runtime_error("not implemented");
            //std::terminate();
            return coroutine_handle<>::from_address(_Ptr);
        }

        constexpr explicit operator bool() const noexcept {
            return true;
        }
        MACORO_NODISCARD constexpr bool done() const noexcept {
            return false;
        }

        constexpr void operator()() const noexcept {}
        constexpr void resume() const noexcept {}
        constexpr void destroy() const noexcept {}

        MACORO_NODISCARD noop_coroutine_promise& promise() const noexcept {
            // Returns a reference to the associated promise
            return _noop_coroutine_promise;
        }

        MACORO_NODISCARD constexpr void* address() const noexcept {
            return _Ptr;
        }

    private:
        coroutine_handle() noexcept = default;

        void * const _Ptr = (void*)43;
    };

    // ALIAS noop_coroutine_handle
    using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

    // FUNCTION noop_coroutine
    MACORO_NODISCARD inline noop_coroutine_handle noop_coroutine() noexcept {
        // Returns a handle to a coroutine that has no observable effects when resumed or destroyed.
        return noop_coroutine_handle{};
    }

    // STRUCT suspend_never
    struct suspend_never {
        MACORO_NODISCARD constexpr bool await_ready() const noexcept {
            return true;
        }

        constexpr void await_suspend(coroutine_handle<>) const noexcept {}
        constexpr void await_resume() const noexcept {}
    };

    // STRUCT suspend_always
    struct suspend_always {
        MACORO_NODISCARD constexpr bool await_ready() const noexcept {
            return false;
        }

        constexpr void await_suspend(coroutine_handle<>) const noexcept {}
        constexpr void await_resume() const noexcept {}
    };

}