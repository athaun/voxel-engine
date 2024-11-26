bool Batch::push_mesh_buffer(const std::vector<Render::Mesh>& meshes) {
    size_t total_vertices = used_vertices;
    size_t total_indices = used_indices;

    if (meshes.empty()) return true;

    // Calculate total vertices and indices required
    size_t vertices_to_add = 0;
    size_t indices_to_add = 0;

    for (const auto& mesh : meshes) {
        vertices_to_add += mesh.vertices.size();
        indices_to_add += mesh.vertex_indices.size();
    }

    total_vertices += vertices_to_add;
    total_indices += indices_to_add;

    // Check if they fit
    if (total_vertices > max_vertices || total_indices > max_indices) {
        return false; // Not enough space in the batch
    }

    // Pre-allocate combined vertex and index buffers
    std::vector<Vertex> combined_vertices;
    std::vector<Index> combined_indices;
    combined_vertices.reserve(vertices_to_add);
    combined_indices.reserve(indices_to_add);

    // Offset indices and populate combined buffers
    size_t current_vertex_offset = used_vertices;
    for (const auto& mesh : meshes) {
        // Append vertices
        combined_vertices.insert(combined_vertices.end(), mesh.vertices.begin(), mesh.vertices.end());

        // Append indices with offset adjustment
        for (Index index : mesh.vertex_indices) {
            combined_indices.push_back(index + current_vertex_offset);
        }
        
        current_vertex_offset += mesh.vertices.size();
    }

    // Update bgfx buffers once for all combined vertices and indices
    bgfx::update(this->vertex_buffer, used_vertices, bgfx::copy(combined_vertices.data(), sizeof(Vertex) * combined_vertices.size()));
    bgfx::update(this->index_buffer, used_indices, bgfx::copy(combined_indices.data(), sizeof(Index) * combined_indices.size()));

    // Update total used vertices and indices
    used_vertices = total_vertices;
    used_indices = total_indices;

    return true;
}