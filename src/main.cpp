#include "core/core.h"
#include "core/log.h"
#include "bx/bx.h"
#include <bx/math.h>
#include <iostream>
#include <brtshaderc.h>
#include "render/batch.h"


// Camera position variables
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;
float cameraPosZ = -5.0f;
float movementSpeed = 0.1f; 

float cameraYaw = 0.0f;   // Rotation around the Y axis (horizontal)
float cameraPitch = 0.0f; // Rotation around the X axis (vertical)
float mouseSensitivity = 0.004f; // Change this if you want to modify the sensitivity.

// Define movement vectors
bx::Vec3 forward(0.0f, 0.0f, 1.0f);
bx::Vec3 right(1.0f, 0.0f, 0.0f);


// Make the cube ////////////////////////
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
////////////////////////////////////////

char* absolute_path(const char* relativePath) {
    char* absPath = _fullpath(nullptr, relativePath, _MAX_PATH);
    return absPath;
}


// Load in Shader
bgfx::ShaderHandle loadShader(const char* FILENAME) {

    // Render::Batch b = Render::Batch(10, 10);
    // b.loadShader("vs_batch");

    // compile vertex shader, with default arguments.
        const bgfx::Memory* memVsh = shaderc::compileShader(shaderc::ST_VERTEX, "shader/vs_cubes.sc");
        bgfx::ShaderHandle vsh = bgfx::createShader(memVsh);


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

    // In Window.cpp, mouse position is initialized and defined to be at the center of the screen.
    // Thus, the last known beginning mouse position, or the first mouse position, will be in the same position.
    int lastMouseX = Window::width / 2;
    int lastMouseY = Window::height / 2;

    // Determine how high we want the max to be to move our camera vertically
    // Recommended to be less than 90 or the camera will invert/flip
    const float maxPitch = 89.0f * (3.14159f / 180.0f); // Convert degrees to radians

    while (!Window::should_close()) {
        Window::begin_update();

        // Handle camera movement
        bx::Vec3 forward = { sinf(cameraYaw) * cosf(cameraPitch),
                             sinf(cameraPitch),
                             cosf(cameraYaw) * cosf(cameraPitch) };

        bx::Vec3 right = { cosf(cameraYaw), 0.0f, -sinf(cameraYaw) };  // Right direction (perpendicular to forward)

        // Normalize directions manually
        // (Since bx doesn't have a vec3normalize.....)
        bx::Vec3 normForward = normalize(forward);
        bx::Vec3 normRight = normalize(right);

        // Movement speed applied to direction vectors
        // WASD stuff, with extras :D
        // Change the new forward position accordingly so forward is relative to where the camera is facing with the mouse
        if (Window::is_key_pressed(Window::Key::W)) {
            cameraPosX += normForward.x * movementSpeed;
            cameraPosY += normForward.y * movementSpeed;
            cameraPosZ += normForward.z * movementSpeed;
        }
        if (Window::is_key_pressed(Window::Key::S)) {
            cameraPosX -= normForward.x * movementSpeed;
            cameraPosY -= normForward.y * movementSpeed;
            cameraPosZ -= normForward.z * movementSpeed;
        }
        if (Window::is_key_pressed(Window::Key::A)) {
            cameraPosX -= normRight.x * movementSpeed;
            cameraPosZ -= normRight.z * movementSpeed;
        }
        if (Window::is_key_pressed(Window::Key::D)) {
            cameraPosX += normRight.x * movementSpeed;
            cameraPosZ += normRight.z * movementSpeed;
        }
        // Go Up
        if (Window::is_key_pressed(Window::Key::SPACE)) {
            cameraPosY += movementSpeed;
        }
        // Go Down
        if (Window::is_key_pressed(Window::Key::CTRL)) {
            cameraPosY -= movementSpeed;
        }
        // Escape
        if (Window::is_key_pressed(Window::Key::ESCAPE)) {
            break;
        }

        // Handle mouse position
        float mouseX, mouseY;
        Window::get_mouse_position(mouseX, mouseY);

        // Calculate the delta from the last known mouse position
        int deltaX = static_cast<int>(mouseX - lastMouseX);
        int deltaY = static_cast<int>(mouseY - lastMouseY);

        // Update camera rotation based on mouse movement
        cameraYaw += deltaX * mouseSensitivity;
        cameraPitch -= deltaY * mouseSensitivity;

        // Clamp the pitch to avoid flipping
        // This will stop the camera moving vertically at 89 degrees.
        if (cameraPitch > maxPitch) {
            cameraPitch = maxPitch;
        }
        else if (cameraPitch < -maxPitch) {
            cameraPitch = -maxPitch;
        }

        // Redefine last mouse position to be current before frame ends
        lastMouseX = mouseX;
        lastMouseY = mouseY;

        // Define camera position and target vectors
        bx::Vec3 eye = { cameraPosX, cameraPosY, cameraPosZ };
        bx::Vec3 at = { cameraPosX + sinf(cameraYaw) * cosf(cameraPitch),
                        cameraPosY + sinf(cameraPitch),
                        cameraPosZ + cosf(cameraYaw) * cosf(cameraPitch) };

        // Up vector (typically, this is (0, 1, 0) for a standard upright camera)
        bx::Vec3 up = { 0.0f, 1.0f, 0.0f };

        float view[16];
        bx::mtxLookAt(view, eye, at, up);

        // Set up projection matrix
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