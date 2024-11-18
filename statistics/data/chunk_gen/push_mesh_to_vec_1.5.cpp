/**
 * This version has reservation but does not push the whole buffer at once
 */

Chunk::Chunk(int x, int y, int z) : global_x(x * CHUNK_WIDTH), global_y(y * CHUNK_DEPTH), global_z(z * CHUNK_DEPTH) {
    ScopedTimer t("Chunk constructor");

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

    ScopedTimer t1("Face culling");

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            for (int y = global_y * CHUNK_HEIGHT; y < (global_y * CHUNK_HEIGHT) + CHUNK_HEIGHT && y < terrain_heights[x][z]; y++) {
                
                uint8_t used_faces = get_visible_faces(x, y, z);
                
                if (used_faces != 0) {
                    // Transform and store the mesh in the buffer
                    Render::Mesh c = Render::transform_mesh(Render::cube(used_faces), global_x + x, y, global_z + z);
                    mesh_buffer.push_back(c);
                }
            }
        }
    }

    // Push the entire buffer of meshes at once to the batch
    for (auto& mesh : mesh_buffer) {
        batch->push_mesh(mesh);
    }
}





bool Batch::push_mesh (Mesh mesh) {
        if ((used_vertices + mesh.vertices.size() > max_vertices) || (used_indices + mesh.vertex_indices.size() > max_indices)) {
            return false;
        }
        
        // Adjust indices to account for the existing vertices in the buffer
        for (size_t i = 0; i < mesh.vertex_indices.size(); ++i) {
            mesh.vertex_indices[i] += used_vertices; // Offset by the number of used vertices
        }

        bgfx::update(this->vertex_buffer, used_vertices, bgfx::copy(mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size()));
        bgfx::update(this->index_buffer, used_indices, bgfx::copy(mesh.vertex_indices.data(), sizeof(Index) * mesh.vertex_indices.size()));

        used_vertices += mesh.vertices.size();
        used_indices += mesh.vertex_indices.size();

        return true;
    }