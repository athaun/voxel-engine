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
        6, 7, 2,    7, 3, 2     // Bottom face
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
            
            const Vertex& vert0 = cube_vertices[v0_idx];
            const Vertex& vert1 = cube_vertices[v1_idx];
            const Vertex& vert2 = cube_vertices[v2_idx];
            
            // Calculate face normal using cross product
            // This will produce a vector which points perpendicular 
            // to the plane which is defined by edge1 and 2, which
            // are edges of the face.
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

            // For each vertex in this face
            for (size_t i = 0; i < 6; ++i) {
                Vertex v = cube_vertices[cube_indices[base_idx + i]];
                v.nx = face_normal.x;
                v.ny = face_normal.y;
                v.nz = face_normal.z;
                
                // Apply AO values if provided
                if (ao_values) {
                    // Map the vertex index to the correct AO value
                    // First triangle: 0,1,2
                    // Second triangle: 1,3,2
                    static const int ao_index_map[] = {0,1,2, 1,3,2};
                    v.ao = ao_values[ao_index_map[i]];
                } else {
                    v.ao = 1.0f;
                }
                
                mesh.vertices.push_back(v);
                
                mesh.vertex_indices.push_back(static_cast<Index>(mesh.vertices.size() - 1));
            }
        }
        
        return mesh;
    }
}