/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "flouds.hpp"

Flouds* create_flouds() {
    BitVector* bv = create_bitvector<WordBitVectorStrategy>(1);
    bv->set(0, true);

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

}

size_t Flouds::children_count(size_t node_id) {
    if (is_empty_folder(node_id)) {
        return 0;
    }

    size_t folder_index = types->rank(1, node_id) + 1;
    size_t start = structure->select1(folder_index);

    size_t total_folders = structure->rank1(structure->size() - 1);

    if (folder_index+1 < total_folders) {
        size_t next_folder_pos = structure->select1(folder_index + 1);
        return next_folder_pos - start;
    } else {
        return structure->size() - start;
    }
}

size_t Flouds::child(size_t node_id, size_t child_index) {
    size_t folder_index = types->rank(1, node_id) + 1;
    try {
        return structure->select1(folder_index) + child_index;
    } catch (std::out_of_range& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return structure->size();
    }
}

std::string Flouds::get_name(size_t node_id) {
    return names->access(node_id);
}

bool Flouds::is_folder(size_t node_id) {
    uint8_t type = types->access(node_id);
    return type == 1 || type == 2;
}

bool Flouds::is_file(size_t node_id) {
    uint8_t type = types->access(node_id);
    return type == 0;
}

bool Flouds::is_empty_folder(size_t node_id) {
    uint8_t type = types->access(node_id);
    return type == 2;
}

size_t Flouds::insert(size_t parent_id, const std::string& name, bool is_folder) {
    bool was_empty = is_empty_folder(parent_id);

    size_t children_count = 0;    
    if (was_empty) {
        types->set(parent_id, 1);   
    } else {
        children_count = this->children_count(parent_id);
    }

    size_t insert_pos = child(parent_id, children_count);
    structure->insert(insert_pos, was_empty);
    names->insert(insert_pos, name);
    types->insert(insert_pos, is_folder ? 2 : 0);

    return insert_pos;
}

void Flouds::remove(size_t node_id) {
    
}   