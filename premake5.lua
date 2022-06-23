workspace "TheForge"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir="%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

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
			"$(VULKAN_SDK)/Include"
		}

		libdirs 
		{ 
			"%VULKAN_SDK%/lib" 
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
		"ThirdParty/OpenSource/EASTL/EAStdC/EASprintf.cpp",
		"ThirdParty/OpenSource/EASTL/hashtable.cpp",
		"ThirdParty/OpenSource/cpu_features/src/impl_x86_windows.c",
		"ThirdParty/OpenSource/basis_universal/transcoder/basisu_transcoder.cpp",
		"ThirdParty/OpenSource/imgui/imconfig.h",
		"ThirdParty/OpenSource/imgui/imgui.cpp",
		"ThirdParty/OpenSource/imgui/imgui.h",
		"ThirdParty/OpenSource/imgui/imgui_demo.cpp",
		"ThirdParty/OpenSource/imgui/imgui_draw.cpp",
		"ThirdParty/OpenSource/imgui/imgui_internal.h",
		"ThirdParty/OpenSource/imgui/imgui_widgets.cpp",
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
		"$(VULKAN_SDK)/Include"
	}

	libdirs 
	{ 
		"%VULKAN_SDK%/lib" 
	}

	links
	{
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
group	""
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
