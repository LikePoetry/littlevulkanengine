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
		"ThirdParty/OpenSource/imgui/imconfig.h",
		"ThirdParty/OpenSource/imgui/imgui.cpp",
		"ThirdParty/OpenSource/imgui/imgui.h",
		"ThirdParty/OpenSource/imgui/imgui_demo.cpp",
		"ThirdParty/OpenSource/imgui/imgui_draw.cpp",
		"ThirdParty/OpenSource/imgui/imgui_internal.h",
		"ThirdParty/OpenSource/imgui/imgui_widgets.cpp",

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

project "gainputstatic"
		location "gainputstatic"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"
		characterset "MBCS"
		compileas "C++"
	
		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")
	
		files
		{
			"ThirdParty/OpenSource/gainput/lib/source/hidapi/windows/hid.c",
			"ThirdParty/OpenSource/gainput/lib/source/hidapi/hidapi.h",

			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/hidparsers/HIDParserPS4Controller.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/hidparsers/HIDParserPS4Controller.cpp",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/hidparsers/HIDParserPS5Controller.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/hidparsers/HIDParserPS5Controller.cpp",

			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/GainputHID.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/GainputHID.cpp",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/GainputHIDTypes.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/GainputHIDWhitelist.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/hid/GainputHIDWhitelist.cpp",

			"ThirdParty/OpenSource/gainput/lib/include/gainput/gainput.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/gainput.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputAllocator.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/GainputAllocator.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/gestures/GainputButtonStickGesture.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/gestures/GainputButtonStickGesture.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputContainers.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputDebugRenderer.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/dev/GainputDev.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/dev/GainputDev.cpp",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/dev/GainputDevProtocol.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/gestures/GainputDoubleClickGesture.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/gestures/GainputDoubleClickGesture.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/gestures/GainputGestures.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputHelpers.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/GainputHelpersEvdev.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/gestures/GainputHoldGesture.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/gestures/GainputHoldGesture.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputDeltaState.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/GainputInputDeltaState.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputDevice.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/GainputInputDevice.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputDeviceBuiltIn.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/builtin/GainputInputDeviceBuiltIn.cpp",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/builtin/GainputInputDeviceBuiltInAndroid.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/builtin/GainputInputDeviceBuiltInImpl.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/builtin/GainputInputDeviceBuiltInIos.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/builtin/GainputInputDeviceBuiltInNull.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputInputDeviceKeyboard.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputDeviceKeyboard.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputInputDeviceKeyboardAndroid.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputInputDeviceKeyboardEvdev.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputInputDeviceKeyboardImpl.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputInputDeviceKeyboardLinux.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputInputDeviceKeyboardNull.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputInputDeviceKeyboardWin.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputInputDeviceKeyboardWinRaw.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputInputDeviceMouse.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputDeviceMouse.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputInputDeviceMouseEvdev.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputInputDeviceMouseImpl.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputInputDeviceMouseLinux.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputInputDeviceMouseMac.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputInputDeviceMouseNull.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputInputDeviceMouseWin.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputInputDeviceMouseWinRaw.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/pad/GainputInputDevicePad.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputDevicePad.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/pad/GainputInputDevicePadAndroid.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/pad/GainputInputDevicePadImpl.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/pad/GainputInputDevicePadIos.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/pad/GainputInputDevicePadLinux.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/pad/GainputInputDevicePadMac.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/pad/GainputInputDevicePadNull.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/pad/GainputInputDevicePadWin.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/touch/GainputInputDeviceTouch.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputDeviceTouch.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/touch/GainputInputDeviceTouchAndroid.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/touch/GainputInputDeviceTouchImpl.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/touch/GainputInputDeviceTouchIos.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/touch/GainputInputDeviceTouchNull.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputListener.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/GainputInputManager.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputManager.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/GainputInputMap.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputMap.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/recorder/GainputInputPlayer.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/recorder/GainputInputPlayer.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/recorder/GainputInputRecorder.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/recorder/GainputInputRecorder.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/recorder/GainputInputRecording.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/recorder/GainputInputRecording.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputInputState.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/GainputInputState.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputIos.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/keyboard/GainputKeyboardKeyNames.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputLog.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputMapFilters.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/GainputMapFilters.cpp",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/dev/GainputMemoryStream.cpp",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/dev/GainputMemoryStream.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/mouse/GainputMouseInfo.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/gestures/GainputPinchGesture.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/gestures/GainputPinchGesture.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/gestures/GainputRotateGesture.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/gestures/GainputRotateGesture.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/gestures/GainputSimultaneouslyDownGesture.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/gestures/GainputSimultaneouslyDownGesture.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/dev/GainputStream.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/gestures/GainputTapGesture.h",
			"ThirdParty/OpenSource/gainput/lib/source/gainput/gestures/GainputTapGesture.cpp",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/touch/GainputTouchInfo.h",
			"ThirdParty/OpenSource/gainput/lib/include/gainput/GainputWindows.h",
		}
		
		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"_WINDOWS"
		}
	
		includedirs
		{
			"OS",
			"ThirdParty/OpenSource/gainput/lib/include"
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
			"gainputstatic",
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
			"SpirvTools",
			"gainputstatic"
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
		"gainputstatic",
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
		"SpirvTools",
		"gainputstatic"
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
