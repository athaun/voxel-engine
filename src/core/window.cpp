#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <GLFW/glfw3.h>
#include <bx/platform.h>
#include <iostream>
#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include "window.h"
#include "log.h"

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

        //Reset mouse position to center of window every frame
        glfwSetCursorPos(window, Window::width / static_cast<double>(2), Window::height / static_cast<double>(2));
        // Disable cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // macOS requirement
        // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        // Initialize BGFX for rendering
        bgfx::Init bgfxInit;
        bgfxInit.type = bgfx::RendererType::Count;
        bgfxInit.resolution.width = 1920;
        bgfxInit.resolution.height = 1080;
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
        bgfxInit.debug = BGFX_DEBUG_IFH;

        bgfxInit.platformData.ndt = nullptr;
        #if BX_PLATFORM_WINDOWS
            Log::info("Windows platform detected for BGFX"); 
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
    }

    void shutdown() {
        glfwDestroyWindow(window);
        glfwTerminate();

        // bgfx::shutdown(); // should come back to this
    }

    void begin_update() {
        glfwPollEvents();

        // bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);

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
    
    GLFWwindow* get_window() {
        return window;  // Return the static window instance
    }
}