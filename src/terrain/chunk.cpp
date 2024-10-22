#include "chunk.h"
#include "../core/Log.h"
#include "../noise/OpenSimplexNoise.h"
#include <iostream>

Chunk::Chunk(int x, int z) : global_x(x * CHUNK_WIDTH), global_z(z * CHUNK_DEPTH) {
    int chunk_volume = CHUNK_WIDTH * CHUNK_DEPTH * CHUNK_HEIGHT;
    int vertex_count = chunk_volume * 8;
    int index_count = chunk_volume * 36;
    batch = new Render::Batch(vertex_count, index_count, "cubes");

    int used_volume = 0;

    uint8_t used_faces = 0b00000000;
    float spacing = 1.0f;

    int terrain_heights[CHUNK_WIDTH][CHUNK_DEPTH] = {};

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            int terrain_height = terrain(global_x + x, 0, global_z + z);
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                voxels[x][z][y].material = (terrain_height > y ? voxel::Material::STONE : voxel::Material::AIR);
            }
            used_volume += terrain_height;
            terrain_heights[x][z] = terrain_height;
        }
    } 

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            for (int y = 0; y < CHUNK_HEIGHT && y < terrain_heights[x][z]; y++) {
                used_faces = get_visible_faces(x, y, z);
                
                Render::Mesh c = Render::cube(used_faces);

                if (used_faces != 0) {
                    batch->push_mesh(Render::transform_mesh(c, global_x + x * spacing, y, global_z + z * spacing));
                }
            }
        }
    }

    // Create a gizmo to visualize x, y, z directions
    int gizmo_size = 10;
    int gizmo_height = CHUNK_HEIGHT + 1; // Spawn above the chunk height

    // X direction (red)
    for (int i = 0; i < gizmo_size; ++i) {
        batch->push_mesh(Render::transform_mesh(Render::colored_cube(0xFF0000FF), i, gizmo_height, 0)); // Red for X direction
    }

    // Y direction (blue)
    for (int i = 0; i < gizmo_size; ++i) {
        batch->push_mesh(Render::transform_mesh(Render::colored_cube(0x00FF0000), 0, gizmo_height + i, 0)); // Blue for Y direction
    }

    // Z direction (green)
    for (int i = 0; i < gizmo_size; ++i) {
        batch->push_mesh(Render::transform_mesh(Render::colored_cube(0xFF00FF00), 0, gizmo_height, i)); // Green for Z direction
    }

    Log::info("Created chunk at (" + std::to_string(x) + ", " + std::to_string(z) + ")" + " of volume " + std::to_string(used_volume));
}

bool neighbor = false;
bool Chunk::is_empty(int x, int y, int z) {
    if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH) {
        return true;
    }
    return voxels[x][z][y].material == voxel::Material::AIR;
}

uint8_t Chunk::get_visible_faces(int x, int y, int z) {
    uint8_t used_faces = 0b00000000;

    if (is_empty(x, y, z + 1)) {
        used_faces |= (1 << 0); // Front face
        if (z != CHUNK_DEPTH - 1) {
            Log::info("Front face neighbor at\t(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z + 1) + ") is empty");
            neighbor = true;
        }
    }

    if (is_empty(x + 1, y, z)) used_faces |= (1 << 1); // Right face
    
    if (is_empty(x, y, z - 1)) {
        used_faces |= (1 << 2); // Back face
        // if (z != 0) {
        //     Log::info("Back face neighbor at\t(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z - 1) + ") is empty");
        //     neighbor = true;
        // }
    }
    
    if (is_empty(x - 1, y, z)) used_faces |= (1 << 3); // Left face
    if (is_empty(x, y + 1, z)) used_faces |= (1 << 4); // Top face
    if (is_empty(x, y - 1, z)) used_faces |= (1 << 5); // Bottom face

    if (neighbor) {
        Log::info("Faces at\t\t\t(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + "): " +
              "\n0 : Front (Z+):\t" + std::to_string((used_faces & (1 << 0)) != 0) + ", " +
              "\n1 : Right (X+):\t" + std::to_string((used_faces & (1 << 1)) != 0) + ", " +
              "\n2 : Back (Z-):\t" + std::to_string((used_faces & (1 << 2)) != 0) + ", " +
              "\n3 : Left (X-):\t" + std::to_string((used_faces & (1 << 3)) != 0) + ", " +
              "\n4 : Top (Y+):\t" + std::to_string((used_faces & (1 << 4)) != 0) + ", " +
              "\n5 : Bot (Y-):\t" + std::to_string((used_faces & (1 << 5)) != 0) + "\n");
        neighbor = false;
    }

    return used_faces;
}

template <typename T>
T map(T value, T start1, T stop1, T start2, T stop2) {
    return start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
}

double Chunk::terrain(int x, int y, int z) {
    // Define noise parameters
    int octaves = 5;   // Number of noise layers
    double persistence = 0.3;  // How much each octave contributes to the overall noise
    double lacunarity = 10.0;   // How much the frequency increases per octave

    double noiseValue = 0.0;
    double amplitude = 1.2;
    double frequency = 0.9;
    double maxValue = 0.0;

    for (int i = 0; i < octaves; ++i) {

        // Create a new instance of the noise generator with a new seed
        OpenSimplexNoise::Noise simplex(i); // Adjust the seed

        // Calculate the noise value for the current octave
        noiseValue += simplex.eval(x * 0.01 * frequency, z * 0.01 * frequency) * amplitude;
        maxValue += amplitude;

        // Update amplitude and frequency for the next octave
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return (double)map((double)noiseValue, -1.0, 0.0, 0.0, (double)CHUNK_HEIGHT);
}


Chunk::~Chunk() {
    delete batch;
}

void Chunk::submit_batch() {
    batch->submit();
}

Voxel Chunk::get_voxel(int x, int y, int z) const {
    return voxels[x][z][y];
}

void Chunk::set_voxel(int x, int y, int z, Voxel voxel) {
    voxels[x][z][y] = voxel;
}