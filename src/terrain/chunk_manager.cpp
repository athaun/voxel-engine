#include "chunk_manager.h"
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <iostream>
#include <shared_mutex>
#include <semaphore>

namespace ChunkManager {
    struct PendingChunk {
        std::pair<int, int> coords;
        std::future<Chunk*> future;
        
        PendingChunk(std::pair<int, int> c, std::future<Chunk*>&& f) 
            : coords(c), future(std::move(f)) {}
    };

    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            return hash1 ^ hash2;
        }
    };

    const int MAX_CONCURRENT_CHUNKS = std::thread::hardware_concurrency() - 1;

    std::counting_semaphore chunk_semaphore{MAX_CONCURRENT_CHUNKS};
    std::shared_mutex chunks_mutex;

    std::unordered_map<std::pair<int, int>, Chunk*, pair_hash> chunks;
    std::vector<PendingChunk> pending_chunks;
    
    void build_chunk(int x, int y, int z) {
        {
            std::unique_lock<std::shared_mutex> write_lock(chunks_mutex);
            if (chunks.find(std::make_pair(x, z)) != chunks.end()) {
                return;
            }
        }

        // Create a task that acquires and releases the semaphore
        auto chunk_task = [x, y, z]() {
            ChunkManager::chunk_semaphore.acquire();  // Wait for available slot
            auto chunk = new Chunk(x, y, z);
            ChunkManager::chunk_semaphore.release();  // Release slot when done
            return chunk;
        };
        
        auto future = std::async(std::launch::async, chunk_task);

        pending_chunks.emplace_back(std::make_pair(x, z), std::move(future));
    }

    void chunk_circle(int x, int z, int radius) {
        // Build chunks in spiral pattern from center outward for better loading pattern
        std::vector<std::pair<int, int>> chunk_positions;
        
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dz = -radius; dz <= radius; ++dz) {
                if (dx * dx + dz * dz <= radius * radius) {
                    // Calculate distance from center
                    float dist = std::sqrt(dx * dx + dz * dz);
                    chunk_positions.push_back({dx, dz});
                }
            }
        }
        
        // Sort by distance from center
        std::sort(chunk_positions.begin(), chunk_positions.end(),
            [](const auto& a, const auto& b) {
                return (a.first * a.first + a.second * a.second) <
                       (b.first * b.first + b.second * b.second);
            });
            
        // Build chunks in order
        for (const auto& [dx, dz] : chunk_positions) {
            build_chunk(x + dx, 0, z + dz);
        }
    }

    void init() {
        chunk_circle(0, 0, 15);
        // 20 breaks it :)
    }
    
    void update() {
        std::vector<PendingChunk> ready_chunks;
            
        auto it = pending_chunks.begin();
        size_t updates = 0;
        
        while (it != pending_chunks.end()) {
            if (it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                ready_chunks.push_back(std::move(*it));
                it = pending_chunks.erase(it);
                updates++;
            } else {
                ++it;
            }
        }
        

        // Process ready chunks
        if (!ready_chunks.empty()) {
            std::unique_lock<std::shared_mutex> lock(chunks_mutex);
            for (auto& pending : ready_chunks) {
                chunks[pending.coords] = pending.future.get();
            }
        }
    }

    void cleanup() {
        std::unique_lock lock(chunks_mutex);

        for (auto& [coords, chunk] : chunks) {
            delete chunk;
        }

        chunks.clear();

        // Should probably cleanup pending chunk threads, but it caused the program to hang which is annoying
        // And I know the OS will do it for me so...
    }

    void render() {
        std::shared_lock<std::shared_mutex> lock(chunks_mutex);
        for (const auto& [_, chunk] : chunks) {
            chunk->submit_batch();
        }
    }
}