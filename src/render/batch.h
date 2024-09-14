#pragma once

#include <bgfx/bgfx.h>

#include <vector>
#include <cstring>
#include <fstream>
// #include <brtshaderc.h>

struct Vertex {
    float x, y, z; // Position
    float u, v;    // Texture coordinates
    uint32_t color;
    static bgfx::VertexLayout layout; 
    static void init() {
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        bgfx::createVertexLayout(layout);
    }
};


namespace Render {
class Batch {
public:
    Batch(unsigned int maxVertices, unsigned int maxIndices) {
        // this->maxVertices = maxVertices;
        // this->maxIndices = maxIndices;
        // this->usedVertices = 0;
        // this->usedIndices = 0;

        // // Initialize vertex layout
        // Vertex::init();

        // // Create dynamic vertex and index buffers
        // vertexBuffer = bgfx::createDynamicVertexBuffer(maxVertices, Vertex::layout);
        // indexBuffer = bgfx::createDynamicIndexBuffer(maxIndices);

        // // Load shaders
        // program = loadProgram("vs_batch", "fs_batch");

        // // Initialize transform uniform
        // transform = bgfx::createUniform("u_transform", bgfx::UniformType::Mat4);
    }

    ~Batch() {
        // bgfx::destroy(vertexBuffer);
        // bgfx::destroy(indexBuffer);
        // bgfx::destroy(program);
        // bgfx::destroy(transform);
    }

    void begin() {
        // usedVertices = 0;
        // usedIndices = 0;
        // textures.clear();
    }

    void end() {
        // // Submit the batch
        // bgfx::setVertexBuffer(0, vertexBuffer, 0, usedVertices);
        // bgfx::setIndexBuffer(indexBuffer, 0, usedIndices);
        // bgfx::submit(0, program);
    }

    void drawQuad(const Vertex* vertices, const uint16_t* indices, bgfx::TextureHandle texture) {
        // // Update vertex and index buffers
        // bgfx::update(vertexBuffer, usedVertices, bgfx::makeRef(vertices, sizeof(Vertex) * 4));
        // bgfx::update(indexBuffer, usedIndices, bgfx::makeRef(indices, sizeof(uint16_t) * 6));
        // usedVertices += 4;
        // usedIndices += 6;

        // // Add texture to the batch
        // addTexture(texture);
    }

    const bgfx::Memory* loadShader(const char* shaderName) {
        // compile vertex shader, with default arguments.
        // const bgfx::Memory* memVsh =  shaderc::compileShader(shaderc::ST_VERTEX, "shader/vs_cubes.sc");
        // bgfx::ShaderHandle vsh = bgfx::createShader(memVsh);

        // compile fragment shader, with specific arguments for defines, varying def file, shader profile.
        // const bgfx::Memory* memFsh =  shaderc::compileShader(shaderc::ST_FRAGMENT, "shader/fs_cubes.sc", "myDefines", "varying.def.sc", "ps_5_0");
        // bgfx::ShaderHandle fsh = bgfx::createShader(memFsh);

        // build program using shaders
        // auto mProgram = bgfx::createProgram(vsh, fsh, true);

        return nullptr;
    }
private:
    // unsigned int maxVertices;
    // unsigned int maxIndices;
    // unsigned int usedVertices;
    // unsigned int usedIndices;

    // bgfx::DynamicVertexBufferHandle vertexBuffer;
    // bgfx::DynamicIndexBufferHandle indexBuffer;
    // bgfx::ProgramHandle program;
    // bgfx::UniformHandle transform;
    // std::vector<bgfx::TextureHandle> textures;

    bgfx::ProgramHandle loadProgram(const char* vsName, const char* fsName) {
        // const bgfx::Memory* vsMem = loadShader(vsName);
        // const bgfx::Memory* fsMem = loadShader(fsName);
        // bgfx::ShaderHandle vs = bgfx::createShader(vsMem);
        // bgfx::ShaderHandle fs = bgfx::createShader(fsMem);
        // return bgfx::createProgram(vs, fs, true);
    }


    void addTexture(bgfx::TextureHandle texture) {
        // if (std::find(textures.begin(), textures.end(), texture) == textures.end()) {
        //     textures.push_back(texture);
        // }
    }
};
} // namespace Render
