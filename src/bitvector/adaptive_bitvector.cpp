/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <vector>
#include <stdexcept>
#include "bitvector.hpp"

extern "C" {
    #include "../../external/adaptive_dynamic_bitvector/hybridBV.h"
}

/**
 * Adaptive dynamic bitvector implementation based on:
 * 
 * @misc{Navarro2025AdaptiveDynamicBitvector,
 * title={(Worst-Case) Optimal Adaptive Dynamic Bitvectors}, 
 * author={Gonzalo Navarro},
 * year={2025},
 * eprint={2405.15088},
 * archivePrefix={arXiv},
 * primaryClass={cs.DS},
 * url={https://arxiv.org/abs/2405.15088}}
 */
class AdaptiveDynamicBitVectorStrategy : public BitVector {
private:
    hybridBV adaptive;

public:
    AdaptiveDynamicBitVectorStrategy(size_t n) {
        if (n == 0) {
            adaptive = hybridCreate();
        } else {
            size_t words = (n + 63) / 64;
            adaptive = hybridCreateFrom(new uint64_t[words](), n);
        }
    }

    ~AdaptiveDynamicBitVectorStrategy() {
        hybridDestroy(adaptive);
    }

    void set(size_t position, bool value) override {
        if (position >= size()) throw std::out_of_range("position out of range");
        hybridWrite(adaptive, position, 1);
    }

    bool access(size_t position) const override {
        if (position >= size()) throw std::out_of_range("position out of range");
        return hybridAccess(adaptive, position);
    }

    size_t size() const override {
        return hybridLength(adaptive);
    }

    size_t rank1(size_t position) const override {
        if (position >= size()) throw std::out_of_range("position out of range");
        return hybridRank(adaptive, position);
    }

    size_t rank0(size_t position) const override {
        if (position >= size()) throw std::out_of_range("position out of range");
        return hybridRank0(adaptive, position);
    }

    size_t select1(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        if (hybridOnes(adaptive) < n) throw std::out_of_range("n exceeds number of 1-bits");
        return hybridSelect(adaptive, n);
    }

    size_t select0(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        if (size() - hybridOnes(adaptive) < n) throw std::out_of_range("n exceeds number of 0-bits");
        return hybridSelect0(adaptive, n);
    }

    void insert(size_t position, bool value) override {
        if (position > size()) throw std::out_of_range("position out of range");
        hybridInsert(adaptive, position, value);
    }

    void remove(size_t position) override {
        if (position >= size()) throw std::out_of_range("position out of range");
        hybridDelete(adaptive, position);
    }
};

template <>
BitVector* create_bitvector<AdaptiveDynamicBitVectorStrategy>(std::size_t n) {
    return new AdaptiveDynamicBitVectorStrategy(n);
}