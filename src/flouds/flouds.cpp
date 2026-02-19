/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "flouds.hpp"

Flouds* create_flouds() {
    BitVector* bv = create_bitvector<WordBitVectorStrategy>(1);
    bv->set(0, true); // root node is a folder

    uint8_t* data = new uint8_t[1];
    data[0] = 2;
    TwoBitWaveletTree<WordBitVectorStrategy>* wt = create_two_bit_wavelet_tree<WordBitVectorStrategy>(data, 1);

    NameSequence* ns = create_name_sequence<ImmerNameSequenceStrategy>();
    ns->insert(0, "root");

    return new Flouds(bv, wt, ns);
}

Flouds::~Flouds() {
    delete structure;
    delete types;
    delete names;
}

size_t Flouds::parent(size_t node_id) {
    if(node_id == 0) throw std::out_of_range("root node has no parent");
    size_t folder_count = structure->rank1(node_id);
    return structure->select1(folder_count);
}

size_t Flouds::children_count(size_t node_id) {
    return 0;
}

size_t Flouds::child(size_t node_id, size_t child_index) {
    return 0;
}

std::string Flouds::get_name(size_t node_id) {
    return "";
}

bool Flouds::is_folder(size_t node_id) {
    uint8_t type = types->access(node_id);
    return type == 1 || type == 2;
}

bool Flouds::is_file(size_t node_id) {
    uint8_t type = types->access(node_id);
    return type == 0;
}

void Flouds::insert(size_t parent_id, const std::string& name, bool is_folder) {
    
}

void Flouds::remove(size_t node_id) {
    
}   