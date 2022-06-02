#include "OS/Interfaces/IApp.h"

class App :public IApp 
{
public:
	const char* GetName() { return "Test Demo"; }
};


DEFINE_APPLICATION_MAIN(App)