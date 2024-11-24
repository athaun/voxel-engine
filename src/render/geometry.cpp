#include "geometry.h"
#include <bx/math.h>
#include <set>
#include "../core/Log.h"

namespace Render {
    Mesh transform_mesh(Mesh mesh, float x, float y, float z) {
        for (auto& vertex : mesh.vertices) {
            vertex.x += x;
            vertex.y += y;
            vertex.z += z;
        }
        return mesh;
    }

    Mesh scale_mesh(Mesh mesh, float x, float y, float z) {
        for (auto& vertex : mesh.vertices) {
            vertex.x *= x;
            vertex.y *= y;
            vertex.z *= z;
        }
        return mesh;
    }

    // The 8 vertices will look like this:
    //   v4 ----------- v5
    //   /|            /|      Axis orientation
    //  / |           / |
    // v0 --------- v1  |      y
    // |  |         |   |      |
    // |  v6 -------|-- v7     +--- x
    // | /          |  /      /
    // |/           | /      z
    // v2 --------- v3
    std::vector<Index> cube_indices = {
        0, 1, 2,    1, 3, 2,    // Front face  *
        1, 5, 3,    5, 7, 3,    // Right face  *
        5, 4, 7,    4, 6, 7,    // Back face
        4, 0, 6,    0, 2, 6,    // Left face
        4, 5, 0,    5, 1, 0,    // Top face 
        6, 2, 7,    7, 2, 3     // Bottom face
    };

    std::vector<Vertex> cube_vertices = {
        // x     y     z         u     v         nx   ny   nz
        {-0.5,  0.5,  0.5,      1.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 0
        { 0.5,  0.5,  0.5,      0.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 1
        {-0.5, -0.5,  0.5,      1.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 2
        { 0.5, -0.5,  0.5,      0.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 3
        {-0.5,  0.5, -0.5,      0.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 4
        { 0.5,  0.5, -0.5,      1.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 5
        {-0.5, -0.5, -0.5,      0.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 6
        { 0.5, -0.5, -0.5,      1.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 7
    };

    Mesh cube(uint8_t used_faces, const float* ao_values) {
        Mesh mesh;
        mesh.vertices.clear();
        mesh.vertex_indices.clear();

        // Process each face
        for (int face = 0; face < 6; ++face) {
            if (!(used_faces & (1 << face))) continue;

            size_t base_idx = face * 6;
            
            // Calculate face normal using first triangle of the face
            Index v0_idx = cube_indices[base_idx];
            Index v1_idx = cube_indices[base_idx + 1];
            Index v2_idx = cube_indices[base_idx + 2];
            Index v3_idx = cube_indices[base_idx + 4]; // Get the fourth vertex for the quad
            
            const Vertex& vert0 = cube_vertices[v0_idx];
            const Vertex& vert1 = cube_vertices[v1_idx];
            const Vertex& vert2 = cube_vertices[v2_idx];
            
            // Calculate face normal
            bx::Vec3 edge1 = {
                vert1.x - vert0.x,
                vert1.y - vert0.y,
                vert1.z - vert0.z
            };
            
            bx::Vec3 edge2 = {
                vert2.x - vert0.x,
                vert2.y - vert0.y,
                vert2.z - vert0.z
            };
            
            bx::Vec3 face_normal = bx::normalize(bx::cross(edge1, edge2));

            // Get AO values for the quad vertices (clockwise order)
            float a00 = ao_values ? ao_values[0] : 1.0f; // top-left
            float a10 = ao_values ? ao_values[1] : 1.0f; // top-right
            float a01 = ao_values ? ao_values[2] : 1.0f; // bottom-left
            float a11 = ao_values ? ao_values[3] : 1.0f; // bottom-right

            // Determine triangle subdivision based on AO values
            bool flip_diagonal = (a00 + a11) > (a10 + a01);

            // Create vertices for the quad
            std::vector<Vertex> quad_verts(4);
            for(int i = 0; i < 4; i++) {
                Vertex v = cube_vertices[cube_indices[base_idx + (i < 3 ? i : 4)]];
                v.nx = face_normal.x;
                v.ny = face_normal.y;
                v.nz = face_normal.z;

                const auto& color = voxel::MATERIAL_COLORS[voxel::GRASS];
                v.r = color.r;
                v.g = color.g;
                v.b = color.b;
                v.ao = ao_values ? ao_values[i] : 1.0f;
                
                quad_verts[i] = v;
            }

            // Add vertices and indices based on diagonal orientation
            if (flip_diagonal) {
                // Triangle 1: top-left -> bottom-right -> bottom-left
                mesh.vertices.push_back(quad_verts[0]); // top-left
                mesh.vertices.push_back(quad_verts[3]); // bottom-right
                mesh.vertices.push_back(quad_verts[2]); // bottom-left
                
                // Triangle 2: top-left -> top-right -> bottom-right
                mesh.vertices.push_back(quad_verts[0]); // top-left
                mesh.vertices.push_back(quad_verts[1]); // top-right
                mesh.vertices.push_back(quad_verts[3]); // bottom-right
            } else {
                // Original triangle subdivision
                // Triangle 1: top-left -> top-right -> bottom-left
                mesh.vertices.push_back(quad_verts[0]); // top-left
                mesh.vertices.push_back(quad_verts[1]); // top-right
                mesh.vertices.push_back(quad_verts[2]); // bottom-left
                
                // Triangle 2: top-right -> bottom-right -> bottom-left
                mesh.vertices.push_back(quad_verts[1]); // top-right
                mesh.vertices.push_back(quad_verts[3]); // bottom-right
                mesh.vertices.push_back(quad_verts[2]); // bottom-left
            }

            // Add indices for both triangles
            for (int i = 0; i < 6; i++) {
                mesh.vertex_indices.push_back(static_cast<Index>(mesh.vertices.size() - 6 + i));
            }
        }
        
        return mesh;
    }
}