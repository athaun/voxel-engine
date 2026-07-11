#include "chunk_manager.h"
#include "../render/frustum.h"
#include "../core/core.h"
#include <shared_mutex>
#include <algorithm>
#include <cmath>

namespace ChunkManager {
    static const int CHUNK_RADIUS = 10;
    // Unload well past the load radius so chunks near the boundary aren't
    // rebuilt and torn down every time the player nudges back and forth.
    static const int UNLOAD_RADIUS = CHUNK_RADIUS + 4;
    static const unsigned int NUM_WORKER_THREADS =
        std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1;

    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            return std::hash<T1>{}(p.first) ^ (std::hash<T2>{}(p.second) << 1);
        }
    };

    static ChunkRequestQueue chunk_queue;
    static std::shared_mutex chunks_mutex;
    static std::unordered_map<std::pair<int, int>, Chunk*, pair_hash> chunks;
    // Coordinates that have been queued but not yet inserted into `chunks`,
    // guarded by chunks_mutex. Prevents re-queueing the same chunk every time
    // update() re-scans the load radius while a build is still in flight.
    static std::unordered_set<std::pair<int, int>, pair_hash> pending;
    static std::vector<std::thread> worker_threads;
    static Frustum current_frustum;

    void ChunkRequestQueue::push(int x, int y, int z) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push({x, y, z});
        }
        cv_.notify_one();
    }

    bool ChunkRequestQueue::pop(int& x, int& y, int& z) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return shutdown_ || !queue_.empty(); });

        if (queue_.empty()) return false; // shutting down, nothing left to drain

        const ChunkRequest req = queue_.front();
        queue_.pop();
        x = req.x; y = req.y; z = req.z;
        return true;
    }

    void ChunkRequestQueue::shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all();
    }

    static void worker_loop() {
        int x, y, z;
        while (chunk_queue.pop(x, y, z)) {
            const auto coords = std::make_pair(x, z);

            {
                std::shared_lock<std::shared_mutex> read_lock(chunks_mutex);
                if (chunks.find(coords) != chunks.end()) continue;
            }

            Chunk* chunk = new Chunk(x, y, z);

            {
                std::unique_lock<std::shared_mutex> write_lock(chunks_mutex);
                pending.erase(coords);
                if (chunks.find(coords) == chunks.end()) {
                    chunks[coords] = chunk;
                } else {
                    delete chunk; // another request for the same coords won the race
                }
            }
        }
    }

    void build_chunk(int x, int y, int z) {
        const auto coords = std::make_pair(x, z);
        {
            std::unique_lock<std::shared_mutex> lock(chunks_mutex);
            if (chunks.count(coords) || pending.count(coords)) return;
            pending.insert(coords);
        }
        chunk_queue.push(x, y, z);
    }

    void chunk_square(int x, int z, int size) {
        for (int dx = -size / 2; dx < size / 2; ++dx) {
            for (int dz = -size / 2; dz < size / 2; ++dz) {
                build_chunk(x + dx, 0, z + dz);
            }
        }
    }

    void chunk_circle(int x, int z, int radius) {
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dz = -radius; dz <= radius; ++dz) {
                if (dx * dx + dz * dz <= radius * radius) {
                    build_chunk(x + dx, 0, z + dz);
                }
            }
        }
    }

    static void unload_distant_chunks(int player_x, int player_z) {
        std::unique_lock<std::shared_mutex> lock(chunks_mutex);
        for (auto it = chunks.begin(); it != chunks.end();) {
            const int dx = it->first.first - player_x;
            const int dz = it->first.second - player_z;
            if (dx * dx + dz * dz > UNLOAD_RADIUS * UNLOAD_RADIUS) {
                delete it->second;
                it = chunks.erase(it);
            } else {
                ++it;
            }
        }
    }

    void init() {
        worker_threads.reserve(NUM_WORKER_THREADS);
        for (unsigned int i = 0; i < NUM_WORKER_THREADS; ++i) {
            worker_threads.emplace_back(worker_loop);
        }

        chunk_circle(0, 0, CHUNK_RADIUS);
    }

    void update() {
        static bool has_last_position = false;
        static int last_player_x = 0, last_player_z = 0;

        const int player_x = static_cast<int>(std::floor(Core::camera.get_position().x / CHUNK_WIDTH));
        const int player_z = static_cast<int>(std::floor(Core::camera.get_position().z / CHUNK_DEPTH));

        if (!has_last_position || player_x != last_player_x || player_z != last_player_z) {
            has_last_position = true;
            last_player_x = player_x;
            last_player_z = player_z;

            chunk_circle(player_x, player_z, CHUNK_RADIUS);
            unload_distant_chunks(player_x, player_z);
        }
    }

    void render() {
        current_frustum.update(Core::camera.get_view(), Core::camera.get_projection());

        std::shared_lock<std::shared_mutex> lock(chunks_mutex);
        for (const auto& [coords, chunk] : chunks) {
            const float minX = coords.first * CHUNK_WIDTH;
            const float minZ = coords.second * CHUNK_DEPTH;
            const float maxX = minX + CHUNK_WIDTH;
            const float maxZ = minZ + CHUNK_DEPTH;
            const float minY = 0.0f;
            const float maxY = CHUNK_HEIGHT;

            if (current_frustum.is_box_visible(minX, minY, minZ, maxX, maxY, maxZ)) {
                chunk->submit_batch();
            }
        }
    }

    void destroy_all() {
        std::unique_lock<std::shared_mutex> lock(chunks_mutex);
        for (auto& [coords, chunk] : chunks) {
            delete chunk;
        }
        chunks.clear();
        pending.clear();
    }

    void cleanup() {
        chunk_queue.shutdown();

        for (auto& thread : worker_threads) {
            if (thread.joinable()) thread.join();
        }
        worker_threads.clear();

        destroy_all();
    }

    void serialize() {
        // Not yet implemented.
    }

    void deserialize() {
        // Not yet implemented.
    }
}
