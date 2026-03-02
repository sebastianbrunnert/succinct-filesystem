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
    size_t parent_folder_index = structure->rank1(node_id);
    size_t parent = types->select(1, parent_folder_index - 1);
    return parent;
}

size_t Flouds::children_count(size_t node_id) {
    if (is_empty_folder(node_id)) {
        return 0;
    }

    // Calculate the folder index of the node (1-based)
    size_t folder_index = types->rank(1, node_id) + 1;

    // Calculate the start position of the childrens
    size_t start = structure->select1(folder_index);

    // Check if the endposition of the children is in the next folder or at the end of the structure
    size_t total_folders = structure->rank1(structure->size() - 1);
    if (folder_index+1 <= total_folders) {
        size_t next_folder_pos = structure->select1(folder_index + 1);
        return next_folder_pos - start;
    } else {
        return structure->size() - start;
    }
}

size_t Flouds::child(size_t node_id, size_t child_index) {
    // Calculate the folder index of the node (1-based)
    size_t folder_index = types->rank(1, node_id) + 1;
    // Calculate the start position of the childrens + child_index
    return structure->select1(folder_index) + child_index;
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

    // Calculate the insertion position (analogusly to child)
    size_t parent_folder_index = types->rank(1, parent_id) + 1;
    size_t insert_pos = 0;
    try {
        insert_pos = structure->select1(parent_folder_index) + children_count;
    } catch (std::out_of_range& e) {
        insert_pos = structure->size() + children_count;
    }

    // Insert into the structure, names and types
    structure->insert(insert_pos, was_empty);
    names->insert(insert_pos, name);
    types->insert(insert_pos, is_folder ? 2 : 0);

    return insert_pos;
}

void Flouds::remove(size_t node_id) {
    size_t parent_index = parent(node_id);
    size_t parent_children_before = children_count(parent_index);

    bool was_first_child = structure->access(node_id);

    structure->remove(node_id);
    names->remove(node_id);
    types->remove(node_id);

    if(parent_children_before == 1) {
        // If the removed node was the only child, we need to update the type of the parent to empty folder
        types->set(parent_index, 2);
    } else if (was_first_child) {
        // If the removed node was the first child, we need to update the structure bit of the new first child
        structure->set(node_id, true);
    }
}

size_t Flouds::path(std::string path) {
    if(path == "/") return 0;

    size_t current = 0;
    size_t start = 1;  // Skip leading '/'

    while (start < path.length()) {
        // Find next '/' or end of string
        size_t end = path.find('/', start);
        if (end == std::string::npos) {
            end = path.length();
        }
        
        std::string_view component(path.data() + start, end - start);
        
        int children_count = this->children_count(current);
        if (children_count == 0) {
            throw std::out_of_range("path does not exist");
        }
        int first_child = this->child(current, 0);
        int last_child = this->child(current, children_count - 1);

        bool found = false;
        for (size_t i = first_child; i <= last_child; i++) {
            std::string child_name = names->access(i);
            if (child_name == component) {
                current = i;
                found = true;
                break;
            }
        }
        
        if (!found) {
            throw std::out_of_range("path does not exist");
        }

        start = end + 1;
    }

    return current;
}

size_t Flouds::get_serialized_size() {
    return structure->get_serialized_size() + types->get_serialized_size() + names->get_serialized_size();
}

void Flouds::serialize(char* buffer, size_t* offset) {
    structure->serialize(buffer, offset);
    types->serialize(buffer, offset);
    names->serialize(buffer, offset);
}

void Flouds::deserialize(const char* buffer, size_t* offset) {
    structure->deserialize(buffer, offset);
    types->deserialize(buffer, offset);
    names->deserialize(buffer, offset);
}