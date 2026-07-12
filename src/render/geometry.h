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
    float material_id;  // voxel::Material, passed through for per-material fragment effects

    static bgfx::VertexLayout init() {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)  // RGB color
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color1, 1, bgfx::AttribType::Float)  // AO
            .add(bgfx::Attrib::TexCoord0, 1, bgfx::AttribType::Float)  // Material id
            .end();
        // Deliberately not calling bgfx::createVertexLayout() here:
        // createDynamicVertexBuffer() (the only caller of this function)
        // already registers the layout internally and releases that
        // registration when the buffer is destroyed. An extra explicit
        // createVertexLayout() call here would register a second,
        // unpaired reference with no corresponding release anywhere --
        // this function is called once per chunk built, so that leak grows
        // for the entire session as you explore, unbounded by however many
        // chunks are actually loaded at once.
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
Mesh colored_cube(uint32_t color);
Mesh cube(uint8_t used_faces, const float* ao_values = nullptr, voxel::Material material = voxel::STONE);

Mesh quad(int face, float width, float height);

}  // namespace Render