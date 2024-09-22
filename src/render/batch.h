#pragma once

#include <bgfx/bgfx.h>

#include <cstring>
#include <filesystem>
#include <fstream>

#include "geometry.h"
#include "log.h"

namespace Render {

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
