/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "two_bit_wavelet_tree.hpp"

template <typename BitVectorStrategy>
TwoBitWaveletTree<BitVectorStrategy>::~TwoBitWaveletTree() {
    delete root_bv;
    delete left_bv;
    delete right_bv;
}

template <typename BitVectorStrategy>
void TwoBitWaveletTree<BitVectorStrategy>::set(size_t position, uint8_t symbol) {
    if (position >= size()) throw std::out_of_range("position out of range");
    if (symbol > 3) throw std::out_of_range("symbol out of range");
    root_bv->set(position, symbol >= 2);
    if (symbol < 2) {
        left_bv->set(position, symbol == 1);
    } else {
        right_bv->set(position, symbol == 3);
    }
}

template <typename BitVectorStrategy>
uint8_t TwoBitWaveletTree<BitVectorStrategy>::access(size_t position) const {
    if (position >= size()) throw std::out_of_range("position out of range");
    bool root_bit = root_bv->access(position);
    if (!root_bit) {
        return left_bv->access(position) ? 1 : 0;
    } else {
        return right_bv->access(position) ? 3 : 2;
    }
}

template <typename BitVectorStrategy>
size_t TwoBitWaveletTree<BitVectorStrategy>::size() const {
    return root_bv->size();
}

template <typename BitVectorStrategy>
size_t TwoBitWaveletTree<BitVectorStrategy>::rank(size_t position, uint8_t symbol) const {
    if (position >= size()) throw std::out_of_range("position out of range");
    if (symbol > 3) throw std::out_of_range("symbol out of range");
    bool root_bit = symbol >= 2;
    if (!root_bit) {
        if (symbol == 0) {
            return root_bv->rank0(position) - left_bv->rank1(position);
        } else {
            return root_bv->rank0(position) - left_bv->rank0(position);
        }
    } else {
        if (symbol == 2) {
            return root_bv->rank1(position) - right_bv->rank1(position);
        } else {
            return root_bv->rank1(position) - right_bv->rank0(position);
        }
    }
}

template <typename BitVectorStrategy>
size_t TwoBitWaveletTree<BitVectorStrategy>::select(size_t n, uint8_t symbol) const {
    if (n == 0) throw std::out_of_range("n must be greater than zero");
    if (symbol > 3) throw std::out_of_range("symbol out of range");
    bool root_bit = symbol >= 2;
    if (!root_bit) {
        if (symbol == 0) {
            size_t root_pos = root_bv->select0(n + left_bv->rank1(root_bv->size() - 1));
            return root_pos - left_bv->rank1(root_pos);
        } else {
            size_t root_pos = root_bv->select0(n + left_bv->rank0(root_bv->size() - 1));
            return root_pos - left_bv-> rank0(root_pos);
        }
    } else {
        if (symbol == 2) {
            size_t root_pos = root_bv->select1(n + right_bv->rank1(root_bv->size() - 1));
            return root_pos - right_bv->rank1(root_pos);
        } else {
            size_t root_pos = root_bv->select1(n + right_bv->rank0(root_bv->size() - 1));
            return root_pos - right_bv->rank0(root_pos);
        }
    }
}

template <typename BitVectorStrategy>
void TwoBitWaveletTree<BitVectorStrategy>::insert(size_t position, uint8_t symbol) {
    if (position > size()) throw std::out_of_range("position out of range");
    if (symbol > 3) throw std::out_of_range("symbol out of range");
    root_bv->insert(position, symbol >= 2);
    if (symbol < 2) {
        left_bv->insert(position, symbol == 1);
        right_bv->insert(position, false);
    } else {
        left_bv->insert(position, false);
        right_bv->insert(position, symbol == 3);
    }
}

template <typename BitVectorStrategy>
void TwoBitWaveletTree<BitVectorStrategy>::remove(size_t position) {
    if (position >= size()) throw std::out_of_range("position out of range");
    bool root_bit = root_bv->access(position);
    root_bv->remove(position);
    if (!root_bit) {
        left_bv->remove(position);
        right_bv->remove(position);
    } else {
        left_bv->remove(position);
        right_bv->remove(position);
    }
}