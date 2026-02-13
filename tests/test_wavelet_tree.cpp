 /**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/wavelet_tree/two_bit_wavelet_tree.hpp"

TEST(WaveletTreeTest, InitialSize) {
    uint8_t data[] = {0, 1, 2, 3, 0, 1, 2, 3}; 
    TwoBitWaveletTree<WordBitVectorStrategy>* tree = create_two_bit_wavelet_tree<WordBitVectorStrategy>(data, 8);
    EXPECT_EQ(tree->size(), 8);
    delete tree;
}

TEST(WaveletTreeTest, Access) {
    uint8_t data[] = {0, 1, 2, 3, 0, 1, 2, 3}; 
    TwoBitWaveletTree<WordBitVectorStrategy>* tree = create_two_bit_wavelet_tree<WordBitVectorStrategy>(data, 8);
    for (size_t i = 0; i < 8; i++) {
        EXPECT_EQ(tree->access(i), data[i]);
    }
    EXPECT_THROW(tree->access(8), std::out_of_range);
    delete tree;
}