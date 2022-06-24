#pragma once
#include "../Core/Config.h"
#include "../Math/MathTypes.h"
#include "../Interfaces/ICameraController.h"

/****************************************************************************/
// MARK: - Public Font System Structs
/****************************************************************************/

/// Creation information for initializing Forge Rendering for fonts and text
typedef struct FontSystemDesc
{

	void* pRenderer = NULL; // Renderer*
	uint32_t    mFontstashRingSizeBytes = 1024 * 1024;

} FontSystemDesc;

/// Creation information for loading a font from a file using The Forge
typedef struct FontDesc
{

	const char* pFontName = "default";
	const char* pFontPath = NULL;
	const char* pFontPassword = NULL;

} FontDesc;


/****************************************************************************/
// MARK: - Application Life Cycle 
/****************************************************************************/

/// Initializes Forge Rendering objects associated with Font Rendering
/// To be called at application initialization time by the App Layer
bool initFontSystem(FontSystemDesc* pDesc);

/****************************************************************************/
// MARK: - Other Font System Functionality
/****************************************************************************/
/// Loads an array of fonts from files and returns an array of their ID handles
void fntDefineFonts(const FontDesc* pDescs, uint32_t count, uint32_t* pOutIDs);