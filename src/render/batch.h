#pragma once

#include <bgfx/bgfx.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace Render {

typedef struct Vertex {
    float x, y, z;  // Position
    // float u, v;     // Texture coordinates
    uint32_t color;
    static bgfx::VertexLayout init() {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            // .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        bgfx::createVertexLayout(layout);
        return layout;
    }
};

typedef std::vector<Vertex> VertexBuffer;
typedef std::vector<uint16_t> VertexBufferIndices;

typedef struct Mesh {
    VertexBuffer vertices;
    VertexBufferIndices vertexIndices;
};

class Batch {
   public:
    Batch(unsigned int maxVertices, unsigned int maxIndices) {
        // this->maxVertices = maxVertices;
        // this->maxIndices = maxIndices;
        // this->usedVertices = 0;
        // this->usedIndices = 0;

        // // Initialize vertex layout
        // Vertex::init();

        // // Create dynamic vertex and index buffers
        // vertexBuffer = bgfx::createDynamicVertexBuffer(maxVertices, Vertex::layout);
        // indexBuffer = bgfx::createDynamicIndexBuffer(maxIndices);

        // // Load shaders
        // program = loadProgram("vs_batch", "fs_batch");

        // // Initialize transform uniform
        // transform = bgfx::createUniform("u_transform", bgfx::UniformType::Mat4);
    }

    ~Batch() {
        // bgfx::destroy(vertexBuffer);
        // bgfx::destroy(indexBuffer);
        // bgfx::destroy(program);
        // bgfx::destroy(transform);
    }

    void begin() {
        // usedVertices = 0;
        // usedIndices = 0;
        // textures.clear();
    }

    void end() {
        // // Submit the batch
        // bgfx::setVertexBuffer(0, vertexBuffer, 0, usedVertices);
        // bgfx::setIndexBuffer(indexBuffer, 0, usedIndices);
        // bgfx::submit(0, program);
    }

    void drawQuad(const Vertex *vertices, const uint16_t *indices, bgfx::TextureHandle texture) {
        // // Update vertex and index buffers
        // bgfx::update(vertexBuffer, usedVertices, bgfx::makeRef(vertices, sizeof(Vertex) * 4));
        // bgfx::update(indexBuffer, usedIndices, bgfx::makeRef(indices, sizeof(uint16_t) * 6));
        // usedVertices += 4;
        // usedIndices += 6;

        // // Add texture to the batch
        // addTexture(texture);
    }

    const bgfx::ProgramHandle load_shader(const char *shader_name) {
        std::string vertex_shader_path =
            std::filesystem::absolute(std::string("build/shaders/v_") + shader_name + ".bin")
                .string();
        std::string fragment_shader_path =
            std::filesystem::absolute(std::string("build/shaders/f_") + shader_name + ".bin")
                .string();

        std::ifstream vertex_shader_file(vertex_shader_path, std::ios::binary | std::ios::ate);
        std::ifstream fragment_shader_file(fragment_shader_path, std::ios::binary | std::ios::ate);

        if (!vertex_shader_file || !fragment_shader_file) {
            Log::error("Failed to open shader files " + vertex_shader_path + " and " +
                       fragment_shader_path);
            return bgfx::ProgramHandle();
        }

        std::streampos vertex_shader_size = vertex_shader_file.tellg();
        std::streampos fragment_shader_size = fragment_shader_file.tellg();

        vertex_shader_file.seekg(0, std::ios::beg);
        fragment_shader_file.seekg(0, std::ios::beg);

        std::vector<char> vertex_shader_data(vertex_shader_size);
        std::vector<char> fragment_shader_data(fragment_shader_size);

        if (!vertex_shader_file.read(vertex_shader_data.data(), vertex_shader_size) ||
            !fragment_shader_file.read(fragment_shader_data.data(), fragment_shader_size)) {
            Log::error("Failed to read shader files");
            return bgfx::ProgramHandle();
        }

        vertex_shader_file.close();
        fragment_shader_file.close();

        const bgfx::Memory *vertex_shader_mem =
            bgfx::copy(vertex_shader_data.data(), static_cast<uint32_t>(vertex_shader_size));
        const bgfx::Memory *fragment_shader_mem =
            bgfx::copy(fragment_shader_data.data(), static_cast<uint32_t>(fragment_shader_size));

        bgfx::ShaderHandle vertex_shader = bgfx::createShader(vertex_shader_mem);
        bgfx::ShaderHandle fragment_shader = bgfx::createShader(fragment_shader_mem);

        // Check if shaders are valid
        if (!bgfx::isValid(vertex_shader) || !bgfx::isValid(fragment_shader)) {
            Log::error("Failed to create shaders");
            return bgfx::ProgramHandle();
        }

        return bgfx::createProgram(vertex_shader, fragment_shader, false);
    }

   private:
    // unsigned int maxVertices;
    // unsigned int maxIndices;
    // unsigned int usedVertices;
    // unsigned int usedIndices;

    // bgfx::DynamicVertexBufferHandle vertexBuffer;
    // bgfx::DynamicIndexBufferHandle indexBuffer;
    // bgfx::ProgramHandle program;
    // bgfx::UniformHandle transform;
    // std::vector<bgfx::TextureHandle> textures;

    void addTexture(bgfx::TextureHandle texture) {
        // if (std::find(textures.begin(), textures.end(), texture) == textures.end()) {
        //     textures.push_back(texture);
        // }
    }
};
}  // namespace Render
