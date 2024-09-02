#include "core/core.h"
#include "core/log.h"

void hello() {
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x330000FF, 1.0f, 0);
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(3, 3, 0x2f, "Hello World!");
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    bgfx::touch(0);
}

int main(int argc, char** argv) {
    Core::init();
    Core::run(hello);
    Core::shutdown();
    return 0;
}