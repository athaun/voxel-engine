#include "batch.h"
#include "texture.h"
#include "../core/Log.h"

namespace Render {
    Batch::Batch(unsigned long max_vertices, unsigned long max_indices, const char *shader_name) {
        this->max_vertices = max_vertices;
        this->max_indices = max_indices;
        this->used_vertices = 0;
        this->used_indices = 0;

        // Create dynamic vertex and index buffers to store the batch data
        this->vertex_buffer = bgfx::createDynamicVertexBuffer(max_vertices, Vertex::init());
        this->index_buffer = bgfx::createDynamicIndexBuffer(max_indices, BGFX_BUFFER_INDEX32);
        // Create the shader program and uniform only once
        this->shader_program = load_shader(shader_name);
        this->u_ambientColor = bgfx::createUniform("u_ambientColor", bgfx::UniformType::Vec4);
        this->u_lightDirection = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4);
    }

    Batch::~Batch() {
        bgfx::destroy(this->vertex_buffer);
        bgfx::destroy(this->index_buffer);
        bgfx::destroy(this->shader_program);
        bgfx::destroy(this->u_ambientColor);
        bgfx::destroy(this->u_lightDirection);   
    }

    void Batch::submit() {
        // Set the ambient color
        float ambientColor[4] = { 0.8f, 0.8f, 0.8f, 1.0f }; // Low intensity gray
        bgfx::setUniform(u_ambientColor, ambientColor);
        // Bind vertex and index buffers
        bgfx::setVertexBuffer(0, this->vertex_buffer);
        bgfx::setIndexBuffer(this->index_buffer);

        // Submit the draw call
        bgfx::submit(0, this->shader_program);
    }

    bool Batch::push_mesh(Mesh mesh) {
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

    // bool Batch::push_mesh_buffer(const std::vector<Render::Mesh>& meshes) {
    //     size_t total_vertices = used_vertices;
    //     size_t total_indices = used_indices;

    //     // Calculate total vertices and indices required
    //     for (const auto& mesh : meshes) {
    //         total_vertices += mesh.vertices.size();
    //         total_indices += mesh.vertex_indices.size();
    //     }

    //     // Check if they fit
    //     if (total_vertices > max_vertices || total_indices > max_indices) {
    //         return false; // Not enough space in the batch
    //     }

    //     // Update buffers in bulk
    //     for (const auto& mesh : meshes) {
    //         // Adjust indices to account for existing vertices
    //         std::vector<Index> adjusted_indices = mesh.vertex_indices;
    //         for (size_t i = 0; i < adjusted_indices.size(); ++i) {
    //             adjusted_indices[i] += used_vertices;
    //         }

    //         // Update vertex and index buffers
    //         bgfx::update(this->vertex_buffer, used_vertices, bgfx::copy(mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size()));
    //         bgfx::update(this->index_buffer, used_indices, bgfx::copy(adjusted_indices.data(), sizeof(Index) * adjusted_indices.size()));

    //         // Track total used vertices and indices
    //         used_vertices += mesh.vertices.size();
    //         used_indices += adjusted_indices.size();
    //     }

    //     return true;
    // }


    bool Batch::push_triangle(Vertex v1, Vertex v2, Vertex v3) {
        if (used_vertices + 3 > max_vertices || used_indices + 3 > max_indices) {
            return false;
        }

        Vertex vertices[] = { v1, v2, v3 };
        uint16_t indices[] = { used_vertices, used_vertices + 1, used_vertices + 2 };

        bgfx::update(this->vertex_buffer, used_vertices, bgfx::copy(vertices, sizeof(vertices)));
        bgfx::update(this->index_buffer, used_indices, bgfx::copy(indices, sizeof(indices)));

        used_vertices += 3;
        used_indices += 3;

        return true;
    }

    const bgfx::ProgramHandle Batch::load_shader(const char *shader_name) {
        std::string vertex_shader_path = std::filesystem::absolute(std::string("build/shaders/v_") + shader_name + ".bin").string();
        std::string fragment_shader_path = std::filesystem::absolute(std::string("build/shaders/f_") + shader_name + ".bin").string();

        std::ifstream vertex_shader_file(vertex_shader_path, std::ios::binary | std::ios::ate);
        std::ifstream fragment_shader_file(fragment_shader_path, std::ios::binary | std::ios::ate);

        if (!vertex_shader_file) {
            Log::error("Failed to open vertex shader file " + vertex_shader_path);
            return bgfx::ProgramHandle();
        }

        if (!fragment_shader_file) {
            Log::error("Failed to open fragment shader file " + fragment_shader_path);
            return bgfx::ProgramHandle();
        }

        std::streampos vertex_shader_size = vertex_shader_file.tellg();
        std::streampos fragment_shader_size = fragment_shader_file.tellg();

        vertex_shader_file.seekg(0, std::ios::beg);
        fragment_shader_file.seekg(0, std::ios::beg);

        std::vector<char> vertex_shader_data(vertex_shader_size);
        std::vector<char> fragment_shader_data(fragment_shader_size);

        if (!vertex_shader_file.read(vertex_shader_data.data(), vertex_shader_size)) {
            Log::error("Failed to read vertex shader file");
            return bgfx::ProgramHandle();
        }

        if (!fragment_shader_file.read(fragment_shader_data.data(), fragment_shader_size)) {
            Log::error("Failed to read fragment shader file");
            return bgfx::ProgramHandle();
        }

        vertex_shader_file.close();
        fragment_shader_file.close();

        const bgfx::Memory *vertex_shader_mem = bgfx::copy(vertex_shader_data.data(), static_cast<uint32_t>(vertex_shader_size));
        const bgfx::Memory *fragment_shader_mem = bgfx::copy(fragment_shader_data.data(), static_cast<uint32_t>(fragment_shader_size));

        bgfx::ShaderHandle vertex_shader = bgfx::createShader(vertex_shader_mem);
        bgfx::ShaderHandle fragment_shader = bgfx::createShader(fragment_shader_mem);

        // Check if shaders are valid
        if (!bgfx::isValid(vertex_shader)) {
            Log::error("Failed to create vertex shader");
            return bgfx::ProgramHandle();
        }

        if (!bgfx::isValid(fragment_shader)) {
            Log::error("Failed to create fragment shader");
            return bgfx::ProgramHandle();
        }

        return bgfx::createProgram(vertex_shader, fragment_shader, false);
    }

}  // namespace Render