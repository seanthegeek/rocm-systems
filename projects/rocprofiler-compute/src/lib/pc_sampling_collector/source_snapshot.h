// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace rocprofiler_compute_tool
{
class source_snapshot_t
{
public:
    virtual ~source_snapshot_t() = default;

    // rfind(':') rule: substring before the LAST ':' is the path. If no ':' OR the last ':' is at
    // position 0 (leading colon, nothing before it) OR comment is empty -> std::nullopt.
    virtual std::optional<std::string> parse_ref(const std::string& comment) const = 0;

    // For each unique existing ref, copy it into output_root/"code_obj_sources"/<the ref path>,
    // preserving the ref's directory structure (e.g. an absolute /a/b/c.cpp goes under
    // code_obj_sources/a/b/c.cpp; a relative p/q.cpp under code_obj_sources/p/q.cpp). Dedup the
    // input. Create parent dirs as needed. Return the number of files actually copied.
    //
    // Source refs originate from ISA debug comments of the profiled (untrusted) binary, so a ref
    // is only read if it resolves inside allowed_root (defaults to the current working directory,
    // i.e. the project tree); refs resolving outside it (e.g. /etc/passwd) are skipped. Refs whose
    // destination would escape code_obj_sources are also skipped. Skipped/missing refs never throw.
    // The allowed_root parameter exists so tests can point it at a temp tree; production uses CWD.
    virtual size_t snapshot(
        const std::vector<std::string>& source_refs,
        const std::filesystem::path&    output_root,
        const std::filesystem::path&    allowed_root = std::filesystem::current_path()) const = 0;
};

class source_snapshot_impl_t : public source_snapshot_t
{
public:
    std::optional<std::string> parse_ref(const std::string& comment) const override;
    size_t snapshot(const std::vector<std::string>& source_refs,
                    const std::filesystem::path&    output_root,
                    const std::filesystem::path& allowed_root = std::filesystem::current_path()) const override;
};
}  // namespace rocprofiler_compute_tool
