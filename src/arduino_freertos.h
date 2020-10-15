/*
 * Copyright (c) 2020 Timo Sandmann
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file    arduino_freertos.h
 * @brief   Wrapper to include arduino.h if available
 * @author  Timo Sandmann
 * @date    15.10.2020
 */

#pragma once

#ifndef _GLIBCXX_HAS_GTHREADS
#define _GLIBCXX_HAS_GTHREADS
#endif

#include "FreeRTOS.h"
#include "task.h"

#ifdef __has_include
#if __has_include ("arduino.h")
#include "arduino.h"
#endif
#endif 
