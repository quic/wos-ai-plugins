/*
**************************************************************************************************
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

#pragma once

#include <string.h>
#include <chrono>

#include "Helpers.hpp"


#define QNN_ERROR(fmt, ...) DEMO_ERROR(fmt, ##__VA_ARGS__)

#define QNN_WARN(fmt, ...) DEMO_WARN(fmt, ##__VA_ARGS__)

#define QNN_DEBUG(fmt, ...) DEMO_DEBUG(fmt, ##__VA_ARGS__)
