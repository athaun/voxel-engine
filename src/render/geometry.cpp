#include "geometry.h"
#include <bx/math.h>
#include "log.h"

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
        {-0.5,  0.5,  0.5, 1.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 0
        { 0.5,  0.5,  0.5, 0.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 1
        {-0.5, -0.5,  0.5, 1.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 2
        { 0.5, -0.5,  0.5, 0.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 3
        {-0.5,  0.5, -0.5, 0.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 4
        { 0.5,  0.5, -0.5, 1.0f, 0.0f,      0.0f, 0.0f, 0.0f}, // 5
        {-0.5, -0.5, -0.5, 0.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 6
        { 0.5, -0.5, -0.5, 1.0f, 1.0f,      0.0f, 0.0f, 0.0f}, // 7
    };


    Mesh cube(uint8_t used_faces) {
        Mesh mesh;

        // Initialize vertices
        mesh.vertices = cube_vertices;
        
        // Clear existing vertex indices
        mesh.vertex_indices.clear();

        // Normals and face contribution tracking
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

        // Process each face based on used_faces
        for (size_t face = 0; face < 6; face++) {
            if (!(used_faces & (1 << face))) continue;  // Skip if face not used

            // Get the four indices for this face (two triangles)
            size_t baseIndex = face * 6;  // 6 indices per face (2 triangles * 3 vertices)

            // Get the vertices that form this face
            std::vector<uint16_t> face_vertices;
            for (size_t i = 0; i < 6; i++) {
                uint16_t idx = cube_indices[baseIndex + i];
                if (std::find(face_vertices.begin(), face_vertices.end(), idx) == face_vertices.end()) {
                    face_vertices.push_back(idx);
                }
            }

            // Add the face normal to each vertex of this face
            bx::Vec3 face_normal = FACE_NORMALS[face];
            for (uint16_t vertex_idx : face_vertices) {
                normals[vertex_idx] = bx::add(normals[vertex_idx], face_normal);
                face_contributed[face * 8 + vertex_idx] = true;
            }
        }

        // Normalize and assign the averaged normals to the vertices
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            bx::Vec3 normalized_normal = bx::normalize(normals[i]);
            
            mesh.vertices[i].nx = normalized_normal.x;
            mesh.vertices[i].ny = normalized_normal.y;
            mesh.vertices[i].nz = normalized_normal.z;
        }

        // Generate vertex indices based on used_faces
        for (int i = 0; i < 6; ++i) {
            if (!(used_faces & (1 << i))) continue;

            switch (i) {
                case 0: // Front face
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), cube_indices.begin(), cube_indices.begin() + 6);
                    break;
                case 1: // Right face
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), cube_indices.begin() + 6, cube_indices.begin() + 12);
                    break;
                case 2: // Back face
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), cube_indices.begin() + 12, cube_indices.begin() + 18);
                    break;
                case 3: // Left face
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), cube_indices.begin() + 18, cube_indices.begin() + 24);
                    break;
                case 4: // Top face
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), cube_indices.begin() + 24, cube_indices.begin() + 30);
                    break;
                case 5: // Bottom face
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), cube_indices.begin() + 30, cube_indices.end());
                    break;
            }
        }

        return mesh;
    }
}