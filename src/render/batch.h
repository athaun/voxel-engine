#pragma once
#include <bgfx/bgfx.h>
#include <bx/math.h>

#include <cstring>
#include <filesystem>
#include <fstream>

#include "geometry.h"
#include "log.h"

namespace Render {

class Batch {
   public:
    Batch(unsigned long max_vertices, unsigned long max_indices, const char *shader_name);
    ~Batch();

    void submit();

    bool push_mesh(Mesh mesh);
    bool push_mesh_buffer(const std::vector<Render::Mesh>& meshes);
    bool push_triangle(Vertex v1, Vertex v2, Vertex v3);

    const bgfx::ProgramHandle load_shader(const char *shader_name);

   
   private:
    uint32_t max_vertices;
    uint32_t max_indices;
    uint32_t used_vertices;
    uint32_t used_indices;

    bgfx::UniformHandle s_texture;
    bgfx::UniformHandle u_ambientColor;

    bgfx::DynamicVertexBufferHandle vertex_buffer;
    bgfx::DynamicIndexBufferHandle index_buffer;

    bgfx::ProgramHandle shader_program;
    
    std::vector<Mesh> meshes;
    };
}  // namespace Render
