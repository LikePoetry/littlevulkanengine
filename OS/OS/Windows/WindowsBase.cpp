


#include "../Interfaces/IOperatingSystem.h"
#include "../Interfaces/IApp.h"
#include "../Interfaces/IMemory.h"


int WindowsMain(int argc,char** argv,IApp* app)
{
	if (!initMemAlloc(app->GetName()))
		return EXIT_FAILURE;

	
	return 0;
}