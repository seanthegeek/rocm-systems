// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#include "environ_cache.h"

using namespace rocprofiler_compute_tool;

extern "C" char** environ;

namespace
{
constexpr std::string_view kRocprofPrefix{"ROCPROF_"};
// The PC sampling feature gate ships as ROCPROFILER_PC_SAMPLING_BETA_ENABLED,
// which does not start with "ROCPROF_" (offset 7 is 'I', not '_'). Capture the
// ROCPROFILER_ family too so that variable reaches InputParameters.
constexpr std::string_view kRocprofilerPrefix{"ROCPROFILER_"};

bool starts_with(std::string_view name, std::string_view prefix)
{
    return name.size() >= prefix.size() && name.substr(0, prefix.size()) == prefix;
}

bool has_rocprof_prefix(std::string_view name)
{
    return starts_with(name, kRocprofPrefix) || starts_with(name, kRocprofilerPrefix);
}

}  // namespace

std::shared_ptr<const EnvironCache> EnvironCache::instance()
{
    static const std::shared_ptr<const EnvironCache> cache{new EnvironCache{environ}};
    return cache;
}

EnvironCache::EnvironCache(char** envp)
{
    if (envp == nullptr)
        return;

    for (char** ep = envp; *ep != nullptr; ++ep)
    {
        const std::string_view entry{*ep};
        const auto             separator = entry.find('=');
        if (separator == std::string_view::npos)
            continue;

        const auto name = entry.substr(0, separator);
        if (!has_rocprof_prefix(name))
            continue;

        m_values.emplace(std::string{name}, std::string{entry.substr(separator + 1)});
    }
}

std::optional<std::string_view> EnvironCache::get(std::string_view name) const
{
    const auto value = m_values.find(std::string{name});
    if (value != m_values.end())
        return std::string_view{value->second};

    return std::nullopt;
}
