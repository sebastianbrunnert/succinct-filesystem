/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <string>

/**
 * The BlockDevice class simulates a storage device that reads and writes blocks of data.
 * It creates a raw file on the original filesystem to store the data, and provides methods to interact with the data.
 */
class BlockDevice {
private:
    int file;
    size_t block_size;
public:
    /**
     * @param file The path to the file that will be used to store the data. If the file does not exist, it will be created.
     * @param block_size The size of each block in bytes. Typically 4096 bytes.
     * @throws std::runtime_error if the file cannot be opened or created.
     */
    BlockDevice(const std::string filename, size_t block_size = 4096);
    

    /**
     * Virtual destructor.
     */
    virtual ~BlockDevice();

    /**
     * @return The block size.
     */
    int get_block_size() const {
        return block_size;
    }

    /**
     * Reads a block into the provided buffer.
     * 
     * @param block_index The index of the block to read. If the block does not exist, the buffer will be empty.
     * @param buffer The buffer to write the data into. Must be block_size bytes.
     */
    virtual void read_block(size_t block_index, char* buffer);

    /**
     * Writes a block of data from the provided buffer.
     * 
     * @param block_index The index of the block to write.
     * @param buffer The buffer containing the data. Must be block_size bytes.
     */
    virtual void write_block(size_t block_index, const char* buffer);

};