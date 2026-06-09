// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#ifndef ROCPROFILER_SDK_EXPERIMENTAL
#    define ROCPROFILER_SDK_EXPERIMENTAL
#endif

#include "pc_sample_writer.h"

#include "gsl_assert.h"
#include "nlohmann/json.hpp"

#include <rocprofiler-sdk/fwd.h>
#include <rocprofiler-sdk/pc_sampling.h>

#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace rocprofiler_compute_tool;

namespace
{
std::string instruction_type_name(uint32_t inst_type)
{
    const char* name = rocprofiler_get_pc_sampling_instruction_type_name(
        static_cast<rocprofiler_pc_sampling_instruction_type_t>(inst_type));
    return name != nullptr ? std::string{name} : std::string{};
}

std::string not_issued_reason_name(uint32_t reason)
{
    const char* name = rocprofiler_get_pc_sampling_instruction_not_issued_reason_name(
        static_cast<rocprofiler_pc_sampling_instruction_not_issued_reason_t>(reason));
    return name != nullptr ? std::string{name} : std::string{};
}

pc_sample_hw_id_t map_hw_id(const rocprofiler_pc_sampling_hw_id_v0_t& hw)
{
    pc_sample_hw_id_t out{};
    out.chiplet          = hw.chiplet;
    out.wave_id          = hw.wave_id;
    out.simd_id          = hw.simd_id;
    out.pipe_id          = hw.pipe_id;
    out.cu_or_wgp_id     = hw.cu_or_wgp_id;
    out.shader_array_id  = hw.shader_array_id;
    out.shader_engine_id = hw.shader_engine_id;
    out.workgroup_id     = hw.workgroup_id;
    out.vm_id            = hw.vm_id;
    out.queue_id         = hw.queue_id;
    out.microengine_id   = hw.microengine_id;
    return out;
}

pc_sample_pc_t map_pc(const rocprofiler_pc_t& pc)
{
    pc_sample_pc_t out{};
    out.code_object_id     = pc.code_object_id;
    out.code_object_offset = pc.code_object_offset;
    return out;
}

pc_sample_dim3_t map_dim3(const rocprofiler_dim3_t& d)
{
    pc_sample_dim3_t out{};
    out.x = d.x;
    out.y = d.y;
    out.z = d.z;
    return out;
}

nlohmann::json hw_id_to_json(const pc_sample_hw_id_t& hw)
{
    return nlohmann::json::object({
        {"chiplet", hw.chiplet},
        {"wave_id", hw.wave_id},
        {"simd_id", hw.simd_id},
        {"pipe_id", hw.pipe_id},
        {"cu_or_wgp_id", hw.cu_or_wgp_id},
        {"shader_array_id", hw.shader_array_id},
        {"shader_engine_id", hw.shader_engine_id},
        {"workgroup_id", hw.workgroup_id},
        {"vm_id", hw.vm_id},
        {"queue_id", hw.queue_id},
        {"microengine_id", hw.microengine_id},
    });
}

nlohmann::json pc_to_json(const pc_sample_pc_t& pc)
{
    return nlohmann::json::object({
        {"code_object_id", pc.code_object_id},
        {"code_object_offset", pc.code_object_offset},
    });
}

nlohmann::json corr_to_json(const pc_sample_correlation_t& corr)
{
    return nlohmann::json::object({
        {"internal", corr.internal},
        {"external", corr.external},
    });
}

nlohmann::json dim3_to_json(const pc_sample_dim3_t& d)
{
    return nlohmann::json::object({
        {"x", d.x},
        {"y", d.y},
        {"z", d.z},
    });
}

nlohmann::json snapshot_to_json(const pc_sample_snapshot_t& s)
{
    return nlohmann::json::object({
        {"stall_reason", s.stall_reason},
        {"dual_issue_valu", s.dual_issue_valu},
        {"arb_state_issue_valu", s.arb_state_issue_valu},
        {"arb_state_issue_matrix", s.arb_state_issue_matrix},
        {"arb_state_issue_lds", s.arb_state_issue_lds},
        {"arb_state_issue_lds_direct", s.arb_state_issue_lds_direct},
        {"arb_state_issue_scalar", s.arb_state_issue_scalar},
        {"arb_state_issue_vmem_tex", s.arb_state_issue_vmem_tex},
        {"arb_state_issue_flat", s.arb_state_issue_flat},
        {"arb_state_issue_exp", s.arb_state_issue_exp},
        {"arb_state_issue_misc", s.arb_state_issue_misc},
        {"arb_state_issue_brmsg", s.arb_state_issue_brmsg},
        {"arb_state_stall_valu", s.arb_state_stall_valu},
        {"arb_state_stall_matrix", s.arb_state_stall_matrix},
        {"arb_state_stall_lds", s.arb_state_stall_lds},
        {"arb_state_stall_lds_direct", s.arb_state_stall_lds_direct},
        {"arb_state_stall_scalar", s.arb_state_stall_scalar},
        {"arb_state_stall_vmem_tex", s.arb_state_stall_vmem_tex},
        {"arb_state_stall_flat", s.arb_state_stall_flat},
        {"arb_state_stall_exp", s.arb_state_stall_exp},
        {"arb_state_stall_misc", s.arb_state_stall_misc},
        {"arb_state_stall_brmsg", s.arb_state_stall_brmsg},
    });
}

nlohmann::json stochastic_record_to_json(const pc_sample_record_t& r)
{
    return nlohmann::json::object({
        {"flags", nlohmann::json::object({{"has_mem_cnt", r.flags.has_mem_cnt}})},
        {"hw_id", hw_id_to_json(r.hw_id)},
        {"pc", pc_to_json(r.pc)},
        {"exec_mask", r.exec_mask},
        {"timestamp", r.timestamp},
        {"dispatch_id", r.dispatch_id},
        {"corr_id", corr_to_json(r.corr_id)},
        {"wrkgrp_id", dim3_to_json(r.wrkgrp_id)},
        {"wave_in_grp", r.wave_in_grp},
        {"wave_issued", r.wave_issued},
        {"inst_type", r.inst_type},
        {"wave_cnt", r.wave_cnt},
        {"snapshot", snapshot_to_json(r.snapshot)},
    });
}

nlohmann::json host_trap_record_to_json(const pc_sample_record_t& r)
{
    return nlohmann::json::object({
        {"hw_id", hw_id_to_json(r.hw_id)},
        {"pc", pc_to_json(r.pc)},
        {"exec_mask", r.exec_mask},
        {"timestamp", r.timestamp},
        {"dispatch_id", r.dispatch_id},
        {"corr_id", corr_to_json(r.corr_id)},
        {"wrkgrp_id", dim3_to_json(r.wrkgrp_id)},
        {"wave_in_grp", r.wave_in_grp},
    });
}
}  // namespace

size_t pc_string_interner_t::intern(const std::string& instruction_text, const std::string& comment)
{
    auto key = std::make_pair(instruction_text, comment);
    auto it  = m_index.find(key);
    if (it != m_index.end())
        return it->second;

    size_t idx = m_instructions.size();
    m_instructions.push_back(instruction_text);
    m_comments.push_back(comment);
    m_index.emplace(std::move(key), idx);
    return idx;
}

const std::vector<std::string>& pc_string_interner_t::instructions() const
{
    return m_instructions;
}

const std::vector<std::string>& pc_string_interner_t::comments() const
{
    return m_comments;
}

std::optional<pc_sample_record_t> rocprofiler_compute_tool::decode_pc_sample_record(
    const rocprofiler_record_header_t& header)
{
    if (header.category != ROCPROFILER_BUFFER_CATEGORY_PC_SAMPLING)
        return std::nullopt;

    if (header.kind == ROCPROFILER_PC_SAMPLING_RECORD_STOCHASTIC_V0_SAMPLE)
    {
        const auto& rec = *reinterpret_cast<const rocprofiler_pc_sampling_record_stochastic_v0_t*>(
            header.payload);

        pc_sample_record_t out{};
        out.kind              = pc_sample_kind_t::Stochastic;
        out.flags.has_mem_cnt = rec.flags.has_memory_counter;
        out.hw_id             = map_hw_id(rec.hw_id);
        out.pc                = map_pc(rec.pc);
        out.exec_mask         = rec.exec_mask;
        out.timestamp         = rec.timestamp;
        out.dispatch_id       = rec.dispatch_id;
        out.corr_id.internal  = rec.correlation_id.internal;
        out.corr_id.external  = rec.correlation_id.external.value;
        out.wrkgrp_id         = map_dim3(rec.workgroup_id);
        out.wave_in_grp       = rec.wave_in_group;
        out.wave_issued       = rec.wave_issued;
        out.inst_type         = instruction_type_name(rec.inst_type);
        out.wave_cnt          = rec.wave_count;

        out.snapshot.stall_reason         = not_issued_reason_name(rec.snapshot.reason_not_issued);
        out.snapshot.dual_issue_valu      = rec.snapshot.dual_issue_valu;
        out.snapshot.arb_state_issue_valu = rec.snapshot.arb_state_issue_valu;
        out.snapshot.arb_state_issue_matrix     = rec.snapshot.arb_state_issue_matrix;
        out.snapshot.arb_state_issue_lds        = rec.snapshot.arb_state_issue_lds;
        out.snapshot.arb_state_issue_lds_direct = rec.snapshot.arb_state_issue_lds_direct;
        out.snapshot.arb_state_issue_scalar     = rec.snapshot.arb_state_issue_scalar;
        out.snapshot.arb_state_issue_vmem_tex   = rec.snapshot.arb_state_issue_vmem_tex;
        out.snapshot.arb_state_issue_flat       = rec.snapshot.arb_state_issue_flat;
        out.snapshot.arb_state_issue_exp        = rec.snapshot.arb_state_issue_exp;
        out.snapshot.arb_state_issue_misc       = rec.snapshot.arb_state_issue_misc;
        out.snapshot.arb_state_issue_brmsg      = rec.snapshot.arb_state_issue_brmsg;
        out.snapshot.arb_state_stall_valu       = rec.snapshot.arb_state_stall_valu;
        out.snapshot.arb_state_stall_matrix     = rec.snapshot.arb_state_stall_matrix;
        out.snapshot.arb_state_stall_lds        = rec.snapshot.arb_state_stall_lds;
        out.snapshot.arb_state_stall_lds_direct = rec.snapshot.arb_state_stall_lds_direct;
        out.snapshot.arb_state_stall_scalar     = rec.snapshot.arb_state_stall_scalar;
        out.snapshot.arb_state_stall_vmem_tex   = rec.snapshot.arb_state_stall_vmem_tex;
        out.snapshot.arb_state_stall_flat       = rec.snapshot.arb_state_stall_flat;
        out.snapshot.arb_state_stall_exp        = rec.snapshot.arb_state_stall_exp;
        out.snapshot.arb_state_stall_misc       = rec.snapshot.arb_state_stall_misc;
        out.snapshot.arb_state_stall_brmsg      = rec.snapshot.arb_state_stall_brmsg;

        return out;
    }

    if (header.kind == ROCPROFILER_PC_SAMPLING_RECORD_HOST_TRAP_V0_SAMPLE)
    {
        const auto& rec = *reinterpret_cast<const rocprofiler_pc_sampling_record_host_trap_v0_t*>(
            header.payload);

        pc_sample_record_t out{};
        out.kind             = pc_sample_kind_t::HostTrap;
        out.hw_id            = map_hw_id(rec.hw_id);
        out.pc               = map_pc(rec.pc);
        out.exec_mask        = rec.exec_mask;
        out.timestamp        = rec.timestamp;
        out.dispatch_id      = rec.dispatch_id;
        out.corr_id.internal = rec.correlation_id.internal;
        out.corr_id.external = rec.correlation_id.external.value;
        out.wrkgrp_id        = map_dim3(rec.workgroup_id);
        out.wave_in_grp      = rec.wave_in_group;

        return out;
    }

    return std::nullopt;
}

void pc_sample_writer_json_t::begin()
{
    m_pid = 0;
    m_stochastic.clear();
    m_host_trap.clear();
    m_instructions.clear();
    m_comments.clear();
    m_kernel_symbols.clear();
}

void pc_sample_writer_json_t::append_stochastic(const pc_sample_record_t& r)
{
    m_stochastic.push_back(r);
}

void pc_sample_writer_json_t::append_host_trap(const pc_sample_record_t& r)
{
    m_host_trap.push_back(r);
}

void pc_sample_writer_json_t::set_strings(const pc_string_interner_t& interner)
{
    m_instructions = interner.instructions();
    m_comments     = interner.comments();
}

void pc_sample_writer_json_t::set_kernel_symbols(const std::vector<kernel_symbol_entry_t>& syms)
{
    m_kernel_symbols = syms;
}

void pc_sample_writer_json_t::set_metadata(int pid)
{
    m_pid = pid;
}

std::string pc_sample_writer_json_t::get_result()
{
    auto stochastic_records = nlohmann::json::array();
    for (const auto& r : m_stochastic)
    {
        stochastic_records.push_back(nlohmann::json::object({
            {"record", stochastic_record_to_json(r)},
            {"inst_index", r.inst_index},
        }));
    }

    auto host_trap_records = nlohmann::json::array();
    for (const auto& r : m_host_trap)
    {
        host_trap_records.push_back(nlohmann::json::object({
            {"record", host_trap_record_to_json(r)},
            {"inst_index", r.inst_index},
        }));
    }

    auto kernel_symbols = nlohmann::json::array();
    for (const auto& s : m_kernel_symbols)
    {
        kernel_symbols.push_back(nlohmann::json::object({
            {"code_object_id", s.code_object_id},
            {"formatted_kernel_name", s.formatted_kernel_name},
        }));
    }

    auto entry = nlohmann::json::object({
        {"metadata", nlohmann::json::object({{"pid", m_pid}})},
        {"agents", nlohmann::json::array()},
        {"counters", nlohmann::json::array()},
        {"summary", nlohmann::json::array()},
        {"host_functions", nlohmann::json::array()},
        {"callback_records", nlohmann::json::object()},
        {"code_objects", nlohmann::json::array()},
        {"kernel_symbols", std::move(kernel_symbols)},
        {"strings",
         nlohmann::json::object({
             {"pc_sample_instructions", m_instructions},
             {"pc_sample_comments", m_comments},
             {"code_object_snapshot_filenames", nlohmann::json::array()},
             {"att_filenames", nlohmann::json::array()},
         })},
        {"buffer_records",
         nlohmann::json::object({
             {"pc_sample_stochastic", std::move(stochastic_records)},
             {"pc_sample_host_trap", std::move(host_trap_records)},
             {"kernel_dispatch", nlohmann::json::array()},
             {"hip_api", nlohmann::json::array()},
             {"hsa_api", nlohmann::json::array()},
             {"memory_copy", nlohmann::json::array()},
             {"marker_api", nlohmann::json::array()},
             {"rccl_api", nlohmann::json::array()},
             {"memory_allocation", nlohmann::json::array()},
             {"scratch_memory", nlohmann::json::array()},
             {"kfd", nlohmann::json::array()},
             {"rocdecode_api", nlohmann::json::array()},
             {"rocjpeg_api", nlohmann::json::array()},
         })},
    });

    auto root = nlohmann::json::object({
        {"rocprofiler-sdk-tool", nlohmann::json::array({std::move(entry)})},
    });

    return root.dump();
}

void pc_sample_writer_json_t::flush(const std::filesystem::path& output_file_path)
{
    Expects(!output_file_path.empty());
    create_parent_dir(output_file_path);

    std::ofstream out_file(output_file_path, std::ios::out);
    if (!out_file.is_open())
    {
        std::cerr << "Failed to open output file: " << output_file_path << "\n";
        return;
    }
    out_file << get_result();
    std::clog << "[rocprofiler-compute] [" << __FUNCTION__
              << "] PC sampling data has been written to: " << output_file_path << "\n";
}

void pc_sample_writer_json_t::create_parent_dir(const std::filesystem::path& output_file_path)
{
    Expects(output_file_path.has_parent_path());
    std::error_code error;
    std::filesystem::create_directories(output_file_path.parent_path(), error);
    if (error)
    {
        throw std::runtime_error("Failed to create output directory: " + output_file_path.string() +
                                 ", error: " + error.message());
    }
}
