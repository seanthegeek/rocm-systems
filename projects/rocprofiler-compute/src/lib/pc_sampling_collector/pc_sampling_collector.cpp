// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#include "pc_sampling_collector.h"

#include "gsl_assert.h"
#include "source_snapshot.h"

#include <unistd.h>

#include <ios>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace rocprofiler_compute_tool;

namespace
{
struct pc_location_hash_t
{
    size_t operator()(const std::pair<uint64_t, uint64_t>& p) const
    {
        const size_t h1 = std::hash<uint64_t>{}(p.first);
        const size_t h2 = std::hash<uint64_t>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};
}  // namespace

pc_sampling_collector_t::ptr pc_sampling_collector_t::create()
{
    return std::make_shared<pc_sampling_collector_impl_t>(
        std::make_shared<code_object_translator_impl_t>());
}

pc_sampling_collector_impl_t::pc_sampling_collector_impl_t(
    const std::shared_ptr<code_object_translator_t>& translator)
    : m_translator(translator)
{
}

void pc_sampling_collector_impl_t::on_code_object_load(
    const rocprofiler_callback_tracing_code_object_load_data_t& info)
{
    if (info.storage_type == ROCPROFILER_CODE_OBJECT_STORAGE_TYPE_FILE)
    {
        m_translator->add_code_object(info.uri, info.code_object_id, info.load_base, info.load_size);
    }
    else if (info.storage_type == ROCPROFILER_CODE_OBJECT_STORAGE_TYPE_MEMORY)
    {
        m_translator->add_code_object(info.memory_base,
                                      info.memory_size,
                                      info.code_object_id,
                                      info.load_base,
                                      info.load_size);
    }
}

void pc_sampling_collector_impl_t::write(code_object_writer_t& writer)
{
    for (const auto& id : m_translator->get_code_object_ids())
    {
        writer.start_code_obj(id);
        const auto& symbols = m_translator->get_symbols(id);
        for (const auto& sym : symbols)
        {
            writer.start_symbol(sym);
            uint64_t       pc  = sym.virtual_address;
            const uint64_t end = sym.virtual_address + sym.size;
            while (pc < end)
            {
                const auto& inst = m_translator->get_instruction(id, pc);
                Expects(inst.size);
                writer.write_instruction(inst);
                pc += inst.size;
            }
            writer.end_symbol();
        }
        writer.end_code_obj();
    }
}

void pc_sampling_collector_impl_t::append_sample(const pc_sample_record_t& record)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_samples.push_back(record);
}

void pc_sampling_collector_impl_t::add_kernel_symbol(uint64_t           code_object_id,
                                                     const std::string& formatted_kernel_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_kernel_symbols.push_back(kernel_symbol_entry_t{code_object_id, formatted_kernel_name});
}

instruction_t pc_sampling_collector_impl_t::resolve_instruction(uint64_t code_object_id,
                                                                uint64_t code_object_offset)
{
    // PC sample offsets are relative to the code object's load base, but
    // get_instruction keys on a global virtual address. Translate before lookup.
    try
    {
        const uint64_t vaddr = code_object_offset + m_translator->get_load_base(code_object_id);
        return m_translator->get_instruction(code_object_id, vaddr);
    }
    catch (const std::out_of_range&)
    {
        return instruction_t{};
    }
}

void pc_sampling_collector_impl_t::write_samples(pc_sample_writer_t& writer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    writer.begin();
    // PC sampling concentrates many samples on the same instruction, so cache
    // the interned index per (code_object_id, code_object_offset) to avoid
    // re-disassembling and re-interning the same PC for every sample.
    std::unordered_map<std::pair<uint64_t, uint64_t>, size_t, pc_location_hash_t> idx_by_location;
    for (const auto& sample : m_samples)
    {
        const auto location = std::make_pair(sample.pc.code_object_id, sample.pc.code_object_offset);
        size_t idx = 0;
        if (const auto it = idx_by_location.find(location); it != idx_by_location.end())
        {
            idx = it->second;
        }
        else
        {
            const instruction_t inst = resolve_instruction(sample.pc.code_object_id,
                                                           sample.pc.code_object_offset);
            idx                      = m_interner.intern(inst.name, inst.comment);
            idx_by_location.emplace(location, idx);
        }

        switch (sample.kind)
        {
        case pc_sample_kind_t::Stochastic:
            writer.append_stochastic(sample, idx);
            break;
        case pc_sample_kind_t::HostTrap:
            writer.append_host_trap(sample, idx);
            break;
        }
    }

    writer.set_strings(m_interner);
    writer.set_kernel_symbols(m_kernel_symbols);
    writer.set_metadata(static_cast<int>(getpid()));
}

size_t pc_sampling_collector_impl_t::snapshot_sources(const std::filesystem::path& output_root)
{
    std::vector<std::string> refs;
    for_each_instruction(
        [&refs](uint64_t /*id*/, const symbol_t& /*sym*/, const instruction_t& inst)
        {
            if (const auto ref = parse_source_ref(inst.comment))
            {
                refs.push_back(*ref);
            }
        });

    // snapshot_source_files dedups internally, so no need to pre-unique here.
    return snapshot_source_files(refs, output_root);
}
