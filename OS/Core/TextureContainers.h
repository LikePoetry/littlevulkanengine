#pragma once
#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/IFileSystem.h"
#include "../Interfaces/ILog.h"

#include "../../Renderer/Include/IRenderer.h"

#include "../../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_base.h"
#include "../../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_query.h"
#include "../../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_bits.h"
#include "../../ThirdParty/OpenSource/tinyimageformat/tinyimageformat_apis.h"


/************************************************************************/
// Surface Utils
/************************************************************************/
static inline bool util_get_surface_info(
	uint32_t width,
	uint32_t height,
	TinyImageFormat fmt,
	uint32_t* outNumBytes,
	uint32_t* outRowBytes,
	uint32_t* outNumRows)
{
	uint64_t numBytes = 0;
	uint64_t rowBytes = 0;
	uint64_t numRows = 0;

	uint32_t bpp = TinyImageFormat_BitSizeOfBlock(fmt);
	bool compressed = TinyImageFormat_IsCompressed(fmt);
	bool planar = TinyImageFormat_IsPlanar(fmt);

	bool packed = false;
	if (compressed)
	{
		uint32_t blockWidth = TinyImageFormat_WidthOfBlock(fmt);
		uint32_t blockHeight = TinyImageFormat_HeightOfBlock(fmt);
		uint32_t numBlocksWide = 0;
		uint32_t numBlocksHigh = 0;
		if (width > 0)
		{
			numBlocksWide = max(1U, (width + (blockWidth - 1)) / blockWidth);
		}
		if (height > 0)
		{
			numBlocksHigh = max(1u, (height + (blockHeight - 1)) / blockHeight);
		}

		rowBytes = numBlocksWide * (bpp >> 3);
		numRows = numBlocksHigh;
		numBytes = rowBytes * numBlocksHigh;
	}
	else if (packed) //-V547
	{
		LOGF(eERROR, "Not implemented");
		return false;
		//rowBytes = ((uint64_t(width) + 1u) >> 1) * bpe;
		//numRows = uint64_t(height);
		//numBytes = rowBytes * height;
	}
	else if (planar)
	{
		uint32_t numOfPlanes = TinyImageFormat_NumOfPlanes(fmt);

		for (uint32_t i = 0; i < numOfPlanes; ++i)
		{
			numBytes += TinyImageFormat_PlaneWidth(fmt, i, width) * TinyImageFormat_PlaneHeight(fmt, i, height) * TinyImageFormat_PlaneSizeOfBlock(fmt, i);
		}

		numRows = 1;
		rowBytes = numBytes;
	}
	else
	{
		if (!bpp)
			return false;

		rowBytes = (uint64_t(width) * bpp + 7u) / 8u; // round up to nearest byte
		numRows = uint64_t(height);
		numBytes = rowBytes * height;
	}

	if (numBytes > UINT32_MAX || rowBytes > UINT32_MAX || numRows > UINT32_MAX) //-V560
		return false;

	if (outNumBytes)
	{
		*outNumBytes = (uint32_t)numBytes;
	}
	if (outRowBytes)
	{
		*outRowBytes = (uint32_t)rowBytes;
	}
	if (outNumRows)
	{
		*outNumRows = (uint32_t)numRows;
	}

	return true;
}