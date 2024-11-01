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
        5, 7, 3,    1, 5, 3,    // Right face  *
        7, 5, 4,    6, 7, 4,    // Back face
        4, 0, 6,    0, 2, 6,    // Left face
        5, 0, 4,    0, 5, 1,    // Top face 
        2, 3, 6,    6, 3, 7     // Bottom faceit 
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

    /*
    Mesh cube(uint8_t used_faces) {
        Mesh mesh;

        // Initialize vertices
        mesh.vertices = cube_vertices;

        // Normals and face contribution tracking
        std::vector<bx::Vec3> normals(8, bx::Vec3(0.0f, 0.0f, 0.0f));

        // Pre-computed face normals for a cube
        const bx::Vec3 FACE_NORMALS[] = {
            bx::Vec3(0.0f, 0.0f, 1.0f),   // Front face
            bx::Vec3(0.0f, 0.0f, -1.0f),  // Back face
            bx::Vec3(-1.0f, 0.0f, 0.0f),  // Left face
            bx::Vec3(1.0f, 0.0f, 0.0f),   // Right face
            bx::Vec3(0.0f, 1.0f, 0.0f),   // Top face
            bx::Vec3(0.0f, -1.0f, 0.0f)   // Bottom face
        };

        // Generate vertex indices based on used_faces
        for (int i = 0; i < 6; ++i) {
            if (!(used_faces & (1 << i))) continue;

            size_t base_idx = i * 6; 

            mesh.vertex_indices.insert(mesh.vertex_indices.end(), cube_indices.begin() + base_idx, cube_indices.begin() + base_idx + 6);

            bx::Vec3 face_normal = FACE_NORMALS[i];
            for (size_t j = base_idx; j < base_idx + 6; j++) {
                Index vertex_idx = cube_indices[j];
                normals[vertex_idx] = bx::add(normals[vertex_idx], face_normal);
            }
        }

        // Normalize and assign the averaged normals to the vertices
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            bx::Vec3 normalized_normal = bx::normalize(normals[i]);
            
            mesh.vertices[i].nx = normalized_normal.x;
            mesh.vertices[i].ny = normalized_normal.y;
            mesh.vertices[i].nz = normalized_normal.z;
        } 

        return mesh;
    }
    */

    // Mesh cube(uint8_t used_faces) {
    //     Mesh mesh;

    //     // Initialize vertices
    //     mesh.vertices = cube_vertices;

    //     // Normals and face contribution tracking
    //     std::vector<bx::Vec3> normals(8, bx::Vec3(0.0f, 0.0f, 0.0f));
    //     std::vector<int> face_contributions(8, 0);

    //     // Pre-computed face normals for a cube
    //     const bx::Vec3 FACE_NORMALS[] = {
    //         bx::Vec3(0.0f, 0.0f, 1.0f),   // Front face
    //         bx::Vec3(0.0f, 0.0f, -1.0f),  // Back face
    //         bx::Vec3(-1.0f, 0.0f, 0.0f),  // Left face
    //         bx::Vec3(1.0f, 0.0f, 0.0f),   // Right face
    //         bx::Vec3(0.0f, 1.0f, 0.0f),   // Top face
    //         bx::Vec3(0.0f, -1.0f, 0.0f)   // Bottom face
    //     };

    //     // Generate vertex indices based on used_faces
    //     for (int i = 0; i < 6; ++i) {
    //         if (!(used_faces & (1 << i))) continue;

    //         size_t base_idx = i * 6; 

    //         mesh.vertex_indices.insert(mesh.vertex_indices.end(), cube_indices.begin() + base_idx, cube_indices.begin() + base_idx + 6);

    //         // Add face normal contribution to each vertex used in this face
    //         std::set<Index> face_vertices;  // Track unique vertices for this face
    //         for (size_t i = 0; i < 6; ++i) {
    //             face_vertices.insert(cube_indices[base_idx + i]);
    //         }

    //         // bx::Vec3 face_normal = FACE_NORMALS[i];
    //         // for (size_t j = base_idx; j < base_idx + 6; j++) {
    //         //     Index vertex_idx = cube_indices[j];
    //         //     normals[vertex_idx] = bx::add(normals[vertex_idx], face_normal);
    //         // }
    //         // Add normal contribution for each unique vertex in the face
    //         for (Index vertex_idx : face_vertices) {
    //             normals[vertex_idx] = bx::add(normals[vertex_idx], FACE_NORMALS[i]);
    //             face_contributions[vertex_idx]++;
    //         }
    //     }

    //     // Normalize and assign the averaged normals to the vertices
    //     for (size_t i = 0; i < mesh.vertices.size(); ++i) {
    //         // Only process vertices that were actually used
    //         if (face_contributions[i] > 0) {
    //             // Average the normal by dividing by the number of contributing faces
    //             bx::Vec3 averaged_normal = bx::mul(normals[i], 1.0f / static_cast<float>(face_contributions[i]));
    //             bx::Vec3 normalized_normal = bx::normalize(averaged_normal);
                
    //             mesh.vertices[i].nx = normalized_normal.x;
    //             mesh.vertices[i].ny = normalized_normal.y;
    //             mesh.vertices[i].nz = normalized_normal.z;
    //         }
    //     }

    //     return mesh;
    // }

    Mesh cube(uint8_t used_faces) {
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
                
                mesh.vertices.push_back(v);
                
                mesh.vertex_indices.push_back(static_cast<Index>(mesh.vertices.size() - 1));
            }
        }
        
        return mesh;
    }
}