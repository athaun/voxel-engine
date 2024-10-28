#pragma once

#include <cstdint>

namespace voxel {
    enum Material : uint8_t {
        AIR,
        STONE,
        DIRT,
        GRASS,
        WATER,
        SAND,
        WOOD,
        LEAVES,
        GLASS,
    };
}
struct Voxel {
    voxel::Material material;
};