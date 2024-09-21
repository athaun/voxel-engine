#pragma once

#include <bgfx/bgfx.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "log.h"

namespace Render {

typedef struct Vertex {
    float x, y, z;  // Position
    // float u, v;  // Texture coordinates
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
   private:
    unsigned long max_vertices;
    unsigned long max_indices;
    unsigned long used_vertices;
    unsigned long used_indices;

    bgfx::DynamicVertexBufferHandle vertex_buffer;
    bgfx::DynamicIndexBufferHandle index_buffer;

    bgfx::ProgramHandle shader_program;

    std::vector<Mesh> meshes;

   public:
    Batch(unsigned long max_vertices, unsigned long max_indices, const char *shader_name);
    ~Batch();

    void submit();

    bool push_mesh (Mesh mesh);

    bool push_triangle(Vertex v1, Vertex v2, Vertex v3);
   
   private:
    const bgfx::ProgramHandle load_shader(const char *shader_name);
};
}  // namespace Render
