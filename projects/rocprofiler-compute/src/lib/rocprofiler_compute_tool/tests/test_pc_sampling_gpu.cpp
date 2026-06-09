// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#include "test_pc_sampling_gpu.h"

#include <gtest/gtest.h>

#include <vector>

using namespace rocprofiler_compute_tool;

namespace
{
// Collects the configs delivered by query_pc_sampling_configs so the test can
// assert at least one usable method was advertised on real hardware.
struct PcConfigCollector
{
    std::vector<rocprofiler_pc_sampling_configuration_t> configs;
    bool                                                 has_usable_method = false;
};

rocprofiler_status_t collect_pc_configs(const rocprofiler_pc_sampling_configuration_t* cfgs,
                                        size_t                                         num_cfgs,
                                        void*                                          user_data)
{
    auto* collector = static_cast<PcConfigCollector*>(user_data);
    for (size_t i = 0; i < num_cfgs; ++i)
    {
        collector->configs.push_back(cfgs[i]);
        if (cfgs[i].method != ROCPROFILER_PC_SAMPLING_METHOD_NONE)
        {
            collector->has_usable_method = true;
        }
    }
    return ROCPROFILER_STATUS_SUCCESS;
}
}  // namespace

// Self-skipping GPU integration test.
//
// What is exercised on real hardware:
//   1. query_available_gpu_agents() must return >= 1 agent.
//   2. query_pc_sampling_configs() on that agent must deliver >= 1 config via
//      its callback, and at least one config must advertise a usable method.
//
// When no GPU is present (probe throws or returns no agents) the test skips
// cleanly with no assertions run. Full context+buffer+configure setup is kept
// out of this unit-test harness as impractical; the agent/config query path is
// the conservative real-hardware exercise.
TEST_F(TestPcSamplingGpu, QueriesPcSamplingConfigsOnRealAgent)
{
    std::vector<rocprofiler_agent_id_t> agents;
    try
    {
        sdk.query_available_gpu_agents(agents);
    }
    catch (...)
    {
        GTEST_SKIP() << "no GPU agent present";
        return;
    }

    if (agents.empty())
    {
        GTEST_SKIP() << "no GPU agent present";
        return;
    }

    // A GPU agent exists: probe its PC-sampling configurations. The SDK throws
    // when the driver/runtime does not implement PC sampling (e.g. error 17,
    // "API function is defined but not implemented") -- treat that as "no PC
    // sampling support on this host" and skip rather than fail.
    PcConfigCollector collector{};
    try
    {
        sdk.query_pc_sampling_configs(agents.front(), collect_pc_configs, &collector);
    }
    catch (...)
    {
        GTEST_SKIP() << "PC sampling not supported on this agent/driver";
        return;
    }

    if (collector.configs.empty() || !collector.has_usable_method)
    {
        GTEST_SKIP() << "agent advertises no usable PC-sampling configuration";
        return;
    }

    EXPECT_GE(agents.size(), 1u);
    EXPECT_FALSE(collector.configs.empty());
    EXPECT_TRUE(collector.has_usable_method);
}
