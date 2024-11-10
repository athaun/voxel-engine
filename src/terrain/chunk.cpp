#include "chunk.h"
#include "../core/Log.h"
#include "../noise/OpenSimplexNoise.h"

Chunk::Chunk(int x, int y, int z) : global_x(x * CHUNK_WIDTH), global_y(y * CHUNK_DEPTH), global_z(z * CHUNK_DEPTH) {
    static int chunk_volume = CHUNK_WIDTH * CHUNK_DEPTH * CHUNK_HEIGHT;
    static int vertex_count = chunk_volume * 8;
    static int index_count = chunk_volume * 36;

    batch = new Render::Batch(vertex_count, index_count, "cubes");

    int terrain_heights[CHUNK_WIDTH][CHUNK_DEPTH] = {};

    size_t estimated_mesh_count = 0;

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            int terrain_height = terrain(global_x + x, 0, global_z + z);

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Voxel v;
                v.material = (terrain_height > y ? voxel::Material::STONE : voxel::Material::AIR);
                set_voxel(x, y, z, v);
                estimated_mesh_count++;
            }

            terrain_heights[x][z] = terrain_height;

        }
    } 

    uint8_t used_faces = 0b00000000;
    std::vector<Render::Mesh> mesh_buffer;
    mesh_buffer.reserve(estimated_mesh_count); // Prevent reallocations

    //ScopedTimer t1("Face culling");

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            for (int y = global_y * CHUNK_HEIGHT; y < (global_y * CHUNK_HEIGHT) + CHUNK_HEIGHT && y < terrain_heights[x][z]; y++) {

                used_faces = get_visible_faces(x, y, z);
                
                if (used_faces != 0) {
                    for (int face = 0; face < 6; ++face) {
                        if (used_faces & (1 << face)) {
                            float raw_values[4];
                            calculate_face_ao(x, y, z, face, raw_values);
                            
                            Render::Mesh c = Render::transform_mesh(Render::cube(1 << face, raw_values), global_x + x, y, global_z + z);
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
    if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_DEPTH) {
        return true;
    }
    return voxels[x][z][y].material == voxel::Material::AIR;
}

// 15.8s to generate radius 5 of 20x20x20 chunks with ifs
// 18.2s-22.7s to generate radius 5 of 20x20x20 chunks using optimized booleans declarations and ifs
// 18.9s-21.2s to generate radius 5 of 20x20x20 chunks without ifs, but with return
// 57.7s to generate radius 5 of 20x20x20 chunks without ifs, with reference parameter

uint8_t Chunk::get_visible_faces(int x, int y, int z) {
    uint8_t used_faces = 0b00000000;

    if (is_empty(x, y, z + 1)) used_faces |= (1 << 0); // Front face
    if (is_empty(x + 1, y, z)) used_faces |= (1 << 1); // Right face
    if (is_empty(x, y, z - 1)) used_faces |= (1 << 2); // Back face
    if (is_empty(x - 1, y, z)) used_faces |= (1 << 3); // Left face
    if (is_empty(x, y + 1, z)) used_faces |= (1 << 4); // Top face
    if (is_empty(x, y - 1, z)) used_faces |= (1 << 5); // Bottom face

    return used_faces;
}

// float Chunk::calculate_ao(int x, int y, int z, int corner1_x, int corner1_y, int corner1_z, 
//                           int corner2_x, int corner2_y, int corner2_z) {
//     // Check three adjacent blocks that affect this vertex's AO:
//     // - Two side blocks (side1 and side2)
//     // - One corner block that connects these sides
//     //
//     // Example for top-left vertex of front face:
//     //   C - Corner block
//     //   S1, S2 - Side blocks
//     //   V - Vertex we're calculating AO for
//     //
//     //   C  S1
//     //   S2 V
    
//     // Convert block presence into weighted occlusion values:
//     // - 0.0f means no occlusion (block is empty)
//     // - 0.5f for side blocks (moderate occlusion)
//     // - 0.75f for corner blocks (stronger occlusion)


//     // Check neighboring blocks
//     float side1 = !is_empty(x + corner1_x, y + corner1_y, z + corner1_z) ? 0.5f : 0.0f;
//     float side2 = !is_empty(x + corner2_x, y + corner2_y, z + corner2_z) ? 0.5f : 0.0f;
//     float corner = !is_empty(x + corner1_x + corner2_x, 
//                             y + corner1_y + corner2_y, 
//                             z + corner1_z + corner2_z) ? 0.75f : 0.0f;

//     // Calculate weighted occlusion value
//     float occlusion = side1 + side2 + corner;

//     // Invert the occlusion value because in graphics:
//     // 1.0 typically means fully lit (no occlusion)
//     // 0.0 typically means fully dark (full occlusion)
//     // Clamp the value to ensure it stays in the range [0, 1]
//     return std::max(0.0f, std::min(1.0f, 1.0f - occlusion));
// }

float Chunk::calculate_ao(int x, int y, int z, int corner1_x, int corner1_y, int corner1_z, 
                          int corner2_x, int corner2_y, int corner2_z) {
    // Check neighboring blocks
    float side1 = !is_empty(x + corner1_x, y + corner1_y, z + corner1_z) ? 0.3f : 0.0f;
    float side2 = !is_empty(x + corner2_x, y + corner2_y, z + corner2_z) ? 0.3f : 0.0f;
    
    // Modify corner calculation to be less aggressive
    float corner = !is_empty(x + corner1_x + corner2_x, 
                            y + corner1_y + corner2_y, 
                            z + corner1_z + corner2_z) ? 0.10f : 0.0f;

    // Only add corner darkening if both sides are empty
    if (side1 > 0.0f && side2 > 0.0f) {
        corner = 0.0f; // Don't darken if both sides are solid
    } else if (side1 > 0.0f || side2 > 0.0f) {
        corner *= 0.5f; // Reduce corner darkness if one side is solid
    }

    // Calculate total occlusion
    float occlusion = side1 + side2 + corner;

    // Ensure we don't over-darken
    return std::max(0.0f, std::min(1.0f, 1.0f - occlusion));
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
            raw_values[3] = calculate_ao(x, y, z, -1, 0, 1, -1, -1, 0);  // bottom-right
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

Voxel Chunk::get_voxel(int x, int y, int z) const {
    return voxels[x][z][y];
}

void Chunk::set_voxel(int x, int y, int z, Voxel voxel) {
    voxels[x][z][y] = voxel;
}

std::pair<int, int> Chunk::get_position() const {
    return std::make_pair(global_x, global_z);
}