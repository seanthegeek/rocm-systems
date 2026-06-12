// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT

// Unit tests for the trace-cache serialization / pending-entry helpers added to
// category_region.hpp. These exercise the pure helper functions in the header's
// anonymous namespace (argument serialization, renumbering, counting, and the
// per-thread pending-entry stack) without driving the full tracing pipeline.

#include "rocprof-sys/library/components/category_region.hpp"

#include "core/categories.hpp"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>

// The trace-cache wire format is a flat string of records, each laid out as
//     <arg_number>;;<arg_type>;;<arg_name>;;<arg_value>;;
// The helpers under test produce and manipulate this exact format.

namespace
{
using category_t = rocprofsys::category::host;

// Returns the number of ";;" delimiters in a string (4 per record).
std::size_t
count_delimiters(const std::string& s)
{
    std::size_t count = 0;
    std::size_t pos   = 0;
    while((pos = s.find(";;", pos)) != std::string::npos)
    {
        ++count;
        pos += 2;
    }
    return count;
}
}  // namespace

TEST(category_region_serialize, arg_type_string_like)
{
    EXPECT_EQ(get_serialized_arg_type<const char*>(), "string");
    EXPECT_EQ(get_serialized_arg_type<std::string>(), "string");
    EXPECT_EQ(get_serialized_arg_type<std::string_view>(), "string");
}

TEST(category_region_serialize, arg_type_arithmetic)
{
    EXPECT_EQ(get_serialized_arg_type<int>(), "int");
    EXPECT_EQ(get_serialized_arg_type<double>(), "double");
}

TEST(category_region_serialize, arg_value_stringifies)
{
    EXPECT_EQ(get_serialized_arg_value(42), "42");
    EXPECT_EQ(get_serialized_arg_value(std::string{ "hello" }), "hello");
}

TEST(category_region_serialize, append_prestringified_record)
{
    std::string args;
    append_serialized_arg(args, 0, "size_t", "size", "4096");
    EXPECT_EQ(args, "0;;size_t;;size;;4096;;");
}

TEST(category_region_serialize, append_typed_record)
{
    std::string args;
    append_serialized_arg(args, 3, std::string_view{ "count" }, 7);
    EXPECT_EQ(args, "3;;int;;count;;7;;");
}

TEST(category_region_serialize, count_records)
{
    EXPECT_EQ(count_serialized_args(""), 0u);
    EXPECT_EQ(count_serialized_args("0;;int;;a;;1;;"), 1u);
    EXPECT_EQ(count_serialized_args("0;;int;;a;;1;;1;;int;;b;;2;;"), 2u);
}

TEST(category_region_serialize, renumber_from_offset)
{
    std::string args = "0;;int;;a;;1;;0;;int;;b;;2;;";
    auto        n    = renumber_serialized_args(args, 5);
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(args, "5;;int;;a;;1;;6;;int;;b;;2;;");
}

TEST(category_region_serialize, renumber_empty_is_noop)
{
    std::string args;
    EXPECT_EQ(renumber_serialized_args(args, 9), 0u);
    EXPECT_TRUE(args.empty());
}

TEST(category_region_serialize, name_value_pairs_serialized)
{
    auto args = serialize_name_value_pairs("size", 4096);
    EXPECT_EQ(args, "0;;int;;size;;4096;;");
    EXPECT_EQ(count_serialized_args(args), 1u);
}

TEST(category_region_serialize, name_value_pairs_multiple)
{
    auto args = serialize_name_value_pairs("size", 4096, "node", 1);
    EXPECT_EQ(count_serialized_args(args), 2u);
    EXPECT_NE(args.find(";;size;;4096;;"), std::string::npos);
    EXPECT_NE(args.find(";;node;;1;;"), std::string::npos);
}

TEST(category_region_serialize, name_value_pairs_reject_non_pairs)
{
    // Odd number of args is not a valid name/value sequence.
    EXPECT_TRUE(serialize_name_value_pairs("size", 4096, "node").empty());
    // No args at all.
    EXPECT_TRUE(serialize_name_value_pairs().empty());
    // A non-string-like name slot disqualifies the whole sequence.
    EXPECT_TRUE(serialize_name_value_pairs(1, 4096).empty());
}

TEST(category_region_serialize, variadic_audit_args)
{
    auto args = serialize_annotation_args(4096, 2);
    EXPECT_EQ(count_serialized_args(args), 2u);
    // Names are synthesized as arg<N>-<type>.
    EXPECT_NE(args.find("0;;int;;arg0-int;;4096;;"), std::string::npos);
    EXPECT_NE(args.find("1;;int;;arg1-int;;2;;"), std::string::npos);
}

TEST(category_region_serialize, return_arg)
{
    EXPECT_EQ(serialize_return_arg(0), "0;;int;;return;;0;;");
}

TEST(category_region_serialize, annotation_record_size_t)
{
    std::size_t             value = 4096;
    rocprofsys_annotation_t ann{ "size", ROCPROFSYS_VALUE_SIZE_T, &value };
    std::string             args;
    EXPECT_TRUE(append_serialized_annotation_record_arg(args, 0, ann));
    EXPECT_EQ(args, "0;;size_t;;size;;4096;;");
}

TEST(category_region_serialize, annotation_record_cstr)
{
    const char*             value = "foo";
    rocprofsys_annotation_t ann{ "label", ROCPROFSYS_VALUE_CSTR,
                                 const_cast<char*>(value) };
    std::string             args;
    EXPECT_TRUE(append_serialized_annotation_record_arg(args, 1, ann));
    EXPECT_EQ(args, "1;;string;;label;;foo;;");
}

TEST(category_region_serialize, annotation_record_invalid)
{
    std::size_t value = 1;
    std::string args;
    // Missing name.
    EXPECT_FALSE(append_serialized_annotation_record_arg(
        args, 0, rocprofsys_annotation_t{ nullptr, ROCPROFSYS_VALUE_SIZE_T, &value }));
    // ROCPROFSYS_VALUE_NONE type.
    EXPECT_FALSE(append_serialized_annotation_record_arg(
        args, 0, rocprofsys_annotation_t{ "x", ROCPROFSYS_VALUE_NONE, &value }));
    // Null value.
    EXPECT_FALSE(append_serialized_annotation_record_arg(
        args, 0, rocprofsys_annotation_t{ "x", ROCPROFSYS_VALUE_SIZE_T, nullptr }));
    EXPECT_TRUE(args.empty());
}

TEST(category_region_serialize, annotation_array_skips_invalid)
{
    std::size_t value0 = 4096;
    std::size_t value2 = 8;
    // Middle record is invalid (null value) and must be skipped while the
    // surviving records are numbered contiguously.
    rocprofsys_annotation_t anns[3] = {
        { "size", ROCPROFSYS_VALUE_SIZE_T, &value0 },
        { "bad", ROCPROFSYS_VALUE_SIZE_T, nullptr },
        { "node", ROCPROFSYS_VALUE_SIZE_T, &value2 },
    };
    auto args = serialize_annotation_args(anns, std::size_t{ 3 });
    EXPECT_EQ(count_serialized_args(args), 2u);
    EXPECT_EQ(args, "0;;size_t;;size;;4096;;1;;size_t;;node;;8;;");
}

TEST(category_region_serialize, annotation_array_empty)
{
    EXPECT_TRUE(serialize_annotation_args(static_cast<rocprofsys_annotation_t*>(nullptr),
                                          std::size_t{ 0 })
                    .empty());
    rocprofsys_annotation_t ann{ "x", ROCPROFSYS_VALUE_NONE, nullptr };
    EXPECT_TRUE(serialize_annotation_args(&ann, std::size_t{ 0 }).empty());
}

// ---------------------------------------------------------------------------
// Pending-entry stack behavior (cache_start / append_cache_args)
// ---------------------------------------------------------------------------

class category_region_cache : public ::testing::Test
{
protected:
    void TearDown() override { map_name_to_args.clear(); }
};

TEST_F(category_region_cache, cache_start_pushes_entry)
{
    map_name_to_args.clear();
    cache_start<category_t>("regionA", serialize_name_value_pairs("size", 4096));

    entry_key key{ "regionA", rocprofsys::trait::name<category_t>::value };
    auto      itr = map_name_to_args.find(key);
    ASSERT_NE(itr, map_name_to_args.end());
    ASSERT_EQ(itr->second.size(), 1u);
    EXPECT_EQ(itr->second.back().arg_count, 1u);
    EXPECT_EQ(itr->second.back().args, "0;;int;;size;;4096;;");
}

TEST_F(category_region_cache, append_renumbers_onto_existing)
{
    map_name_to_args.clear();
    cache_start<category_t>("regionB", serialize_name_value_pairs("size", 4096));
    append_cache_args<category_t>("regionB", serialize_annotation_args(7));

    entry_key key{ "regionB", rocprofsys::trait::name<category_t>::value };
    auto      itr = map_name_to_args.find(key);
    ASSERT_NE(itr, map_name_to_args.end());
    ASSERT_EQ(itr->second.size(), 1u);
    const auto& entry = itr->second.back();
    EXPECT_EQ(entry.arg_count, 2u);
    // Appended arg is renumbered to continue from the existing arg_count.
    EXPECT_EQ(entry.args, "0;;int;;size;;4096;;1;;int;;arg0-int;;7;;");
}

TEST_F(category_region_cache, append_to_empty_start_initializes_numbering)
{
    map_name_to_args.clear();
    cache_start<category_t>("regionC");
    append_cache_args<category_t>("regionC", serialize_annotation_args(1, 2));

    entry_key key{ "regionC", rocprofsys::trait::name<category_t>::value };
    auto      itr = map_name_to_args.find(key);
    ASSERT_NE(itr, map_name_to_args.end());
    const auto& entry = itr->second.back();
    EXPECT_EQ(entry.arg_count, 2u);
    EXPECT_EQ(entry.args, "0;;int;;arg0-int;;1;;1;;int;;arg1-int;;2;;");
}

TEST_F(category_region_cache, append_with_no_pending_entry_is_noop)
{
    map_name_to_args.clear();
    // No cache_start was called, so there is nothing to append to.
    append_cache_args<category_t>("missing", serialize_annotation_args(1));
    EXPECT_TRUE(map_name_to_args.empty());
}

TEST_F(category_region_cache, empty_append_is_noop)
{
    map_name_to_args.clear();
    cache_start<category_t>("regionD", serialize_name_value_pairs("size", 4096));
    append_cache_args<category_t>("regionD", {});

    entry_key key{ "regionD", rocprofsys::trait::name<category_t>::value };
    auto      itr = map_name_to_args.find(key);
    ASSERT_NE(itr, map_name_to_args.end());
    EXPECT_EQ(itr->second.back().arg_count, 1u);
    EXPECT_EQ(itr->second.back().args, "0;;int;;size;;4096;;");
}

TEST_F(category_region_cache, nested_entries_stack)
{
    map_name_to_args.clear();
    // Recursive / self-nested regions push independent frames.
    cache_start<category_t>("recur", serialize_name_value_pairs("depth", 0));
    cache_start<category_t>("recur", serialize_name_value_pairs("depth", 1));

    entry_key key{ "recur", rocprofsys::trait::name<category_t>::value };
    auto      itr = map_name_to_args.find(key);
    ASSERT_NE(itr, map_name_to_args.end());
    ASSERT_EQ(itr->second.size(), 2u);
    // Append affects only the top-of-stack frame.
    append_cache_args<category_t>("recur", serialize_annotation_args(99));
    EXPECT_EQ(itr->second.front().args, "0;;int;;depth;;0;;");
    EXPECT_EQ(itr->second.back().args, "0;;int;;depth;;1;;1;;int;;arg0-int;;99;;");
}
