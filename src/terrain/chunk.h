#pragma once

#include <array>
#include <cstdint>

#include "../render/batch.h"
#include "voxel.h"

const int CHUNK_WIDTH = 20;
const int CHUNK_DEPTH = 20;
const int CHUNK_HEIGHT = 20;

class Chunk {
public:
    Chunk(int x, int y, int z);
    ~Chunk();

    Render::Batch& get_batch();
    void submit_batch();

    Voxel get_voxel(int x, int y, int z) const;
    void set_voxel(int x, int y, int z, Voxel voxel);
    std::pair<int, int> get_position() const;

private:
    double terrain(int x, int y, int z);
    bool is_empty(int x, int y, int z);
    uint8_t get_visible_faces(int x, int y, int z);
    voxel::Material get_material(int x, int y, int z);

    int global_x, global_y, global_z;
    std::array<std::array<std::array<Voxel, CHUNK_HEIGHT>, CHUNK_DEPTH>, CHUNK_WIDTH> voxels;
    Render::Batch* batch;
};