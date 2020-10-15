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
#include <chrono>
#include <cstdio>


static void task1(void*) {
    while (true) {
        ::puts("TICK 1");
        ::vTaskDelay(pdMS_TO_TICKS(5'000));

        ::puts("TOCK 1");
        ::vTaskDelay(pdMS_TO_TICKS(5'000));
    }
}

static void task2(void*) {
    std::thread t1 { []() {
        ::vTaskPrioritySet(nullptr, 3);

        struct timeval tv;

        while (true) {
            using namespace std::chrono_literals;

            ::puts("TICK 2");
            std::this_thread::sleep_for(1'000ms);

            ::gettimeofday(&tv, nullptr);
            ::printf("TOCK 2\tnow: %lu s\n\r", tv.tv_sec);
            std::this_thread::sleep_for(1'000ms);
        }
    } };

    ::vTaskSuspend(nullptr);
}

int main() {
    ::xTaskCreate(task1, "task1", 128, nullptr, 2, nullptr);
    ::xTaskCreate(task2, "task2", 128, nullptr, 2, nullptr);

    ::puts("main(): starting scheduler...");

    ::vTaskStartScheduler();
}
