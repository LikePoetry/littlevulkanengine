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
		"ThirdParty/OpenSource/cpu_features/src/impl_x86_windows.c"
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
		"%{prj.name}/**.cpp"
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
		"$(VULKAN_SDK)/Include"
	}

	libdirs 
	{ 
		"%VULKAN_SDK%/lib" 
	}

	links
	{
		"OS",
		"Renderer"
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