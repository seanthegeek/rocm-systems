// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace rocprofiler_compute_tool
{
// rfind(':') rule: substring before the LAST ':' is the path. If no ':' OR the last ':' is at
// position 0 (leading colon, nothing before it) OR comment is empty -> std::nullopt.
std::optional<std::string> parse_source_ref(const std::string& comment);

// For each unique existing ref, copy it into output_root/"code_obj_sources"/<the ref path>,
// preserving the ref's directory structure (e.g. an absolute /a/b/c.cpp goes under
// code_obj_sources/a/b/c.cpp; a relative p/q.cpp under code_obj_sources/p/q.cpp). Dedup the
// input. Skip refs that do not exist on disk (no throw). Create parent dirs as needed.
// Return the number of files actually copied.
size_t snapshot_source_files(const std::vector<std::string>& source_refs,
                             const std::filesystem::path&    output_root);
}  // namespace rocprofiler_compute_tool
