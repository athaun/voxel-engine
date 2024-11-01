#pragma once

#include <bgfx/bgfx.h>
#include "log.h"

// Function to load a texture from a file
bgfx::TextureHandle loadTexture(const char* filePath);

bgfx::TextureHandle createSolidColorTexture(uint32_t color, int width, int height);


// Internal function to load memory from a file (not intended for external use)
const bgfx::Memory* loadMem(const char* filePath);
