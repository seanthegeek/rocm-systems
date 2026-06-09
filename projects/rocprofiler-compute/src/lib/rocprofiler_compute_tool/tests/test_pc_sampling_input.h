// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#pragma once

#define ROCPROFILER_SDK_EXPERIMENTAL

#include "mocks.h"
#include "rocprofiler_compute_tool.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

// Fixture for the PC-sampling tool-side wiring tests. Mirrors
// TestRocprofilerComputeTool: it owns the Mock* shared_ptrs and injects them
// through test_knobs so that rocprofiler_configure()/initialize() drive the
// real tool code against the mocks. It additionally exposes the Envp helper
// (copied from test_environ_cache.h) so EnvInputParameters can be exercised
// with an injected EnvironCache instead of the live process environment.
class TestPcSamplingInput : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    // Owns the storage backing a NULL-terminated char** envp.
    // Non-copyable and non-movable: tests must declare it as a named local
    // so the validity scope of data() is explicit.
    class Envp
    {
    public:
        explicit Envp(const std::vector<std::string>& entries)
        {
            m_owned.reserve(entries.size());
            m_pointers.reserve(entries.size() + 1);

            for (const auto& entry : entries)
            {
                m_owned.emplace_back(entry.begin(), entry.end());
                m_owned.back().push_back('\0');
                m_pointers.push_back(m_owned.back().data());
            }

            m_pointers.push_back(nullptr);
        }

        Envp(const Envp&)            = delete;
        Envp& operator=(const Envp&) = delete;
        Envp(Envp&&)                 = delete;
        Envp& operator=(Envp&&)      = delete;

        char** data() { return m_pointers.data(); }

    private:
        std::vector<std::vector<char>> m_owned;
        std::vector<char*>             m_pointers;
    };

    static rocprofiler_compute_tool::tool_data_t* get_tool_data(const rocprofiler_tool_configure_result_t* cfg);

    // Drives the on_hsa_runtime_loaded path: registers the HSA intercept by
    // running initialize(), then fires the recorded intercept callback once.
    void drive_hsa_runtime_loaded();

    rocprofiler_client_id_t              m_client_id{};
    std::shared_ptr<MockInputParameters> m_input_parameters;
    std::shared_ptr<MockSdkWrapper>      m_sdk_wrapper;
    std::shared_ptr<MockCountersWriter>  m_counters_writer;
};
