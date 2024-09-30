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
            {-size,  size,  size, 0xff000000, 0.0f, 0.0f},  // Add u, v for each vertex
            { size,  size,  size, 0xff0000ff, 1.0f, 0.0f},
            {-size, -size,  size, 0xff00ff00, 0.0f, 1.0f},
            { size, -size,  size, 0xff00ffff, 1.0f, 1.0f},
            {-size,  size, -size, 0xffff0000, 0.0f, 0.0f},
            { size,  size, -size, 0xffff00ff, 1.0f, 0.0f},
            {-size, -size, -size, 0xffffff00, 0.0f, 1.0f},
            { size, -size, -size, 0xffffffff, 1.0f, 1.0f},
        };
        mesh.vertexIndices = {
            0, 1, 2,
            1, 3, 2,
            4, 6, 5,
            5, 6, 7,
            0, 2, 4,
            4, 2, 6,
            1, 5, 3,
            5, 7, 3,
            0, 4, 1,
            4, 5, 1,
            2, 3, 6,
            6, 3, 7,
        };
        return mesh;
    }
}