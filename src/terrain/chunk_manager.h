#pragma once

#include "chunk.h"
#include <unordered_map>

namespace ChunkManager {
    void init();
    void update();
    void render(const bx::Vec3& lightDir);
    void destroy();

    void chunk_circle(int x, int z, int radius);
    void build_chunk(int x, int y, int z);

    void serialize();
    void deserialize();
}