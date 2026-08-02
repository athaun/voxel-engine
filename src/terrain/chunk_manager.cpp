#include "chunk_manager.h"
#include "../render/frustum.h"
#include "../core/core.h"
#include <shared_mutex>
#include <algorithm>
#include <cmath>

namespace ChunkManager {
    // Unload well past the load radius so chunks near the boundary aren't
    // rebuilt and torn down every time the player nudges back and forth.
    static const int UNLOAD_RADIUS = CHUNK_RADIUS + 4;
    static const unsigned int NUM_WORKER_THREADS =
        std::thread::hardware_concurrency() > 1 ? std::min(8u, std::thread::hardware_concurrency() - 1) : 1;

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

    // Coordinates of chunks whose CPU-side mesh is ready but whose GPU
    // buffers haven't been created yet -- drained on the main thread by
    // process_upload_queue(). Storing coords (not Chunk*) and re-looking-up
    // through `chunks` at drain time means a chunk unloaded before its turn
    // to upload just quietly disappears from here instead of leaving a
    // dangling pointer.
    static std::mutex upload_queue_mutex;
    static std::queue<std::pair<int, int>> upload_queue;

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
                    continue;
                }
            }

            {
                std::lock_guard<std::mutex> lock(upload_queue_mutex);
                upload_queue.push(coords);
            }
        }
    }

    // Main-thread-only: creates GPU buffers for a small, bounded number of
    // chunks whose CPU-side mesh is already done, instead of every worker
    // thread creating its own GPU buffers the instant it finishes meshing.
    // bgfx resource creation goes through one global lock regardless of
    // which thread calls it, so doing it from N worker threads didn't add
    // throughput, just contention, right when several chunks finished
    // generating together (e.g. crossing into freshly-built terrain).
    static void process_upload_queue() {
        static constexpr int MAX_UPLOADS_PER_FRAME = 4;

        for (int i = 0; i < MAX_UPLOADS_PER_FRAME; ++i) {
            std::pair<int, int> coords;
            {
                std::lock_guard<std::mutex> lock(upload_queue_mutex);
                if (upload_queue.empty()) return;
                coords = upload_queue.front();
                upload_queue.pop();
            }

            std::shared_lock<std::shared_mutex> read_lock(chunks_mutex);
            auto it = chunks.find(coords);
            if (it != chunks.end()) {
                it->second->upload_to_gpu();
            }
        }
    }

    void build_chunk(int x, int y, int z) {
        const auto coords = std::make_pair(x, z);

        // Fast path: the overwhelming majority of build_chunk calls during
        // a chunk_circle sweep are for chunks that are already loaded or
        // already queued (the caller re-sweeps the whole circle on every
        // crossing, see chunk_circle). A shared (read) lock lets all of
        // those near-instant no-op checks run concurrently with each other
        // and with worker threads' own reads, instead of every single one
        // serializing on an exclusive lock.
        {
            std::shared_lock<std::shared_mutex> read_lock(chunks_mutex);
            if (chunks.count(coords) || pending.count(coords)) return;
        }

        std::unique_lock<std::shared_mutex> write_lock(chunks_mutex);
        if (chunks.count(coords) || pending.count(coords)) return; // lost a race since the check above
        pending.insert(coords);
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
        // The (dx,dz) offset pattern only depends on radius, which in
        // practice is always CHUNK_RADIUS -- but this function reruns on
        // every single chunk-boundary crossing (every ~32 units the player
        // moves), not just once at startup. Recomputing and re-sorting a
        // ~5000-entry list (an O(r^2 log r) sort) every crossing was a big
        // chunk of the "stutter the moment you leave the starting circle"
        // cost: inside the original circle every build_chunk call this
        // produces is already a no-op, so the redundant rescan was mostly
        // masked, but the moment new chunks actually need building, this
        // main-thread work lands in the same window as worker threads doing
        // real GPU resource creation. Cache it and only rebuild if the
        // radius actually changes. Safe as a plain (unsynchronized) static
        // because chunk_circle is only ever called from the main thread.
        static int cached_radius = -1;
        static std::vector<std::pair<int, int>> cached_offsets;

        if (cached_radius != radius) {
            cached_offsets.clear();
            cached_offsets.reserve(static_cast<size_t>(radius) * radius * 4);

            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dz = -radius; dz <= radius; ++dz) {
                    if (dx * dx + dz * dz <= radius * radius) {
                        cached_offsets.emplace_back(dx, dz);
                    }
                }
            }

            // Queue nearest-first so the worker pool visibly fills in as an
            // expanding ring around the player rather than in scan order.
            std::sort(cached_offsets.begin(), cached_offsets.end(), [](const auto& a, const auto& b) {
                return (a.first * a.first + a.second * a.second) < (b.first * b.first + b.second * b.second);
            });

            cached_radius = radius;
        }

        for (const auto& [dx, dz] : cached_offsets) {
            build_chunk(x + dx, 0, z + dz);
        }
    }

    static void unload_distant_chunks(int player_x, int player_z) {
        // Deleting a chunk destroys its GPU buffers synchronously on this
        // (main) thread, contending the same bgfx resource lock worker
        // threads use to build new chunks. At a large render distance, one
        // player-movement crossing can push dozens of chunks out of range
        // at once -- destroying all of them in a single call/frame while
        // holding the chunk map's exclusive lock is a real stutter source.
        // Capping how many get torn down per call spreads that cost across
        // the next several frames/crossings instead.
        static constexpr int MAX_UNLOADS_PER_CALL = 16;

        std::unique_lock<std::shared_mutex> lock(chunks_mutex);
        int unloaded = 0;
        for (auto it = chunks.begin(); it != chunks.end() && unloaded < MAX_UNLOADS_PER_CALL;) {
            const int dx = it->first.first - player_x;
            const int dz = it->first.second - player_z;
            if (dx * dx + dz * dz > UNLOAD_RADIUS * UNLOAD_RADIUS) {
                delete it->second;
                it = chunks.erase(it);
                ++unloaded;
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
        }

        // Runs every frame rather than only on a chunk-boundary crossing.
        // A single crossing can expose on the order of ~2*UNLOAD_RADIUS
        // newly-out-of-range chunks (the crescent a circle sweeps shifting
        // by one chunk-width scales with its radius) -- at this render
        // distance that comfortably outruns a small per-call throttle
        // during sustained travel, so the backlog never actually drains,
        // just grows with every further crossing. Running the same small
        // budget every frame instead gives it ~60 chances/sec to catch up,
        // which comfortably outpaces any realistic movement speed while
        // keeping each individual frame's GPU-teardown cost small.
        unload_distant_chunks(player_x, player_z);

        process_upload_queue();
    }

    void render() {
        current_frustum.update(Core::camera.get_view(), Core::camera.get_projection());

        std::vector<Chunk*> visible_chunks;
        {
            std::shared_lock<std::shared_mutex> lock(chunks_mutex);
            visible_chunks.reserve(chunks.size());
            for (const auto& [coords, chunk] : chunks) {
                const float minX = coords.first * CHUNK_WIDTH;
                const float minZ = coords.second * CHUNK_DEPTH;
                const float maxX = minX + CHUNK_WIDTH;
                const float maxZ = minZ + CHUNK_DEPTH;
                const float minY = 0.0f;
                const float maxY = CHUNK_HEIGHT;

                if (current_frustum.is_box_visible(minX, minY, minZ, maxX, maxY, maxZ)) {
                    visible_chunks.push_back(chunk);
                }
            }
        }

        // Two passes so transparent water always blends against fully
        // written opaque depth, without needing to sort individual water
        // quads across chunks.
        for (Chunk* chunk : visible_chunks) {
            chunk->submit_opaque_batch();
        }
        for (Chunk* chunk : visible_chunks) {
            chunk->submit_water_batch();
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
