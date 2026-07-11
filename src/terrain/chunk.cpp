#include "chunk.h"
#include "../core/Log.h"
#include "../noise/OpenSimplexNoise.h"
#include "../core/timer.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
    // Below this Y, terrain basins fill with water instead of sitting open
    // to air.
    constexpr int SEA_LEVEL = 20;
    // Peaks at or above this Y get a snow cap.
    constexpr int SNOW_LINE = 80;
    // How many layers below the surface still count as "near-surface" (dirt/
    // sand) before it's just deep stone.
    constexpr int SUBSURFACE_DEPTH = 4;
}

Chunk::Chunk(int x, int y, int z) : global_x(x * CHUNK_WIDTH), global_y(y * CHUNK_DEPTH), global_z(z * CHUNK_DEPTH) {
    // First pass: Generate terrain heights including buffer zone
    for (int x = 0; x < CHUNK_WIDTH + BUFFER_SIZE; x++) {
        for (int z = 0; z < CHUNK_DEPTH + BUFFER_SIZE; z++) {
            // Clamp the terrain height to CHUNK_HEIGHT
            int height = terrain(global_x + x - BUFFER_SIZE / 2, 0, global_z + z - BUFFER_SIZE / 2);
            terrain_heights[x][z] = std::clamp(height, 1, CHUNK_HEIGHT); // never let the terrain floor vanish entirely
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
                    // Above the surface: water fills basins below sea level,
                    // otherwise it's open air.
                    v.material = (y <= SEA_LEVEL) ? voxel::WATER : voxel::AIR;
                } else if (y > height - SUBSURFACE_DEPTH) {
                    // Surface and near-surface layers follow elevation, so a
                    // single slope naturally runs beach -> grass -> snow cap
                    // instead of the whole column being one material.
                    if (height <= SEA_LEVEL + 2 + random(0, 2)) {
                        v.material = voxel::SAND;
                    } else if (height >= SNOW_LINE + random(-3, 3)) {
                        v.material = (y == height) ? voxel::SNOW : voxel::STONE;
                    } else {
                        v.material = (y == height) ? voxel::GRASS : voxel::DIRT;
                    }
                } else {
                    v.material = voxel::STONE;
                }

                set_voxel(x, y, z, v);
            }
        }
    }

    // Third pass: scatter simple trees on grass. Kept away from the chunk
    // edges (by more than the canopy radius) so a canopy never needs to
    // write into a neighboring chunk we don't own.
    const int tree_margin = 3;
    for (int x = tree_margin; x < CHUNK_WIDTH - tree_margin; x++) {
        for (int z = tree_margin; z < CHUNK_DEPTH - tree_margin; z++) {
            int height = terrain_heights[x + BUFFER_SIZE / 2][z + BUFFER_SIZE / 2];

            bool is_sandy = height <= SEA_LEVEL + 4;
            bool is_snowy = height >= SNOW_LINE - 4;
            if (is_sandy || is_snowy || height >= CHUNK_HEIGHT - 8) continue;

            if (random(0, 999) >= 12) continue; // ~1.2% of eligible columns

            int trunk_height = random(4, 6);
            for (int dy = 1; dy <= trunk_height; dy++) {
                Voxel trunk;
                trunk.material = voxel::WOOD;
                set_voxel(x, height + dy, z, trunk);
            }

            int canopy_center = height + trunk_height;
            for (int dx = -2; dx <= 2; dx++) {
                for (int dz = -2; dz <= 2; dz++) {
                    for (int dy = -1; dy <= 2; dy++) {
                        if (dx == 0 && dz == 0 && dy <= 0) continue; // leave the trunk's top clear
                        if (dx * dx + dz * dz + dy * dy * 2 > 8) continue; // rounded canopy

                        int ly = canopy_center + dy;
                        if (ly >= CHUNK_HEIGHT) continue;

                        Voxel leaf;
                        leaf.material = voxel::LEAVES;
                        set_voxel(x + dx, ly, z + dz, leaf);
                    }
                }
            }
        }
    }

    // Fourth pass: greedy-mesh the chunk, then allocate the GPU batch at the
    // mesh's actual size instead of the worst-case (every voxel exposing
    // all 6 faces) bound the old path used.
    generate_greedy_mesh();
}

template <typename T>
T Chunk::random(T lower, T upper) {
    // std::rand()/std::srand() share global state that isn't thread-safe --
    // every chunk-worker thread called this per voxel, which is a data race
    // (and likely UB) now that many chunks build concurrently. A thread_local
    // engine gives each worker thread its own independent state.
    thread_local std::mt19937 engine{std::random_device{}()};
    std::uniform_int_distribution<T> dist(lower, upper);
    return dist(engine);
}

int Chunk::buffered_terrain_height(int x, int z) {
    static int offset = BUFFER_SIZE / 2;
    // calculate_ao's diagonal/corner checks reach 2 voxels out along a face's
    // normal axis (corner1 and corner2 each carry a redundant 1-step normal
    // offset), which can overrun this buffer's single column of padding at a
    // chunk edge. Clamp to the buffer's actual valid range instead of
    // indexing past it -- the clamped border height is as good an
    // approximation as any for terrain 2 voxels beyond data we actually have.
    int cx = std::clamp(x, -offset, CHUNK_WIDTH + offset - 1);
    int cz = std::clamp(z, -offset, CHUNK_DEPTH + offset - 1);
    return terrain_heights[cx + offset][cz + offset];
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
    const int detail_octaves = 5;
    const double persistence = 0.3;
    const double lacunarity = 2.0;

    // All generators below are built once (thread-safe static init) and
    // reused for every call -- constructing a fresh Noise per call rebuilds
    // its permutation table and was previously the dominant cost of chunk
    // generation. Noise::eval is const with no mutable state, so sharing
    // across chunk-worker threads is safe.
    static const std::array<OpenSimplexNoise::Noise, detail_octaves> detail_noise = {
        OpenSimplexNoise::Noise(0), OpenSimplexNoise::Noise(1), OpenSimplexNoise::Noise(2),
        OpenSimplexNoise::Noise(3), OpenSimplexNoise::Noise(4)
    };
    static const OpenSimplexNoise::Noise continent_noise(100);
    static const OpenSimplexNoise::Noise erosion_noise(200);
    static const std::array<OpenSimplexNoise::Noise, 4> ridge_noise = {
        OpenSimplexNoise::Noise(300), OpenSimplexNoise::Noise(301),
        OpenSimplexNoise::Noise(302), OpenSimplexNoise::Noise(303)
    };
    static const OpenSimplexNoise::Noise warp_noise_x(400);
    static const OpenSimplexNoise::Noise warp_noise_z(401);

    // Domain warp the sample point with a separate low-frequency noise field
    // so coastlines and ridgelines wind naturally instead of looking aligned
    // to the underlying noise grid.
    const double warp_scale = 0.006;
    const double warp_strength = 30.0;
    double wx = x + warp_noise_x.eval(x * warp_scale, z * warp_scale) * warp_strength;
    double wz = z + warp_noise_z.eval(x * warp_scale + 500.0, z * warp_scale - 500.0) * warp_strength;

    // Continentalness in [-1, 1]: negative is ocean, positive is inland.
    // Scaled ~4x smaller-wavelength than the original pass so oceans/coasts/
    // inland regions ("biomes") are noticeably smaller and change more often
    // as you walk around, instead of one region spanning the whole map.
    double continent = continent_noise.eval(wx * 0.0045, wz * 0.0045);

    // Erosion in [0, 1]: how much relief a region is allowed -- biased so
    // most terrain stays gently rolling and only patches get sharp and
    // craggy, rather than mountains everywhere.
    double erosion_raw = erosion_noise.eval(wx * 0.007, wz * 0.007);
    double erosion = std::pow((erosion_raw + 1.0) * 0.5, 1.6);

    // Ridged multifractal (1 - |noise|) carves actual mountain ridgelines
    // and valleys instead of smooth Perlin bumps.
    double ridge = 0.0, ridge_amp = 1.0, ridge_freq = 0.014, ridge_amp_sum = 0.0;
    for (auto& octave : ridge_noise) {
        ridge += (1.0 - std::abs(octave.eval(wx * ridge_freq, wz * ridge_freq))) * ridge_amp;
        ridge_amp_sum += ridge_amp;
        ridge_amp *= 0.5;
        ridge_freq *= 2.1;
    }
    ridge = ridge / ridge_amp_sum;
    ridge *= ridge; // sharpen peaks, widen valleys

    // Fine detail: small-scale roughness layered on top of the large-scale
    // shape above, rather than defining the whole terrain by itself.
    double noiseValue = 0.0, amplitude = 1.0, frequency = 1.0, maxAmplitude = 0.0;
    for (int i = 0; i < detail_octaves; ++i) {
        noiseValue += detail_noise[i].eval(wx * 0.015 * frequency, wz * 0.015 * frequency) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    double detail = noiseValue / maxAmplitude;

    const double OCEAN_FLOOR = 6.0;
    const double SHORE = 22.0;
    const double PLAINS = 42.0;
    const double MOUNTAIN_PEAK = 118.0;

    double base = continent < -0.15
        ? map(continent, -1.0, -0.15, OCEAN_FLOOR, SHORE)
        : map(continent, -0.15, 1.0, SHORE, PLAINS);

    // Mountains rise out of the plains where erosion allows it, shaped by
    // the ridged noise.
    double mountain = ridge * erosion * (MOUNTAIN_PEAK - PLAINS);

    double height = base + mountain + detail * 4.0;

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