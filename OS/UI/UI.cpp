#include "../Interfaces/IUI.h"

#include "../Interfaces/ILog.h"
#include "../Interfaces/IFont.h"
#include "../Interfaces/IInput.h"
#include "../Interfaces/IFileSystem.h"
#include "../Renderer/Include/IResourceLoader.h"

#include "../../ThirdParty/OpenSource/imgui/imgui.h"
#include "../../ThirdParty/OpenSource/imgui/imgui_internal.h"
#include "../../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"

#include "../Interfaces/IMemory.h"

typedef struct UserInterface
{
	float					mWidth = 0.f;
	float					mHeight = 0.f;
	uint32_t				mMaxDynamicUIUpdatesPerBatch = 20;

	// Following var is useful for seeing UI capailities and tweaking style settings.
	// Will only take effect if at least one GUI Component is active.
	bool					mShowDemoUiWindow = false;

	Renderer* pRenderer = NULL;
	eastl::vector<UIComponent*> mComponents;
	bool					mUpdated = false;

	PipelineCache* pPipelineCache = NULL;
	ImGuiContext* context = NULL;
	eastl::vector<Texture*> mFontTextures;
	float                   dpiScale[2] = { 0.0f };
	uint32_t				frameIdx = 0;

	Shader* pShaderTextured = NULL;
	DescriptorSet* pDescriptorSetUniforms = NULL;
	DescriptorSet* pDescriptorSetTexture = NULL;

} UserInterface;

static UserInterface* pUserInterface = NULL;

/****************************************************************************/
// MARK: - Non-static Function Definitions
/****************************************************************************/
bool addImguiFont(void* pFontBuffer, uint32_t fontBufferSize, void* pFontGlyphRanges, float fontSize, uintptr_t* pFont)
{
	ImGuiIO& io = ImGui::GetIO();
	// Build and load the texture atlas into a texture
	int width, height, bytesPerPixel;
	unsigned char* pixels = NULL;

	if (pFontBuffer == NULL)
	{
		*pFont = (uintptr_t)io.Fonts->AddFontDefault();
	}
	else
	{
		ImFontConfig config = {};
		config.FontDataOwnedByAtlas = false;
		ImFont* font = io.Fonts->AddFontFromMemoryTTF(pFontBuffer, fontBufferSize,
			fontSize * min(pUserInterface->dpiScale[0], pUserInterface->dpiScale[1]), &config,
			(const ImWchar*)pFontGlyphRanges);

		if (font != NULL)
		{
			io.FontDefault = font;
			*pFont = (uintptr_t)font;
		}
		else
		{
			*pFont = (uintptr_t)io.Fonts->AddFontDefault();
		}


	}

	io.Fonts->Build();
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);
	// At this point you've got the texture data and you need to upload that your your graphic system:
	// After we have created the texture, store its pointer/identifier (_in whichever format your engine uses_) in 'io.Fonts->TexID'.
	// This will be passed back to your via the renderer. Basically ImTextureID == void*. Read FAQ below for details about ImTextureID.

	Texture* pTexture = NULL;
	SyncToken token = {};
	TextureLoadDesc loadDesc = {};
	TextureDesc textureDesc = {};
	textureDesc.mArraySize = 1;
	textureDesc.mDepth = 1;
	textureDesc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
	textureDesc.mFormat = TinyImageFormat_R8G8B8A8_UNORM;
	textureDesc.mHeight = height;
	textureDesc.mMipLevels = 1;
	textureDesc.mSampleCount = SAMPLE_COUNT_1;
	textureDesc.mStartState = RESOURCE_STATE_COMMON;
	textureDesc.mWidth = width;
	textureDesc.pName = "ImGui Font Texture";
	loadDesc.pDesc = &textureDesc;
	loadDesc.ppTexture = &pTexture;
	addResource(&loadDesc, &token);
	waitForToken(&token);

	TextureUpdateDesc updateDesc = { pTexture };
	beginUpdateResource(&updateDesc);
	for (uint32_t r = 0; r < updateDesc.mRowCount; ++r)
	{
		memcpy(updateDesc.pMappedData + r * updateDesc.mDstRowStride,
			pixels + r * updateDesc.mSrcRowStride, updateDesc.mSrcRowStride);
	}
	endUpdateResource(&updateDesc, &token);

	pUserInterface->mFontTextures.emplace_back(pTexture);
	io.Fonts->TexID = (void*)(pUserInterface->mFontTextures.size() - 1);

	DescriptorData params[1] = {};
	params[0].pName = "uTex";
	params[0].ppTextures = &pTexture;
	vk_updateDescriptorSet(pUserInterface->pRenderer, (uint32_t)pUserInterface->mFontTextures.size() - 1, pUserInterface->pDescriptorSetTexture, 1, params);

	return true;
}

/****************************************************************************/
// MARK: - UI Component Public Functions
/****************************************************************************/

void uiCreateComponent(const char* pTitle, const UIComponentDesc* pDesc, UIComponent** ppUIComponent)
{
	ASSERT(ppUIComponent);
	UIComponent* pComponent = (UIComponent*)(tf_calloc(1, sizeof(UIComponent)));
	pComponent->mHasCloseButton = false;
	pComponent->mFlags = GUI_COMPONENT_FLAGS_ALWAYS_AUTO_RESIZE;

	// Functions not accessible via normal interface header
	extern void* fntGetRawFontData(uint32_t fontID);
	extern uint32_t fntGetRawFontDataSize(uint32_t fontID);

	// Use Requested Forge Font
	void* pFontBuffer = fntGetRawFontData(pDesc->mFontID);
	uint32_t fontBufferSize = fntGetRawFontDataSize(pDesc->mFontID);
	if (pFontBuffer)
		addImguiFont(pFontBuffer, fontBufferSize, NULL, pDesc->mFontSize, &pComponent->pFont);

	pComponent->mInitialWindowRect = { pDesc->mStartPosition.getX(), 
		pDesc->mStartPosition.getY(), 
		pDesc->mStartSize.getX(),
		pDesc->mStartSize.getY() };

	pComponent->mActive = true;
	strcpy(pComponent->mTitle, pTitle);
	pComponent->mAlpha = 1.0f;
	pUserInterface->mComponents.emplace_back(pComponent);
	
	*ppUIComponent = pComponent;
}

/****************************************************************************/
// MARK: - Safe Public Setter Functions
/****************************************************************************/

void uiSetComponentFlags(UIComponent* pGui, int32_t flags)
{
	ASSERT(pGui);

	pGui->mFlags = flags;
}