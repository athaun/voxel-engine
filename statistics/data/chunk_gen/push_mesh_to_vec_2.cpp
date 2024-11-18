/**
 * This version has reservation and pushes the whole buffer at once using the new push_mesh_buffer method in Batch
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
    batch->push_mesh_buffer(mesh_buffer);
}




bool Batch::push_mesh_buffer(const std::vector<Render::Mesh>& meshes) {
        size_t total_vertices = used_vertices;
        size_t total_indices = used_indices;

        // Calculate total vertices and indices required
        for (const auto& mesh : meshes) {
            total_vertices += mesh.vertices.size();
            total_indices += mesh.vertex_indices.size();
        }

        // Check if they fit
        if (total_vertices > max_vertices || total_indices > max_indices) {
            return false; // Not enough space in the batch
        }

        // Update buffers in bulk
        for (const auto& mesh : meshes) {
            // Adjust indices to account for existing vertices
            std::vector<Index> adjusted_indices = mesh.vertex_indices;
            for (size_t i = 0; i < adjusted_indices.size(); ++i) {
                adjusted_indices[i] += used_vertices;
            }

            // Update vertex and index buffers
            bgfx::update(this->vertex_buffer, used_vertices, bgfx::copy(mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size()));
            bgfx::update(this->index_buffer, used_indices, bgfx::copy(adjusted_indices.data(), sizeof(Index) * adjusted_indices.size()));

            // Track total used vertices and indices
            used_vertices += mesh.vertices.size();
            used_indices += adjusted_indices.size();
        }

        return true;
    }