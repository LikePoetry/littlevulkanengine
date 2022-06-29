#pragma once
#include "../Renderer/Include/IRenderer.h"

void initScreenshotInterface(Renderer* pRenderer, Queue* pQueue);
// Use one renderpass prior to calling captureScreenshot() to prepare pSwapChain for copy.
bool prepareScreenshot(SwapChain* pSwapChain);

void captureScreenshot(SwapChain* pSwapChain, uint32_t swapChainRtIndex, ResourceState renderTargetCurrentState, const char* pngFileName);
void captureScreenshot(SwapChain* pSwapChain, uint32_t swapChainRtIndex, ResourceState renderTargetCurrentState, const char* pngFileName, bool noAlpha);
void exitScreenshotInterface();