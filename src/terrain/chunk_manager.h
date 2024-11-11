#pragma once

#include "chunk.h"
#include <unordered_map>

namespace ChunkManager {
    void init();
    void update();
    void render();
    void destroy();

    void chunk_circle(int x, int z, int radius);
    void chunk_square(int x, int z, int size);
    void build_chunk(int x, int y, int z);

    void serialize();
    void deserialize();
}