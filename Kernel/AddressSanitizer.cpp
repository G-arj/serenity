/*
 * Copyright (c) 2021, Brian Gianforcaro <bgianf@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/AddressSanitizer.h>

#if defined(__SANITIZE_ADDRESS__)

#    include <AK/Format.h>
#    include <Kernel/VM/MemoryManager.h>

namespace Kernel::AddressSanitizer {

bool sm_sanitizer_enabled { false };

void initialize()
{
    //MM.allocate_kernel_region_identity()
    dmesgln("KASAN Initialized... {}", 1);
    sm_sanitizer_enabled = true;
}

inline VirtualAddress address_to_shadow(const FlatPtr address)
{
    // The formula for shadow address translation is:
    //   shadow_address = (address >> 3) + Constants::ShadowOffset;
    //
    // We (>> 3) because we are trying to divide by 8, as each byte
    // in the shadow memory region covers 8 bytes of kernel memory.
    // Hence one bit in shadow memory represents one byte of kernel memory.
    return (VirtualAddress(address) >> Constants::ShadowScaleShift) + Constants::ShadowOffset;
}

void shadow_va_check_load(unsigned long address, size_t size, void* return_address)
{
    (void)size;
    (void)return_address;

    if (!sm_sanitizer_enabled) [[unlikely]]
        return;

    auto shadow_address = address_to_shadow(address);
    (void)shadow_address;
}

void shadow_va_check_store(unsigned long address, size_t size, void* return_address)
{
    (void)size;
    (void)return_address;

    if (!sm_sanitizer_enabled) [[unlikely]]
        return;

    auto shadow_address = address_to_shadow(address);
    (void)shadow_address;
}

}

using namespace Kernel::AddressSanitizer;

extern "C" {

// Define a macro to easily declare the KASAN load and store callbacks for
// the various sizes of data type.
//
#    define ADDRESS_SANITIZER_LOAD_STORE(size)                                 \
        void __asan_load##size(unsigned long);                                 \
        void __asan_load##size(unsigned long address)                          \
        {                                                                      \
            shadow_va_check_load(address, size, __builtin_return_address(0));  \
        }                                                                      \
        void __asan_load##size##_noabort(unsigned long);                       \
        void __asan_load##size##_noabort(unsigned long address)                \
        {                                                                      \
            shadow_va_check_load(address, size, __builtin_return_address(0));  \
        }                                                                      \
        void __asan_store##size(unsigned long);                                \
        void __asan_store##size(unsigned long address)                         \
        {                                                                      \
            shadow_va_check_store(address, size, __builtin_return_address(0)); \
        }                                                                      \
        void __asan_store##size##_noabort(unsigned long);                      \
        void __asan_store##size##_noabort(unsigned long address)               \
        {                                                                      \
            shadow_va_check_store(address, size, __builtin_return_address(0)); \
        }

ADDRESS_SANITIZER_LOAD_STORE(1);
ADDRESS_SANITIZER_LOAD_STORE(2);
ADDRESS_SANITIZER_LOAD_STORE(4);
ADDRESS_SANITIZER_LOAD_STORE(8);
ADDRESS_SANITIZER_LOAD_STORE(16);

#    undef ADDRESS_SANITIZER_LOAD_STORE

void __asan_loadN(unsigned long, size_t);
void __asan_loadN(unsigned long address, size_t size)
{
    shadow_va_check_load(address, size, __builtin_return_address(0));
}

void __asan_loadN_noabort(unsigned long, size_t);
void __asan_loadN_noabort(unsigned long address, size_t size)
{
    shadow_va_check_load(address, size, __builtin_return_address(0));
}

void __asan_storeN(unsigned long, size_t);
void __asan_storeN(unsigned long address, size_t size)
{
    shadow_va_check_store(address, size, __builtin_return_address(0));
}

void __asan_storeN_noabort(unsigned long, size_t);
void __asan_storeN_noabort(unsigned long address, size_t size)
{
    shadow_va_check_store(address, size, __builtin_return_address(0));
}

// Performs shadow memory cleanup of the current thread's stack before a
// function marked with the [[noreturn]] attribute is called.
//
void __asan_handle_no_return(void);
void __asan_handle_no_return(void)
{
}

void __asan_before_dynamic_init(const char*);
void __asan_before_dynamic_init(const char* /* module_name */)
{
}

void __asan_after_dynamic_init();
void __asan_after_dynamic_init()
{
}
}

#else

namespace Kernel::AddressSanitizer {

void initialize()
{
    // NOP when ASAN is not enabled.
}

}

#endif
