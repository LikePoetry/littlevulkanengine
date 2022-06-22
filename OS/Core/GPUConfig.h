#pragma once
#include "Config.h"
#include "../Interfaces/ILog.h"
#include "../Interfaces/IFileSystem.h"
#include "../Renderer/Include/IRenderer.h"


#include <stddef.h>
#include <regex>

inline void fsReadFromStreamLine(FileStream* stream, char* pOutLine)
{
	uint32_t charIndex = 0;
	while (!fsStreamAtEnd(stream))
	{
		char nextChar = 0;
		fsReadFromStream(stream, &nextChar, sizeof(nextChar));
		if (nextChar == 0 || nextChar == '\n')
		{
			break;
		}
		if (nextChar == '\r')
		{
			char newLine = 0;
			fsReadFromStream(stream, &newLine, sizeof(newLine));
			if (newLine == '\n')
			{
				break;
			}
			else
			{
				// We're not looking at a "\r\n" sequence, so add the '\r' to the buffer.
				fsSeekStream(stream, SBO_CURRENT_POSITION, -1);
			}
		}
		pOutLine[charIndex++] = nextChar;
	}

	pOutLine[charIndex] = 0;
}

inline GPUPresetLevel stringToPresetLevel(const char* presetLevel)
{
	if (!stricmp(presetLevel, "office"))
		return GPU_PRESET_OFFICE;
	if (!stricmp(presetLevel, "low"))
		return GPU_PRESET_LOW;
	if (!stricmp(presetLevel, "medium"))
		return GPU_PRESET_MEDIUM;
	if (!stricmp(presetLevel, "high"))
		return GPU_PRESET_HIGH;
	if (!stricmp(presetLevel, "ultra"))
		return GPU_PRESET_ULTRA;

	return GPU_PRESET_NONE;
}

inline const char* presetLevelToString(GPUPresetLevel preset)
{
	switch (preset)
	{
	case GPU_PRESET_NONE: return "";
	case GPU_PRESET_OFFICE: return "office";
	case GPU_PRESET_LOW: return "low";
	case GPU_PRESET_MEDIUM: return "medium";
	case GPU_PRESET_HIGH: return "high";
	case GPU_PRESET_ULTRA: return "ultra";
	default: return NULL;
	}
}

inline bool parseConfigLine(
	const char* pLine,
	const char* pInVendorId,
	const char* pInModelId,
	const char* pInModelName,
	char pOutVendorId[MAX_GPU_VENDOR_STRING_LENGTH],
	char pOutModelId[MAX_GPU_VENDOR_STRING_LENGTH],
	char pOutRevisionId[MAX_GPU_VENDOR_STRING_LENGTH],
	char pOutModelName[MAX_GPU_VENDOR_STRING_LENGTH],
	GPUPresetLevel* pOutPresetLevel)
{
	const char* pOrigLine = pLine;
	ASSERT(pLine && pOutPresetLevel);
	*pOutPresetLevel = GPU_PRESET_LOW;

	// exclude comments from line
	size_t lineSize = strcspn(pLine, "#");
	const char* pLineEnd = pLine + lineSize;

	// Exclude whitespace in the begining (for early exit)
	while (pLine != pLineEnd && isspace(*pLine))
		++pLine;

	if (pLine == pLineEnd)
		return false;

	char presetLevel[MAX_GPU_VENDOR_STRING_LENGTH];

	char* tokens[] = {
		pOutVendorId,
		pOutModelId,
		presetLevel,
		pOutModelName,
		pOutRevisionId,
		// codename is not used
	};

	// Initialize all to empty string
	for (uint32_t i = 0; i < sizeof(tokens) / sizeof(tokens[0]); ++i)
	{
		tokens[i][0] = '\0';
	}

	// Tokenize line
	for (uint32_t i = 0; pLine != pLineEnd && i < sizeof(tokens) / sizeof(tokens[0]); ++i)
	{
		const char* begin = pLine;
		const char* end = pLine + strcspn(pLine, ";");

		if (end > pLineEnd)
			end = pLineEnd;

		pLine = end + 1;
		pLine = pLine < pLineEnd ? pLine : pLineEnd;

		/* trim whitespace characters in the begining*/
		while (begin != end && isspace(*begin))
			++begin;

		/* trim whitespace characters in the end*/
		while (begin != end && isspace(*(end - 1)))
			--end;


		ptrdiff_t length = end - begin;
		ASSERT(length < MAX_GPU_VENDOR_STRING_LENGTH);

		strncpy(tokens[i], begin, length);
		tokens[i][length] = '\0';
	}


	// validate required fields
	if (pOutVendorId[0] == '\0' ||
		pOutModelId[0] == '\0' ||
		presetLevel[0] == '\0' ||
		pOutModelName[0] == '\0')
	{
		LOGF(eWARNING, "GPU config requires VendorId, DeviceId, Classification and Name. "
			"Following line has invalid format:\n'%s'", pOrigLine);
		return false;
	}

	// convert ids to lower case
	for (char* p = pOutVendorId; *p != '\0'; ++p)
		*p = tolower(*p);
	for (char* p = pOutModelId; *p != '\0'; ++p)
		*p = tolower(*p);
	for (char* p = presetLevel; *p != '\0'; ++p)
		*p = tolower(*p);


	// Parsing logic

	*pOutPresetLevel = stringToPresetLevel(presetLevel);

	bool success = true;

	if (pInVendorId)
		success = success && strcmp(pInVendorId, pOutVendorId) == 0;

	if (pInModelId)
		success = success && strcmp(pInModelId, pOutModelId) == 0;

	if (pInModelName)
		success = success && strcmp(pInModelName, pOutModelName) == 0;

	return success;
}

static GPUPresetLevel getSinglePresetLevel(const char* line, const char* inVendorId, const char* inModelId, const char* inRevId)
{
	char vendorId[MAX_GPU_VENDOR_STRING_LENGTH] = {};
	char deviceId[MAX_GPU_VENDOR_STRING_LENGTH] = {};
	GPUPresetLevel presetLevel = {};
	char gpuName[MAX_GPU_VENDOR_STRING_LENGTH] = {};
	char revisionId[MAX_GPU_VENDOR_STRING_LENGTH] = {};

	//check if current vendor line is one of the selected gpu's
	if (!parseConfigLine(line, inVendorId, inModelId, NULL, vendorId, deviceId, revisionId, gpuName, &presetLevel))
		return GPU_PRESET_NONE;

	//if we have a revision Id then we want to match it as well
	if (stricmp(inRevId, "0x00") != 0 && strlen(revisionId) && stricmp(revisionId, "0x00") != 0 && stricmp(inRevId, revisionId) != 0)
		return GPU_PRESET_NONE;

	return presetLevel;
}

//Reads the gpu config and sets the preset level of all available gpu's
static GPUPresetLevel getGPUPresetLevel(const char* vendorId, const char* modelId, const char* revId)
{
	FileStream fh = {};
	if (!fsOpenStreamFromPath(RD_GPU_CONFIG, "gpu.cfg", FM_READ, NULL, &fh))
	{
		LOGF(LogLevel::eWARNING, "gpu.cfg could not be found, setting preset to Low as a default.");
		return GPU_PRESET_LOW;
	}

	GPUPresetLevel foundLevel = GPU_PRESET_LOW;

	char gpuCfgString[1024] = {};
	while (!fsStreamAtEnd(&fh))
	{
		fsReadFromStreamLine(&fh, gpuCfgString);
		GPUPresetLevel level = getSinglePresetLevel(gpuCfgString, vendorId, modelId, revId);
		// Do something with the tok
		if (level != GPU_PRESET_NONE)
		{
			foundLevel = level;
			break;
		}
	}

	fsCloseStream(&fh);
	return foundLevel;
}


