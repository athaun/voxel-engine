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
        {-0.5,  0.5,  0.5, 0xff000000}, // 0
        { 0.5,  0.5,  0.5, 0xff0000ff}, // 1
        {-0.5, -0.5,  0.5, 0xff00ff00}, // 2
        { 0.5, -0.5,  0.5, 0xff00ffff}, // 3
        {-0.5,  0.5, -0.5, 0xffff0000}, // 4
        { 0.5,  0.5, -0.5, 0xffff00ff}, // 5
        {-0.5, -0.5, -0.5, 0xffffff00}, // 6
        { 0.5, -0.5, -0.5, 0xffffffff}, // 7
    };


    Mesh cube() {
        Mesh mesh;
        mesh.vertices = cube_vertices;
        mesh.vertex_indices = cube_indices;
        return mesh;
    }

    Mesh colored_cube(uint32_t color) {
        Mesh mesh;
        mesh.vertices = cube_vertices;
        for (auto& vertex : mesh.vertices) {
            vertex.color = color;
        }
        mesh.vertex_indices = cube_indices;
        return mesh;
    }

    Mesh cube(uint8_t used_faces) {
        Mesh mesh;

        mesh.vertices = cube_vertices;
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