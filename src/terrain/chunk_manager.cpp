#include "chunk_manager.h"
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <iostream>

namespace ChunkManager {
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            return hash1 ^ hash2;
        }
    };

    std::unordered_map<std::pair<int, int>, Chunk*, pair_hash> chunks;
    std::mutex chunks_mutex;
    
    struct PendingChunk {
        std::pair<int, int> coords;
        std::future<Chunk*> future;
        
        PendingChunk(std::pair<int, int> c, std::future<Chunk*>&& f) 
            : coords(c), future(std::move(f)) {}
    };
    std::vector<PendingChunk> pending_chunks;
    
    void build_chunk(int x, int y, int z) {
        // Check if chunk already exists
        {
            std::lock_guard<std::mutex> lock(chunks_mutex);
            if (chunks.find(std::make_pair(x, z)) != chunks.end()) {
                return;
            }
        }
        
        // Launch async task
        auto future = std::async(std::launch::async, [x, y, z]() {
            return new Chunk(x, y, z);
        });
        
        pending_chunks.emplace_back(std::make_pair(x, z), std::move(future));
    }

    void chunk_circle(int x, int z, int radius) {
        // Build chunks around the player
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dz = -radius; dz <= radius; ++dz) {
                if (dx * dx + dz * dz <= radius * radius) {
                    build_chunk(x + dx, 0, z + dz);
                }
            }
        }
    }

    void init() {
        chunk_circle(0, 0, 2);
    }

    void update() {
        // Check for completed chunks without blocking
        std::vector<PendingChunk> ready_chunks;
        
        // Move ready chunks to a temporary vector
        for (auto it = pending_chunks.begin(); it != pending_chunks.end();) {
            if (it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                ready_chunks.push_back(std::move(*it));
                it = pending_chunks.erase(it);
            } else {
                ++it;
            }
        }

        // Insert ready chunks into the chunks map
        {
            std::lock_guard<std::mutex> lock(chunks_mutex);
            for (auto& pending : ready_chunks) {
            chunks[pending.coords] = pending.future.get();
            }
        }
    }

    void render() {
        std::lock_guard<std::mutex> lock(chunks_mutex);
        for (auto& [_, chunk] : chunks) {
            chunk->submit_batch();
        }
    }

}