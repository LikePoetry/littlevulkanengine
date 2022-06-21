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

// UIWidget functions
UIWidget* cloneWidget(const UIWidget* pWidget);

/****************************************************************************/
// MARK: - Static Value Definitions
/****************************************************************************/

static const uint64_t VERTEX_BUFFER_SIZE = 1024 * 64 * sizeof(ImDrawVert);
static const uint64_t INDEX_BUFFER_SIZE = 128 * 1024 * sizeof(ImDrawIdx);

/****************************************************************************/
// MARK: - Base UIWidget Helper Functions
/****************************************************************************/
// CollapsingHeaderWidget private functions
CollapsingHeaderWidget* cloneCollapsingHeaderWidget(const void* pWidget)
{
	const CollapsingHeaderWidget* pOriginalWidget = (const CollapsingHeaderWidget*)pWidget;
	CollapsingHeaderWidget* pClonedWidget = (CollapsingHeaderWidget*)tf_calloc(1, sizeof(CollapsingHeaderWidget));

	pClonedWidget->mCollapsed = pOriginalWidget->mCollapsed;
	pClonedWidget->mDefaultOpen = pOriginalWidget->mDefaultOpen;
	pClonedWidget->mHeaderIsVisible = pOriginalWidget->mHeaderIsVisible;
	pClonedWidget->mGroupedWidgets = pOriginalWidget->mGroupedWidgets;

	return pClonedWidget;
}

// DebugTexturesWidget private functions
DebugTexturesWidget* cloneDebugTexturesWidget(const void* pWidget)
{
	const DebugTexturesWidget* pOriginalWidget = (const DebugTexturesWidget*)pWidget;
	DebugTexturesWidget* pClonedWidget = (DebugTexturesWidget*)tf_calloc(1, sizeof(DebugTexturesWidget));

	pClonedWidget->mTextureDisplaySize = pOriginalWidget->mTextureDisplaySize;
	pClonedWidget->mTextures = pOriginalWidget->mTextures;

	return pClonedWidget;
}

// LabelWidget private functions
LabelWidget* cloneLabelWidget(const void* pWidget)
{
	LabelWidget* pClonedWidget = (LabelWidget*)tf_calloc(1, sizeof(LabelWidget));

	return pClonedWidget;
}

// ColorLabelWidget private functions
ColorLabelWidget* cloneColorLabelWidget(const void* pWidget)
{
	const ColorLabelWidget* pOriginalWidget = (const ColorLabelWidget*)pWidget;
	ColorLabelWidget* pClonedWidget = (ColorLabelWidget*)tf_calloc(1, sizeof(ColorLabelWidget));

	pClonedWidget->mColor = pOriginalWidget->mColor;

	return pClonedWidget;
}

// HorizontalSpaceWidget private functions
HorizontalSpaceWidget* cloneHorizontalSpaceWidget(const void* pWidget)
{
	HorizontalSpaceWidget* pClonedWidget = (HorizontalSpaceWidget*)tf_calloc(1, sizeof(HorizontalSpaceWidget));

	return pClonedWidget;
}

// SeparatorWidget private functions
SeparatorWidget* cloneSeparatorWidget(const void* pWidget)
{
	SeparatorWidget* pClonedWidget = (SeparatorWidget*)tf_calloc(1, sizeof(SeparatorWidget));

	return pClonedWidget;
}

// VerticalSeparatorWidget private functions
VerticalSeparatorWidget* cloneVerticalSeparatorWidget(const void* pWidget)
{
	const VerticalSeparatorWidget* pOriginalWidget = (const VerticalSeparatorWidget*)pWidget;
	VerticalSeparatorWidget* pClonedWidget = (VerticalSeparatorWidget*)tf_calloc(1, sizeof(VerticalSeparatorWidget));

	pClonedWidget->mLineCount = pOriginalWidget->mLineCount;

	return pClonedWidget;
}

// ButtonWidget private functions
ButtonWidget* cloneButtonWidget(const void* pWidget)
{
	ButtonWidget* pClonedWidget = (ButtonWidget*)tf_calloc(1, sizeof(ButtonWidget));

	return pClonedWidget;
}

// SliderFloatWidget private functions
SliderFloatWidget* cloneSliderFloatWidget(const void* pWidget)
{
	const SliderFloatWidget* pOriginalWidget = (const SliderFloatWidget*)pWidget;
	SliderFloatWidget* pClonedWidget = (SliderFloatWidget*)tf_calloc(1, sizeof(SliderFloatWidget));

	memset(pClonedWidget->mFormat, 0, MAX_FORMAT_STR_LENGTH);
	strcpy(pClonedWidget->mFormat, pOriginalWidget->mFormat);

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mMin = pOriginalWidget->mMin;
	pClonedWidget->mMax = pOriginalWidget->mMax;
	pClonedWidget->mStep = pOriginalWidget->mStep;

	return pClonedWidget;
}

// SliderFloat2Widget private functions
SliderFloat2Widget* cloneSliderFloat2Widget(const void* pWidget)
{
	const SliderFloat2Widget* pOriginalWidget = (const SliderFloat2Widget*)pWidget;
	SliderFloat2Widget* pClonedWidget = (SliderFloat2Widget*)tf_calloc(1, sizeof(SliderFloat2Widget));

	memset(pClonedWidget->mFormat, 0, MAX_FORMAT_STR_LENGTH);
	strcpy(pClonedWidget->mFormat, pOriginalWidget->mFormat);

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mMin = pOriginalWidget->mMin;
	pClonedWidget->mMax = pOriginalWidget->mMax;
	pClonedWidget->mStep = pOriginalWidget->mStep;

	return pClonedWidget;
}

// SliderFloat3Widget private functions
SliderFloat3Widget* cloneSliderFloat3Widget(const void* pWidget)
{
	const SliderFloat3Widget* pOriginalWidget = (const SliderFloat3Widget*)pWidget;
	SliderFloat3Widget* pClonedWidget = (SliderFloat3Widget*)tf_calloc(1, sizeof(SliderFloat3Widget));

	memset(pClonedWidget->mFormat, 0, MAX_FORMAT_STR_LENGTH);
	strcpy(pClonedWidget->mFormat, pOriginalWidget->mFormat);

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mMin = pOriginalWidget->mMin;
	pClonedWidget->mMax = pOriginalWidget->mMax;
	pClonedWidget->mStep = pOriginalWidget->mStep;

	return pClonedWidget;
}

// SliderFloat4Widget private functions
SliderFloat4Widget* cloneSliderFloat4Widget(const void* pWidget)
{
	const SliderFloat4Widget* pOriginalWidget = (const SliderFloat4Widget*)pWidget;
	SliderFloat4Widget* pClonedWidget = (SliderFloat4Widget*)tf_calloc(1, sizeof(SliderFloat4Widget));

	memset(pClonedWidget->mFormat, 0, MAX_FORMAT_STR_LENGTH);
	strcpy(pClonedWidget->mFormat, pOriginalWidget->mFormat);

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mMin = pOriginalWidget->mMin;
	pClonedWidget->mMax = pOriginalWidget->mMax;
	pClonedWidget->mStep = pOriginalWidget->mStep;

	return pClonedWidget;
}

// SliderIntWidget private functions
SliderIntWidget* cloneSliderIntWidget(const void* pWidget)
{
	const SliderIntWidget* pOriginalWidget = (const SliderIntWidget*)pWidget;
	SliderIntWidget* pClonedWidget = (SliderIntWidget*)tf_calloc(1, sizeof(SliderIntWidget));

	memset(pClonedWidget->mFormat, 0, MAX_FORMAT_STR_LENGTH);
	strcpy(pClonedWidget->mFormat, pOriginalWidget->mFormat);

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mMin = pOriginalWidget->mMin;
	pClonedWidget->mMax = pOriginalWidget->mMax;
	pClonedWidget->mStep = pOriginalWidget->mStep;

	return pClonedWidget;
}

// SliderUintWidget private functions
SliderUintWidget* cloneSliderUintWidget(const void* pWidget)
{
	const SliderUintWidget* pOriginalWidget = (const SliderUintWidget*)pWidget;
	SliderUintWidget* pClonedWidget = (SliderUintWidget*)tf_calloc(1, sizeof(SliderUintWidget));

	memset(pClonedWidget->mFormat, 0, MAX_FORMAT_STR_LENGTH);
	strcpy(pClonedWidget->mFormat, pOriginalWidget->mFormat);

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mMin = pOriginalWidget->mMin;
	pClonedWidget->mMax = pOriginalWidget->mMax;
	pClonedWidget->mStep = pOriginalWidget->mStep;

	return pClonedWidget;
}

// RadioButtonWidget private functions
RadioButtonWidget* cloneRadioButtonWidget(const void* pWidget)
{
	const RadioButtonWidget* pOriginalWidget = (const RadioButtonWidget*)pWidget;
	RadioButtonWidget* pClonedWidget = (RadioButtonWidget*)tf_calloc(1, sizeof(RadioButtonWidget));

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mRadioId = pOriginalWidget->mRadioId;

	return pClonedWidget;
}

// CheckboxWidget private functions
CheckboxWidget* cloneCheckboxWidget(const void* pWidget)
{
	const CheckboxWidget* pOriginalWidget = (const CheckboxWidget*)pWidget;
	CheckboxWidget* pClonedWidget = (CheckboxWidget*)tf_calloc(1, sizeof(CheckboxWidget));

	pClonedWidget->pData = pOriginalWidget->pData;

	return pClonedWidget;
}

// OneLineCheckboxWidget private functions
OneLineCheckboxWidget* cloneOneLineCheckboxWidget(const void* pWidget)
{
	const OneLineCheckboxWidget* pOriginalWidget = (const OneLineCheckboxWidget*)pWidget;
	OneLineCheckboxWidget* pClonedWidget = (OneLineCheckboxWidget*)tf_calloc(1, sizeof(OneLineCheckboxWidget));

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mColor = pOriginalWidget->mColor;

	return pClonedWidget;
}

// CursorLocationWidget private functions
CursorLocationWidget* cloneCursorLocationWidget(const void* pWidget)
{
	const CursorLocationWidget* pOriginalWidget = (const CursorLocationWidget*)pWidget;
	CursorLocationWidget* pClonedWidget = (CursorLocationWidget*)tf_calloc(1, sizeof(CursorLocationWidget));

	pClonedWidget->mLocation = pOriginalWidget->mLocation;

	return pClonedWidget;
}

// DropdownWidget private functions
DropdownWidget* cloneDropdownWidget(const void* pWidget)
{
	const DropdownWidget* pOriginalWidget = (const DropdownWidget*)pWidget;
	DropdownWidget* pClonedWidget = (DropdownWidget*)tf_calloc(1, sizeof(DropdownWidget));

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mValues = pOriginalWidget->mValues;
	pClonedWidget->mNames.resize(pOriginalWidget->mNames.size());

	for (uint32_t i = 0; i < pOriginalWidget->mNames.size(); ++i)
	{
		pClonedWidget->mNames[i] = (char*)tf_calloc(strlen(pOriginalWidget->mNames[i]) + 1, sizeof(char));
		strcpy(pClonedWidget->mNames[i], pOriginalWidget->mNames[i]);
	}

	return pClonedWidget;
}

// ColumnWidget private functions
ColumnWidget* cloneColumnWidget(const void* pWidget)
{
	const ColumnWidget* pOriginalWidget = (const ColumnWidget*)pWidget;
	ColumnWidget* pClonedWidget = (ColumnWidget*)tf_calloc(1, sizeof(ColumnWidget));

	pClonedWidget->mPerColumnWidgets = pOriginalWidget->mPerColumnWidgets;
	pClonedWidget->mNumColumns = pOriginalWidget->mNumColumns;

	return pClonedWidget;
}

// ProgressBarWidget private functions
ProgressBarWidget* cloneProgressBarWidget(const void* pWidget)
{
	const ProgressBarWidget* pOriginalWidget = (const ProgressBarWidget*)pWidget;
	ProgressBarWidget* pClonedWidget = (ProgressBarWidget*)tf_calloc(1, sizeof(ProgressBarWidget));

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mMaxProgress = pOriginalWidget->mMaxProgress;

	return pClonedWidget;
}

// ColorSliderWidget private functions
ColorSliderWidget* cloneColorSliderWidget(const void* pWidget)
{
	const ColorSliderWidget* pOriginalWidget = (const ColorSliderWidget*)pWidget;
	ColorSliderWidget* pClonedWidget = (ColorSliderWidget*)tf_calloc(1, sizeof(ColorSliderWidget));

	pClonedWidget->pData = pOriginalWidget->pData;

	return pClonedWidget;
}

// HistogramWidget private functions
HistogramWidget* cloneHistogramWidget(const void* pWidget)
{
	const HistogramWidget* pOriginalWidget = (const HistogramWidget*)pWidget;
	HistogramWidget* pClonedWidget = (HistogramWidget*)tf_calloc(1, sizeof(HistogramWidget));

	pClonedWidget->pValues = pOriginalWidget->pValues;
	pClonedWidget->mCount = pOriginalWidget->mCount;
	pClonedWidget->mMinScale = pOriginalWidget->mMinScale;
	pClonedWidget->mMaxScale = pOriginalWidget->mMaxScale;
	pClonedWidget->mHistogramSize = pOriginalWidget->mHistogramSize;
	pClonedWidget->mHistogramTitle = pOriginalWidget->mHistogramTitle;

	return pClonedWidget;
}

// PlotLinesWidget private functions
PlotLinesWidget* clonePlotLinesWidget(const void* pWidget)
{
	const PlotLinesWidget* pOriginalWidget = (const PlotLinesWidget*)pWidget;
	PlotLinesWidget* pClonedWidget = (PlotLinesWidget*)tf_calloc(1, sizeof(PlotLinesWidget));

	pClonedWidget->mValues = pOriginalWidget->mValues;
	pClonedWidget->mNumValues = pOriginalWidget->mNumValues;
	pClonedWidget->mScaleMin = pOriginalWidget->mScaleMin;
	pClonedWidget->mScaleMax = pOriginalWidget->mScaleMax;
	pClonedWidget->mPlotScale = pOriginalWidget->mPlotScale;
	pClonedWidget->mTitle = pOriginalWidget->mTitle;

	return pClonedWidget;
}

// ColorPickerWidget private functions
ColorPickerWidget* cloneColorPickerWidget(const void* pWidget)
{
	const ColorPickerWidget* pOriginalWidget = (const ColorPickerWidget*)pWidget;
	ColorPickerWidget* pClonedWidget = (ColorPickerWidget*)tf_calloc(1, sizeof(ColorPickerWidget));

	pClonedWidget->pData = pOriginalWidget->pData;

	return pClonedWidget;
}

// TextboxWidget private functions
TextboxWidget* cloneTextboxWidget(const void* pWidget)
{
	const TextboxWidget* pOriginalWidget = (const TextboxWidget*)pWidget;
	TextboxWidget* pClonedWidget = (TextboxWidget*)tf_calloc(1, sizeof(TextboxWidget));

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mLength = pOriginalWidget->mLength;
	pClonedWidget->mAutoSelectAll = pOriginalWidget->mAutoSelectAll;

	return pClonedWidget;
}

// DynamicTextWidget private functions
DynamicTextWidget* cloneDynamicTextWidget(const void* pWidget)
{
	const DynamicTextWidget* pOriginalWidget = (const DynamicTextWidget*)pWidget;
	DynamicTextWidget* pClonedWidget = (DynamicTextWidget*)tf_calloc(1, sizeof(DynamicTextWidget));

	pClonedWidget->pData = pOriginalWidget->pData;
	pClonedWidget->mLength = pOriginalWidget->mLength;
	pClonedWidget->pColor = pOriginalWidget->pColor;

	return pClonedWidget;
}

// FilledRectWidget private functions
FilledRectWidget* cloneFilledRectWidget(const void* pWidget)
{
	const FilledRectWidget* pOriginalWidget = (const FilledRectWidget*)pWidget;
	FilledRectWidget* pClonedWidget = (FilledRectWidget*)tf_calloc(1, sizeof(FilledRectWidget));

	pClonedWidget->mPos = pOriginalWidget->mPos;
	pClonedWidget->mScale = pOriginalWidget->mScale;
	pClonedWidget->mColor = pOriginalWidget->mColor;

	return pClonedWidget;
}

// DrawTextWidget private functions
DrawTextWidget* cloneDrawTextWidget(const void* pWidget)
{
	const DrawTextWidget* pOriginalWidget = (const DrawTextWidget*)pWidget;
	DrawTextWidget* pClonedWidget = (DrawTextWidget*)tf_calloc(1, sizeof(DrawTextWidget));

	pClonedWidget->mPos = pOriginalWidget->mPos;
	pClonedWidget->mColor = pOriginalWidget->mColor;

	return pClonedWidget;
}

// DrawTooltipWidget private functions
DrawTooltipWidget* cloneDrawTooltipWidget(const void* pWidget)
{
	const DrawTooltipWidget* pOriginalWidget = (const DrawTooltipWidget*)pWidget;
	DrawTooltipWidget* pClonedWidget = (DrawTooltipWidget*)tf_calloc(1, sizeof(DrawTooltipWidget));

	pClonedWidget->mShowTooltip = pOriginalWidget->mShowTooltip;
	pClonedWidget->mText = pOriginalWidget->mText;

	return pClonedWidget;
}

// DrawLineWidget private functions
DrawLineWidget* cloneDrawLineWidget(const void* pWidget)
{
	const DrawLineWidget* pOriginalWidget = (const DrawLineWidget*)pWidget;
	DrawLineWidget* pClonedWidget = (DrawLineWidget*)tf_calloc(1, sizeof(DrawLineWidget));

	pClonedWidget->mPos1 = pOriginalWidget->mPos1;
	pClonedWidget->mPos2 = pOriginalWidget->mPos2;
	pClonedWidget->mColor = pOriginalWidget->mColor;
	pClonedWidget->mAddItem = pOriginalWidget->mAddItem;

	return pClonedWidget;
}

// DrawCurveWidget private functions
DrawCurveWidget* cloneDrawCurveWidget(const void* pWidget)
{
	const DrawCurveWidget* pOriginalWidget = (const DrawCurveWidget*)pWidget;
	DrawCurveWidget* pClonedWidget = (DrawCurveWidget*)tf_calloc(1, sizeof(DrawCurveWidget));

	pClonedWidget->mPos = pOriginalWidget->mPos;
	pClonedWidget->mNumPoints = pOriginalWidget->mNumPoints;
	pClonedWidget->mThickness = pOriginalWidget->mThickness;
	pClonedWidget->mColor = pOriginalWidget->mColor;

	return pClonedWidget;
}


// UIWidget private functions
void cloneWidgetBase(UIWidget* pDstWidget, const UIWidget* pSrcWidget)
{
	pDstWidget->mType = pSrcWidget->mType;
	strcpy(pDstWidget->mLabel, pSrcWidget->mLabel);

	pDstWidget->pOnHover = pSrcWidget->pOnHover;
	pDstWidget->pOnActive = pSrcWidget->pOnActive;
	pDstWidget->pOnFocus = pSrcWidget->pOnFocus;
	pDstWidget->pOnEdited = pSrcWidget->pOnEdited;
	pDstWidget->pOnDeactivated = pSrcWidget->pOnDeactivated;
	pDstWidget->pOnDeactivatedAfterEdit = pSrcWidget->pOnDeactivatedAfterEdit;

	pDstWidget->mDeferred = pSrcWidget->mDeferred;
}

// UIWidget functions
UIWidget* cloneWidget(const UIWidget* pOtherWidget)
{
	UIWidget* pWidget = (UIWidget*)tf_calloc(1, sizeof(UIWidget));
	cloneWidgetBase(pWidget, pOtherWidget);

	switch (pOtherWidget->mType)
	{
	case WIDGET_TYPE_COLLAPSING_HEADER:
	{
		pWidget->pWidget = cloneCollapsingHeaderWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_DEBUG_TEXTURES:
	{
		pWidget->pWidget = cloneDebugTexturesWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_LABEL:
	{
		pWidget->pWidget = cloneLabelWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_COLOR_LABEL:
	{
		pWidget->pWidget = cloneColorLabelWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_HORIZONTAL_SPACE:
	{
		pWidget->pWidget = cloneHorizontalSpaceWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_SEPARATOR:
	{
		pWidget->pWidget = cloneSeparatorWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_VERTICAL_SEPARATOR:
	{
		pWidget->pWidget = cloneVerticalSeparatorWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_BUTTON:
	{
		pWidget->pWidget = cloneButtonWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_SLIDER_FLOAT:
	{
		pWidget->pWidget = cloneSliderFloatWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_SLIDER_FLOAT2:
	{
		pWidget->pWidget = cloneSliderFloat2Widget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_SLIDER_FLOAT3:
	{
		pWidget->pWidget = cloneSliderFloat3Widget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_SLIDER_FLOAT4:
	{
		pWidget->pWidget = cloneSliderFloat4Widget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_SLIDER_INT:
	{
		pWidget->pWidget = cloneSliderIntWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_SLIDER_UINT:
	{
		pWidget->pWidget = cloneSliderUintWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_RADIO_BUTTON:
	{
		pWidget->pWidget = cloneRadioButtonWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_CHECKBOX:
	{
		pWidget->pWidget = cloneCheckboxWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_ONE_LINE_CHECKBOX:
	{
		pWidget->pWidget = cloneOneLineCheckboxWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_CURSOR_LOCATION:
	{
		pWidget->pWidget = cloneCursorLocationWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_DROPDOWN:
	{
		pWidget->pWidget = cloneDropdownWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_COLUMN:
	{
		pWidget->pWidget = cloneColumnWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_PROGRESS_BAR:
	{
		pWidget->pWidget = cloneProgressBarWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_COLOR_SLIDER:
	{
		pWidget->pWidget = cloneColorSliderWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_HISTOGRAM:
	{
		pWidget->pWidget = cloneHistogramWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_PLOT_LINES:
	{
		pWidget->pWidget = clonePlotLinesWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_COLOR_PICKER:
	{
		pWidget->pWidget = cloneColorPickerWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_TEXTBOX:
	{
		pWidget->pWidget = cloneTextboxWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_DYNAMIC_TEXT:
	{
		pWidget->pWidget = cloneDynamicTextWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_FILLED_RECT:
	{
		pWidget->pWidget = cloneFilledRectWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_DRAW_TEXT:
	{
		pWidget->pWidget = cloneDrawTextWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_DRAW_TOOLTIP:
	{
		pWidget->pWidget = cloneDrawTooltipWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_DRAW_LINE:
	{
		pWidget->pWidget = cloneDrawLineWidget(pOtherWidget->pWidget);
		break;
	}

	case WIDGET_TYPE_DRAW_CURVE:
	{
		pWidget->pWidget = cloneDrawCurveWidget(pOtherWidget->pWidget);
		break;
	}

	default:
		ASSERT(0);
	}

	return pWidget;
}
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

// CollapsingHeaderWidget public functions
UIWidget* uiCreateCollapsingHeaderSubWidget(CollapsingHeaderWidget* pWidget, const char* pLabel, const void* pSubWidget, WidgetType type)
{
#ifdef ENABLE_FORGE_UI
	UIWidget widget{};
	widget.mType = type;
	widget.pWidget = (void*)pSubWidget;
	strcpy(widget.mLabel, pLabel);

	pWidget->mGroupedWidgets.emplace_back(cloneWidget(&widget));
	return pWidget->mGroupedWidgets.back();
#else
	return NULL;
#endif
}

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
// MARK: - Public UIWidget Add/Remove Functions
/****************************************************************************/

// UIComponent public functions
UIWidget* uiCreateComponentWidget(UIComponent* pGui,const char* pLabel, const void* pWidget, WidgetType type, bool clone /* = true*/)
{
	UIWidget* pBaseWidget = (UIWidget*)tf_calloc(1, sizeof(UIWidget));
	pBaseWidget->mType = type;
	pBaseWidget->pWidget = (void*)pWidget;
	strcpy(pBaseWidget->mLabel, pLabel);

	pGui->mWidgets.emplace_back(clone ? cloneWidget(pBaseWidget) : pBaseWidget);
	pGui->mWidgetsClone.emplace_back(clone);

	if (clone)
		tf_free(pBaseWidget);

	return pGui->mWidgets.back();
}

/****************************************************************************/
// MARK: - Safe Public Setter Functions
/****************************************************************************/

void uiSetComponentFlags(UIComponent* pGui, int32_t flags)
{
#ifdef ENABLE_FORGE_UI
	ASSERT(pGui);

	pGui->mFlags = flags;
#endif
}

void uiSetWidgetDeferred(UIWidget* pWidget, bool deferred)
{
#ifdef ENABLE_FORGE_UI
	ASSERT(pWidget);

	pWidget->mDeferred = deferred;
#endif
}

void uiSetWidgetOnHoverCallback(UIWidget* pWidget, WidgetCallback callback)
{
#ifdef ENABLE_FORGE_UI
	ASSERT(pWidget);
	ASSERT(callback);

	pWidget->pOnHover = callback;
#endif
}

void uiSetWidgetOnActiveCallback(UIWidget* pWidget, WidgetCallback callback)
{
#ifdef ENABLE_FORGE_UI
	ASSERT(pWidget);
	ASSERT(callback);

	pWidget->pOnActive = callback;
#endif
}

void uiSetWidgetOnFocusCallback(UIWidget* pWidget, WidgetCallback callback)
{
#ifdef ENABLE_FORGE_UI
	ASSERT(pWidget);
	ASSERT(callback);

	pWidget->pOnFocus = callback;
#endif
}

void uiSetWidgetOnEditedCallback(UIWidget* pWidget, WidgetCallback callback)
{
#ifdef ENABLE_FORGE_UI
	ASSERT(pWidget);
	ASSERT(callback);

	pWidget->pOnEdited = callback;
#endif
}

void uiSetWidgetOnDeactivatedCallback(UIWidget* pWidget, WidgetCallback callback)
{
#ifdef ENABLE_FORGE_UI
	ASSERT(pWidget);
	ASSERT(callback);

	pWidget->pOnDeactivated = callback;
#endif
}

void uiSetWidgetOnDeactivatedAfterEditCallback(UIWidget* pWidget, WidgetCallback callback)
{
#ifdef ENABLE_FORGE_UI
	ASSERT(pWidget);
	ASSERT(callback);

	pWidget->pOnDeactivatedAfterEdit = callback;
#endif
}