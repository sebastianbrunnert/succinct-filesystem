/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/bitvector/bitvector.hpp"
#include <memory>

// Parameterized test class for different strategies
class BitVectorTest : public ::testing::Test, public ::testing::WithParamInterface<std::function<BitVector*(size_t)>> {
protected:
    BitVector* create_bitvector(size_t n) {
        return GetParam()(n);
    }
};

TEST_P(BitVectorTest, InitialSize) {
    BitVector* bv = create_bitvector(0);
    EXPECT_EQ(bv->size(), 0);

    bv = create_bitvector(10);
    EXPECT_EQ(bv->size(), 10);

    bv = create_bitvector(100);
    EXPECT_EQ(bv->size(), 100);
}

TEST_P(BitVectorTest, SetAndAccess) {
    BitVector* bv = create_bitvector(10);
    bv->set(3, true);
    EXPECT_FALSE(bv->access(0));
    EXPECT_TRUE(bv->access(3));

    bv = create_bitvector(100);
    bv->set(50, true);
    bv->set(64, true);
    for (size_t i = 0; i < 100; i++) {
        if (i == 50 || i == 64) {
            EXPECT_TRUE(bv->access(i));
        } else {
            EXPECT_FALSE(bv->access(i));
        }
    }
}

TEST_P(BitVectorTest, Rank) {
    BitVector* bv = create_bitvector(10);
    bv->set(3, true);
    bv->set(5, true);
    EXPECT_EQ(bv->rank0(0), 1);
    EXPECT_EQ(bv->rank0(3), 3);
    EXPECT_EQ(bv->rank1(3), 1);
    EXPECT_EQ(bv->rank0(9), 8);
    EXPECT_EQ(bv->rank1(9), 2);
    EXPECT_THROW(bv->rank0(10), std::out_of_range);
    EXPECT_THROW(bv->rank1(10), std::out_of_range);

    bv = create_bitvector(100);
    for (size_t i = 0; i < 100; i += 2) {
        bv->set(i, true);
    }
    EXPECT_EQ(bv->rank0(99), 50);
    EXPECT_EQ(bv->rank1(99), 50);
    EXPECT_THROW(bv->rank1(100), std::out_of_range);
}

TEST_P(BitVectorTest, Select) {
    BitVector* bv = create_bitvector(10);
    bv->set(3, true);
    bv->set(5, true);
    EXPECT_EQ(bv->select0(1), 0);
    EXPECT_EQ(bv->select0(4), 4);
    EXPECT_EQ(bv->select1(1), 3);
    EXPECT_EQ(bv->select1(2), 5);
    EXPECT_THROW(bv->select0(0), std::out_of_range);
    EXPECT_THROW(bv->select1(0), std::out_of_range);
    EXPECT_THROW(bv->select0(9), std::out_of_range);
    EXPECT_THROW(bv->select1(3), std::out_of_range);

    bv = create_bitvector(100);
    for (size_t i = 0; i < 100; i += 2) {
        bv->set(i, true);
    }
    EXPECT_EQ(bv->select0(1), 1);
    EXPECT_EQ(bv->select0(50), 99);
    EXPECT_EQ(bv->select1(1), 0);
    EXPECT_EQ(bv->select1(50), 98);
    EXPECT_THROW(bv->select0(51), std::out_of_range);
    EXPECT_THROW(bv->select1(51), std::out_of_range);
}

TEST_P(BitVectorTest, Insert) {
    BitVector* bv = create_bitvector(10);
    bv->insert(5, true);
    EXPECT_EQ(bv->size(), 11);
    EXPECT_TRUE(bv->access(5));
    EXPECT_FALSE(bv->access(6));

    bv = create_bitvector(100);
    bv->insert(0, true);
    bv->insert(64, true);
    EXPECT_EQ(bv->size(), 102);
    EXPECT_TRUE(bv->access(0));
    EXPECT_TRUE(bv->access(64));
    EXPECT_FALSE(bv->access(1));
    EXPECT_FALSE(bv->access(65));
    EXPECT_EQ(bv->rank1(101), 2);
    EXPECT_EQ(bv->select1(2), 64);
    EXPECT_THROW(bv->insert(103, true), std::out_of_range);

    bv = create_bitvector(129);
    for(size_t i = 0; i < 129; i++) {
        bv->set(i, true);
    }
    bv->insert(64, false);
    EXPECT_EQ(bv->size(), 130);
    for (size_t i = 0; i < 130; i++) {
        if (i == 64) {
            EXPECT_FALSE(bv->access(i));
        } else {
            EXPECT_TRUE(bv->access(i));
        }
    }
}

TEST_P(BitVectorTest, Remove) {
    BitVector* bv = create_bitvector(10);
    bv->set(3, true);
    bv->set(5, true);
    bv->remove(4);
    EXPECT_EQ(bv->size(), 9);
    EXPECT_TRUE(bv->access(3));
    EXPECT_TRUE(bv->access(4));
    EXPECT_FALSE(bv->access(5));
    EXPECT_THROW(bv->remove(9), std::out_of_range);

    bv = create_bitvector(100);
    for (size_t i = 0; i < 100; i += 2) {
        bv->set(i, true);
    }
    bv->remove(0);
    bv->remove(63);
    EXPECT_EQ(bv->size(), 98);
    for (size_t i = 0; i < 98; i++) {
        if (i % 2 == 0) {
            if (i < 63) {
                EXPECT_FALSE(bv->access(i));
            } else {
                EXPECT_TRUE(bv->access(i));
            }
        } else {
            if (i < 63) {
                EXPECT_TRUE(bv->access(i));
            } else {
                EXPECT_FALSE(bv->access(i));
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    BitVectorStrategies,
    BitVectorTest,
    ::testing::Values(
        [](size_t n) { return create_bitvector<ArrayBitVectorStrategy>(n); },
        [](size_t n) { return create_bitvector<WordBitVectorStrategy>(n); },
        [](size_t n) { return create_bitvector<SaskeliBitVectorStrategy>(n); }
    )
);
