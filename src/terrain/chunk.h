#pragma once

#include <array>
#include <cstdint>

#include "../render/batch.h"
#include "voxel.h"

const int CHUNK_WIDTH = 32;
const int CHUNK_DEPTH = 32;
const int CHUNK_HEIGHT = 32;

class Chunk {
public:
    Chunk(int x, int z);
    ~Chunk();

    Render::Batch& get_batch();
    void submit_batch();

    Voxel get_voxel(int x, int y, int z) const;
    void set_voxel(int x, int y, int z, Voxel voxel);

private:
    double terrain(int x, int y, int z);
    bool is_empty(int x, int y, int z);
    uint8_t get_visible_faces(int x, int y, int z);
    voxel::Material get_material(int x, int y, int z);

    int global_x, global_z;
    std::array<std::array<std::array<Voxel, CHUNK_HEIGHT>, CHUNK_DEPTH>, CHUNK_WIDTH> voxels;
    Render::Batch* batch;
};