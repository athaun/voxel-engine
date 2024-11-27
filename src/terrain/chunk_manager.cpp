// #include "chunk_manager.h"
// #include <vector>
// #include <thread>
// #include <future>
// #include <mutex>
// #include <queue>
// #include <iostream>
// #include <shared_mutex>
// #include <semaphore>

// #include "../render/frustum.h"
// #include "../core/core.h"

// namespace ChunkManager {
    
//     static Frustum current_frustum;

//     struct PendingChunk {
//         std::pair<int, int> coords;
//         std::future<Chunk*> future;
        
//         PendingChunk(std::pair<int, int> c, std::future<Chunk*>&& f) 
//             : coords(c), future(std::move(f)) {}
//     };

//     struct pair_hash {
//         template <class T1, class T2>
//         std::size_t operator()(const std::pair<T1, T2>& p) const {
//             auto hash1 = std::hash<T1>{}(p.first);
//             auto hash2 = std::hash<T2>{}(p.second);
//             return hash1 ^ hash2;
//         }
//     };

//     const int MAX_CONCURRENT_CHUNKS = std::thread::hardware_concurrency() - 1;

//     std::counting_semaphore chunk_semaphore{MAX_CONCURRENT_CHUNKS};
//     std::shared_mutex chunks_mutex;

//     std::unordered_map<std::pair<int, int>, Chunk*, pair_hash> chunks;
//     std::vector<PendingChunk> pending_chunks;
    
//     void build_chunk(int x, int y, int z) {
//         {
//             std::unique_lock<std::shared_mutex> write_lock(chunks_mutex);
//             if (chunks.find(std::make_pair(x, z)) != chunks.end()) {
//                 return;
//             }
//         }

//         // Create a task that acquires and releases the semaphore
//         auto chunk_task = [x, y, z]() {
//             ChunkManager::chunk_semaphore.acquire();  // Wait for available slot
//             auto chunk = new Chunk(x, y, z);
//             ChunkManager::chunk_semaphore.release();  // Release slot when done
//             return chunk;
//         };
        
//         auto future = std::async(std::launch::async, chunk_task);

//         pending_chunks.emplace_back(std::make_pair(x, z), std::move(future));
//     }

//     void chunk_circle(int x, int z, int radius) {
//         // Build chunks in spiral pattern from center outward for better loading pattern
//         std::vector<std::pair<int, int>> chunk_positions;
        
//         for (int dx = -radius; dx <= radius; ++dx) {
//             for (int dz = -radius; dz <= radius; ++dz) {
//                 if (dx * dx + dz * dz <= radius * radius) {
//                     // Calculate distance from center
//                     float dist = std::sqrt(dx * dx + dz * dz);
//                     chunk_positions.push_back({dx, dz});
//                 }
//             }
//         }
        
//         // Sort by distance from center
//         std::sort(chunk_positions.begin(), chunk_positions.end(),
//             [](const auto& a, const auto& b) {
//                 return (a.first * a.first + a.second * a.second) <
//                        (b.first * b.first + b.second * b.second);
//             });
            
//         // Build chunks in order
//         for (const auto& [dx, dz] : chunk_positions) {
//             build_chunk(x + dx, 0, z + dz);
//         }
//     }

//     void init() {
//         chunk_circle(0, 0, 4);
//         // 20 breaks it :)
//     }
    
//     void update() {
//         std::vector<PendingChunk> ready_chunks;
            
//         auto it = pending_chunks.begin();
//         size_t updates = 0;
        
//         while (it != pending_chunks.end()) {
//             if (it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
//                 ready_chunks.push_back(std::move(*it));
//                 it = pending_chunks.erase(it);
//                 updates++;
//             } else {
//                 ++it;
//             }
//         }        

//         // Process ready chunks
//         if (!ready_chunks.empty()) {
//             std::unique_lock<std::shared_mutex> lock(chunks_mutex);
//             for (auto& pending : ready_chunks) {
//                 chunks[pending.coords] = pending.future.get();
//             }
//         }
//     }

//     void cleanup() {
//         std::unique_lock lock(chunks_mutex);

//         for (auto& [coords, chunk] : chunks) {
//             delete chunk;
//         }

//         chunks.clear();

//         // Should probably cleanup pending chunk threads, but it caused the program to hang which is annoying
//         // And I know the OS will do it for me so...
//     }

//     void render() {
//         // Update frustum planes
//         current_frustum.update(Core::camera.get_view(), Core::camera.get_projection());
        
//         std::shared_lock<std::shared_mutex> lock(chunks_mutex);
//         for (const auto& [coords, chunk] : chunks) {
//             // Assuming each chunk is 16x16x16 units
//             float minX = coords.first * CHUNK_WIDTH;
//             float minZ = coords.second * CHUNK_DEPTH;
//             float maxX = minX + CHUNK_WIDTH;
//             float maxZ = minZ + CHUNK_DEPTH;
            
//             float minY = 0.0f;
//             float maxY = CHUNK_HEIGHT;
            
//             if (current_frustum.is_box_visible(minX, minY, minZ, maxX, maxY, maxZ)) {
//                 chunk->submit_batch();
//             }
//         }
//     }
// }

#include "chunk_manager.h"
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <iostream>
#include <shared_mutex>
#include <semaphore>
#include <cmath>

#include "../render/frustum.h"
#include "../core/core.h"
#include "../core/keyboard.h"  // Assuming this for player input (if required)

namespace ChunkManager {
    static Frustum current_frustum;
    static const int CHUNK_RADIUS = 14;  // Number of chunks to generate around the player
    
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

    float distance_between(int x1, int z1, int x2, int z2) {
        return std::sqrt(std::pow(x2 - x1, 2) + std::pow(z2 - z1, 2));
    }

    // Generate a chunk at a specific position (x, y, z)
    void build_chunk(int x, int y, int z) {
        {
            std::unique_lock<std::shared_mutex> write_lock(chunks_mutex);
            if (chunks.find(std::make_pair(x, z)) != chunks.end()) {
                return;  // Skip if chunk already exists
            }
        }

        for (const auto& pending : pending_chunks) {
            if (pending.coords == std::make_pair(x, z)) {
                return;  // Skip if chunk is already in pending state
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

    // Generate chunks in a circular area around the player's current position
    void generate_chunks_near_player(int player_x, int player_z) {
        // Calculate the chunk range based on the player position
        int min_x = (player_x - CHUNK_RADIUS);
        int max_x = (player_x + CHUNK_RADIUS);
        int min_z = (player_z - CHUNK_RADIUS);
        int max_z = (player_z + CHUNK_RADIUS);

        // Loop over the range and generate chunks
        for (int x = min_x; x <= max_x; ++x) {
            for (int z = min_z; z <= max_z; ++z) {
                // Skip chunks that are too far away from the player
                if (distance_between(player_x, player_z, x, z) <= CHUNK_RADIUS) {
                    build_chunk(x, 0, z);
                }
            }
        }
    }

    // Unload chunks that are far away from the player
    void unload_distant_chunks(int player_x, int player_z) {
        std::unique_lock<std::shared_mutex> write_lock(chunks_mutex);
        for (auto it = chunks.begin(); it != chunks.end();) {
            const auto& chunk_coords = it->first;
            if (distance_between(player_x, player_z, chunk_coords.first, chunk_coords.second) > CHUNK_RADIUS) {
                delete it->second;  // Clean up memory
                it = chunks.erase(it);  // Remove from map
            } else {
                ++it;
            }
        }
    }

    void chunk_square(int x, int z, int size) {
        for (int dx = -size / 2; dx < size / 2; ++dx) {
            for (int dz = -size / 2; dz < size / 2; ++dz) {
                build_chunk(x + dx, 0, z + dz);
            }
        }
    }

    void init() {
        // Initial chunk generation, centered at player position (0, 0)
        generate_chunks_near_player(0, 0);
    }

    void update() {

        static int last_player_x, last_player_z, player_x, player_z;

        last_player_x = player_x;
        last_player_z = player_z;

        player_x = static_cast<int>(Core::camera.get_position().x / CHUNK_WIDTH);
        player_z = static_cast<int>(Core::camera.get_position().z / CHUNK_DEPTH);

        if (last_player_x != player_x || last_player_z != player_z) {        
            // generate_chunks_near_player(player_x, player_z);
            // unload_distant_chunks(player_x, player_z);
        }

        // Process the ready chunks
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

        // Update the chunks map with the ready chunks
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
            chunks.erase(coords);
        }

        chunks.clear();
    }

    void destroy_all() {
        std::unique_lock<std::shared_mutex> write_lock(chunks_mutex);

        for (auto& [coords, chunk] : chunks) {
            delete chunk;
        }

        chunks.clear();

    }

    void render() {
        // Update frustum planes
        current_frustum.update(Core::camera.get_view(), Core::camera.get_projection());
        
        std::shared_lock<std::shared_mutex> lock(chunks_mutex);
        for (const auto& [coords, chunk] : chunks) {
            // if (!chunk->is_visible) continue;

            // Assuming each chunk is 16x16x16 units
            float minX = coords.first * CHUNK_WIDTH;
            float minZ = coords.second * CHUNK_DEPTH;
            float maxX = minX + CHUNK_WIDTH;
            float maxZ = minZ + CHUNK_DEPTH;
            
            float minY = 0.0f;
            float maxY = CHUNK_HEIGHT;
            
            if (current_frustum.is_box_visible(minX, minY, minZ, maxX, maxY, maxZ)) {
                chunk->submit_batch();
            }
        }
    }
}
