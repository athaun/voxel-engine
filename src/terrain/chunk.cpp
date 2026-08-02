#include "chunk.h"
#include "../core/Log.h"
#include "../noise/OpenSimplexNoise.h"
#include "../core/timer.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
    // Peaks at or above this Y get a snow cap.
    constexpr int SNOW_LINE = 80;
    // How many layers below the surface still count as "near-surface" (dirt/
    // sand) before it's just deep stone.
    constexpr int SUBSURFACE_DEPTH = 4;
    // Aridity above this counts as desert (mesa terrain + sand cover).
    constexpr double DESERT_THRESHOLD = 0.35;
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
            bool is_desert = aridity(global_x + x, global_z + z) > DESERT_THRESHOLD;

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Voxel v;

                if (y > height) {
                    // Above the surface: water fills basins below sea level,
                    // otherwise it's open air.
                    v.material = (y <= SEA_LEVEL) ? voxel::WATER : voxel::AIR;
                } else if (y > height - SUBSURFACE_DEPTH) {
                    // Surface and near-surface layers follow elevation, so a
                    // single slope naturally runs beach -> grass -> snow cap
                    // instead of the whole column being one material. Desert
                    // mesas stay sandy regardless of elevation (that's what
                    // makes them read as desert instead of a snowy plateau).
                    if (is_desert || height <= SEA_LEVEL + 2 + random(0, 2)) {
                        v.material = voxel::SAND;
                    } else if (height >= SNOW_LINE + random(-3, 3)) {
                        v.material = (y == height) ? voxel::SNOW : voxel::STONE;
                    } else if (height >= SNOW_LINE - 4 + random(-3, 3)) {
                        v.material = voxel::STONE;
                    } else {
                        v.material = (y == height) ? voxel::GRASS : voxel::DIRT;
                    }
                } else {
                    // Deep underground -- also what shows on a mesa's sheer
                    // terrace walls once they drop more than SUBSURFACE_DEPTH,
                    // giving the classic sandy-cap-over-bare-rock look.
                    v.material = voxel::STONE;
                }

                set_voxel(x, y, z, v);
            }
        }
    }

    // Third pass: scatter varied trees on grass -- oak (round canopy), pine
    // (tall and conical), and bush (short and wide) -- instead of one
    // identical shape everywhere. Kept away from the chunk edges (by more
    // than the largest canopy radius) so a canopy never needs to write into
    // a neighboring chunk we don't own.
    static const OpenSimplexNoise::Noise forest_noise(700);
    const int tree_margin = 4;
    for (int x = tree_margin; x < CHUNK_WIDTH - tree_margin; x++) {
        for (int z = tree_margin; z < CHUNK_DEPTH - tree_margin; z++) {
            int height = terrain_heights[x + BUFFER_SIZE / 2][z + BUFFER_SIZE / 2];
            int wx = global_x + x, wz = global_z + z;

            bool is_sandy = height <= SEA_LEVEL + 4;
            bool is_snowy = height >= SNOW_LINE - 4;
            bool is_desert = aridity(wx, wz) > DESERT_THRESHOLD;
            if (is_sandy || is_snowy || is_desert || height >= CHUNK_HEIGHT - 12) continue;

            // Forests are patches, not an even scatter: outside a forest
            // region there are no trees at all (real open plains), and
            // density ramps up toward the middle of a forest instead of
            // being uniform everywhere.
            double forest = forest_noise.eval(wx * 0.004, wz * 0.004); // [-1,1]
            if (forest < 0.15) continue;
            double forest_t = std::clamp((forest - 0.15) / 0.5, 0.0, 1.0);
            int chance = static_cast<int>(4 + forest_t * 40); // 0.4% at the edge, 4.4% at the core

            if (random(0, 999) >= chance) continue;

            auto place_leaf = [&](int lx, int ly, int lz) {
                if (ly < 0 || ly >= CHUNK_HEIGHT) return;
                Voxel leaf;
                leaf.material = voxel::LEAVES;
                set_voxel(lx, ly, lz, leaf);
            };
            auto place_trunk = [&](int ty) {
                if (ty < 0 || ty >= CHUNK_HEIGHT) return;
                Voxel trunk;
                trunk.material = voxel::WOOD;
                set_voxel(x, ty, z, trunk);
            };

            int tree_type = random(0, 9); // 0-5 oak, 6-8 pine, 9 bush

            if (tree_type <= 5) {
                // Oak: round, slightly ragged canopy on a medium trunk.
                int trunk_height = random(4, 6);
                for (int dy = 1; dy <= trunk_height; dy++) place_trunk(height + dy);

                int radius = random(2, 3);
                int canopy_center = height + trunk_height;
                for (int dx = -radius; dx <= radius; dx++) {
                    for (int dz = -radius; dz <= radius; dz++) {
                        for (int dy = -1; dy <= radius; dy++) {
                            if (dx == 0 && dz == 0 && dy <= 0) continue; // leave the trunk's top clear
                            int dist2 = dx * dx + dz * dz + dy * dy * 2;
                            if (dist2 > radius * radius + 2) continue;
                            if (dist2 > radius * radius - 1 && random(0, 3) == 0) continue; // ragged edge
                            place_leaf(x + dx, canopy_center + dy, z + dz);
                        }
                    }
                }
            } else if (tree_type <= 8) {
                // Pine: tall trunk, stacked rings narrowing toward the top.
                int trunk_height = random(7, 11);
                for (int dy = 1; dy <= trunk_height; dy++) place_trunk(height + dy);

                int layers = random(4, 6);
                int top = height + trunk_height;
                for (int layer = 0; layer < layers; layer++) {
                    int ly = top - layers + layer + 1;
                    int r = std::max(1, (layers - layer) / 2 + 1);
                    for (int dx = -r; dx <= r; dx++) {
                        for (int dz = -r; dz <= r; dz++) {
                            if (dx == 0 && dz == 0) continue; // trunk column stays clear
                            if (dx * dx + dz * dz > r * r + 1) continue;
                            place_leaf(x + dx, ly, z + dz);
                        }
                    }
                }
                place_leaf(x, top + 1, z); // tip
            } else {
                // Bush: short trunk, wide flat canopy.
                int trunk_height = random(1, 2);
                for (int dy = 1; dy <= trunk_height; dy++) place_trunk(height + dy);

                int radius = random(2, 3);
                int canopy_center = height + trunk_height;
                for (int dx = -radius; dx <= radius; dx++) {
                    for (int dz = -radius; dz <= radius; dz++) {
                        for (int dy = 0; dy <= 1; dy++) {
                            if (dx * dx + dz * dz + dy * dy * 3 > radius * radius) continue;
                            place_leaf(x + dx, canopy_center + dy, z + dz);
                        }
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
    static const OpenSimplexNoise::Noise mesa_noise(600);

    // Domain warp the sample point with a separate low-frequency noise field
    // so coastlines and ridgelines wind naturally instead of looking aligned
    // to the underlying noise grid.
    const double warp_scale = 0.006;
    const double warp_strength = 45.0;
    double wx = x + warp_noise_x.eval(x * warp_scale, z * warp_scale) * warp_strength;
    double wz = z + warp_noise_z.eval(x * warp_scale + 500.0, z * warp_scale - 500.0) * warp_strength;

    double continent = continent_noise.eval(wx * 0.0045, wz * 0.0045);

    const double PLAINS = 42.0;
    const double MOUNTAIN_PEAK = 129.0;

    // Desert mesas: terrace a broad noise field into discrete flat-topped
    // steps with sheer sides instead of smooth hills -- rounding to a fixed
    // number of levels is what actually produces flat mesa tops and cliff
    // faces rather than a continuous slope. Computed unconditionally and
    // blended in below by aridity/continentalness rather than switched to
    // via a hard branch -- a branch here made height snap between this and
    // the normal formula below wherever aridity/continentalness crossed
    // their threshold, even mid-slope, since the two come from unrelated
    // noise fields with no reason to agree at the boundary.
    double mesaRaw = mesa_noise.eval(wx * 0.006, wz * 0.006); // [-1, 1]
    const double mesaLevels = 5.0;
    double mesaTerraced = std::round(mesaRaw * mesaLevels) / mesaLevels;
    double mesaHeight = map(mesaTerraced, -1.0, 1.0, PLAINS - 8.0, PLAINS + 50.0);
    mesaHeight += detail_noise[0].eval(wx * 0.02, wz * 0.02) * 1.5; // slight roughness, tops stay mostly flat

    // Erosion in [0, 1]: how much relief a region is allowed. Thresholded
    // (not just a power-curve dampener) so a sizeable fraction of erosion-
    // high regions reach *full* ridge amplitude instead of every region
    // needing erosion and ridge to both independently peak at once -- that
    // multiplicative requirement was why mountains almost never actually
    // appeared. Kept at the original large wavelength (unlike
    // continentalness) so a mountainous patch is big enough to contain a
    // whole range instead of clipping it.
    double erosion_raw = erosion_noise.eval(wx * 0.0022, wz * 0.0022); // [-1, 1]
    double erosion_t = std::clamp((erosion_raw + 0.1) / 0.4, 0.0, 1.0);
    double erosion = 0.15 + erosion_t * 0.85; // always some relief, ramping up to full mountains

    // Ridged multifractal (1 - |noise|) carves actual mountain ridgelines
    // and valleys instead of smooth Perlin bumps. Also kept at the original
    // large wavelength so ranges are grand instead of choppy little bumps.
    double ridge = 0.0, ridge_amp = 1.0, ridge_freq = 0.0065, ridge_amp_sum = 0.0;
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

    // Base elevation from continentalness. The ocean branch is deliberately
    // capped well below sea level (not just barely under it) so ocean is a
    // real, reliable body of water instead of a thin puddle at the extreme
    // edge of the noise range.
    const double OCEAN_FLOOR = 4.0;
    double base;
    if (continent < 0.0) {
        base = map(continent, -1.0, 0.0, OCEAN_FLOOR, SEA_LEVEL - 3.0);
    } else if (continent < 0.12) {
        base = map(continent, 0.0, 0.12, SEA_LEVEL - 3.0, SEA_LEVEL + 8.0);
    } else {
        base = map(continent, 0.12, 1.0, SEA_LEVEL + 8.0, PLAINS);
    }

    // Mountains rise out of the plains where erosion allows it, shaped by
    // the ridged noise.
    double mountain = ridge * erosion * (MOUNTAIN_PEAK - PLAINS);

    double height = base + mountain + detail * 4.0;

    // Fade the mesa formula in by aridity/continentalness instead of
    // switching to it outright, so desert edges taper into the surrounding
    // terrain (and into the coast) instead of cutting a cliff right at the
    // threshold.
    auto smoothstep = [](double edge0, double edge1, double v) {
        double t = std::clamp((v - edge0) / (edge1 - edge0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    };
    double desertBlend = smoothstep(DESERT_THRESHOLD - 0.08, DESERT_THRESHOLD + 0.08, aridity(x, z))
                        * smoothstep(-0.10, 0.0, continent);

    return height + (mesaHeight - height) * desertBlend;
}

// Large-scale field deciding where desert (mesa terrain + sand cover) shows
// up, independent of elevation -- a desert can be inland at any height, not
// just a strip near sea level.
double Chunk::aridity(int x, int z) {
    static const OpenSimplexNoise::Noise aridity_noise(500);
    return aridity_noise.eval(x * 0.001, z * 0.001);
}

Chunk::~Chunk() {
    delete batch;
    delete water_batch;
}

void Chunk::submit_opaque_batch() {
    // Null until upload_to_gpu() runs on the main thread -- a chunk waiting
    // in the upload queue just doesn't render yet rather than crashing.
    if (batch) batch->submit();
}

void Chunk::submit_water_batch() {
    if (water_batch) {
        water_batch->submit();
    }
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