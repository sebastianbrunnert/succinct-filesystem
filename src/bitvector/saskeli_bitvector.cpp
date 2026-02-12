/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "bitvector.hpp"
#include "../../external/saskeli_bit_vector/bit_vector/bv.hpp"
#include <stdexcept>

/**
 * Fast dynamic bit vector implementtion based:
 * @INPROCEEDINGS{9810697,
 * author={Dönges, Saska and Puglisi, Simon J. and Raman, Rajeev},
 * booktitle={2022 Data Compression Conference (DCC)}, 
 * title={On Dynamic Bitvector Implementations}, 
 * year={2022},
 * volume={},
 * number={},
 * pages={252-261},
 * doi={10.1109/DCC52660.2022.00033}}
 */
class SaskeliBitVectorStrategy : public BitVector {
private:
    bv::bv saskeli;

public:
    SaskeliBitVectorStrategy(size_t n) : saskeli(n, false) {}

    void set(size_t position, bool value) override {
        if (position >= saskeli.size()) throw std::out_of_range("position out of range");
        saskeli.set(position, value);
    }

    bool access(size_t position) const override {
        if (position >= saskeli.size()) throw std::out_of_range("position out of range");
        return saskeli.at(position);
    }

    size_t size() const override {
        return saskeli.size();
    }

    size_t rank1(size_t position) const override {
        if (position >= saskeli.size()) throw std::out_of_range("position out of range");
        return saskeli.rank(position + 1);
    }

    size_t rank0(size_t position) const override {
        if (position >= saskeli.size()) throw std::out_of_range("position out of range");
        return saskeli.rank0(position + 1);
    }

    size_t select1(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        return saskeli.select(n);
    }

    size_t select0(size_t n) const override {
        if (n == 0) throw std::out_of_range("n must be greater than zero");
        return saskeli.select0(n);
    }

    void insert(size_t position, bool value) override {
        if (position > saskeli.size()) throw std::out_of_range("position out of range");
        saskeli.insert(position, value);
    }

    void remove(size_t position) override {
        if (position >= saskeli.size()) throw std::out_of_range("position out of range");
        saskeli.remove(position);
    }
};

template <>
BitVector* create_bitvector<SaskeliBitVectorStrategy>(std::size_t n) {
    return new SaskeliBitVectorStrategy(n);
}