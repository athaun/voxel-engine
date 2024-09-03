#include "core/core.h"
#include "core/log.h"
#include "bx/bx.h"
#include <bx/math.h>
#include <iostream>

// Camera position variables
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;
float cameraPosZ = -5.0f;
float movementSpeed = 0.1f; 

struct PosColorVertex
{
    float x;
    float y;
    float z;
    uint32_t abgr;
};

static PosColorVertex cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff000000 },
    { 1.0f,  1.0f,  1.0f, 0xff0000ff },
    {-1.0f, -1.0f,  1.0f, 0xff00ff00 },
    { 1.0f, -1.0f,  1.0f, 0xff00ffff },
    {-1.0f,  1.0f, -1.0f, 0xffff0000 },
    { 1.0f,  1.0f, -1.0f, 0xffff00ff },
    {-1.0f, -1.0f, -1.0f, 0xffffff00 },
    { 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t cubeTriList[] =
{
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

char* absolute_path(const char* relativePath) {
    char* absPath = _fullpath(nullptr, relativePath, _MAX_PATH);
    return absPath;
}

bgfx::ShaderHandle loadShader(const char* FILENAME) {
    const char* shaderPath = nullptr;

    switch (bgfx::getRendererType()) {
    case bgfx::RendererType::Noop:
    case bgfx::RendererType::Direct3D9:  shaderPath = "src/shaders/dx9/";   break;
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12: shaderPath = "src/shaders/dx11/";  break;
    case bgfx::RendererType::Gnm:        shaderPath = "src/shaders/pssl/";  break;
    case bgfx::RendererType::Metal:      shaderPath = "src/shaders/metal/"; break;
    case bgfx::RendererType::OpenGL:     shaderPath = "src/shaders/glsl/";  break;
    case bgfx::RendererType::OpenGLES:   shaderPath = "src/shaders/essl/";  break;
    case bgfx::RendererType::Vulkan:     shaderPath = "src/shaders/spirv/"; break;
    }

    size_t shaderLen = strlen(shaderPath);
    size_t fileLen = strlen(FILENAME);
    char* relativePath = (char*)malloc(shaderLen + fileLen + 1);
    memcpy(relativePath, shaderPath, shaderLen);
    memcpy(&relativePath[shaderLen], FILENAME, fileLen);
    relativePath[shaderLen + fileLen] = '\0'; // null terminator

    char* filePath = absolute_path(relativePath);
    free(relativePath);

    Log::info(filePath);
    FILE* file = fopen(filePath, "rb");
    if (!file) {
        Log::error("Failed to open file:");
        Log::error(filePath);
        free(filePath);
        return BGFX_INVALID_HANDLE;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    const bgfx::Memory* mem = bgfx::alloc(fileSize + 1);
    fread(mem->data, 1, fileSize, file);
    mem->data[mem->size - 1] = '\0';
    fclose(file);

    free(filePath);

    return bgfx::createShader(mem);
}

int main(int argc, char** argv) {
    Core::init();

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, Window::width, Window::height);

    unsigned int counter = 0;

    bgfx::VertexLayout pcvDecl;
    pcvDecl.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), pcvDecl);
    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));


    bgfx::ShaderHandle vsh = loadShader("vs_cubes.bin");
    bgfx::ShaderHandle fsh = loadShader("fs_cubes.bin");
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);


    while (!Window::should_close()) {
        Window::begin_update();

        // Handle camera movement
        if (Window::is_key_pressed(Window::Key::W)) {
            cameraPosZ += movementSpeed;
        }
        if (Window::is_key_pressed(Window::Key::S)) {
            cameraPosZ -= movementSpeed;
        }
        if (Window::is_key_pressed(Window::Key::A)) {
            cameraPosX -= movementSpeed;
        }
        if (Window::is_key_pressed(Window::Key::D)) {
            cameraPosX += movementSpeed;
        }
        if (Window::is_key_pressed(Window::Key::SPACE)) {
            cameraPosY += movementSpeed;
        }
        // LEFT CTRL, btw.
        if (Window::is_key_pressed(Window::Key::CTRL)) {
            cameraPosY -= movementSpeed;
        }
        if (Window::is_key_pressed(Window::Key::ESCAPE)) {
            break;
        }

        // Set up view matrix
        const bx::Vec3 at = { cameraPosX, cameraPosY, cameraPosZ + 5.0f };
        const bx::Vec3 eye = { cameraPosX, cameraPosY, cameraPosZ };
        float view[16];
        bx::mtxLookAt(view, eye, at);

        //// Set up projection matrix
        float proj[16];
        bx::mtxProj(proj, 60.0f, float(Window::width) / float(Window::height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, view, proj);

        float mtx[16];
        bx::mtxRotateXY(mtx, counter * 0.01f, counter * 0.01f);
        bgfx::setTransform(mtx); // Apply rotation to the cube

        // Render the scene
        bgfx::setVertexBuffer(0, vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::submit(0, program);
        bgfx::frame();
        counter++;

        Window::end_update();
    }
    Core::shutdown();
    return 0;
}