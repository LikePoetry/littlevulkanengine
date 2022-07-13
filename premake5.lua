workspace "TheForge"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir="%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir={}
IncludeDir["GLFW"]="ThirdParty/OpenSource/GLFW/include"

group "Dependencies"
	include "ThirdParty/OpenSource/GLFW"

group ""

project "Renderer"
		location "Renderer"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"
	
		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")
	
		files
		{
			"%{prj.name}/*/**.h",
			"%{prj.name}/*/**.cpp",
			"%{prj.name}/*/**.c",
			"ThirdParty/OpenSource/meshoptimizer/src/allocator.cpp",
			"ThirdParty/OpenSource/meshoptimizer/src/indexgenerator.cpp",
			"ThirdParty/OpenSource/meshoptimizer/src/overdrawoptimizer.cpp",
			"ThirdParty/OpenSource/meshoptimizer/src/vcacheoptimizer.cpp",
			"ThirdParty/OpenSource/meshoptimizer/src/vfetchoptimizer.cpp",

		}
		
		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"_WINDOWS"
		}
	
		includedirs
		{
			"Renderer",
			"OS",
			"%{IncludeDir.GLFW}",
			"$(VULKAN_SDK)/Include"
		}

		libdirs 
		{ 
			"%VULKAN_SDK%/lib" 
		}

		links
		{
			"GLFW",
		}
	
		filter "system:windows"
			systemversion "latest"
	
		filter "configurations:Debug"
			defines ""
			runtime "Debug"
			symbols "on"
	
		filter "configurations:Release"
			defines ""
			runtime "Release"
			optimize "on"

project "OS"
	location "OS"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir("bin/" ..outputdir.. "/%{prj.name}")
	objdir("bin-int/" ..outputdir.. "/%{prj.name}")

	files
	{
		"%{prj.name}/*/**.h",
		"%{prj.name}/*/**.cpp",
		"%{prj.name}/*/**.c",
		"ThirdParty/OpenSource/EASTL/allocator_forge.cpp",
		"ThirdParty/OpenSource/EASTL/assert.cpp",
		"ThirdParty/OpenSource/EASTL/fixed_pool.cpp",
		"ThirdParty/OpenSource/EASTL/hashtable.cpp",
		"ThirdParty/OpenSource/EASTL/intrusive_list.cpp",
		"ThirdParty/OpenSource/EASTL/numeric_limits.cpp",
		"ThirdParty/OpenSource/EASTL/red_black_tree.cpp",
		"ThirdParty/OpenSource/EASTL/string.cpp",
		"ThirdParty/OpenSource/EASTL/thread_support.cpp",
		"ThirdParty/OpenSource/EASTL/EAStdC/EASprintf.cpp",
		"ThirdParty/OpenSource/EASTL/EAStdC/EAMemory.cpp",
		"ThirdParty/OpenSource/cpu_features/src/impl_x86_windows.c",
		"ThirdParty/OpenSource/basis_universal/transcoder/basisu_transcoder.cpp",
		-- "ThirdParty/OpenSource/imgui/imconfig.h",
		-- "ThirdParty/OpenSource/imgui/imgui.cpp",
		-- "ThirdParty/OpenSource/imgui/imgui.h",
		-- "ThirdParty/OpenSource/imgui/imgui_demo.cpp",
		-- "ThirdParty/OpenSource/imgui/imgui_draw.cpp",
		-- "ThirdParty/OpenSource/imgui/imgui_internal.h",
		-- "ThirdParty/OpenSource/imgui/imgui_widgets.cpp",

		"ThirdParty/OpenSource/lua-5.3.5/src/lapi.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lauxlib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lbaselib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lbitlib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lcode.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lcorolib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lctype.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/ldblib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/ldebug.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/ldo.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/ldump.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lfunc.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lgc.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/linit.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/liolib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/llex.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lmathlib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lmem.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/loadlib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lobject.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lopcodes.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/loslib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lparser.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lstate.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lstring.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lstrlib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/ltable.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/ltablib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/ltm.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lundump.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lutf8lib.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lvm.c",
		"ThirdParty/OpenSource/lua-5.3.5/src/lzio.c"
	}
	
	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"_WINDOWS"
	}

	includedirs
	{
		"OS",
		"Renderer",
		"%{IncludeDir.GLFW}",
		"$(VULKAN_SDK)/Include"
	}

	libdirs 
	{ 
		"%VULKAN_SDK%/lib" 
	}

	links
	{
		"GLFW",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines ""
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines ""
		runtime "Release"
		optimize "on"



group "Tools"
project "SpirvTools"
		location "SpirvTools"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"
	
		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")
	
		files
		{
			"%{prj.name}/**.h",
			"%{prj.name}/**.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_cfg.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_cpp.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_cross.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_cross_parsed_ir.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_cross_util.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_glsl.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_hlsl.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_msl.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_parser.cpp",
			"ThirdParty/OpenSource/SPIRV_Cross/spirv_reflect.cpp"
		}
		
		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"_WINDOWS"
		}
	
		includedirs
		{
			"OS",
			"SpirvTools"
		}
	
		filter "system:windows"
			systemversion "latest"
	
		filter "configurations:Debug"
			defines ""
			runtime "Debug"
			symbols "on"
	
		filter "configurations:Release"
			defines ""
			runtime "Release"
			optimize "on"

group	"Examples"
		project "Sandbox"

		location "Sandbox"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"
		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")
	
		files
		{
			"%{prj.name}/**.h",
			"%{prj.name}/**.cpp",
			"%{prj.name}/**.c",
	
		}
		
		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"_WINDOWS"
		}
	
		includedirs
		{
			"%{prj.name}",
			"OS",
			"Renderer",
			"SpirvTools",
			"$(VULKAN_SDK)/Include"
		}
	
		libdirs 
		{ 
			"%VULKAN_SDK%/lib" 
		}
	
		links
		{
			"OS",
			"Renderer",
			"SpirvTools"
		}
	
		filter "system:windows"
			systemversion "latest"
			
		filter "configurations:Debug"
			defines ""
			runtime "Debug"
			symbols "on"
	
		filter "configurations:Release"
			defines ""
			runtime "Release"
			optimize "on"


project "TriangleDemo"

	location "TriangleDemo"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	targetdir("bin/" ..outputdir.. "/%{prj.name}")
	objdir("bin-int/" ..outputdir.. "/%{prj.name}")

	files
	{
		"%{prj.name}/**.h",
		"%{prj.name}/**.cpp",
		"%{prj.name}/**.c",

	}
	
	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"_WINDOWS"
	}

	includedirs
	{
		"%{prj.name}",
		"OS",
		"Renderer",
		"SpirvTools",
		"%{IncludeDir.GLFW}",
		"$(VULKAN_SDK)/Include"
	}

	libdirs 
	{ 
		"%VULKAN_SDK%/lib" 
	}

	links
	{
		"OS",
		"Renderer",
		"GLFW",
		"SpirvTools"
	}

	filter "system:windows"
		systemversion "latest"
		
	filter "configurations:Debug"
		defines ""
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines ""
		runtime "Release"
		optimize "on"
