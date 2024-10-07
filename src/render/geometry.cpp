#include "geometry.h"

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
            // Position                // Texture coordinates // Normal
            {-size,  size,  size, 1.0f, 1.0f,  0.0f,  0.0f,  1.0f},  // Front face
            { size,  size,  size, 0.0f, 1.0f,  0.0f,  0.0f,  1.0f},
            {-size, -size,  size, 1.0f, 0.0f,  0.0f,  0.0f,  1.0f},
            { size, -size,  size, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f},
            {-size,  size, -size, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f},  // Back face
            { size,  size, -size, 1.0f, 0.0f,  0.0f,  0.0f, -1.0f},
            {-size, -size, -size, 0.0f, 1.0f,  0.0f,  0.0f, -1.0f},
            { size, -size, -size, 1.0f, 1.0f,  0.0f,  0.0f, -1.0f},
        };
        mesh.vertexIndices = {
            0, 1, 2, 1, 3, 2,  // Front face
            4, 6, 5, 5, 6, 7,  // Back face
            0, 2, 4, 4, 2, 6,  // Left face
            1, 5, 3, 5, 7, 3,  // Right face
            0, 4, 1, 4, 5, 1,  // Top face
            2, 3, 6, 6, 3, 7,  // Bottom face
        };
        return mesh;
    }
}