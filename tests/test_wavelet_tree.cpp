 /**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <gtest/gtest.h>
#include "../src/wavelet_tree/two_bit_wavelet_tree.hpp"
#include <random>

namespace {
    class WaveletTreeTest : public ::testing::Test {

    public:
        TwoBitWaveletTree<WordBitVectorStrategy>* tree;
        uint8_t* data;

    protected:
        virtual void SetUp() override {
            // Create random data for testing
            data = new uint8_t[200];
            // Fixed seed for reproducibility
            std::mt19937 rng(42);
            std::uniform_int_distribution<uint8_t> dist(0, 3);
            for(size_t i = 0; i < 200; i++) {
                data[i] = dist(rng);
            }
            tree = create_two_bit_wavelet_tree<WordBitVectorStrategy>(data, 200);
        }

        virtual void TearDown() override {
            delete tree;
            delete[] data;
        }
    };


    TEST_F(WaveletTreeTest, InitialSize) {
        EXPECT_EQ(tree->size(), 200);
    }

    TEST_F(WaveletTreeTest, SetAndAccess) {
        for(size_t i = 0; i < tree->size(); i++) {
            tree->set(i, (data[i]+1) % 4);
        }
        EXPECT_THROW(tree->set(tree->size(), 1), std::out_of_range);

        for(size_t i = 0; i < tree->size(); i++) {
            EXPECT_EQ(tree->access(i), (data[i]+1) % 4);
        }
        EXPECT_THROW(tree->access(tree->size()), std::out_of_range);
    }
    
    TEST_F(WaveletTreeTest, Rank) {
        for(size_t i = 0; i < tree->size(); i++) {
            uint8_t symbol = data[i];
            size_t rank = 0;
            for(size_t j = 0; j <= i; j++) {
                if (data[j] == symbol) {
                    rank++;
                }
            }
            EXPECT_EQ(tree->rank(symbol, i), rank);
        }
        
        for(uint8_t symbol = 0; symbol < 4; symbol++) {
            EXPECT_THROW(tree->rank(symbol, tree->size()), std::out_of_range);
        }
    }

    TEST_F(WaveletTreeTest, Select) {
        for(uint8_t symbol = 0; symbol < 4; symbol++) {
            size_t count = 0;
            for(size_t i = 0; i < tree->size(); i++) {
                if (data[i] == symbol) {
                    count++;
                    EXPECT_EQ(tree->select(symbol, count), i);
                }
            }
            EXPECT_THROW(tree->select(symbol, count + 1), std::out_of_range);
        }
    }

    TEST_F(WaveletTreeTest, Insert) {
        tree->insert(0,1);
        tree->insert(100, 2);
        tree->insert(tree->size(),3);
        for (size_t i = 0; i < 200; i++) {
            if(i == 0) {
                EXPECT_EQ(tree->access(i), 1);
            } else if (i == 100) {
                EXPECT_EQ(tree->access(i), 2);
            } else if (i == tree->size() - 1) {
                EXPECT_EQ(tree->access(i), 3);
            } else if (i < 100) {
                EXPECT_EQ(tree->access(i), data[i - 1]);
            } else {
                EXPECT_EQ(tree->access(i), data[i - 2]);
            }
        }

        EXPECT_THROW(tree->insert(tree->size() + 1, 1), std::out_of_range);
    }

    TEST_F(WaveletTreeTest, Remove) {
        tree->remove(0);
        tree->remove(99);
        tree->remove(tree->size() - 1);
        for (size_t i = 0; i < tree->size(); i++) {
            if(i == 0) {
                EXPECT_EQ(tree->access(i), data[1]);
            } else if (i == 99) {
                EXPECT_EQ(tree->access(i), data[101]);
            } else if (i == tree->size() - 1) {
                EXPECT_EQ(tree->access(i), data[198]);
            } else if (i < 99) {
                EXPECT_EQ(tree->access(i), data[i + 1]);
            } else {
                EXPECT_EQ(tree->access(i), data[i + 2]);
            }
        }

        EXPECT_THROW(tree->remove(tree->size()), std::out_of_range);
    }

    TEST_F(WaveletTreeTest, SerializeDeserialize) {
        size_t serialized_size = tree->get_serialized_size();
        char* buffer = new char[serialized_size];
        size_t offset = 0;
        tree->serialize(buffer, &offset);
        EXPECT_EQ(offset, serialized_size);

        TwoBitWaveletTree<WordBitVectorStrategy>* deserialized_tree = create_two_bit_wavelet_tree<WordBitVectorStrategy>(nullptr, 0);
        offset = 0;
        deserialized_tree->deserialize(buffer, &offset);
        EXPECT_EQ(offset, serialized_size);

        for(size_t i = 0; i < tree->size(); i++) {
            EXPECT_EQ(tree->access(i), deserialized_tree->access(i));
        }

        delete[] buffer;
        delete deserialized_tree;
    }
}