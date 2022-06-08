workspace "TheForge"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir="%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

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
		"%{prj.name}/OS/*/**.h",
		"%{prj.name}/OS/*/**.cpp",
		"%{prj.name}/OS/*/**.c",
		"%{prj.name}/OS/**.h",
		"%{prj.name}/OS/**.cpp"
	}
	
	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"_WINDOWS"
	}

	includedirs
	{
		"%{prj.name}/OS/"
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
			"%{prj.name}/Renderer/*/**.h",
			"%{prj.name}/Renderer/*/**.cpp",
		}
		
		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"_WINDOWS"
		}
	
		includedirs
		{
			"%{prj.name}/Renderer/"
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
		"OS"
	}

	links
	{
		"OS"
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