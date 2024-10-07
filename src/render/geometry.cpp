#include "geometry.h"
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

    std::vector<Vertex> cube_vertices = {
        {-0.5,  0.5,  0.5, 0xff000000},
        { 0.5,  0.5,  0.5, 0xff0000ff},
        {-0.5, -0.5,  0.5, 0xff00ff00},
        { 0.5, -0.5,  0.5, 0xff00ffff},
        {-0.5,  0.5, -0.5, 0xffff0000},
        { 0.5,  0.5, -0.5, 0xffff00ff},
        {-0.5, -0.5, -0.5, 0xffffff00},
        { 0.5, -0.5, -0.5, 0xffffffff},
    };

    std::vector<Index> cube_indices = {
        0, 1, 2, 1, 3, 2, // 0 Front  (Z+)
        4, 6, 5, 5, 6, 7, // 1 Back   (Z-)
        0, 2, 4, 4, 2, 6, // 2 Left   (X-)
        1, 5, 3, 5, 7, 3, // 3 Right  (X+)
        0, 4, 1, 4, 5, 1, // 4 Top    (Y+)
        2, 3, 6, 6, 3, 7, // 5 Bottom (Y-)
    };

    Mesh cube() {
        Mesh mesh;
        mesh.vertices = cube_vertices;
        mesh.vertex_indices = cube_indices;
        return mesh;
    }

    Mesh colored_cube(uint32_t color) {
        Mesh mesh;
        mesh.vertices = {
            {-0.5,  0.5,  0.5, color},
            { 0.5,  0.5,  0.5, color},
            {-0.5, -0.5,  0.5, color},
            { 0.5, -0.5,  0.5, color},
            {-0.5,  0.5, -0.5, color},
            { 0.5,  0.5, -0.5, color},
            {-0.5, -0.5, -0.5, color},
            { 0.5, -0.5, -0.5, color},
        };
        mesh.vertex_indices = cube_indices;
        return mesh;
    }

    Mesh cube(uint8_t used_faces) {
        Mesh mesh;

        mesh.vertices = cube_vertices;
        for (int i = 0; i < 6; ++i) {
            if (!(used_faces & (1 << i))) continue;

            switch (i) {
                case 0: // Front
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), {0, 1, 2, 1, 3, 2});
                    // mesh.vertices[0].color = 0xFF00FF00;
                    // mesh.vertices[1].color = 0xFF00FF01;
                    // mesh.vertices[2].color = 0xFF00FF02;
                    // mesh.vertices[3].color = 0xFF00FF03;
                    break;
                case 1: // Back
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), {4, 6, 5, 5, 6, 7});
                    break;
                case 2: // Left
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), {0, 2, 4, 4, 2, 6});
                    break;
                case 3: // Right
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), {1, 5, 3, 5, 7, 3});
                    break;
                case 4: // Bottom
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), {0, 4, 1, 4, 5, 1});
                    break;
                case 5: // Top
                    mesh.vertex_indices.insert(mesh.vertex_indices.end(), {2, 3, 6, 6, 3, 7});
                    break;
            }
        }

        return mesh;
    }
}