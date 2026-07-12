#include "chunk.h"
#include "bitcolumn.h"
#include <algorithm>
#include <array>

namespace {
    // Wide enough to hold any of the chunk's 3 axes plus 1 bit of padding on
    // each end (used to detect faces exposed at the chunk boundary).
    using Col = BitOps::BitColumn<CHUNK_HEIGHT + 2>;

    // World-space (x,y,z) for a given (width, height, normal) triple, per
    // direction's axis roles. Mirrors the scale_mesh axis mapping in
    // Chunk::generate_mesh_from_faces.
    std::array<int, 3> world_pos(uint8_t direction, int w, int h, int n) {
        switch (direction) {
            case 0: case 2: return {w, h, n};  // Front/Back: normal = Z
            case 1: case 3: return {n, h, w};  // Right/Left: normal = X, width = Z
            default:        return {w, n, h};  // Top/Bottom: normal = Y, height = Z
        }
    }

    // Whether corner `i` (as returned by Chunk::calculate_face_ao: 0 =
    // top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right) sits at the
    // max end of the run's width/height axis, per direction.
    const bool WIDTH_MAX[6][4] = {
        {false, true,  false, true},   // 0 Front (+Z)
        {true,  false, true,  false},  // 1 Right (+X)
        {true,  false, true,  false},  // 2 Back (-Z)
        {false, true,  false, true},   // 3 Left (-X)
        {false, true,  false, true},   // 4 Top (+Y)
        {false, true,  false, true},   // 5 Bottom (-Y)
    };
    const bool HEIGHT_MAX[6][4] = {
        {true,  true,  false, false},  // 0
        {true,  true,  false, false},  // 1
        {true,  true,  false, false},  // 2
        {true,  true,  false, false},  // 3
        {false, false, true,  true},   // 4
        {false, false, true,  true},   // 5
    };
}

// Whether the voxel at (x,y,z) counts as opaque for meshing purposes, i.e.
// blocks a neighboring face regardless of what that neighbor is. Water is
// deliberately excluded -- it's translucent, so a face between solid ground
// and the water sitting on top of it still needs to be meshed (the ground
// renders through). Interior positions read the real voxel array; positions
// exactly one step outside the chunk fall back to the analytic
// terrain-height buffer, since no neighbor Chunk's voxel data is guaranteed
// to exist yet.
bool Chunk::is_opaque_for_mesh(int x, int y, int z) {
    if (y < 0 || y >= CHUNK_HEIGHT) return false; // open sky above/below the world

    if (x >= 0 && x < CHUNK_WIDTH && z >= 0 && z < CHUNK_DEPTH) {
        voxel::Material mat = get_voxel(x, y, z).material;
        return mat != voxel::AIR && mat != voxel::WATER;
    }

    if (x == -1 || x == CHUNK_WIDTH || z == -1 || z == CHUNK_DEPTH) {
        return y <= buffered_terrain_height(x, z);
    }

    return false;
}

// Whether the voxel at (x,y,z) is occupied by anything at all, opaque or
// water -- used to tell whether a WATER face should be exposed (only
// against bare air; water sitting on water or on the lakebed both stay
// internal). The chunk-boundary fallback has to account for the sea-level
// water fill on top of the raw terrain height here, or a lake that
// continues into a not-yet-built neighbor chunk reads as open air there and
// grows a spurious water wall right at the chunk seam.
bool Chunk::is_any_for_mesh(int x, int y, int z) {
    if (y < 0 || y >= CHUNK_HEIGHT) return false;

    if (x >= 0 && x < CHUNK_WIDTH && z >= 0 && z < CHUNK_DEPTH) {
        return get_voxel(x, y, z).material != voxel::AIR;
    }

    if (x == -1 || x == CHUNK_WIDTH || z == -1 || z == CHUNK_DEPTH) {
        return y <= std::max(buffered_terrain_height(x, z), SEA_LEVEL);
    }

    return false;
}

// Binary occupancy + greedy merge for one of the 6 face directions. Builds a
// packed bitmask column per (width, height) position so an entire column's
// face exposure comes from a couple of word ops, then sweeps layer by layer
// along the normal axis doing a classic 2D greedy rectangle merge (grouped
// by material) over the exposed cells.
void Chunk::mesh_direction(uint8_t direction, std::vector<Face>& faces) {
    int Wdim, Hdim, Ndim;
    bool positive;

    switch (direction) {
        case 0: Wdim = CHUNK_WIDTH; Hdim = CHUNK_HEIGHT; Ndim = CHUNK_DEPTH;  positive = true;  break; // Front +Z
        case 1: Wdim = CHUNK_DEPTH; Hdim = CHUNK_HEIGHT; Ndim = CHUNK_WIDTH; positive = true;  break; // Right +X
        case 2: Wdim = CHUNK_WIDTH; Hdim = CHUNK_HEIGHT; Ndim = CHUNK_DEPTH;  positive = false; break; // Back -Z
        case 3: Wdim = CHUNK_DEPTH; Hdim = CHUNK_HEIGHT; Ndim = CHUNK_WIDTH; positive = false; break; // Left -X
        case 4: Wdim = CHUNK_WIDTH; Hdim = CHUNK_DEPTH;  Ndim = CHUNK_HEIGHT; positive = true;  break; // Top +Y
        default: Wdim = CHUNK_WIDTH; Hdim = CHUNK_DEPTH; Ndim = CHUNK_HEIGHT; positive = false; break; // Bottom -Y
    }

    const size_t plane = static_cast<size_t>(Wdim) * static_cast<size_t>(Hdim);

    std::vector<Col> exposure(plane);
    Col active_layers; // union of every column's exposure, so the layer
                        // sweep below can skip layers nothing faces on.
    for (int w = 0; w < Wdim; ++w) {
        for (int h = 0; h < Hdim; ++h) {
            Col opaqueCol, anyCol;
            for (int n = -1; n <= Ndim; ++n) {
                auto p = world_pos(direction, w, h, n);
                if (is_opaque_for_mesh(p[0], p[1], p[2])) opaqueCol.set(n + 1);
                if (is_any_for_mesh(p[0], p[1], p[2])) anyCol.set(n + 1);
            }

            // Opaque voxels expose a face against anything non-opaque (air
            // or water). Water only exposes a face against bare air -- the
            // `anyCol` shift (rather than a water-only shift) is what keeps
            // water from rendering a face against the ground it's resting
            // on.
            Col waterCol = anyCol & ~opaqueCol;
            Col opaqueExposure = positive ? (opaqueCol & ~opaqueCol.shiftDown()) : (opaqueCol & ~opaqueCol.shiftUp());
            Col waterExposure  = positive ? (waterCol  & ~anyCol.shiftDown())    : (waterCol  & ~anyCol.shiftUp());

            Col& exp = exposure[w * Hdim + h];
            exp = opaqueExposure | waterExposure;
            active_layers = active_layers | exp;
        }
    }

    std::vector<uint8_t> visited(plane);
    std::vector<uint8_t> hasFace(plane);
    std::vector<voxel::Material> materialAt(plane);
    std::vector<std::array<float, 4>> aoAt(plane);

    auto ao_matches = [](const std::array<float, 4>& a, const std::array<float, 4>& b) {
        for (int i = 0; i < 4; ++i) {
            if (std::abs(a[i] - b[i]) > 0.001f) return false;
        }
        return true;
    };

    active_layers.forEachSetBit([&](int padded_n) {
        const int n = padded_n - 1;
        if (n < 0 || n >= Ndim) return; // padding bit from the chunk-edge fallback, not a real layer

        std::fill(visited.begin(), visited.end(), 0);

        for (int w = 0; w < Wdim; ++w) {
            for (int h = 0; h < Hdim; ++h) {
                size_t idx = w * Hdim + h;
                bool exposed = exposure[idx].test(n + 1);
                hasFace[idx] = exposed;
                if (exposed) {
                    auto p = world_pos(direction, w, h, n);
                    materialAt[idx] = get_voxel(p[0], p[1], p[2]).material;
                    // Own AO joins material as part of the merge key below --
                    // merging across an AO transition would linearly smear it
                    // into a long gradient/seam across the whole merged quad
                    // (Render::cube only splits a quad into 2 triangles), so
                    // runs must stay within a region of uniform shading.
                    calculate_face_ao(p[0], p[1], p[2], direction, aoAt[idx].data());
                }
            }
        }

        for (int h0 = 0; h0 < Hdim; ++h0) {
            for (int w0 = 0; w0 < Wdim; ++w0) {
                size_t idx0 = w0 * Hdim + h0;
                if (visited[idx0] || !hasFace[idx0]) continue;

                voxel::Material mat = materialAt[idx0];
                const std::array<float, 4>& ao = aoAt[idx0];

                int w1 = w0 + 1;
                while (w1 < Wdim) {
                    size_t idx = w1 * Hdim + h0;
                    if (visited[idx] || !hasFace[idx] || materialAt[idx] != mat || !ao_matches(aoAt[idx], ao)) break;
                    ++w1;
                }

                int h1 = h0 + 1;
                while (h1 < Hdim) {
                    bool row_matches = true;
                    for (int w = w0; w < w1; ++w) {
                        size_t idx = w * Hdim + h1;
                        if (visited[idx] || !hasFace[idx] || materialAt[idx] != mat || !ao_matches(aoAt[idx], ao)) { row_matches = false; break; }
                    }
                    if (!row_matches) break;
                    ++h1;
                }

                for (int h = h0; h < h1; ++h) {
                    for (int w = w0; w < w1; ++w) {
                        visited[w * Hdim + h] = 1;
                    }
                }

                auto origin = world_pos(direction, w0, h0, n);
                Face face(origin[0], origin[1], origin[2], direction);
                face.width = w1 - w0;
                face.height = h1 - h0;
                calculate_merged_face_ao(w0, h0, w1 - 1, h1 - 1, n, direction, face.ao);
                faces.push_back(face);
            }
        }
    });
}

// Recomputes the merged quad's 4 corner AO values by reusing the existing
// per-voxel calculate_face_ao at whichever end-voxel of the run each corner
// actually sits on. A 1x1 run degenerates to exactly the original per-voxel
// result, so merging never changes shading outside the run's outer edge.
void Chunk::calculate_merged_face_ao(int w0, int h0, int w1, int h1, int normal, uint8_t direction, float ao_out[4]) {
    bool have_cache = false;
    int cache_x = 0, cache_y = 0, cache_z = 0;
    float cached[4];

    for (int i = 0; i < 4; ++i) {
        int w = WIDTH_MAX[direction][i] ? w1 : w0;
        int h = HEIGHT_MAX[direction][i] ? h1 : h0;
        auto p = world_pos(direction, w, h, normal);

        if (!have_cache || p[0] != cache_x || p[1] != cache_y || p[2] != cache_z) {
            calculate_face_ao(p[0], p[1], p[2], direction, cached);
            cache_x = p[0]; cache_y = p[1]; cache_z = p[2];
            have_cache = true;
        }
        ao_out[i] = cached[i];
    }
}

void Chunk::generate_greedy_mesh() {
    std::vector<Face> faces;
    faces.reserve(256);

    for (uint8_t direction = 0; direction < 6; ++direction) {
        mesh_direction(direction, faces);
    }

    // Water is meshed and batched separately from opaque terrain so it can
    // be drawn with a transparent, depth-write-disabled render state after
    // all opaque geometry (see Batch's `transparent` flag and
    // ChunkManager::render()'s two-pass submission).
    std::vector<Face> opaque_faces, water_faces;
    opaque_faces.reserve(faces.size());
    for (const Face& face : faces) {
        if (get_voxel(face.x, face.y, face.z).material == voxel::WATER) {
            water_faces.push_back(face);
        } else {
            opaque_faces.push_back(face);
        }
    }

    // CPU-side only -- no bgfx calls here. This runs on a worker thread;
    // GPU buffer creation happens later on the main thread via
    // upload_to_gpu(), see ChunkManager's upload queue.
    pending_opaque_mesh = generate_mesh_from_faces(opaque_faces);
    if (!water_faces.empty()) {
        pending_water_mesh = generate_mesh_from_faces(water_faces);
        has_water_mesh = true;
    }
}

void Chunk::upload_to_gpu() {
    const uint32_t vertex_count = std::max<uint32_t>(1, static_cast<uint32_t>(pending_opaque_mesh.vertices.size()));
    const uint32_t index_count = std::max<uint32_t>(1, static_cast<uint32_t>(pending_opaque_mesh.vertex_indices.size()));

    batch = new Render::Batch(vertex_count, index_count, "cubes");
    if (!pending_opaque_mesh.vertices.empty()) {
        batch->push_mesh(pending_opaque_mesh);
    }

    if (has_water_mesh) {
        const uint32_t water_vertex_count = static_cast<uint32_t>(pending_water_mesh.vertices.size());
        const uint32_t water_index_count = static_cast<uint32_t>(pending_water_mesh.vertex_indices.size());

        water_batch = new Render::Batch(water_vertex_count, water_index_count, "water", /*transparent=*/true);
        water_batch->push_mesh(pending_water_mesh);
    }

    pending_opaque_mesh = Render::Mesh();
    pending_water_mesh = Render::Mesh();
}

Render::Mesh Chunk::generate_mesh_from_faces(const std::vector<Face>& faces) {
    Render::Mesh combined;

    for (const Face& face : faces) {
        Render::Mesh m = Render::cube(1 << face.direction, face.ao, get_voxel(face.x, face.y, face.z).material);

        float center_x = static_cast<float>(global_x + face.x);
        float center_y = static_cast<float>(face.y);
        float center_z = static_cast<float>(global_z + face.z);

        switch (face.direction) {
            case 0: case 2: // Front/Back: width = X, height = Y
                m = Render::scale_mesh(m, static_cast<float>(face.width), static_cast<float>(face.height), 1.0f);
                center_x += (face.width - 1) / 2.0f;
                center_y += (face.height - 1) / 2.0f;
                break;
            case 1: case 3: // Right/Left: width = Z, height = Y
                m = Render::scale_mesh(m, 1.0f, static_cast<float>(face.height), static_cast<float>(face.width));
                center_z += (face.width - 1) / 2.0f;
                center_y += (face.height - 1) / 2.0f;
                break;
            default: // Top/Bottom: width = X, height = Z
                m = Render::scale_mesh(m, static_cast<float>(face.width), 1.0f, static_cast<float>(face.height));
                center_x += (face.width - 1) / 2.0f;
                center_z += (face.height - 1) / 2.0f;
                break;
        }

        m = Render::transform_mesh(m, center_x, center_y, center_z);

        const uint32_t base = static_cast<uint32_t>(combined.vertices.size());
        combined.vertices.insert(combined.vertices.end(), m.vertices.begin(), m.vertices.end());
        for (Render::Index idx : m.vertex_indices) {
            combined.vertex_indices.push_back(idx + base);
        }
    }

    return combined;
}
