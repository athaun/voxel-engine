#pragma once

#include <vector>
#include <bgfx/bgfx.h>
#include <terrain/voxel.h>

namespace Render {

typedef struct Vertex {
    float x, y, z;      // Position
    float r, g, b;      // Color
    float nx, ny, nz;   // Normal
    float ao;           // Ambient Occlusion

    static bgfx::VertexLayout init() {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)  // RGB color
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color1, 1, bgfx::AttribType::Float)  // AO
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
    VertexBufferIndices vertex_indices;
};

Mesh transform_mesh(Mesh mesh, float x, float y, float z);
Mesh scale_mesh(Mesh mesh, float x, float y, float z);

Mesh cube();
Mesh colored_cube(uint32_t color);
Mesh cube(uint8_t used_faces, const float* ao_values = nullptr);

Mesh quad(int face, float width, float height);

}  // namespace Render