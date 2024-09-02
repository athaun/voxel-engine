#include "window.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <GLFW/glfw3.h>
#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#include "log.h"
#include <iostream>

namespace Window {
    static GLFWwindow* window = nullptr;

    int width = 0;
    int height = 0;

    void init() {
        // Initialize GLFW for windowing
        if (!glfwInit()) {
            Log::error("Failed to initialize GLFW");
            return;
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(1920, 1080, "Hello World", nullptr, nullptr);
        if (!window) {
            Log::error("Failed to create window");
            return;
        }

        // Initialize BGFX for rendering
        bgfx::Init bgfxInit;
        bgfxInit.type = bgfx::RendererType::Count;
        bgfxInit.resolution.width = 1920;
        bgfxInit.resolution.height = 1080;
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;

        #if BX_PLATFORM_WINDOWS
            bgfxInit.platformData.nwh = glfwGetWin32Window(window);
        #elif BX_PLATFORM_LINUX
            bgfxInit.platformData.ndt = glfwGetX11Display();
            bgfxInit.platformData.nwh = (void*)(uintptr_t)glfwGetX11Window(window);
        #elif BX_PLATFORM_OSX
            bgfxInit.platformData.nwh = glfwGetCocoaWindow(window);
        #endif

        if (!bgfx::init(bgfxInit)) {
            Log::error("Failed to initialize BGFX");
            return;
        }

	    bgfx::setViewClear(0, BGFX_CLEAR_COLOR);

        glfwGetWindowSize(window, &Window::width, &Window::height);
        std::cout << Window::width << "\n";
    }

    void shutdown() {
        glfwDestroyWindow(window);
        glfwTerminate();

        // bgfx::shutdown(); // should come back to this
    }

    void begin_update() {
        glfwPollEvents();

		// int oldWidth = width, oldHeight = height;
		// glfwGetWindowSize(window, &Window::width, &Window::height);
		// if (width != oldWidth || height != oldHeight) {
		// 	// Resize window :o
		// }
    }

    void end_update() {
        bgfx::frame();
    }

    bool should_close() {
        return glfwWindowShouldClose(window);
    }
}