#pragma once

#include <array>
#include <cstdint>

#include "../render/batch.h"
#include "voxel.h"

const int CHUNK_WIDTH = 16;
const int CHUNK_DEPTH = 16;
const int CHUNK_HEIGHT = 32;

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

    Voxel get_voxel(int x, int y, int z);
    void set_voxel(int x, int y, int z, Voxel voxel);
    std::pair<int, int> get_position();
    void set_neighbor(int direction, Chunk* neighbor);

private:
    // Add terrain height buffer to store terrain height information about neighboring blocks outside chunk boundaries.
    static const int BUFFER_SIZE = 2;
    int terrain_heights[CHUNK_WIDTH + BUFFER_SIZE][CHUNK_DEPTH + BUFFER_SIZE];

    size_t voxel_index(int x, int y, int z);
    double terrain(int x, int y, int z);
    bool is_empty(int x, int y, int z);
    uint8_t get_visible_faces(int x, int y, int z);
    voxel::Material get_material(int x, int y, int z);
    float calculate_ao(int x, int y, int z, int corner1_x, int corner1_y, int corner1_z, 
                       int corner2_x, int corner2_y, int corner2_z);
    void calculate_face_ao(int x, int y, int z, uint8_t face, float ao_values[4]);
    
    int global_x, global_y, global_z;
    std::array<Voxel, CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH> voxels;
    Render::Batch* batch;
};