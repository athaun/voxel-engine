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

    Render::Mesh cube = Render::cube(0.5f);
    float spacing = 1.0f;

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {

            int terrain_height = terrain(global_x + x, 0, global_z + z);
            used_volume += terrain_height;

            for (int y = 0; y < CHUNK_HEIGHT && y < terrain_height; y++) {
                voxels[x][z][y] = {voxel::Material::STONE};
                Render::Mesh c(cube);
                batch->push_mesh(Render::transform_mesh(c, global_x + x * spacing, y, global_z + z * spacing));
            }
            for (int y = terrain_height; y < CHUNK_HEIGHT; y++) {
                voxels[x][z][y] = {voxel::Material::AIR};
            }
        }
    }

    Log::info("Created chunk at (" + std::to_string(x) + ", " + std::to_string(z) + ")" + " of volume " + std::to_string(used_volume));
}

template <typename T>
T map(T value, T start1, T stop1, T start2, T stop2) {
    return start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
}

double Chunk::terrain(int x, int y, int z) {
    // Define noise parameters
    int octaves = 5;   // Number of noise layers
    double persistence = 0.3;  // How much each octave contributes to the overall noise
    double lacunarity = 2.0;   // How much the frequency increases per octave

    double noiseValue = 0.0;
    double amplitude = 1.2;
    double frequency = 0.4;
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