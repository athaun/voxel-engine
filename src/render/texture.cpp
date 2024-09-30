#include <bgfx/bgfx.h>
#include <fstream>
#include <iostream>
#include <vector>
#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

bgfx::TextureHandle loadTexture(const char* filePath) {
    // Load the image using stb_image or another image loading library
    int width, height, channels;
    unsigned char* imageData = stbi_load(filePath, &width, &height, &channels, STBI_rgb_alpha); // Ensure 4 channels (RGBA)
    if (!imageData) {
        std::cout << "BAD";
        return BGFX_INVALID_HANDLE;
    }

    // Calculate the size of the texture data (RGBA means 4 bytes per pixel)
    const bgfx::Memory* mem = bgfx::copy(imageData, width * height * 4);

    // Free the image data as it's now copied into bgfx memory
    stbi_image_free(imageData);

    // Create the texture
    bgfx::TextureHandle texture = bgfx::createTexture2D(
        (uint16_t)width,
        (uint16_t)height,
        false, // No mip-maps
        1,     // Number of layers (1 for 2D textures)
        bgfx::TextureFormat::RGBA8, // Format of the texture
        0,     // Texture flags
        mem    // Texture data
    );

    if (!bgfx::isValid(texture)) {
        std::cout << "NO TEXTURE VALID";
        return BGFX_INVALID_HANDLE;
    }

    return texture;
    
}

bgfx::TextureHandle createSolidColorTexture(uint32_t color, int width = 16, int height = 16) {
    // Create a buffer to hold the color data (RGBA format, 4 bytes per pixel)
    std::vector<uint32_t> colorData(width * height, color);

    // Copy the color data into bgfx's memory structure
    const bgfx::Memory* mem = bgfx::copy(colorData.data(), width * height * sizeof(uint32_t));

    // Create a 2D texture using the color data
    bgfx::TextureHandle texture = bgfx::createTexture2D(
        (uint16_t)width,
        (uint16_t)height,
        false, // No mipmaps
        1,     // Number of layers
        bgfx::TextureFormat::RGBA8, // 4 bytes per pixel (RGBA)
        0,     // Texture flags
        mem    // The actual texture data
    );

    if (!bgfx::isValid(texture)) {
        std::cout << "Failed to create solid color texture." << std::endl;
        return BGFX_INVALID_HANDLE;
    }

    return texture;
}

