//#include "macoro/type_traits.h"
//#include "macoro/macros.h"
//#include "macoro/optional.h"
#include <iostream>
#include <type_traits>
#include <chrono>
#include <numeric>
#include <fstream>
//#include "tests/tests.h"
#include <mutex>

//using namespace macoro;

class _Mutex_base { // base class for all mutex types
public:
#ifdef _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
    _Mutex_base(int _Flags = 0) noexcept {
        _Mtx_init_in_situ(_Mymtx(), _Flags | _Mtx_try);
    }
#else // ^^^ defined(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR) / !defined(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR) vvv
    constexpr _Mutex_base(int _Flags = 0) noexcept {
        _Mtx_storage._Critical_section = {};
        _Mtx_storage._Thread_id = -1;
        _Mtx_storage._Type = _Flags | _Mtx_try;
        _Mtx_storage._Count = 0;
    }
#endif // ^^^ !defined(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR) ^^^

    _Mutex_base(const _Mutex_base&) = delete;
    _Mutex_base& operator=(const _Mutex_base&) = delete;

    void lock() {
        if (_Mtx_lock(_Mymtx()) != _Thrd_result::_Success) {
            // undefined behavior, only occurs for plain mutexes (N4950 [thread.mutex.requirements.mutex.general]/6)
            _STD _Throw_Cpp_error(std::_RESOURCE_DEADLOCK_WOULD_OCCUR);
        }

        if (!_Verify_ownership_levels()) {
            // only occurs for recursive mutexes (N4950 [thread.mutex.recursive]/3)
            // POSIX specifies EAGAIN in the corresponding situation:
            // https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_lock.html
            _STD _Throw_Cpp_error(std::_RESOURCE_UNAVAILABLE_TRY_AGAIN);
        }
    }

    _NODISCARD_TRY_CHANGE_STATE bool try_lock() noexcept /* strengthened */ {
        // false may be from undefined behavior for plain mutexes (N4950 [thread.mutex.requirements.mutex.general]/6)
        return _Mtx_trylock(_Mymtx()) == _Thrd_result::_Success;
    }

    void unlock() noexcept /* strengthened */ {
        _Mtx_unlock(_Mymtx());
    }

    // native_handle_type and native_handle() have intentionally been removed. See GH-3820.

    _NODISCARD_TRY_CHANGE_STATE bool _Verify_ownership_levels() noexcept {
        if (_Mtx_storage._Count == INT_MAX) {
            // only occurs for recursive mutexes (N4950 [thread.mutex.recursive]/3)
            --_Mtx_storage._Count;
            return false;
        }

        return true;
    }

    friend std::condition_variable;
    friend std::condition_variable_any;

    _Mtx_internal_imp_t _Mtx_storage{};

    _Mtx_t _Mymtx() noexcept {
        return &_Mtx_storage;
    }
};

int main(int argc, char** argv)
{
	std::mutex m_mutex;
    _Mutex_base& m = *(_Mutex_base*)&m_mutex;

	{
        //_Mtx_lock(&m._Mtx_storage);
        m_mutex.lock();
		std::cout << "pre " << std::endl;
        m_mutex.unlock();
        //_Mtx_unlock(&m._Mtx_storage);

	}
	//macoro::CLP cmd(argc, argv);
	//macoro::testCollection.runIf(cmd);
	return 0;
}

