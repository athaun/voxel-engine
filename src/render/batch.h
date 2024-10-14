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
   private:
    unsigned long max_vertices;
    unsigned long max_indices;
    unsigned long used_vertices;
    unsigned long used_indices;

    bgfx::UniformHandle s_texture;
    
    bgfx::UniformHandle u_ambientColor;
    bgfx::UniformHandle u_lightDirection;

    bgfx::DynamicVertexBufferHandle vertex_buffer;
    bgfx::DynamicIndexBufferHandle index_buffer;

    bgfx::ProgramHandle shader_program;
    bgfx::TextureHandle grassTexture;
    

    std::vector<Mesh> meshes;

   public:
    Batch(unsigned long max_vertices, unsigned long max_indices, const char *shader_name);
    ~Batch();

    void submit();

    bool push_mesh (Mesh mesh);

    bool push_triangle(Vertex v1, Vertex v2, Vertex v3);

    void setLightDirection(const bx::Vec3& direction);
   
   private:
    const bgfx::ProgramHandle load_shader(const char *shader_name);
};
}  // namespace Render
