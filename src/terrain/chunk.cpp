#include "chunk.h"
#include "../core/Log.h"
#include "../noise/OpenSimplexNoise.h"
#include "../core/timer.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>

Chunk::Chunk(int x, int y, int z) : global_x(x * CHUNK_WIDTH), global_y(y * CHUNK_DEPTH), global_z(z * CHUNK_DEPTH) {
    // First pass: Generate terrain heights including buffer zone
    for (int x = 0; x < CHUNK_WIDTH + BUFFER_SIZE; x++) {
        for (int z = 0; z < CHUNK_DEPTH + BUFFER_SIZE; z++) {
            // Clamp the terrain height to CHUNK_HEIGHT
            int height = terrain(global_x + x - BUFFER_SIZE / 2, 0, global_z + z - BUFFER_SIZE / 2);
            terrain_heights[x][z] = std::clamp(height, 5, CHUNK_HEIGHT); // 5 is sea-level
        }
    }

    // Second pass: Generate voxels for the actual chunk (without buffer)
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            // Use the buffered terrain height array (offset by BUFFER_SIZE / 2)
            int height = terrain_heights[x + BUFFER_SIZE / 2][z + BUFFER_SIZE / 2];

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Voxel v;

                if (y > height) {
                    v.material = voxel::AIR;
                } else if (y > height - 1) {
                    v.material = voxel::GRASS;
                } else if (y >= height - 2) {
                    v.material = voxel::DIRT;
                } else {
                    v.material = voxel::STONE;
                }

                if (height < 12 + random(0, 2)) {
                    v.material = voxel::SAND;
                } else if (height > CHUNK_HEIGHT - 20 + random(-2, 4)) {
                    v.material = voxel::SNOW;
                } else if (height > CHUNK_HEIGHT - 30 + random(0, 3)) {
                    v.material = voxel::STONE;
                }

                if (height == 5) {
                    v.material = voxel::WATER;
                }

                set_voxel(x, y, z, v);
            }
        }
    }

    // Third pass: greedy-mesh the chunk, then allocate the GPU batch at the
    // mesh's actual size instead of the worst-case (every voxel exposing
    // all 6 faces) bound the old path used.
    generate_greedy_mesh();
}

template <typename T>
T Chunk::random(T lower, T upper) {
    // Seed the random number generator once
    static bool seeded = false;
    if (!seeded) {
        std::srand(std::time(0));  // Seed the random number generator with current time
        seeded = true;
    }

    // Generate a random number in the range [lower, upper]
    return lower + (std::rand() % (upper - lower + 1));
}

inline int Chunk::buffered_terrain_height(int x, int z) {
    static int offset = BUFFER_SIZE / 2;
    return terrain_heights[x + offset][z + offset];
}

inline bool Chunk::is_above_terrain(int x, int y, int z) {
    return y > std::min(buffered_terrain_height(x, z), CHUNK_HEIGHT - 1);
}

float Chunk::calculate_ao(int x, int y, int z, int corner1_x, int corner1_y, int corner1_z, 
                          int corner2_x, int corner2_y, int corner2_z) {
    // Check neighboring blocks
    float side1 = !is_above_terrain(x + corner1_x, y + corner1_y, z + corner1_z) ? 0.20f : 0.0f;
    float side2 = !is_above_terrain(x + corner2_x, y + corner2_y, z + corner2_z) ? 0.20f : 0.0f;
    
    // Check corner block
    float corner = !is_above_terrain(x + corner1_x + corner2_x, 
                            y + corner1_y + corner2_y, 
                            z + corner1_z + corner2_z) ? 0.05f : 0.0f;
    // float side1 = !is_above_terrain(x + corner1_x, y + corner1_y, z + corner1_z) ? 0.25f : 0.0f;
    // float side2 = !is_above_terrain(x + corner2_x, y + corner2_y, z + corner2_z) ? 0.25f : 0.0f;
    
    // // Modify corner calculation to be less aggressive
    // float corner = !is_above_terrain(x + corner1_x + corner2_x, 

    // Check diagonal-lower block
    float diagonal_lower = !is_above_terrain(x + corner1_x + corner2_x, 
                                   y + corner1_y + corner2_y - 1, 
                                   z + corner1_z + corner2_z) ? 0.18f : 0.0f;

    // Calculate total occlusion
    float occlusion = 0.0f;
    
    // If both sides are present, maximum occlusion
    if (side1 > 0.0f && side2 > 0.0f) {
        occlusion = side1 + side2;
    }
    
    /////////
    // Programming note: This is a bit of a mess, especially with all the scaling on the diagonal_lower value, but it's the smoothest I could make it without having something look too dark.
    /////////

    // If one side is present, add its occlusion plus partial corner/diagonal influence
    else if (side1 > 0.0f || side2 > 0.0f) {
        occlusion = (std::max(side1, side2)* 0.7) + (corner * 0.5);
        // Only add diagonal if corner is empty
        if (corner == 0.0f) {
            occlusion += diagonal_lower * 0.4f;
        }
    }
    // If no sides, use corner and diagonal
    else {
        occlusion = corner;
        if (corner == 0.0f) {
            occlusion += diagonal_lower * 0.8;
        }
    }

    return 1.0f - std::min(1.0f, occlusion);
}

void Chunk::calculate_face_ao(int x, int y, int z, uint8_t face, float raw_values[4]) {
    /*
        CONTEXT:
        For a front face (+Z), looking at the face:
        [0]----[1]    [0] = top-left vertex
        |      |     [1] = top-right vertex
        |      |     [2] = bottom-left vertex
        [2]----[3]    [3] = bottom-right vertex
            
                      
            v4 ----------- v5  +y
            /|            /|   |
        / |           / |   |
        v0 --+--------- v1 |   | 
        |   |    +    |   |   |
        |  v6 --------+--v7   +x
        | /           | /    /
        |/            |/    /
        v2 ------------v3   +z

        '+' is (0,0,0),the origin point

        The parameters in calculate_ao(x, y, z, -1, 0, 1, 0, 1, 1) mean:
        - (-1, 0, 1) = offset to check left adjacent block (C)
        FROM THE ORIGIN: Go left on x, stay on y, go forward on z
        This will give us the side from v0 to v2
        - (0, 1, 1) = offset to check top adjacent block (B)
        FROM THE ORIGIN: Stay on x, go up on y, go forward on z
        This will give us the side from v0 to v1

        Now we have our two sides for the top-left vertex, and can calculate the corner block (A)
        This is the same process for each vertex on each face that exists.
    */

    switch(face) {
        case 0: // Front face (+Z) - this one looks correct
            raw_values[0] = calculate_ao(x, y, z, -1, 0, 1, 0, 1, 1);  // top-left
            raw_values[1] = calculate_ao(x, y, z, 1, 0, 1, 0, 1, 1);   // top-right
            raw_values[2] = calculate_ao(x, y, z, -1, 0, 1, 0, -1, 1); // bottom-left
            raw_values[3] = calculate_ao(x, y, z, 1, 0, 1, 0, -1, 1);  // bottom-right
            break;
            
        case 1: // Right face (+X)
            raw_values[0] = calculate_ao(x, y, z, 1, 0, 1, 1, 1, 0);  // top-left (when looking at right face)
            raw_values[1] = calculate_ao(x, y, z, 1, 0, -1, 1, 1, 0);   // top-right
            raw_values[2] = calculate_ao(x, y, z, 1, 0, 1, 1, -1, 0); // bottom-left
            raw_values[3] = calculate_ao(x, y, z, 1, 0, -1, 1, -1, 0);  // bottom-right
            break;
            
        case 2: // Back face (-Z)
            raw_values[0] = calculate_ao(x, y, z, 1, 0, -1, 0, 1, -1);   // top-left
            raw_values[1] = calculate_ao(x, y, z, -1, 0, -1, 0, 1, -1);  // top-right
            raw_values[2] = calculate_ao(x, y, z, 1, 0, -1, 0, -1, -1);  // bottom-left
            raw_values[3] = calculate_ao(x, y, z, -1, 0, -1, 0, -1, -1); // bottom-right
            break;
            
        case 3: // Left face (-X)
            raw_values[0] = calculate_ao(x, y, z, -1, 0, -1, -1, 1, 0);  // top-left
            raw_values[1] = calculate_ao(x, y, z, -1, 0, 1, -1, 1, 0);   // top-right
            raw_values[2] = calculate_ao(x, y, z, -1, 0, -1, -1, -1, 0); // bottom-left
            raw_values[3] = calculate_ao(x, y, z, -1, 0, 1, -1, -1, 0); // bottom-right
            break;
            
        case 4: // Top face (+Y)
            raw_values[0] = calculate_ao(x, y, z, -1, 1, 0, 0, 1, -1);  // top-left
            raw_values[1] = calculate_ao(x, y, z, 1, 1, 0, 0, 1, -1);   // top-right
            raw_values[2] = calculate_ao(x, y, z, -1, 1, 0, 0, 1, 1);   // bottom-left
            raw_values[3] = calculate_ao(x, y, z, 1, 1, 0, 0, 1, 1);    // bottom-right
            break;
            
        case 5: // Bottom face (-Y)
            raw_values[0] = calculate_ao(x, y, z, -1, -1, 0, 0, -1, -1); // top-left
            raw_values[1] = calculate_ao(x, y, z, 1, -1, 0, 0, -1, -1);  // top-right
            raw_values[2] = calculate_ao(x, y, z, -1, -1, 0, 0, -1, 1);  // bottom-left
            raw_values[3] = calculate_ao(x, y, z, 1, -1, 0, 0, -1, 1);   // bottom-right
            break;
    }
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

    double height = (double)map(noiseValue, -1.0, 1.0, 5.0, (double)CHUNK_HEIGHT - 5);
    height = height * height / (CHUNK_HEIGHT);

    return height;
}


Chunk::~Chunk() {
    delete batch;
}

void Chunk::submit_batch() {
    batch->submit();
}

Voxel Chunk::get_voxel(int x, int y, int z) {
    return voxels[voxel_index(x, y, z)];
}

void Chunk::set_voxel(int x, int y, int z, Voxel voxel) {
    voxels[voxel_index(x, y, z)] = voxel;
}

inline size_t Chunk::voxel_index(int x, int y, int z) {
    size_t x_i = x % CHUNK_WIDTH;
    size_t y_i = y % CHUNK_HEIGHT;
    size_t z_i = z % CHUNK_DEPTH;

    return x_i + (y_i * CHUNK_WIDTH) + (z_i * CHUNK_WIDTH * CHUNK_HEIGHT);
}

std::pair<int, int> Chunk::get_position() {
    return std::make_pair(global_x, global_z);
}

void Chunk::set_neighbor(int direction, Chunk* neighbor) {
    neighbors[direction] = neighbor;
}

// double Chunk::water_noise(int x, int z) {
//     static OpenSimplexNoise::Noise noise(42); // Different seed than terrain
//     double scale = 0.02;
//     return noise.eval(x * scale, z * scale);
// }