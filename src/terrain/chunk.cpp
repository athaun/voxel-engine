#include "chunk.h"
#include "../core/Log.h"
#include "../noise/OpenSimplexNoise.h"
#include "../core/timer.h"
#include <algorithm>


// In chunk.cpp:
Chunk::Chunk(int x, int y, int z) : global_x(x * CHUNK_WIDTH), global_y(y * CHUNK_DEPTH), global_z(z * CHUNK_DEPTH) {
    static int chunk_volume = CHUNK_WIDTH * CHUNK_DEPTH * CHUNK_HEIGHT;
    static int vertex_count = chunk_volume * 8;
    static int index_count = chunk_volume * 36;

    batch = new Render::Batch(vertex_count, index_count, "cubes");
    size_t estimated_mesh_count = 0;

    // First pass: Generate terrain heights including buffer zone
    for (int x = -BUFFER_SIZE/2; x < CHUNK_WIDTH + BUFFER_SIZE/2; x++) {
        for (int z = -BUFFER_SIZE/2; z < CHUNK_DEPTH + BUFFER_SIZE/2; z++) {
            int array_x = x + BUFFER_SIZE/2;
            int array_z = z + BUFFER_SIZE/2;
            
            // Clamp the terrain height to CHUNK_HEIGHT
            int height = terrain(global_x + x, 0, global_z + z);
            terrain_heights[array_x][array_z] = std::min(height, CHUNK_HEIGHT - 1);
        }
    }

    // Second pass: Generate voxels for the actual chunk (without buffer)
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            int height = terrain_heights[x + BUFFER_SIZE/2][z + BUFFER_SIZE/2];
            double water_value = water_noise(global_x + x, global_z + z);
            
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Voxel v;
                
                // Check for water at sea level
                // if (y == SEA_LEVEL && height <= SEA_LEVEL && water_value > 0.1) {
                //     v.material = voxel::WATER;
                // }
                if (y >= height) {
                    v.material = voxel::AIR;
                }
                else if (y == height - 1) {
                    v.material = voxel::GRASS;
                }
                else if (y == height - 2) {
                    v.material = voxel::DIRT;
                }
                else if (y == height - 3) {
                    v.material = voxel::DIRT;
                }
                else {
                    v.material = voxel::STONE;
                }
                set_voxel(x, y, z, v);
            }
        }
    }

    uint8_t used_faces = 0b00000000;
    std::vector<Render::Mesh> mesh_buffer;
    mesh_buffer.reserve(estimated_mesh_count);

    // Third pass: Generate meshes using the buffered terrain data
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            int current_height = terrain_heights[x + BUFFER_SIZE/2][z + BUFFER_SIZE/2];
            
            for (int y = 0; y < current_height; y++) {
                used_faces = get_visible_faces(x, y, z);
                
                if (used_faces != 0) {
                    Voxel current_voxel = get_voxel(x, y, z); // Get current voxel material
                    
                    for (int face = 0; face < 6; ++face) {
                        if (used_faces & (1 << face)) {
                            float raw_values[4];
                            calculate_face_ao(x, y, z, face, raw_values);
                            
                            Render::Mesh c = Render::transform_mesh(
                                Render::cube(static_cast<uint8_t>(1 << face), raw_values, current_voxel.material), global_x + x, y, global_z + z
                            );
                            mesh_buffer.push_back(c);
                        }
                    }
                }
            }
        }
    }
    batch->push_mesh_buffer(mesh_buffer);
}

inline bool Chunk::is_empty(int x, int y, int z) {
    // Convert chunk coordinates to buffered array coordinates
    int buffer_x = x + BUFFER_SIZE/2;
    int buffer_z = z + BUFFER_SIZE/2;
    
    // Check if we're within the buffered bounds
    if (buffer_x < 0 || buffer_x >= CHUNK_WIDTH + BUFFER_SIZE || 
        y < 0 || y >= CHUNK_HEIGHT ||
        buffer_z < 0 || buffer_z >= CHUNK_DEPTH + BUFFER_SIZE) {
        return true;
    }
    
    // Make sure we don't exceed CHUNK_HEIGHT
    int height = std::min(terrain_heights[buffer_x][buffer_z], CHUNK_HEIGHT - 1);
    return y >= height;
}

// 15.8s to generate radius 5 of 20x20x20 chunks with ifs
// 18.2s-22.7s to generate radius 5 of 20x20x20 chunks using optimized booleans declarations and ifs
// 18.9s-21.2s to generate radius 5 of 20x20x20 chunks without ifs, but with return
// 57.7s to generate radius 5 of 20x20x20 chunks without ifs, with reference parameter

uint8_t Chunk::get_visible_faces(int x, int y, int z) {
    uint8_t used_faces = 0b00000000;

    // Get heights of neighboring blocks (including buffer)
    int current_height = terrain_heights[x + BUFFER_SIZE/2][z + BUFFER_SIZE/2];
    int front_height = terrain_heights[x + BUFFER_SIZE/2][z + BUFFER_SIZE/2 + 1];
    int right_height = terrain_heights[x + BUFFER_SIZE/2 + 1][z + BUFFER_SIZE/2];
    int back_height = terrain_heights[x + BUFFER_SIZE/2][z + BUFFER_SIZE/2 - 1];
    int left_height = terrain_heights[x + BUFFER_SIZE/2 - 1][z + BUFFER_SIZE/2];

    // Check front face (+Z)
    if (z == CHUNK_DEPTH - 1 || y >= front_height) used_faces |= (1 << 0);
    
    // Check right face (+X)
    if (x == CHUNK_WIDTH - 1 || y >= right_height) used_faces |= (1 << 1);
    
    // Check back face (-Z)
    if (z == 0 || y >= back_height) used_faces |= (1 << 2);
    
    // Check left face (-X)
    if (x == 0 || y >= left_height) used_faces |= (1 << 3);
    
    // Check top face (+Y)
    if (y == current_height - 1) used_faces |= (1 << 4);
    
    // Check bottom face (-Y)
    if (y == 0) used_faces |= (1 << 5);

    return used_faces;
}

float Chunk::calculate_ao(int x, int y, int z, int corner1_x, int corner1_y, int corner1_z, 
                          int corner2_x, int corner2_y, int corner2_z) {
    // Check neighboring blocks
    float side1 = !is_empty(x + corner1_x, y + corner1_y, z + corner1_z) ? 0.20f : 0.0f;
    float side2 = !is_empty(x + corner2_x, y + corner2_y, z + corner2_z) ? 0.20f : 0.0f;
    
    // Check corner block
    float corner = !is_empty(x + corner1_x + corner2_x, 
                            y + corner1_y + corner2_y, 
                            z + corner1_z + corner2_z) ? 0.05f : 0.0f;

    // Check diagonal-lower block
    float diagonal_lower = !is_empty(x + corner1_x + corner2_x, 
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

    return (double)map((double)noiseValue, -1.0, 0.0, 0.0, (double)CHUNK_HEIGHT);
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

double Chunk::water_noise(int x, int z) {
    static OpenSimplexNoise::Noise noise(42); // Different seed than terrain
    double scale = 0.02;
    return noise.eval(x * scale, z * scale);
}