// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#include "test_source_snapshot.h"

#include <optional>
#include <string>
#include <vector>

namespace
{
bool copied_somewhere_with_tail(const std::filesystem::path& code_obj_sources,
                                const std::filesystem::path& original,
                                const std::string&           expected_contents)
{
    if (!std::filesystem::exists(code_obj_sources))
        return false;

    const std::string tail = original.filename().string();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(code_obj_sources))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().filename().string() != tail)
            continue;

        std::ifstream     ifs(entry.path());
        const std::string contents{std::istreambuf_iterator<char>(ifs),
                                   std::istreambuf_iterator<char>{}};
        if (contents == expected_contents)
            return true;
    }
    return false;
}
}  // namespace

// ---- parse_source_ref ----

TEST_F(test_source_snapshot_t, ParseSourceRef_PathBeforeLastColon)
{
    EXPECT_EQ(rocprofiler_compute_tool::parse_source_ref("foo/bar.cpp:42"),
              std::optional<std::string>{"foo/bar.cpp"});
    EXPECT_EQ(rocprofiler_compute_tool::parse_source_ref("a:b:10"), std::optional<std::string>{"a:b"});
}

TEST_F(test_source_snapshot_t, ParseSourceRef_NoColon_ReturnsNullopt)
{
    EXPECT_EQ(rocprofiler_compute_tool::parse_source_ref("no colon here"), std::nullopt);
}

TEST_F(test_source_snapshot_t, ParseSourceRef_LeadingColonOrEmpty_ReturnsNullopt)
{
    EXPECT_EQ(rocprofiler_compute_tool::parse_source_ref(":30"), std::nullopt);
    EXPECT_EQ(rocprofiler_compute_tool::parse_source_ref(""), std::nullopt);
}

// ---- snapshot_source_files ----

TEST_F(test_source_snapshot_t, SnapshotSourceFiles_CopiesExistingFilesPreservingTail)
{
    const std::vector<std::string> refs{m_file_a.string(), m_file_b.string()};

    const size_t copied = rocprofiler_compute_tool::snapshot_source_files(refs, m_output_root);

    const auto code_obj_sources = m_output_root / "code_obj_sources";
    EXPECT_EQ(copied, 2u);
    EXPECT_TRUE(copied_somewhere_with_tail(code_obj_sources, m_file_a, m_contents_a));
    EXPECT_TRUE(copied_somewhere_with_tail(code_obj_sources, m_file_b, m_contents_b));
}

TEST_F(test_source_snapshot_t, SnapshotSourceFiles_SkipsMissingRefs)
{
    const auto                     missing = (m_tmp_root / "proj" / "does_not_exist.cpp").string();
    const std::vector<std::string> refs{m_file_a.string(), missing, m_file_b.string()};

    size_t copied = 0;
    EXPECT_NO_THROW(copied = rocprofiler_compute_tool::snapshot_source_files(refs, m_output_root));

    const auto code_obj_sources = m_output_root / "code_obj_sources";
    EXPECT_EQ(copied, 2u);
    EXPECT_TRUE(copied_somewhere_with_tail(code_obj_sources, m_file_a, m_contents_a));
    EXPECT_TRUE(copied_somewhere_with_tail(code_obj_sources, m_file_b, m_contents_b));
}

TEST_F(test_source_snapshot_t, SnapshotSourceFiles_DedupsDuplicateRefs)
{
    const std::vector<std::string> refs{m_file_a.string(), m_file_a.string()};

    const size_t copied = rocprofiler_compute_tool::snapshot_source_files(refs, m_output_root);

    const auto code_obj_sources = m_output_root / "code_obj_sources";
    EXPECT_EQ(copied, 1u);
    EXPECT_TRUE(copied_somewhere_with_tail(code_obj_sources, m_file_a, m_contents_a));
}

TEST_F(test_source_snapshot_t, SnapshotSourceFiles_RefEscapingViaDotDot_IsRejected)
{
    // A pre-existing file outside the output root that a traversal ref would target.
    const auto outside = m_tmp_root / "victim.txt";
    write_file(outside, "original\n");

    // A ref that, joined under code_obj_sources/, would resolve back up to the
    // victim via "..". snapshot_source_files must refuse it.
    const auto traversal = (m_output_root / "code_obj_sources" / ".." / ".." / "victim.txt").string();

    size_t copied = 0;
    EXPECT_NO_THROW(copied = rocprofiler_compute_tool::snapshot_source_files({traversal}, m_output_root));

    EXPECT_EQ(copied, 0u);
    EXPECT_EQ(read_file(outside), "original\n");  // unchanged: not clobbered
}
