#pragma once

#include <vector>
#include <bgfx/bgfx.h>

namespace Render {

typedef struct Vertex {
    float x, y, z;  // Position
    float u, v;     // Texture coordinates
    float nx, ny, nz; // Normal
    static bgfx::VertexLayout init() {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float) // Add normals
            .end();
        bgfx::createVertexLayout(layout);
        return layout;
    }
};


typedef uint32_t Index;
typedef std::vector<Vertex> VertexBuffer;
typedef std::vector<Index> VertexBufferIndices;

typedef struct Mesh {
    VertexBuffer vertices;
    VertexBufferIndices vertexIndices;
};

Mesh transform_mesh(Mesh mesh, float x, float y, float z);
Mesh scale_mesh(Mesh mesh, float x, float y, float z);

Mesh cube(float size);

}  // namespace Render