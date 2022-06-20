#pragma once

#include "../Core/Config.h"

#include "../../ThirdParty/OpenSource/EASTL/vector.h"

#include "../Math/MathTypes.h"

#define MAX_LABEL_STR_LENGTH 128
#define MAX_FORMAT_STR_LENGTH 30
#define MAX_TITLE_STR_LENGTH 128

/****************************************************************************/
// MARK: - UI Widget Data Structures
/****************************************************************************/
typedef void(*WidgetCallback)();

enum WidgetType
{
	WIDGET_TYPE_COLLAPSING_HEADER,
	WIDGET_TYPE_DEBUG_TEXTURES,
	WIDGET_TYPE_LABEL,
	WIDGET_TYPE_COLOR_LABEL,
	WIDGET_TYPE_HORIZONTAL_SPACE,
	WIDGET_TYPE_SEPARATOR,
	WIDGET_TYPE_VERTICAL_SEPARATOR,
	WIDGET_TYPE_BUTTON,
	WIDGET_TYPE_SLIDER_FLOAT,
	WIDGET_TYPE_SLIDER_FLOAT2,
	WIDGET_TYPE_SLIDER_FLOAT3,
	WIDGET_TYPE_SLIDER_FLOAT4,
	WIDGET_TYPE_SLIDER_INT,
	WIDGET_TYPE_SLIDER_UINT,
	WIDGET_TYPE_RADIO_BUTTON,
	WIDGET_TYPE_CHECKBOX,
	WIDGET_TYPE_ONE_LINE_CHECKBOX,
	WIDGET_TYPE_CURSOR_LOCATION,
	WIDGET_TYPE_DROPDOWN,
	WIDGET_TYPE_COLUMN,
	WIDGET_TYPE_PROGRESS_BAR,
	WIDGET_TYPE_COLOR_SLIDER,
	WIDGET_TYPE_HISTOGRAM,
	WIDGET_TYPE_PLOT_LINES,
	WIDGET_TYPE_COLOR_PICKER,
	WIDGET_TYPE_TEXTBOX,
	WIDGET_TYPE_DYNAMIC_TEXT,
	WIDGET_TYPE_FILLED_RECT,
	WIDGET_TYPE_DRAW_TEXT,
	WIDGET_TYPE_DRAW_TOOLTIP,
	WIDGET_TYPE_DRAW_LINE,
	WIDGET_TYPE_DRAW_CURVE
};

typedef struct UIWidget
{
	WidgetType mType = {};                 // Type of the underlying widget
	void* pWidget = NULL;                  // Underlying widget

	WidgetCallback pOnHover = NULL;        // Widget is hovered, usable, and not blocked by anything.
	WidgetCallback pOnActive = NULL;       // Widget is currently active (ex. button being held)
	WidgetCallback pOnFocus = NULL;        // Widget is currently focused (for keyboard/gamepad nav)
	WidgetCallback pOnEdited = NULL;       // Widget just changed its underlying value or was pressed.
	WidgetCallback pOnDeactivated = NULL;  // Widget was just made inactive from an active state.  This is useful for undo/redo patterns.
	WidgetCallback
		pOnDeactivatedAfterEdit = NULL;    // Widget was just made inactive from an active state and changed its underlying value.  This is useful for undo/redo patterns.

	char mLabel[MAX_LABEL_STR_LENGTH]{};

	// Set this to process deferred callbacks that may cause global program state changes.
	bool mDeferred = false;

	bool mHovered = false;
	bool mActive = false;
	bool mFocused = false;
	bool mEdited = false;
	bool mDeactivated = false;
	bool mDeactivatedAfterEdit = false;
} UIWidget;

typedef struct CollapsingHeaderWidget
{
	eastl::vector<UIWidget*>	  mGroupedWidgets;
	bool                      mCollapsed = true;
	bool					  mPreviousCollapsed = false;
	bool                      mDefaultOpen = false;
	bool					  mHeaderIsVisible = true;
}CollapsingHeaderWidget;

typedef struct DebugTexturesWidget
{
	eastl::vector<void* /* Texture* */> mTextures;
	float2                              mTextureDisplaySize = float2(512.f, 512.f);
}DebugTexturesWidget;

typedef struct LabelWidget
{
}LabelWidget;

typedef struct ColorLabelWidget
{
	float4 mColor = float4(0.f, 0.f, 0.f, 0.f);
}ColorLabelWidget;

typedef struct HorizontalSpaceWidget
{
}HorizontalSpaceWidget;

typedef struct SeparatorWidget
{
}SeparatorWidget;

typedef struct VerticalSeparatorWidget
{
	uint32_t mLineCount = 0;
}VerticalSeparatorWidget;

typedef struct ButtonWidget
{
}ButtonWidget;

typedef struct SliderFloatWidget
{
	char    mFormat[MAX_FORMAT_STR_LENGTH] = { "%.3f" };
	float* pData = NULL;
	float   mMin = 0.f;
	float   mMax = 0.f;
	float   mStep = 0.01f;
}SliderFloatWidget;

typedef struct SliderFloat2Widget
{
	char     mFormat[MAX_FORMAT_STR_LENGTH] = { "%.3f" };
	float2* pData = NULL;
	float2   mMin = float2(0.f, 0.f);
	float2   mMax = float2(0.f, 0.f);
	float2   mStep = float2(0.01f, 0.01f);
}SliderFloat2Widget;

typedef struct SliderFloat3Widget
{
	char     mFormat[MAX_FORMAT_STR_LENGTH] = { "%.3f" };
	float3* pData = NULL;
	float3   mMin = float3(0.f, 0.f, 0.f);
	float3   mMax = float3(0.f, 0.f, 0.f);
	float3   mStep = float3(0.01f, 0.01f, 0.01f);
}SliderFloat3Widget;

typedef struct SliderFloat4Widget
{
	char     mFormat[MAX_FORMAT_STR_LENGTH] = { "%.3f" };
	float4* pData = NULL;
	float4   mMin = float4(0.f, 0.f, 0.f, 0.f);
	float4   mMax = float4(0.f, 0.f, 0.f, 0.f);
	float4   mStep = float4(0.01f, 0.01f, 0.01f, 0.01f);
}SliderFloat4Widget;

typedef struct SliderIntWidget
{
	char      mFormat[MAX_FORMAT_STR_LENGTH] = { "%d" };
	int32_t* pData = NULL;
	int32_t   mMin = 0;
	int32_t   mMax = 0;
	int32_t   mStep = 1;
}SliderIntWidget;

typedef struct SliderUintWidget
{
	char       mFormat[MAX_FORMAT_STR_LENGTH] = { "%d" };
	uint32_t* pData = NULL;
	uint32_t   mMin = 0;
	uint32_t   mMax = 0;
	uint32_t   mStep = 1;
}SliderUintWidget;

typedef struct RadioButtonWidget
{
	int32_t* pData = NULL;
	int32_t  mRadioId = 0;
}RadioButtonWidget;

typedef struct CheckboxWidget
{
	bool* pData = NULL;
}CheckboxWidget;

typedef struct OneLineCheckboxWidget
{
	bool* pData = NULL;
	float4   mColor = float4(0.f, 0.f, 0.f, 0.f);
}OneLineCheckboxWidget;

typedef struct CursorLocationWidget
{
	float2 mLocation = float2(0.f, 0.f);
}CursorLocationWidget;

typedef struct DropdownWidget
{
	uint32_t* pData = NULL;
	eastl::vector<uint32_t>  mValues;
	eastl::vector<char*>	 mNames;
}DropdownWidget;

typedef struct ColumnWidget
{
	eastl::vector<UIWidget*> mPerColumnWidgets;
	uint32_t				mNumColumns = 0;
}ColumnWidget;

typedef struct ProgressBarWidget
{
	size_t* pData = NULL;
	size_t  mMaxProgress = 0;
}ProgressBarWidget;

typedef struct ColorSliderWidget
{
	float4* pData = NULL;
}ColorSliderWidget;

typedef struct HistogramWidget
{
	float* pValues = NULL;
	uint32_t	mCount = 0;
	float* mMinScale = NULL;
	float* mMaxScale = NULL;
	float2		mHistogramSize = float2(0.f, 0.f);
	const char* mHistogramTitle = NULL;
}HistogramWidget;

typedef struct PlotLinesWidget
{
	float* mValues = NULL;
	uint32_t	 mNumValues = 0;
	float* mScaleMin = NULL;
	float* mScaleMax = NULL;
	float2* mPlotScale = NULL;
	const char* mTitle = NULL;
}PlotLinesWidget;

typedef struct ColorPickerWidget
{
	float4* pData = NULL;
}ColorPickerWidget;

typedef struct TextboxWidget
{
	char* pData = NULL;
	uint32_t mLength = 0;
	bool     mAutoSelectAll = true;
}TextboxWidget;

typedef struct DynamicTextWidget
{
	char* pData = NULL;
	uint32_t mLength = 0;
	float4* pColor = NULL;
}DynamicTextWidget;

typedef struct FilledRectWidget
{
	float2	 mPos = float2(0.f, 0.f);
	float2	 mScale = float2(0.f, 0.f);
	float4   mColor = float4(0.f, 0.f, 0.f, 0.f);
}FilledRectWidget;

typedef struct DrawTextWidget
{
	float2	 mPos = float2(0.f, 0.f);
	float4   mColor = float4(0.f, 0.f, 0.f, 0.f);
}DrawTextWidget;

typedef struct DrawTooltipWidget
{
	bool* mShowTooltip = NULL;
	char* mText = NULL;
}DrawTooltipWidget;

typedef struct DrawLineWidget
{
	float2	 mPos1 = float2(0.f, 0.f);
	float2	 mPos2 = float2(0.f, 0.f);
	float4   mColor = float4(0.f, 0.f, 0.f, 0.f);
	bool	 mAddItem = false;
}DrawLineWidget;

typedef struct DrawCurveWidget
{
	float2* mPos = NULL;
	uint32_t mNumPoints = 0;
	float	 mThickness = 0.f;
	float4	 mColor = float4(0.f, 0.f, 0.f, 0.f);
}DrawCurveWidget;

/****************************************************************************/
// MARK: - UI Component Data Structures
/****************************************************************************/

enum GuiComponentFlags
{
	GUI_COMPONENT_FLAGS_NONE = 0,
	GUI_COMPONENT_FLAGS_NO_TITLE_BAR = 1 << 0,                 // Disable title-bar
	GUI_COMPONENT_FLAGS_NO_RESIZE = 1 << 1,                    // Disable user resizing
	GUI_COMPONENT_FLAGS_NO_MOVE = 1 << 2,                      // Disable user moving the window
	GUI_COMPONENT_FLAGS_NO_SCROLLBAR = 1 << 3,                 // Disable scrollbars (window can still scroll with mouse or programatically)
	GUI_COMPONENT_FLAGS_NO_COLLAPSE = 1 << 4,                  // Disable user collapsing window by double-clicking on it
	GUI_COMPONENT_FLAGS_ALWAYS_AUTO_RESIZE = 1 << 5,           // Resize every window to its content every frame
	GUI_COMPONENT_FLAGS_NO_INPUTS = 1 << 6,                    // Disable catching mouse or keyboard inputs, hovering test with pass through.
	GUI_COMPONENT_FLAGS_MEMU_BAR = 1 << 7,                     // Has a menu-bar
	GUI_COMPONENT_FLAGS_HORIZONTAL_SCROLLBAR = 1 << 8,         // Allow horizontal scrollbar to appear (off by default).
	GUI_COMPONENT_FLAGS_NO_FOCUS_ON_APPEARING = 1 << 9,        // Disable taking focus when transitioning from hidden to visible state
	GUI_COMPONENT_FLAGS_NO_BRING_TO_FRONT_ON_FOCUS = 1 << 10,  // Disable bringing window to front when taking focus (e.g. clicking on it or programatically giving it focus)
	GUI_COMPONENT_FLAGS_ALWAYS_VERTICAL_SCROLLBAR = 1 << 11,   // Always show vertical scrollbar (even if ContentSize.y < Size.y)
	GUI_COMPONENT_FLAGS_ALWAYS_HORIZONTAL_SCROLLBAR = 1 << 12, // Always show horizontal scrollbar (even if ContentSize.x < Size.x)
	GUI_COMPONENT_FLAGS_ALWAYS_USE_WINDOW_PADDING = 1 << 13,   // Ensure child windows without border uses style.WindowPadding (ignored by default for non-bordered child windows, because more convenient)
	GUI_COMPONENT_FLAGS_NO_NAV_INPUT = 1 << 14,                // No gamepad/keyboard navigation within the window
	GUI_COMPONENT_FLAGS_NO_NAV_FOCUS = 1 << 15,                // No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB)
	GUI_COMPONENT_FLAGS_START_COLLAPSED = 1 << 16
};


typedef struct UIComponentDesc
{
	vec2 mStartPosition = vec2{ 0.0f,150.0f };
	vec2 mStartSize = vec2{ 600.0f,550.0f };

	uint32_t mFontID = 0;
	float mFontSize = 16.0f;
} UIComponentDesc;

typedef struct UIComponent
{
	eastl::vector<UIWidget*>		mWidgets;
	eastl::vector<bool>				mWidgetsClone;
	// Contextual menus when right clicking the title bar
	eastl::vector<char*>		   mContextualMenuLabels;
	eastl::vector<WidgetCallback>  mContextualMenuCallbacks;
	float4                         mInitialWindowRect = float4(0.f, 0.f, 0.f, 0.f);
	float4                         mCurrentWindowRect = float4(0.f, 0.f, 0.f, 0.f);
	char                           mTitle[MAX_TITLE_STR_LENGTH]{};
	uintptr_t                      pFont = 0;
	float                          mAlpha = 0.f;

	// defaults to GUI_COMPONENT_FLAGS_ALWAYS_AUTO_RESIZE
	// on mobile, GUI_COMPONENT_FLAGS_START_COLLAPSED is also set
	int32_t                        mFlags = 0;

	bool                           mActive = false;

	// UI Component settings that can be modified at runtime by the client.
	bool                           mHasCloseButton = false;

} UIComponent;


/****************************************************************************/
// MARK: - UI Component Public Functions
/****************************************************************************/

/// Create a UI Component "window" to which Widgets can be added
/// User is NOT responsible for freeing this memory at application exit
void uiCreateComponent(const char* pTitle, const UIComponentDesc* pDesc, UIComponent** ppGuiComponent);

/// Create a Widget to be assigned to a given UI Component
/// User is NOT responsible for freeing this memory at application exit
UIWidget* uiCreateComponentWidget(UIComponent* pGui, const char* pLabel, const void* pWidget, WidgetType type, bool clone = true); //-V1071

/****************************************************************************/
// MARK: - Safe UI Component and Widget Setter Functions
/****************************************************************************/

// NOTE: These functions exist to protect scope against null-pointer derefs
// on UI Component and Widget handles if this functionality still exists in 
// app code while the UI Master Switch is disabled.

/// Assign "GuiComponentFlags" enum values to a given UI Component
void uiSetComponentFlags(UIComponent* pGui, int32_t flags);

