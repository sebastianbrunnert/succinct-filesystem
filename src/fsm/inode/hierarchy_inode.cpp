#include "inode.hpp"
#include <vector>
#include <algorithm>
#include <cstring>
#include <optional>

/**
 * This class implements an inode manager strategy that stores inodes in multiple chunks of a fixed size. Each chunk is allocated sepateley on the block device. We always cache one chunk in memory, so we can read or write inodes of the current context without additional accesses.
 */
class HierarchyInodeManagerStrategy : public InodeManager {
private:
    // Inode is 48 bytes, so we can fit 85 inodes into a 4096 byte block
    static constexpr size_t CHUNK_SIZE = 85;

    /**
     * This struct represents a chunk of inodes. It contains the handle where the data is stored, the starting inode number of the chunk and the number of inodes in the chunk.
     */
    struct InodeChunk {
        size_t handle;
        size_t start_inode;
        size_t num_inodes;
    };

    // All chunks sorted by start_inode are stored here and loaded initially from the block device. The actual inodes are loaded on demand and cached.
    std::vector<InodeChunk> chunks;

    // Cache for currently loaded chunk
    std::optional<size_t> cache_handle = std::nullopt;
    std::vector<Inode> cached_inodes;
    bool cache_dirty = false;

    /**
     * Finds the chunk containing the given inode number. Returns nullptr if no such chunk exists.
     * 
     * @param inode The inode number to find the chunk for.
     * @return The chunk containing the given inode number or nullptr if no such chunk exists.
     */
    InodeChunk* find_chunk(size_t inode) {
        for (InodeChunk& c : chunks) {
            if (c.start_inode <= inode && inode < c.start_inode + c.num_inodes) {
                return &c;
            }
        }
        return nullptr;
    }

    /**
     * Flushes the currently cached chunk to the block device if it is dirty.
     */
    void flush_cache() {
        if (!cache_handle || !cache_dirty) return;

        allocation_manager->write(*cache_handle, (char*)cached_inodes.data(), cached_inodes.size() * sizeof(Inode), 0);
        cache_dirty = false;
    }

    /**
     * Loads a chunk into the cache.
     * 
     * @param chunk The chunk to load into the cache.
     */
    void load_chunk(InodeChunk& chunk) {
        if (cache_handle && *cache_handle == chunk.handle) return;
        flush_cache();
        cached_inodes.resize(chunk.num_inodes);
        allocation_manager->read(chunk.handle, (char*)cached_inodes.data(), chunk.num_inodes * sizeof(Inode), 0);
        cache_handle = chunk.handle;
        cache_dirty = false;
    }

    /**
     * Shifts the start inode numbers of all chunks starting from the given index by the given delta.
     * 
     * @param from_index The index to start shifting from. Must be a valid index in the chunks vector.
     * @param delta The amount to shift the start inode numbers by.
     */
    void shift_starts(size_t from_index, int delta) {
        for (size_t i = from_index; i < chunks.size(); i++) {
            chunks[i].start_inode += delta;
        }
    }

public:
    HierarchyInodeManagerStrategy(AllocationManager* allocation_manager)
        : InodeManager(allocation_manager) {}

    Inode* get_inode(size_t inode) override {
        InodeChunk* chunk = find_chunk(inode);
        if (!chunk) return nullptr;
        load_chunk(*chunk);
        return &cached_inodes[inode - chunk->start_inode];
    }

    Inode* insert_inode(size_t inode) override {
        if (chunks.empty()) {
            // First inode, create first chunk
            InodeChunk chunk{0, 0, 1};
            chunk.handle = allocation_manager->allocate(CHUNK_SIZE * sizeof(Inode));
            cached_inodes = {Inode{}};
            allocation_manager->write(chunk.handle, (char*)cached_inodes.data(), sizeof(Inode), 0);
            chunks.push_back(chunk);
            cache_handle = chunk.handle;
            cache_dirty = false;
            return &cached_inodes[0];
        }

        // Find the chunk to insert into
        InodeChunk* chunk = find_chunk(inode);
        if (!chunk) {
            chunk = &chunks.back();
        }
        load_chunk(*chunk);

        // Insert the chunk
        size_t local_index = inode - chunk->start_inode;
        cached_inodes.insert(cached_inodes.begin() + local_index, Inode{});
        chunk->num_inodes++;
        cache_dirty = true;

        // Shift all following chunks
        size_t chunk_index = chunk - chunks.data();
        shift_starts(chunk_index + 1, 1);

        // If the chunk is not full, we are done
        if (chunk->num_inodes <= CHUNK_SIZE) {
            return &cached_inodes[local_index];
        }

        // Split
        std::vector<Inode> right_data(cached_inodes.begin() + CHUNK_SIZE, cached_inodes.end());
        cached_inodes.resize(CHUNK_SIZE);
        chunk->num_inodes = CHUNK_SIZE;
        allocation_manager->write(chunk->handle, (char*)cached_inodes.data(), CHUNK_SIZE * sizeof(Inode), 0);

        InodeChunk new_chunk{0, chunk->start_inode + CHUNK_SIZE, right_data.size()};
        new_chunk.handle = allocation_manager->allocate(CHUNK_SIZE * sizeof(Inode));
        allocation_manager->write(new_chunk.handle, (char*)right_data.data(), right_data.size() * sizeof(Inode), 0);
        chunks.insert(chunks.begin() + chunk_index + 1, new_chunk);

        cache_dirty = false;

        if (local_index < CHUNK_SIZE) {
            // inode is in left chunk, cache is already correct
            return &cached_inodes[local_index];
        }

        // inode is in right chunk, update cache to point there
        cached_inodes = std::move(right_data);
        cache_handle = new_chunk.handle;
        return &cached_inodes[local_index - CHUNK_SIZE];
    }

    void remove_inode(size_t inode) override {
        // Find the chunk containing the inode
        InodeChunk* chunk = find_chunk(inode);
        if (!chunk) return;

        size_t chunk_index = chunk - chunks.data();
        load_chunk(*chunk);

        size_t local_index = inode - chunk->start_inode;
        size_t old_num = chunk->num_inodes;
        cached_inodes.erase(cached_inodes.begin() + local_index);
        chunk->num_inodes--;
        cache_dirty = true;

        // If the chunk is empty, remove it
        if (chunk->num_inodes == 0) {
            allocation_manager->free(chunk->handle, old_num * sizeof(Inode));
            chunks.erase(chunks.begin() + chunk_index);
            cache_handle = std::nullopt;
            cached_inodes.clear();
            cache_dirty = false;
            shift_starts(chunk_index, -1);
            return;
        }

        shift_starts(chunk_index + 1, -1);
    }

    void serialize(char* buffer, size_t* offset) override {
        flush_cache();
        size_t n = chunks.size();
        std::memcpy(buffer + *offset, &n, sizeof(size_t));
        *offset += sizeof(size_t);
        for (const InodeChunk& c : chunks) {
            std::memcpy(buffer + *offset, &c, sizeof(InodeChunk));
            *offset += sizeof(InodeChunk);
        }
    }

    void deserialize(const char* buffer, size_t* offset) override {
        size_t n;
        std::memcpy(&n, buffer + *offset, sizeof(size_t));
        *offset += sizeof(size_t);
        chunks.resize(n);
        for (InodeChunk& c : chunks) {
            std::memcpy(&c, buffer + *offset, sizeof(InodeChunk));
            *offset += sizeof(InodeChunk);
        }
        cache_handle = std::nullopt;
        cached_inodes.clear();
        cache_dirty = false;
    }

    size_t get_serialized_size() override {
        return sizeof(size_t) + chunks.size() * sizeof(InodeChunk);
    }
};

template <>
InodeManager* create_inode_manager<HierarchyInodeManagerStrategy>(AllocationManager* allocation_manager) {
    return new HierarchyInodeManagerStrategy(allocation_manager);
}