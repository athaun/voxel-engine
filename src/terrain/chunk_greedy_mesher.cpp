#include "chunk.h"

void Chunk::generate_greedy_mesh() {
    static int chunk_volume = CHUNK_WIDTH * CHUNK_DEPTH * CHUNK_HEIGHT;
    static int max_faces = chunk_volume * 6; // Worst case: each voxel has 6 faces
    
    std::vector<Face> faces;
    faces.reserve(max_faces);

    bool visited[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_DEPTH] = {false};

    for (uint8_t dir = 0; dir < 6; dir++) {
        memset(visited, 0, sizeof(visited));

        // Iterate through all possible face positions
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                for (int z = 0; z < CHUNK_DEPTH; z++) {

                    Voxel starting_voxel = get_voxel(x, y, z);
                    if (starting_voxel.material == voxel::Material::AIR) continue;
                    
                    // Check if there should be a face in this direction
                    if (!can_extend_face(starting_voxel, x, y, z, dir)) continue;

                    // Start a new face
                    Face face(x, y, z, dir);

                    // Now extend the face in the given direction                
                    faces.push_back(face);
                }
            }
        }
    }

    generate_mesh_from_faces(faces);
    faces.clear();
}

bool Chunk::can_extend_face(Voxel starting_voxel, int x, int y, int z, uint8_t dir) {   
    // Check adjacent voxel based on direction
    const int* d = DIRS[dir];
    int nx = x + d[0];
    int ny = y + d[1];
    int nz = z + d[2];
    
    // If adjacent position is outside chunk, check neighbor chunks or assume air
    if (nx < 0 || nx >= CHUNK_WIDTH || ny < 0 || ny >= CHUNK_HEIGHT || nz < 0 || nz >= CHUNK_DEPTH) {
        return true; // Or implement neighbor chunk checking here
    }
    
    return get_voxel(nx, ny, nz).material == starting_voxel.material;
}


void Chunk::generate_mesh_from_faces(const std::vector<Face>& faces) {
    std::vector<Render::Mesh> mesh_buffer;
    mesh_buffer.reserve(faces.size());
    
    for (const Face& face : faces) {
        // Create base face mesh
        Render::Mesh m = Render::cube(1 << face.direction, face.ao);
        
        // Scale the mesh based on face dimensions and direction
        switch (face.direction) {
            case 0: // Front (+Z)
            case 2: // Back (-Z)
                Render::scale_mesh(m, face.width, face.height, 1);
                break;
            case 1: // Right (+X)
            case 3: // Left (-X)
                Render::scale_mesh(m, 1, face.height, face.width);
                break;
            case 4: // Top (+Y)
            case 5: // Bottom (-Y)
                Render::scale_mesh(m, face.width, 1, face.height);
                break;
        }
        
        // Transform to world position
        m = Render::transform_mesh(m, global_x + face.x, face.y, global_z + face.z);
        mesh_buffer.push_back(m);
    }
    
    batch->push_mesh_buffer(mesh_buffer);
}

