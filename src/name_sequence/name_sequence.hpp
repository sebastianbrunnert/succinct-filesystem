/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <cstddef>
#include <string>

/**
 * This class represents a dynamic sequence of strings.
 * It holds the names of the files and directories in the filesystem.
 * As different strategies that need different usage patterns for the name sequence exist, the interface is extended.
 */
class NameSequence {
public:
    /**
     * Virtual destructor.
     */
    virtual ~NameSequence() = default;

    /**
     * Sets the name at the specified position.
     * 
     * @param position The 0-based position of the name to set.
     * @param name The name to set at the specified position. Must be non-empty.
     * @throws std::out_of_range if the position exceeds the size of the name sequence.
     */
    virtual void set(size_t position, const std::string& name) = 0;

    /**
     * Gets the name at the specified position.
     * 
     * @param position The 0-based position of the name to get.
     * @return The name at the specified position.
     * @throws std::out_of_range if the position exceeds the size of the name sequence.
     */
    virtual std::string access(size_t position) const = 0;

    /**
     * Gets the current size of the name sequence.
     * 
     * @return The number of names in the name sequence.
     */
    virtual size_t size() const = 0;

    /**
     * Inserts a name at the specified position.
     * 
     * @param position The 0-based position at which to insert the new name.
     * @param name The name to insert at the specified position.
     * @throws std::out_of_range if the position exceeds the size of the name sequence plus one.
     */ 
    virtual void insert(size_t position, const std::string& name) = 0;

    /**
     * Removes the name at the specified position.
     * 
     * @param position The 0-based position of the name to remove.
     * @throws std::out_of_range if the position exceeds the size of the name sequence.
     */    
    virtual void remove(size_t position) = 0;
};

/**
 * Factory function to create a NameSequence instance based on a specified strategy.
 * 
 * @param NameSequenceStrategy The strategy to use for the NameSequence implementation.
 * @return A pointer to a new NameSequence instance.
 */
template <typename NameSequenceStrategy> NameSequence* create_name_sequence();

// Different strategies for implementing the interface
class ConcatenatedNameSequenceStrategy;
template <> NameSequence* create_name_sequence<ConcatenatedNameSequenceStrategy>();

class ArrayNameSequenceStrategy;
template <> NameSequence* create_name_sequence<ArrayNameSequenceStrategy>();
