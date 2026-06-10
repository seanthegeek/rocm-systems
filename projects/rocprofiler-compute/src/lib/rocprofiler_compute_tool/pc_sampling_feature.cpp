// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#include "pc_sampling_feature.h"

#include "code_object_writer.h"

using namespace rocprofiler_compute_tool;

PcSamplingMode rocprofiler_compute_tool::parse_pc_sampling_mode(const std::string& mode)
{
    if (mode == "stochastic")
        return PcSamplingMode::Stochastic;
    if (mode == "host_trap")
        return PcSamplingMode::HostTrap;
    return PcSamplingMode::Disabled;
}

pc_sampling_feature_t::pc_sampling_feature_t(PcSamplingMode        mode,
                                             std::filesystem::path output_root,
                                             std::filesystem::path code_obj_path,
                                             std::filesystem::path pc_samples_path)
    : pc_sampling_feature_t(mode,
                            std::move(output_root),
                            std::move(code_obj_path),
                            std::move(pc_samples_path),
                            pc_sampling_collector_t::create())
{
}

pc_sampling_feature_t::pc_sampling_feature_t(PcSamplingMode               mode,
                                             std::filesystem::path        output_root,
                                             std::filesystem::path        code_obj_path,
                                             std::filesystem::path        pc_samples_path,
                                             pc_sampling_collector_t::ptr collector)
    : m_enabled(true)
    , m_mode(mode)
    , m_output_path(std::move(code_obj_path))
    , m_output_root(std::move(output_root))
    , m_pc_samples_path(std::move(pc_samples_path))
    , m_collector(std::move(collector))
{
}

void pc_sampling_feature_t::on_code_object_load(const rocprofiler_callback_tracing_code_object_load_data_t& info)
{
    m_collector->on_code_object_load(info);
}

void pc_sampling_feature_t::append_sample(const pc_sample_record_t& record)
{
    m_collector->append_sample(record);
}

void pc_sampling_feature_t::add_kernel_symbol(uint64_t           code_object_id,
                                              const std::string& formatted_kernel_name)
{
    m_collector->add_kernel_symbol(code_object_id, formatted_kernel_name);
}

void pc_sampling_feature_t::finalize()
{
    code_object_writer_json_t writer;
    m_collector->write(writer);
    writer.flush(m_output_path);

    pc_sample_writer_json_t pc_writer;
    m_collector->write_samples(pc_writer);
    pc_writer.flush(m_pc_samples_path);

    m_collector->snapshot_sources(m_output_root);
}
