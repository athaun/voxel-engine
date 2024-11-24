#pragma once

#include <cstdint>

namespace voxel {
    struct MaterialData {
        float r, g, b;
    };

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

    // Material color definitions
    static const MaterialData MATERIAL_COLORS[] = {
        {0.0f, 0.0f, 0.0f},     // AIR
        {0.5f, 0.5f, 0.5f},     // STONE
        {0.6f, 0.4f, 0.2f},     // DIRT
        {0.3f, 0.8f, 0.2f},     // GRASS
        {0.2f, 0.3f, 0.9f},     // WATER
        {0.9f, 0.9f, 0.7f},     // SAND
        {0.6f, 0.4f, 0.2f},     // WOOD
        {0.2f, 0.6f, 0.2f},     // LEAVES
        {0.9f, 0.9f, 1.0f},     // GLASS
    };
}

struct Voxel {
    voxel::Material material;
};