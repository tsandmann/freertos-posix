/*
 * This file is part of the FreeRTOS port for posix plattform.
 * Copyright (c) 2020 Timo Sandmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file    posix.cpp
 * @brief   FreeRTOS support implementations for posix platform
 * @author  Timo Sandmann
 * @date    13.10.2020
 */

#define _DEFAULT_SOURCE
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <unwind.h>
#include <cstdio>
#include <cmath>
#include <tuple>
#include <new>

#ifdef _POSIX_PRIORITY_SCHEDULING
#include <sched.h>
#endif

#include "posix.h"
#include "freertos_time.h"
#include "semphr.h"


extern "C" {
__attribute__((weak)) void serialport_put(const char c) {
    ::putchar(c);
}

__attribute__((weak)) void serialport_puts(const char* str) {
    ::puts(str);
}

__attribute__((weak)) void serialport_flush() {
    ::fflush(stdout);
}

_Unwind_Reason_Code trace_fcn(_Unwind_Context* ctx, void* depth) {
    int* p_depth { static_cast<int*>(depth) };
    ::printf("\t#%d: pc at 0x%lx\r\n", *p_depth, _Unwind_GetIP(ctx));
    (*p_depth)++;
    return _URC_NO_REASON;
}

/**
 * @brief Print assert message and blink one short pulse every two seconds
 * @param[in] file: Filename as C-string
 * @param[in] line: Line number
 * @param[in] func: Function name as C-string
 * @param[in] expr: Expression that failed as C-string
 */
void assert_blink(const char* file, int line, const char* func, const char* expr) {
    ::printf("\r\nASSERT in [%s:%d]\r\n\t%s: %s\r\n\n", file, line, func, expr);

    ::puts("Stack trace:");
    int depth { 0 };
    _Unwind_Backtrace(&trace_fcn, &depth);
    ::puts("");

    freertos::error_blink(1);
}

void mcu_shutdown() {
    freertos::error_blink(0);
}
} // extern C


namespace freertos {
uint32_t get_ms() {
    struct timespec spec;
    ::clock_gettime(CLOCK_MONOTONIC, &spec);

    const auto s { spec.tv_sec };
    const auto ms { static_cast<uint64_t>(round(spec.tv_nsec / 1.0e6)) }; // Convert nanoseconds to milliseconds

    return s * 1'000UL + ms;
}

uint64_t get_us() {
    struct timespec spec;
    ::clock_gettime(CLOCK_MONOTONIC, &spec);

    const auto s { spec.tv_sec };
    const auto us { static_cast<uint64_t>(round(spec.tv_nsec / 1.0e3)) }; // Convert nanoseconds to microseconds

    return s * 1'000'000UL + us;
}

uint64_t get_us_from_isr() {
    return get_us();
}

void delay_ms(const uint32_t ms) {
    const auto start { get_ms() };
    while (get_ms() - start < ms) {
#ifdef _POSIX_PRIORITY_SCHEDULING
        sched_yield();
#endif
    }
}

void error_blink(const uint8_t n) {
    if (n == 10) {
        exit(0);
    }

    ::vTaskSuspendAll();

    struct timeval end, now;
    while (true) {
        ::gettimeofday(&end, nullptr);
        end.tv_sec += 2;
        ::gettimeofday(&now, nullptr);
        ::printf("error_blink(%u)\r\n", n);
        ::fflush(stdout);

        while (timercmp(&now, &end, <)) {
            ::gettimeofday(&now, nullptr);
        }
    }
}

void print_ram_usage() {
    // not implemented
}

std::tuple<size_t, size_t, size_t, size_t, size_t, size_t> ram1_usage() {
    return { 0, 0, 0, 0, 0, 0 };
}

std::tuple<size_t, size_t> ram2_usage() {
    return { 0, 0 };
}

void print_stack_trace(void*) {
    // not implemented
}
} // namespace freertos

extern "C" {
#if configSUPPORT_STATIC_ALLOCATION == 1
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer, uint32_t* pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    static std::align_val_t stack_align { portBYTE_ALIGNMENT };
    static StackType_t* uxIdleTaskStack { new (stack_align) StackType_t[configMINIMAL_STACK_SIZE] };

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

#if configUSE_TIMERS == 1
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer, uint32_t* pulTimerTaskStackSize) {
    static StaticTask_t xTimerTaskTCB;
    static std::align_val_t stack_align { portBYTE_ALIGNMENT };
    static StackType_t* uxTimerTaskStack { new (stack_align) StackType_t[configTIMER_TASK_STACK_DEPTH] };

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
#endif // configUSE_TIMERS
#endif // configSUPPORT_STATIC_ALLOCATION

#if configCHECK_FOR_STACK_OVERFLOW > 0
void vApplicationStackOverflowHook(TaskHandle_t, char* task_name) {
    static char taskname[configMAX_TASK_NAME_LEN + 1];

    std::memcpy(taskname, task_name, configMAX_TASK_NAME_LEN);
    ::puts("STACK OVERFLOW: ");
    ::puts(taskname);

    freertos::error_blink(3);
}
#endif // configCHECK_FOR_STACK_OVERFLOW
} // extern C
