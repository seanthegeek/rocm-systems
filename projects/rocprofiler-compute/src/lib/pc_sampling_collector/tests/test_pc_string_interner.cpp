// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier:  MIT
#include "test_pc_string_interner.h"

TEST_F(test_pc_string_interner_t, ProvidedFreshInterner_InternReturnsZeroAndStoresPair)
{
    EXPECT_EQ(m_interner.intern("a", "x"), 0u);
    ASSERT_EQ(m_interner.instructions().size(), 1u);
    ASSERT_EQ(m_interner.comments().size(), 1u);
    EXPECT_EQ(m_interner.instructions()[0], "a");
    EXPECT_EQ(m_interner.comments()[0], "x");
}

TEST_F(test_pc_string_interner_t, ProvidedSamePairTwice_DedupsToSameIndexWithoutGrowing)
{
    EXPECT_EQ(m_interner.intern("a", "x"), 0u);
    EXPECT_EQ(m_interner.intern("a", "x"), 0u);
    EXPECT_EQ(m_interner.instructions().size(), 1u);
    EXPECT_EQ(m_interner.comments().size(), 1u);
}

TEST_F(test_pc_string_interner_t, ProvidedNewPair_ReturnsNextIndexAndGrowsBothArrays)
{
    EXPECT_EQ(m_interner.intern("a", "x"), 0u);
    EXPECT_EQ(m_interner.intern("b", "y"), 1u);
    ASSERT_EQ(m_interner.instructions().size(), 2u);
    ASSERT_EQ(m_interner.comments().size(), 2u);
    EXPECT_EQ(m_interner.instructions()[1], "b");
    EXPECT_EQ(m_interner.comments()[1], "y");
}

TEST_F(test_pc_string_interner_t, ProvidedPartialMatches_KeyIsTheFullPairNotEitherFieldAlone)
{
    EXPECT_EQ(m_interner.intern("a", "x"), 0u);

    // Same text, different comment -> new entry.
    const size_t same_text_idx = m_interner.intern("a", "z");
    EXPECT_NE(same_text_idx, 0u);

    // Different text, same comment -> also new entry.
    const size_t same_comment_idx = m_interner.intern("c", "x");
    EXPECT_NE(same_comment_idx, 0u);
    EXPECT_NE(same_comment_idx, same_text_idx);

    EXPECT_EQ(m_interner.instructions().size(), 3u);
    EXPECT_EQ(m_interner.comments().size(), 3u);
}

TEST_F(test_pc_string_interner_t, ProvidedSeveralPairs_ReturnedIndexEqualsPositionInBothParallelArrays)
{
    const size_t i0 = m_interner.intern("mov", "// a");
    const size_t i1 = m_interner.intern("add", "// b");
    const size_t i2 = m_interner.intern("mul", "// c");

    EXPECT_EQ(m_interner.instructions()[i0], "mov");
    EXPECT_EQ(m_interner.comments()[i0], "// a");
    EXPECT_EQ(m_interner.instructions()[i1], "add");
    EXPECT_EQ(m_interner.comments()[i1], "// b");
    EXPECT_EQ(m_interner.instructions()[i2], "mul");
    EXPECT_EQ(m_interner.comments()[i2], "// c");

    ASSERT_EQ(m_interner.instructions().size(), m_interner.comments().size());
    EXPECT_EQ(m_interner.intern("add", "// b"), i1);
}
