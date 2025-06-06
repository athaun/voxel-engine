/**
 * This has no reservation and does not use a bulk push method
 */

Chunk::Chunk(int x, int y, int z) : global_x(x * CHUNK_WIDTH), global_y(y * CHUNK_DEPTH), global_z(z * CHUNK_DEPTH) {
    ScopedTimer t("Chunk constructor");

    static int chunk_volume = CHUNK_WIDTH * CHUNK_DEPTH * CHUNK_HEIGHT;
    static int vertex_count = chunk_volume * 8;
    static int index_count = chunk_volume * 36;

    batch = new Render::Batch(vertex_count, index_count, "cubes");

    int terrain_heights[CHUNK_WIDTH][CHUNK_DEPTH] = {};

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_DEPTH; z++) {
            int terrain_height = terrain(global_x + x, 0, global_z + z);

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                Voxel v;
                v.material = (terrain_height > y ? voxel::Material::STONE : voxel::Material::AIR);
                set_voxel(x, y, z, v);
            }

            terrain_heights[x][z] = terrain_height;\
        }
    } 

    uint8_t used_faces = 0b00000000;

    // Buffer to store meshes
    std::vector<Render::Mesh> mesh_buffer;

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