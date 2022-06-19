#include "../OS/Interfaces/IApp.h"
#include "../OS/Interfaces/ILog.h"
#include "../Renderer/Include/IRenderer.h"


class App :public IApp 
{
public:
	bool Init()
	{
		LOGF(LogLevel::eERROR,"error ocure");
		return true;
	}
	const char* GetName() { return "Test Demo"; }
};


DEFINE_APPLICATION_MAIN(App)