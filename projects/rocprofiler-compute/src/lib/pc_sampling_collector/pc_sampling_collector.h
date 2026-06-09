// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#pragma once
#include "code_object_translator.h"
#include "code_object_writer.h"
#include "pc_sample_writer.h"

#include <rocprofiler-sdk/rocprofiler.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace rocprofiler_compute_tool
{

enum class PcSamplingMode : uint8_t
{
    Disabled,
    Stochastic,
    HostTrap
};

class pc_sampling_collector_t
{
public:
    using ptr = std::shared_ptr<pc_sampling_collector_t>;
    static ptr create();

    virtual ~pc_sampling_collector_t() = default;
    virtual void on_code_object_load(const rocprofiler_callback_tracing_code_object_load_data_t& info) = 0;
    virtual void write(code_object_writer_t& writer) = 0;

    virtual void append_sample(const pc_sample_record_t& record) = 0;
    virtual void add_kernel_symbol(uint64_t code_object_id, const std::string& formatted_kernel_name) = 0;
    virtual instruction_t resolve_instruction(uint64_t code_object_id, uint64_t code_object_offset) = 0;
    virtual void   write_samples(pc_sample_writer_t& writer)                  = 0;
    virtual size_t snapshot_sources(const std::filesystem::path& output_root) = 0;
};

class pc_sampling_collector_impl_t : public pc_sampling_collector_t
{
public:
    pc_sampling_collector_impl_t(const std::shared_ptr<code_object_translator_t>& translator);
    void on_code_object_load(const rocprofiler_callback_tracing_code_object_load_data_t& info) override;
    void write(code_object_writer_t& writer) override;

    void append_sample(const pc_sample_record_t& record) override;
    void add_kernel_symbol(uint64_t code_object_id, const std::string& formatted_kernel_name) override;
    instruction_t resolve_instruction(uint64_t code_object_id, uint64_t code_object_offset) override;
    void   write_samples(pc_sample_writer_t& writer) override;
    size_t snapshot_sources(const std::filesystem::path& output_root) override;

private:
    std::shared_ptr<code_object_translator_t> m_translator;

    std::mutex                         m_mutex;
    std::vector<pc_sample_record_t>    m_samples;
    std::vector<kernel_symbol_entry_t> m_kernel_symbols;
    pc_string_interner_t               m_interner;
};
}  // namespace rocprofiler_compute_tool
