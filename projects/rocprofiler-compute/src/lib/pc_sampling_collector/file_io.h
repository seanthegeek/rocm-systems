// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#pragma once
#include <filesystem>
#include <string>

namespace rocprofiler_compute_tool
{
class file_io_t
{
public:
    virtual ~file_io_t() = default;
    // Creates the parent directory of output_file_path and writes contents to it.
    // Throws std::runtime_error if the path has no parent, the directory cannot be
    // created, or the file cannot be opened. Callers that must not abort (e.g. the
    // tool-fini serialization path) should wrap this in a try/catch.
    virtual void write(const std::filesystem::path& output_file_path, const std::string& contents) = 0;
};

class file_io_json_t : public file_io_t
{
public:
    void write(const std::filesystem::path& output_file_path, const std::string& contents) override;
};
}  // namespace rocprofiler_compute_tool
