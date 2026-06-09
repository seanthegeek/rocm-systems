// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#include "test_pc_sample_writer.h"

#include "nlohmann/json.hpp"

// (a) top key "rocprofiler-sdk-tool" is a 1-element array.
TEST_F(test_pc_sample_writer_t, ProvidedBegin_TopKeyIsSingleElementArray)
{
    m_writer.begin();

    const auto json = nlohmann::json::parse(m_writer.get_result());
    ASSERT_TRUE(json.contains("rocprofiler-sdk-tool"));
    ASSERT_TRUE(json["rocprofiler-sdk-tool"].is_array());
    EXPECT_EQ(json["rocprofiler-sdk-tool"].size(), 1u);
}

// (b) A stochastic record lands in buffer_records.pc_sample_stochastic[0] with
//     inst_index as a sibling of record and all distinctive fields serialized.
TEST_F(test_pc_sample_writer_t, ProvidedStochasticRecord_SerializesUnderStochasticBuffer)
{
    const auto record = make_stochastic_record();

    m_writer.begin();
    m_writer.append_stochastic(record);

    const auto  json       = nlohmann::json::parse(m_writer.get_result());
    const auto& root       = json["rocprofiler-sdk-tool"][0];
    const auto& stochastic = root["buffer_records"]["pc_sample_stochastic"];
    ASSERT_EQ(stochastic.size(), 1u);

    const auto& entry = stochastic[0];

    // inst_index is a SIBLING of "record", not inside it.
    ASSERT_TRUE(entry.contains("inst_index"));
    EXPECT_EQ(entry["inst_index"], 5);

    const auto& rec = entry["record"];
    ASSERT_FALSE(rec.contains("inst_index"));

    // flags
    EXPECT_EQ(rec["flags"]["has_mem_cnt"], record.flags.has_mem_cnt);

    // hw_id (all members)
    EXPECT_EQ(rec["hw_id"]["chiplet"], record.hw_id.chiplet);
    EXPECT_EQ(rec["hw_id"]["wave_id"], record.hw_id.wave_id);
    EXPECT_EQ(rec["hw_id"]["simd_id"], record.hw_id.simd_id);
    EXPECT_EQ(rec["hw_id"]["pipe_id"], record.hw_id.pipe_id);
    EXPECT_EQ(rec["hw_id"]["cu_or_wgp_id"], record.hw_id.cu_or_wgp_id);
    EXPECT_EQ(rec["hw_id"]["shader_array_id"], record.hw_id.shader_array_id);
    EXPECT_EQ(rec["hw_id"]["shader_engine_id"], record.hw_id.shader_engine_id);
    EXPECT_EQ(rec["hw_id"]["workgroup_id"], record.hw_id.workgroup_id);
    EXPECT_EQ(rec["hw_id"]["vm_id"], record.hw_id.vm_id);
    EXPECT_EQ(rec["hw_id"]["queue_id"], record.hw_id.queue_id);
    EXPECT_EQ(rec["hw_id"]["microengine_id"], record.hw_id.microengine_id);

    // pc
    EXPECT_EQ(rec["pc"]["code_object_id"], record.pc.code_object_id);
    EXPECT_EQ(rec["pc"]["code_object_offset"], record.pc.code_object_offset);

    // scalars
    EXPECT_EQ(rec["exec_mask"], record.exec_mask);
    EXPECT_EQ(rec["timestamp"], record.timestamp);
    EXPECT_EQ(rec["dispatch_id"], record.dispatch_id);

    // corr_id
    EXPECT_EQ(rec["corr_id"]["internal"], record.corr_id.internal);
    EXPECT_EQ(rec["corr_id"]["external"], record.corr_id.external);

    // wrkgrp_id
    EXPECT_EQ(rec["wrkgrp_id"]["x"], record.wrkgrp_id.x);
    EXPECT_EQ(rec["wrkgrp_id"]["y"], record.wrkgrp_id.y);
    EXPECT_EQ(rec["wrkgrp_id"]["z"], record.wrkgrp_id.z);

    EXPECT_EQ(rec["wave_in_grp"], record.wave_in_grp);
    EXPECT_EQ(rec["wave_issued"], record.wave_issued);
    EXPECT_EQ(rec["wave_cnt"], record.wave_cnt);

    // inst_type is a JSON string equal to the set value.
    ASSERT_TRUE(rec["inst_type"].is_string());
    EXPECT_EQ(rec["inst_type"], record.inst_type);

    // snapshot.stall_reason is a JSON string equal to the set value.
    const auto& snap = rec["snapshot"];
    ASSERT_TRUE(snap["stall_reason"].is_string());
    EXPECT_EQ(snap["stall_reason"], record.snapshot.stall_reason);
    EXPECT_EQ(snap["dual_issue_valu"], record.snapshot.dual_issue_valu);

    EXPECT_EQ(snap["arb_state_issue_valu"], record.snapshot.arb_state_issue_valu);
    EXPECT_EQ(snap["arb_state_issue_matrix"], record.snapshot.arb_state_issue_matrix);
    EXPECT_EQ(snap["arb_state_issue_lds"], record.snapshot.arb_state_issue_lds);
    EXPECT_EQ(snap["arb_state_issue_lds_direct"], record.snapshot.arb_state_issue_lds_direct);
    EXPECT_EQ(snap["arb_state_issue_scalar"], record.snapshot.arb_state_issue_scalar);
    EXPECT_EQ(snap["arb_state_issue_vmem_tex"], record.snapshot.arb_state_issue_vmem_tex);
    EXPECT_EQ(snap["arb_state_issue_flat"], record.snapshot.arb_state_issue_flat);
    EXPECT_EQ(snap["arb_state_issue_exp"], record.snapshot.arb_state_issue_exp);
    EXPECT_EQ(snap["arb_state_issue_misc"], record.snapshot.arb_state_issue_misc);
    EXPECT_EQ(snap["arb_state_issue_brmsg"], record.snapshot.arb_state_issue_brmsg);

    EXPECT_EQ(snap["arb_state_stall_valu"], record.snapshot.arb_state_stall_valu);
    EXPECT_EQ(snap["arb_state_stall_matrix"], record.snapshot.arb_state_stall_matrix);
    EXPECT_EQ(snap["arb_state_stall_lds"], record.snapshot.arb_state_stall_lds);
    EXPECT_EQ(snap["arb_state_stall_lds_direct"], record.snapshot.arb_state_stall_lds_direct);
    EXPECT_EQ(snap["arb_state_stall_scalar"], record.snapshot.arb_state_stall_scalar);
    EXPECT_EQ(snap["arb_state_stall_vmem_tex"], record.snapshot.arb_state_stall_vmem_tex);
    EXPECT_EQ(snap["arb_state_stall_flat"], record.snapshot.arb_state_stall_flat);
    EXPECT_EQ(snap["arb_state_stall_exp"], record.snapshot.arb_state_stall_exp);
    EXPECT_EQ(snap["arb_state_stall_misc"], record.snapshot.arb_state_stall_misc);
    EXPECT_EQ(snap["arb_state_stall_brmsg"], record.snapshot.arb_state_stall_brmsg);
}

// (c) A host_trap record lands under pc_sample_host_trap[0] and the record has
//     NO snapshot / wave_cnt / inst_type keys.
TEST_F(test_pc_sample_writer_t, ProvidedHostTrapRecord_SerializesWithoutStochasticOnlyFields)
{
    const auto record = make_host_trap_record();

    m_writer.begin();
    m_writer.append_host_trap(record);

    const auto  json      = nlohmann::json::parse(m_writer.get_result());
    const auto& root      = json["rocprofiler-sdk-tool"][0];
    const auto& host_trap = root["buffer_records"]["pc_sample_host_trap"];
    ASSERT_EQ(host_trap.size(), 1u);

    const auto& entry = host_trap[0];
    ASSERT_TRUE(entry.contains("inst_index"));
    EXPECT_EQ(entry["inst_index"], 8);

    const auto& rec = entry["record"];
    EXPECT_FALSE(rec.contains("snapshot"));
    EXPECT_FALSE(rec.contains("wave_cnt"));
    EXPECT_FALSE(rec.contains("inst_type"));

    // Host-trap fields that should still be present.
    ASSERT_TRUE(rec.contains("hw_id"));
    ASSERT_TRUE(rec.contains("pc"));
    EXPECT_EQ(rec["exec_mask"], record.exec_mask);
    EXPECT_EQ(rec["timestamp"], record.timestamp);
    EXPECT_EQ(rec["dispatch_id"], record.dispatch_id);
    EXPECT_EQ(rec["corr_id"]["internal"], record.corr_id.internal);
    EXPECT_EQ(rec["corr_id"]["external"], record.corr_id.external);
    EXPECT_EQ(rec["wrkgrp_id"]["x"], record.wrkgrp_id.x);
    EXPECT_EQ(rec["wave_in_grp"], record.wave_in_grp);
}

// (d) set_strings -> strings.pc_sample_instructions / pc_sample_comments.
TEST_F(test_pc_sample_writer_t, ProvidedInternedStrings_SerializesInstructionsAndComments)
{
    rocprofiler_compute_tool::pc_string_interner_t interner;
    interner.intern("s_nop", "a.cpp:1");
    interner.intern("v_add", "b.cpp:2");

    m_writer.begin();
    m_writer.set_strings(interner);

    const auto  json    = nlohmann::json::parse(m_writer.get_result());
    const auto& strings = json["rocprofiler-sdk-tool"][0]["strings"];

    ASSERT_TRUE(strings["pc_sample_instructions"].is_array());
    ASSERT_EQ(strings["pc_sample_instructions"].size(), 2u);
    EXPECT_EQ(strings["pc_sample_instructions"][0], "s_nop");
    EXPECT_EQ(strings["pc_sample_instructions"][1], "v_add");

    ASSERT_TRUE(strings["pc_sample_comments"].is_array());
    ASSERT_EQ(strings["pc_sample_comments"].size(), 2u);
    EXPECT_EQ(strings["pc_sample_comments"][0], "a.cpp:1");
    EXPECT_EQ(strings["pc_sample_comments"][1], "b.cpp:2");
}

// (e) set_kernel_symbols -> kernel_symbols array entries.
TEST_F(test_pc_sample_writer_t, ProvidedKernelSymbols_SerializesThemInOrder)
{
    const std::vector<rocprofiler_compute_tool::kernel_symbol_entry_t> syms{
        {2, "foo"},
        {3, "bar"},
    };

    m_writer.begin();
    m_writer.set_kernel_symbols(syms);

    const auto  json           = nlohmann::json::parse(m_writer.get_result());
    const auto& kernel_symbols = json["rocprofiler-sdk-tool"][0]["kernel_symbols"];

    ASSERT_EQ(kernel_symbols.size(), 2u);
    EXPECT_EQ(kernel_symbols[0]["code_object_id"], 2);
    EXPECT_EQ(kernel_symbols[0]["formatted_kernel_name"], "foo");
    EXPECT_EQ(kernel_symbols[1]["code_object_id"], 3);
    EXPECT_EQ(kernel_symbols[1]["formatted_kernel_name"], "bar");
}

// (f) Every API-trace category is present and is an empty array.
TEST_F(test_pc_sample_writer_t, ProvidedNoApiTraces_AllApiCategoriesAreEmptyArrays)
{
    m_writer.begin();

    const auto  json           = nlohmann::json::parse(m_writer.get_result());
    const auto& buffer_records = json["rocprofiler-sdk-tool"][0]["buffer_records"];

    for (const char* category : {"hip_api",
                                 "hsa_api",
                                 "memory_copy",
                                 "marker_api",
                                 "rccl_api",
                                 "memory_allocation",
                                 "scratch_memory",
                                 "kfd",
                                 "rocdecode_api",
                                 "rocjpeg_api"})
    {
        ASSERT_TRUE(buffer_records.contains(category)) << category;
        ASSERT_TRUE(buffer_records[category].is_array()) << category;
        EXPECT_EQ(buffer_records[category].size(), 0u) << category;
    }
}

// (g) Fresh begin() with no records: valid JSON, empty stochastic/host_trap arrays.
TEST_F(test_pc_sample_writer_t, ProvidedNoRecords_ReturnsValidJsonWithEmptySampleArrays)
{
    m_writer.begin();

    const auto& result = m_writer.get_result();
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(nlohmann::json::accept(result));

    const auto  json           = nlohmann::json::parse(result);
    const auto& buffer_records = json["rocprofiler-sdk-tool"][0]["buffer_records"];

    ASSERT_TRUE(buffer_records["pc_sample_stochastic"].is_array());
    EXPECT_EQ(buffer_records["pc_sample_stochastic"].size(), 0u);

    ASSERT_TRUE(buffer_records["pc_sample_host_trap"].is_array());
    EXPECT_EQ(buffer_records["pc_sample_host_trap"].size(), 0u);
}

// (h) flush("") throws std::runtime_error.
TEST_F(test_pc_sample_writer_t, ProvidedEmptyOutputFilePath_Throws)
{
    m_writer.begin();
    EXPECT_THROW(m_writer.flush(""), std::runtime_error);
}
