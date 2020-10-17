/*
 * This file is part of the FreeRTOS port for posix plattform
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
 * @file    main.cpp
 * @brief   FreeRTOS example for posix plattform
 * @author  Timo Sandmann
 * @date    17.05.2020
 */

#include "arduino_freertos.h"

#include <thread>
#include <future>
#include <chrono>
#include <cstdio>


#ifdef __cpp_lib_jthread
static std::jthread* g_t1 {};
#endif

static void task1(void*) {
    while (true) {
        ::puts("TICK 1");
        ::vTaskDelay(pdMS_TO_TICKS(5'000));

        ::puts("TOCK 1");
        ::vTaskDelay(pdMS_TO_TICKS(5'000));
    }
}

static void task2(void*) {
#ifdef __cpp_lib_jthread
    g_t1 = new std::jthread { [](std::stop_token stop) {
        ::vTaskPrioritySet(nullptr, 3);

        while (!stop.stop_requested()) {
            using namespace std::chrono_literals;

            ::puts("TICK 2");
            std::this_thread::sleep_for(500ms);

            ::printf("TOCK 2\tnow: %lu s\r\n", freertos::get_us() / 1'000'000UL);
            std::this_thread::sleep_for(500ms);
        }
        ::puts("Thread stopped.");
    } };
    configASSERT(g_t1);
#else // ! __cpp_lib_jthread
    std::thread t1 { []() {
        ::vTaskPrioritySet(nullptr, 3);

        while (true) {
            using namespace std::chrono_literals;

            ::puts("TICK 2");
            std::this_thread::sleep_for(500ms);

            ::printf("TOCK 2\tnow: %lu s\r\n", freertos::get_us() / 1'000'000UL);
            std::this_thread::sleep_for(500ms);
        }
    } };
#endif // __cpp_lib_jthread

    ::vTaskSuspend(nullptr);
}

static void task3(void*) {
    using namespace std::chrono_literals;
    ::puts("task3:");

    std::this_thread::sleep_for(5s);

    ::puts("task3: creating futures...");

    std::future<int32_t> result0 { std::async([]() -> int32_t { return 2; }) };
    std::future<int32_t> result1 { std::async(std::launch::async, []() -> int32_t { return 3; }) };
    std::future<int32_t> result2 { std::async(std::launch::deferred, []() -> int32_t { return 5; }) };

    int32_t r { result0.get() + result1.get() + result2.get() };
    ::printf("r=%d\r\n", r);
    configASSERT(2 + 3 + 5 == r);

    {
        // future from a packaged_task
        std::packaged_task<int()> task { [] { return 7; } }; // wrap the function
        std::future<int> f1 { task.get_future() }; // get a future
        std::jthread t2 { std::move(task) }; // launch on a thread

        // future from an async()
        std::future<int> f2 { std::async(std::launch::async, [] { return 8; }) };

        // future from a promise
        std::promise<int> p;
        std::future<int> f3 { p.get_future() };
        std::thread([&p] { p.set_value_at_thread_exit(9); }).detach();

        ::puts("Waiting...");
        f1.wait();
        f2.wait();
        f3.wait();
        const auto r1 { f1.get() };
        const auto r2 { f2.get() };
        const auto r3 { f3.get() };
        ::printf("Done!\r\nResults are: %d %d %d\r\n", r1, r2, r3);
        configASSERT(7 + 8 + 9 == r1 + r2 + r3);
    }

#ifdef __cpp_lib_jthread
    if (g_t1 && g_t1->request_stop()) {
        ::puts("t1 stop_request successful.");
    } else {
        ::puts("t1 stop_request failed.");
    }

    delete g_t1;
    g_t1 = nullptr;
    ::puts("t1 deleted.");
#endif // __cpp_lib_jthread

    ::vTaskSuspend(nullptr);
}

int main() {
    ::xTaskCreate(task1, "task1", 1024, nullptr, 2, nullptr);
    ::xTaskCreate(task2, "task2", 1024, nullptr, configMAX_PRIORITIES - 1, nullptr);
    ::xTaskCreate(task3, "task3", 1024, nullptr, 3, nullptr);

    ::puts("main(): starting scheduler...");

    ::vTaskStartScheduler();
}
