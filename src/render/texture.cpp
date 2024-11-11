#include <bgfx/bgfx.h>
#include <fstream>
#include <iostream>
#include <vector>
#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

bgfx::TextureHandle load_texture(const char* filePath, uint32_t flags) {
    // Load the image using stb_image
    int width, height, channels;
    char absPath[PATH_MAX];
    if (realpath(filePath, absPath) == NULL) {
        std::cout << "Failed to resolve path: " << filePath << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    unsigned char* imageData = stbi_load(absPath, &width, &height, &channels, STBI_rgb_alpha);
    if (!imageData) {
        std::cout << "Failed to load texture: " << filePath << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    // Calculate the size of the texture data (RGBA means 4 bytes per pixel)
    const bgfx::Memory* mem = bgfx::copy(imageData, width * height * 4);

    // Free the image data as it's now copied into bgfx memory
    stbi_image_free(imageData);

    // Create the texture with the provided flags
    bgfx::TextureHandle texture = bgfx::createTexture2D(
        (uint16_t)width,
        (uint16_t)height,
        false,  // No mip-maps
        1,      // Number of layers
        bgfx::TextureFormat::RGBA8,
        flags,
        mem
    );

    if (!bgfx::isValid(texture)) {
        std::cout << "Failed to create texture handle for: " << filePath << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    return texture;
}

