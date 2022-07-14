#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../Interfaces/IUI.h"
#include "../Interfaces/IMemory.h"
#include "../Interfaces/IOperatingSystem.h"


typedef struct UserInterface
{
	bool mShowDemoUiWindow = false;

	float                   dpiScale[2] = { 0.0f };

	bool             mActive = false;
	bool             mPostUpdateKeyDownStates[512] = { false };
	// Since gestures events always come first, we want to dismiss any other inputs after that
	bool mHandledGestures = false;

} UserInterface;


static UserInterface* pUserInterface = NULL;

static void* alloc_func(size_t size, void* user_data)
{
	return tf_malloc(size);
}

static void dealloc_func(void* ptr, void* user_data)
{
	tf_free(ptr);
}

/****************************************************************************/
// MARK: - Private Platform Layer Life Cycle Functions
/****************************************************************************/

// UIApp public functions

bool platformInitUserInterface() 
{
	UserInterface* pAppUI = tf_new(UserInterface);

	pAppUI->mShowDemoUiWindow = true;

	pAppUI->mHandledGestures = false;
	pAppUI->mActive = true;
	memset(pAppUI->mPostUpdateKeyDownStates, 0, sizeof(pAppUI->mPostUpdateKeyDownStates));

	getDpiScale(pAppUI->dpiScale);

	//// init UI (input)
	ImGui::SetAllocatorFunctions(alloc_func, dealloc_func);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	pUserInterface = pAppUI;

	return true;
}


/****************************************************************************/
// MARK: - Public App Layer Life Cycle Functions
/****************************************************************************/

void initUserInterface(UserInterfaceDesc* pDesc)
{

}