/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <cstddef>
#include <string>
#include "../bitvector/bitvector.hpp"
#include "../wavelet_tree/two_bit_wavelet_tree.hpp"
#include "../name_sequence/name_sequence.hpp"

/**
 * This class represents the FLOUDS data structure.
 * @article{FLOUDS,
 * author = {Peters, Daniel and Fischer, Johannes and Thiel, F. and Seifert, Jean-Pierre},
 * year = {2017},
 * month = {09},
 * pages = {51-57},
 * title = {FLOUDS: A Succinct File System Structure},
 * doi = {10.15439/2017F535}}
 */
class Flouds {
private:
    BitVector* structure;
    // 2-bit wavelet tree to store the type of each node (0 = file, 1 = folder, 2 = empty folder, 3 = will be used for future extensions)
    TwoBitWaveletTree<WordBitVectorStrategy>* types;
    NameSequence* names;

public:
    /**
     * Parameterized constructor.
     */
    Flouds(BitVector* structure, TwoBitWaveletTree<WordBitVectorStrategy>* types, NameSequence* names)
        : structure(structure), types(types), names(names) {}

    /**
     * Virtual destructor
     */
    virtual ~Flouds();


    /**
     * Gets the parent nodes index of the node.
     * 
     * @param node_id The index of the node for which to get the parent.
     * @return The index of the parent node.
     * @throws std::out_of_range if the node_id does not exist or is the root node.
     */
    virtual size_t parent(size_t node_id);

    /**
     * Gets the number of children of the node.
     * 
     * @param node_id The index of the node for which to get the number of children. Must be a valid folder node.
     * @return The number of children of the node.
     * @throws std::out_of_range if the node_id does not exist.
     */
    virtual size_t children_count(size_t node_id);

    /**
     * Gets the index of the n-th child of the node.
     * 
     * @param node_id The index of the node for which to get the child. Must be a valid folder node.
     * @param child_index The 0-based index of the child to get.
     * @return The index of the n-th child of the node.
     * @throws std::out_of_range if the node_id does not exist or if child_index exceeds the number of children of the node.
     */
    virtual size_t child(size_t node_id, size_t child_index);

    /**
     * Gets the name of the node.
     * 
     * @param node_id The index of the node for which to get the name.
     * @return The name of the node.
     * @throws std::out_of_range if the node_id does not exist.
     */
    virtual std::string get_name(size_t node_id);

    /**
     * Checks if the node is a folder.
     * 
     * @param node_id The index of the node for which to check if it is a folder.
     * @return true if the node is a folder, false otherwise.
     * @throws std::out_of_range if the node_id does not exist.
     */
    virtual bool is_folder(size_t node_id);

    /**
     * Checks if the node is a file.
     * 
     * @param node_id The index of the node for which to check if it is a file.
     * @return true if the node is a file, false otherwise.
     * @throws std::out_of_range if the node_id does not exist.
     */
    virtual bool is_file(size_t node_id);

    /**
     * Inserts a new node as a child of the specified parent node.
     * 
     * @param parent_id The index of the parent node to which to add the new node. Must be a valid folder node.
     * @param name The name of the new node.
     * @param is_folder true if the new node is a folder, false if it is a file.
     * @throws std::out_of_range if the parent_id does not exist.
     */
    virtual void insert(size_t parent_id, const std::string& name, bool is_folder);

    /**
     * Removes the node with the specified index from the FLOUDS structure, along with all of its children if it is a folder.
     * 
     * @param node_id The index of the node to remove. Must be a leaf node and not the root node.
     * @throws std::out_of_range if the node_id does not exist or is the root node.
     */
    virtual void remove(size_t node_id);

};

/**
 * Factory function to create a new instance of the Flouds class.
 */
Flouds* create_flouds();