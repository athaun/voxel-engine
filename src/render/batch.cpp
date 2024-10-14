#include "batch.h"
#include "texture.h"

namespace Render {
    Batch::Batch(unsigned long max_vertices, unsigned long max_indices, const char *shader_name) {
        this->max_vertices = max_vertices;
        this->max_indices = max_indices;
        this->used_vertices = 0;
        this->used_indices = 0;

        // Create dynamic vertex and index buffers to store the batch data
        this->vertex_buffer = bgfx::createDynamicVertexBuffer(max_vertices, Vertex::init());
        this->index_buffer = bgfx::createDynamicIndexBuffer(max_indices, BGFX_BUFFER_INDEX32);

        // Load texture (solid color or actual image)
        this->grassTexture = loadTexture("src/static/grass.jpg");
        // Create the shader program and uniform only once
        this->shader_program = load_shader(shader_name);
        this->s_texture = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
        this->u_ambientColor = bgfx::createUniform("u_ambientColor", bgfx::UniformType::Vec4);
        this->u_lightDirection = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4);
    }

    Batch::~Batch() {
        bgfx::destroy(this->vertex_buffer);
        bgfx::destroy(this->index_buffer);
        bgfx::destroy(this->shader_program);
        bgfx::destroy(this->grassTexture);
        bgfx::destroy(this->s_texture);
        bgfx::destroy(this->u_ambientColor);
        bgfx::destroy(this->u_lightDirection);
    }

    void Batch::submit() {
        // Bind the texture before rendering
        bgfx::setTexture(0, this->s_texture, this->grassTexture);
        
        // Set the ambient color
        float ambientColor[3] = { 1, 1, 1 }; // Low intensity gray
        bgfx::setUniform(u_ambientColor, ambientColor);

        // Bind vertex and index buffers
        bgfx::setVertexBuffer(0, this->vertex_buffer);
        bgfx::setIndexBuffer(this->index_buffer);

        // Submit the draw call
        bgfx::submit(0, this->shader_program);
    }

    bool Batch::push_mesh (Mesh mesh) {
        if ((used_vertices + mesh.vertices.size() > max_vertices) || (used_indices + mesh.vertexIndices.size() > max_indices)) {
            return false;
        }
        
        // Adjust indices to account for the existing vertices in the buffer
        for (size_t i = 0; i < mesh.vertexIndices.size(); ++i) {
            mesh.vertexIndices[i] += used_vertices; // Offset by the number of used vertices
        }

        bgfx::update(this->vertex_buffer, used_vertices, bgfx::copy(mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size()));
        bgfx::update(this->index_buffer, used_indices, bgfx::copy(mesh.vertexIndices.data(), sizeof(Index) * mesh.vertexIndices.size()));

        used_vertices += mesh.vertices.size();
        used_indices += mesh.vertexIndices.size();

        return true;
    }

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

    void Batch::setLightDirection(const bx::Vec3& direction) {
        bgfx::setUniform(u_lightDirection, &direction);  // Set the uniform value
    }
}  // namespace Render