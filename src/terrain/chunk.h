#pragma once

#include <array>
#include <cstdint>

#include "../render/batch.h"
#include "voxel.h"

const int CHUNK_WIDTH = 10;
const int CHUNK_DEPTH = 10;
const int CHUNK_HEIGHT = 10;

class Chunk {

// Add references/pointers to neighboring chunks
Chunk* neighbors[6] = {nullptr}; // front, right, back, left, top, bottom

// Helper to check voxels across chunk boundaries
bool is_empty_with_neighbors(int x, int y, int z);

public:
    Chunk(int x, int y, int z);
    ~Chunk();

    Render::Batch& get_batch();
    void submit_batch();

    Voxel get_voxel(int x, int y, int z) const;
    void set_voxel(int x, int y, int z, Voxel voxel);
    std::pair<int, int> get_position() const;
    void set_neighbor(int direction, Chunk* neighbor) {
    neighbors[direction] = neighbor;
    }

private:
    double terrain(int x, int y, int z);
    bool is_empty(int x, int y, int z);
    uint8_t get_visible_faces(int x, int y, int z);
    voxel::Material get_material(int x, int y, int z);
    float calculate_ao(int x, int y, int z, int corner1_x, int corner1_y, int corner1_z, 
                       int corner2_x, int corner2_y, int corner2_z);
    void calculate_face_ao(int x, int y, int z, uint8_t face, float ao_values[4]);
    
    int global_x, global_y, global_z;
    std::array<std::array<std::array<Voxel, CHUNK_HEIGHT>, CHUNK_DEPTH>, CHUNK_WIDTH> voxels;
    Render::Batch* batch;
};