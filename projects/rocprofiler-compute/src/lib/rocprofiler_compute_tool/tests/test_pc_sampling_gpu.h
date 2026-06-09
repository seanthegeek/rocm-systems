// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#pragma once

// PC-sampling enums/types are experimental in the rocprofiler SDK; this macro
// must be defined before any <rocprofiler-sdk/...> include.
#define ROCPROFILER_SDK_EXPERIMENTAL

#include "sdk_wrapper.h"

#include <gtest/gtest.h>

#include <vector>

// GPU-gated integration fixture. The TEST_F self-skips when no real GPU agent
// is present so the suite stays green on CPU-only CI, but it references the
// not-yet-implemented SdkWrapperImpl PC-sampling methods so the link fails RED
// until those methods exist.
class TestPcSamplingGpu : public ::testing::Test
{
protected:
    rocprofiler_compute_tool::SdkWrapperImpl sdk{};
};
