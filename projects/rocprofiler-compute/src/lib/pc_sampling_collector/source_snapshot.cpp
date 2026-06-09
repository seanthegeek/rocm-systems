// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#include "source_snapshot.h"

#include <set>
#include <system_error>

using namespace rocprofiler_compute_tool;

std::optional<std::string> rocprofiler_compute_tool::parse_source_ref(const std::string& comment)
{
    if (comment.empty())
    {
        return std::nullopt;
    }

    const auto pos = comment.rfind(':');
    if (pos == std::string::npos || pos == 0)
    {
        return std::nullopt;
    }

    return comment.substr(0, pos);
}

size_t rocprofiler_compute_tool::snapshot_source_files(const std::vector<std::string>& source_refs,
                                                       const std::filesystem::path&    output_root)
{
    const std::filesystem::path sources_root = output_root / "code_obj_sources";

    std::set<std::string> unique_refs;
    for (const auto& ref : source_refs)
    {
        unique_refs.insert(ref);
    }

    size_t copied = 0;
    for (const auto& ref : unique_refs)
    {
        std::error_code             ec;
        const std::filesystem::path src{ref};
        if (!std::filesystem::exists(src, ec) || ec)
        {
            continue;
        }

        // Strip any leading '/' so the ref nests under code_obj_sources.
        std::string relative = ref;
        while (!relative.empty() && relative.front() == '/')
        {
            relative.erase(relative.begin());
        }

        const std::filesystem::path dst = sources_root / relative;

        // Reject refs that escape code_obj_sources (e.g. via "..") so that a
        // hostile ref cannot clobber files outside the snapshot directory.
        const auto norm_root = sources_root.lexically_normal();
        const auto norm_dst  = dst.lexically_normal();
        const auto rel       = norm_dst.lexically_relative(norm_root);
        if (rel.empty() || *rel.begin() == "..")
        {
            continue;  // ref escapes code_obj_sources; skip it
        }

        std::filesystem::create_directories(dst.parent_path(), ec);
        if (ec)
        {
            continue;
        }

        const bool ok = std::filesystem::copy_file(src,
                                                   dst,
                                                   std::filesystem::copy_options::overwrite_existing,
                                                   ec);
        if (ok && !ec)
        {
            ++copied;
        }
    }

    return copied;
}
