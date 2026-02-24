/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <cstddef>

/**
 * This class defines the interface for serializable objects to store and load them on block devices.
 */
class Serializable {

    /**
     * Serializes the object into a byte array.
     * 
     * @param buffer The buffer to write the serialized data into. Must be at least get_serialized_size() bytes including the offset.
     * @param offset The position within the buffer to start writing the serialized data to.
     */
    virtual void serialize(char* buffer, size_t* offset) = 0;

    /**
     * Deserializes the object from a byte array.
     * 
     * @param buffer The buffer containing the serialized data. Must be at least get_serialized_size() bytes including the offset and must be in the format produced by serialize().
     * @param offset The position within the buffer to start reading the serialized data from.
     */
    virtual void deserialize(const char* buffer, size_t* offset) = 0;

    /**
     * Gets the size of the serialized object in bytes.
     * 
     * @return The size of the serialized object in bytes.
     */
    virtual size_t get_serialized_size() = 0;

};