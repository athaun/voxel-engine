#include "geometry.h"
#include <bx/math.h>
#include <iostream>

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

    Mesh cube(float size) {
        Mesh mesh;
        mesh.vertices = {
            // Position              // Texture UVs    // Smoothed Normals (Will be modified below)
            {-size,  size,  size,     1.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 0
            { size,  size,  size,     0.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 1
            {-size, -size,  size,     1.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 2
            { size, -size,  size,     0.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 3
            {-size,  size, -size,     0.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 4
            { size,  size, -size,     1.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 5
            {-size, -size, -size,     0.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 6
            { size, -size, -size,     1.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 7
        };
        // Indices unchanged
        mesh.vertexIndices = {
            0, 1, 2, 1, 3, 2,  // Front face
            4, 6, 5, 5, 6, 7,  // Back face
            0, 2, 4, 4, 2, 6,  // Left face
            1, 5, 3, 5, 7, 3,  // Right face
            0, 4, 1, 4, 5, 1,  // Top face
            2, 3, 6, 6, 3, 7,  // Bottom face
        };
        
    std::vector<bx::Vec3> normals(8, bx::Vec3(0.0f, 0.0f, 0.0f));
    std::vector<bool> face_contributed(8 * 6, false);  // Track which faces have contributed to each vertex

    // Pre-computed face normals for a cube
    const bx::Vec3 FACE_NORMALS[] = {
        bx::Vec3(0.0f, 0.0f, 1.0f),   // Front face
        bx::Vec3(0.0f, 0.0f, -1.0f),  // Back face
        bx::Vec3(-1.0f, 0.0f, 0.0f),  // Left face
        bx::Vec3(1.0f, 0.0f, 0.0f),   // Right face
        bx::Vec3(0.0f, 1.0f, 0.0f),   // Top face
        bx::Vec3(0.0f, -1.0f, 0.0f)   // Bottom face
    };

    // Process each face
    for (size_t face = 0; face < 6; face++) {
        // Get the four indices for this face (two triangles)
        size_t baseIndex = face * 6;  // 6 indices per face (2 triangles * 3 vertices)
        
        // Get the vertices that form this face
        std::vector<uint16_t> face_vertices;
        for (size_t i = 0; i < 6; i++) {
            uint16_t idx = mesh.vertexIndices[baseIndex + i];
            if (std::find(face_vertices.begin(), face_vertices.end(), idx) == face_vertices.end()) {
                face_vertices.push_back(idx);
            }
        }

        // Add the face normal to each vertex of this face
        bx::Vec3 face_normal = FACE_NORMALS[face];
        for (uint16_t vertex_idx : face_vertices) {
            if (!face_contributed[face * 8 + vertex_idx]) {
                normals[vertex_idx] = bx::add(normals[vertex_idx], face_normal);
                face_contributed[face * 8 + vertex_idx] = true;
            }
        }
    }

    // Normalize and assign the averaged normals
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        // Each vertex should have exactly 3 face normals contributing
        bx::Vec3 normalized_normal = bx::normalize(normals[i]);
        
        mesh.vertices[i].nx = normalized_normal.x;
        mesh.vertices[i].ny = normalized_normal.y;
        mesh.vertices[i].nz = normalized_normal.z;
    }

    return mesh;
    }
}