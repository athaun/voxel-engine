#pragma once

#include <array>
#include <cstdint>

#include "../render/batch.h"
#include "voxel.h"

const int CHUNK_WIDTH = 32;
const int CHUNK_DEPTH = 32;
const int CHUNK_HEIGHT = 128;

struct Face {
    int x, y, z;       // Starting position
    int width, height; // Dimensions of the merged face
    uint8_t direction; // Face direction (0-5)
    float ao[4];      // Ambient occlusion values for corners

    Face(int x, int y, int z, uint8_t dir) : x(x), y(y), z(z), direction(dir) {
        width = 1;
        height = 1;
        memset(ao, 0, sizeof(ao));
    }

    void extend(int deltaX, int deltaY, int deltaZ) {
        if (deltaX != 0) width += deltaX;
        if (deltaY != 0) height += deltaY;
    }
};

// Direction vectors for each face
static const int DIRS[6][3] = {
    {0, 0, 1},  // Front (+Z)
    {1, 0, 0},  // Right (+X)
    {0, 0, -1}, // Back (-Z)
    {-1, 0, 0}, // Left (-X)
    {0, 1, 0},  // Top (+Y)
    {0, -1, 0}  // Bottom (-Y)
};

class Chunk {

// Add references/pointers to neighboring chunks
Chunk* neighbors[6] = {nullptr}; // front, right, back, left, top, bottom

public:
    Chunk(int x, int y, int z);
    ~Chunk();

    Render::Batch& get_batch();
    void submit_batch();

    Voxel get_voxel(int x, int y, int z);
    void set_voxel(int x, int y, int z, Voxel voxel);
    std::pair<int, int> get_position();
    void set_neighbor(int direction, Chunk* neighbor);

    bool is_visible;
private:
    // Add terrain height buffer to store terrain height information about neighboring blocks outside chunk boundaries.
    static const int BUFFER_SIZE = 2;
    int terrain_heights[CHUNK_WIDTH + BUFFER_SIZE][CHUNK_DEPTH + BUFFER_SIZE];

    // // Stuff for water :P
    // static constexpr int SEA_LEVEL = 0;
    // double water_noise(int x, int z); // New noise function for water distribution
    template <typename T>
    T random(T lower, T upper);

    size_t voxel_index(int x, int y, int z);
    int buffered_terrain_height(int x, int z);

    double terrain(int x, int y, int z);
    bool is_above_terrain(int x, int y, int z);

    float calculate_ao(int x, int y, int z, int corner1_x, int corner1_y, int corner1_z,
                       int corner2_x, int corner2_y, int corner2_z);
    void calculate_face_ao(int x, int y, int z, uint8_t face, float ao_values[4]);

    // Binary greedy mesher: occupancy is packed into per-column bitmasks
    // (see bitcolumn.h) so face exposure for a whole column is a handful of
    // word ops instead of one comparison per voxel, then exposed faces are
    // greedily merged into maximal same-material rectangles.
    void generate_greedy_mesh();
    bool is_solid_for_mesh(int x, int y, int z);
    void mesh_direction(uint8_t direction, std::vector<Face>& faces);
    void calculate_merged_face_ao(int w0, int h0, int w1, int h1, int normal, uint8_t direction, float ao_out[4]);
    Render::Mesh generate_mesh_from_faces(const std::vector<Face>& faces);

    int global_x, global_y, global_z;
    std::array<Voxel, CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH> voxels;
    Render::Batch* batch;
};