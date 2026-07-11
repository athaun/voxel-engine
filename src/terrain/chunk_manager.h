#pragma once
#include "chunk.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>

namespace ChunkManager {
    // Thread-safe blocking queue of chunk-build requests. Multiple worker
    // threads pop from it concurrently (multi-producer, multi-consumer).
    class ChunkRequestQueue {
    public:
        void push(int x, int y, int z);

        // Blocks until a request is available or the queue is shut down.
        // Returns false only once shut down with no requests left to drain.
        bool pop(int& x, int& y, int& z);

        void shutdown();

    private:
        struct ChunkRequest {
            int x, y, z;
        };

        std::queue<ChunkRequest> queue_;
        std::mutex mutex_;
        std::condition_variable cv_;
        bool shutdown_ = false;
    };

    // Main interface functions
    void init();
    void update();
    void render();
    void cleanup();
    void chunk_circle(int x, int z, int radius);
    void chunk_square(int x, int z, int size);
    void build_chunk(int x, int y, int z);
    void serialize();
    void deserialize();
    void destroy_all();
}
