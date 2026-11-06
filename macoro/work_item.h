#pragma once

#include "macoro/coro_frame.h"

namespace macoro
{
    // A borrowed operation. Copying this descriptor does not retain its context.
    // execute must handle its own errors and may destroy the context before returning.
    struct work_item
    {
        void* context = nullptr;
        void (*execute)(void*) noexcept = nullptr;

        explicit operator bool() const noexcept { return execute != nullptr; }

        void operator()() const noexcept
        {
            execute(context);
        }

        static work_item resume(coroutine_handle<> handle) noexcept
        {
            return {handle.address(), [](void* address) noexcept {
                coroutine_handle<>::from_address(address).resume();
            }};
        }
    };
}
