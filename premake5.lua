local BUILD_DIR = path.join("build", _ACTION)
if _OPTIONS["cc"] ~= nil then
    BUILD_DIR = BUILD_DIR .. "_" .. _OPTIONS["cc"]
end
local BGFX_DIR = "lib/bgfx"
local BIMG_DIR = "lib/bimg"
local BX_DIR = "lib/bx"
local GLFW_DIR = "lib/glfw"

solution "voxels-senior-project"
location(BUILD_DIR)
startproject "voxels"
configurations { "Release", "Debug" }
platforms { "x86", "x86_64", "arm64" }

if os.istarget("macosx") then
    platforms { "x86_64", "arm64" }
elseif os.is64bit() and not os.istarget("windows") then
    platforms "x86_64"
else
    platforms { "x86", "x86_64" }
end

filter "configurations:Release"
defines "NDEBUG"
optimize "Full"

filter "configurations:Debug*"
defines "_DEBUG"
optimize "Debug"
symbols "On"

filter "platforms:x86"
architecture "x86"
filter "platforms:x86_64"
architecture "x86_64"
filter "platforms:arm64"
architecture "arm64"

filter "system:macosx"
architecture "arm64"
xcodebuildsettings {
    ["MACOSX_DEPLOYMENT_TARGET"] = "10.9",
    ["ALWAYS_SEARCH_USER_PATHS"] = "YES", -- This is the minimum version of macos we'll be able to run on
};

function setBxCompat()
	filter "action:vs*"
		includedirs { path.join(BX_DIR, "include/compat/msvc") }
	filter { "system:windows", "action:gmake" }
		includedirs { path.join(BX_DIR, "include/compat/mingw") }
    filter { "system:windows"}
		buildoptions { "/Zc:__cplusplus", "/Zc:preprocessor" }
	filter { "system:macosx" }
		includedirs { path.join(BX_DIR, "include/compat/osx") }
		buildoptions { "-x objective-c++" }
end

function compileShaders()
    local platform = ""
    if os.host() == "windows" then
        platform = "windows"
    elseif os.host() == "linux" then
        platform = "linux"
    elseif os.host() == "macosx" then
        platform = "osx"
    end

	local shadercPath = path.join("bin", "bx_tools", platform, "shadercRelease")
	local shaderDir = path.join("src", "shaders")
	local includeDir = path.join(BGFX_DIR, "src")
	local outputDir = path.join("build", "shaders")
	local commonDir = path.join(BGFX_DIR, "examples", "common")
	local varyingDef = path.join(shaderDir)
	local profilePrefix = ""
	
	-- Clean the output directory
	if os.isdir(outputDir) then
		os.rmdir(outputDir)
	end
    os.mkdir(outputDir)

    -- Find all .sc files in the shader directory and subdirectories
    local shaderFiles = os.matchfiles(path.join(shaderDir, "**.sc"))

    for _, shaderFile in ipairs(shaderFiles) do
        local shaderType = ""
        if string.find(shaderFile, "v_") then
            shaderType = "vertex"
			profilePrefix = "vs_5_0"
        elseif string.find(shaderFile, "f_") then
            shaderType = "fragment"
			profilePrefix = "ps_5_0"
        elseif string.find(shaderFile, "c_") then
            shaderType = "compute"
			profilePrefix = "cs_5_0"
        end

		-- Compile the shader then output the binary to the output directory
		local outputFile = path.join(outputDir, path.getbasename(shaderFile) .. ".bin")
		local command = string.format('"%s" -f "%s" -o "%s" --profile %s --platform %s --type %s --varyingdef "%s/varying.def.sc" -O 3 -i "%s" "%s"', shadercPath, shaderFile, outputFile, profilePrefix, platform, shaderType, varyingDef, includeDir, shaderDir)

		if shaderType ~= "" then
			print("Compiling shader: " .. command)
			if os.host() == "windows" then
				os.execute('powershell -Command "' .. command .. '"')
			else
				os.execute(command)
			end
		end

    end
end

project "voxels"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
	rtti "Off"
	files { "src/**.h", "src/**.cpp" }
	defines { "BX_CONFIG_DEBUG=1" }
	includedirs {
		"src",
		"src/core",
		path.join(BGFX_DIR, "include"),
		path.join(BGFX_DIR, "3rdparty"),
		path.join(BX_DIR, "include"),
		path.join(GLFW_DIR, "include"),
		path.join(BIMG_DIR, "include"),
	}
	links { "bgfx", "bimg", "bx", "glfw" }
	filter "system:linux" links { "dl", "GL", "pthread", "X11" }
	filter "system:macosx" links { "QuartzCore.framework", "Metal.framework", "Cocoa.framework", "IOKit.framework", "CoreVideo.framework" }
	filter "system:windows" 
        links { "gdi32", "kernel32", "psapi" }
	setBxCompat()
    compileShaders()
    
	
project "bgfx"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
	rtti "Off"
	
	defines { "BX_CONFIG_DEBUG=1", "__STDC_FORMAT_MACROS" }
	files
	{
		path.join(BGFX_DIR, "include/bgfx/**.h"),
		path.join(BGFX_DIR, "src/*.cpp"),
		path.join(BGFX_DIR, "src/*.h"),
	}
	excludes
	{
		path.join(BGFX_DIR, "src/amalgamated.cpp"),
	}
	includedirs
	{
		path.join(BX_DIR, "include"),
		path.join(BIMG_DIR, "include"),
		path.join(BGFX_DIR, "include"),
		path.join(BGFX_DIR, "3rdparty"),
		path.join(BGFX_DIR, "3rdparty/directx-headers/include"),
		path.join(BGFX_DIR, "3rdparty/directx-headers/include/directx"),
		path.join(BGFX_DIR, "3rdparty/khronos"),
		path.join(BIMG_DIR, "3rdparty/astc-encoder/source/**.cpp"),
		path.join(BIMG_DIR, "3rdparty/astc-encoder/source/**.h"),
	}
	-- filter "configurations:Debug"
	defines { "BGFX_CONFIG_DEBUG=1", "BX_CONFIG_DEBUG=1" }
	filter "action:vs*"
		defines "_CRT_SECURE_NO_WARNINGS"
		excludes
		{
			path.join(BGFX_DIR, "src/glcontext_glx.cpp"),
			path.join(BGFX_DIR, "src/glcontext_egl.cpp")
		}
	filter "system:macosx"
		files
		{
			path.join(BGFX_DIR, "src/*.mm"),
		}
	setBxCompat()

project "bimg"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
	rtti "Off"
	defines { "BX_CONFIG_DEBUG=1", }
	files
	{
		path.join(BIMG_DIR, "include/bimg/*.h"),
		path.join(BIMG_DIR, "src/image.cpp"),
		path.join(BIMG_DIR, "src/image_gnf.cpp"),
		path.join(BIMG_DIR, "src/*.h"),
		path.join(BIMG_DIR, "3rdparty/astc-encoder/source/**.cpp"),
		path.join(BIMG_DIR, "3rdparty/astc-encoder/source/**.h"),
	}
	includedirs
	{
		path.join(BX_DIR, "include"),
		path.join(BIMG_DIR, "include"),
		path.join(BIMG_DIR, "3rdparty/astc-encoder/include"),
	}
	setBxCompat()

project "bx"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
	rtti "Off"
	defines { "__STDC_FORMAT_MACROS", "BX_CONFIG_DEBUG" }
	files
	{
		path.join(BX_DIR, "include/bx/*.h"),
		path.join(BX_DIR, "include/bx/inline/*.inl"),
		path.join(BX_DIR, "src/*.cpp")
	}
	excludes
	{
		path.join(BX_DIR, "src/amalgamated.cpp"),
		path.join(BX_DIR, "src/crtnone.cpp")
	}
	includedirs
	{
		path.join(BX_DIR, "3rdparty"),
		path.join(BX_DIR, "include")
	}
	filter "action:vs*"
		defines "_CRT_SECURE_NO_WARNINGS"
	setBxCompat()
		
project "glfw"
	kind "StaticLib"
	language "C"
	files
	{
		path.join(GLFW_DIR, "include/GLFW/glfw3.h"),
		path.join(GLFW_DIR, "include/GLFW/glfw3native.h"),
		path.join(GLFW_DIR, "src/internal.h"),
		path.join(GLFW_DIR, "src/platform.h"),
		path.join(GLFW_DIR, "src/mappings.h"),
		path.join(GLFW_DIR, "src/context.c"),
		path.join(GLFW_DIR, "src/init.c"),
		path.join(GLFW_DIR, "src/input.c"),
		path.join(GLFW_DIR, "src/monitor.c"),
		path.join(GLFW_DIR, "src/platform.c"),
		path.join(GLFW_DIR, "src/vulkan.c"),
		path.join(GLFW_DIR, "src/window.c"),
		path.join(GLFW_DIR, "src/egl_context.c"),
		path.join(GLFW_DIR, "src/osmesa_context.c"),
		path.join(GLFW_DIR, "src/null_platform.h"),
		path.join(GLFW_DIR, "src/null_joystick.h"),
		path.join(GLFW_DIR, "src/null_init.c"),
		path.join(GLFW_DIR, "src/null_monitor.c"),
		path.join(GLFW_DIR, "src/null_window.c"),
		path.join(GLFW_DIR, "src/null_joystick.c"),
	}
	includedirs { path.join(GLFW_DIR, "include") }
	filter "system:windows"
		defines "_GLFW_WIN32"
		files
		{
			path.join(GLFW_DIR, "src/win32_*.*"),
			path.join(GLFW_DIR, "src/wgl_context.*")
		}
	filter "system:linux"
		defines "_GLFW_X11"
		files
		{
			path.join(GLFW_DIR, "src/glx_context.*"),
			path.join(GLFW_DIR, "src/linux*.*"),
			path.join(GLFW_DIR, "src/posix*.*"),
			path.join(GLFW_DIR, "src/x11*.*"),
			path.join(GLFW_DIR, "src/xkb*.*")
		}
	filter "system:macosx"
		defines "_GLFW_COCOA"
		files
		{
			path.join(GLFW_DIR, "src/cocoa_*.*"),
			path.join(GLFW_DIR, "src/posix_thread.h"),
			path.join(GLFW_DIR, "src/nsgl_context.h"),
			path.join(GLFW_DIR, "src/egl_context.h"),
			path.join(GLFW_DIR, "src/osmesa_context.h"),

    path.join(GLFW_DIR, "src/posix_thread.c"),
    path.join(GLFW_DIR, "src/nsgl_context.m"),
    path.join(GLFW_DIR, "src/egl_context.c"),
    path.join(GLFW_DIR, "src/nsgl_context.m"),
    path.join(GLFW_DIR, "src/osmesa_context.c"),
}

filter "action:vs*"
defines "_CRT_SECURE_NO_WARNINGS"
