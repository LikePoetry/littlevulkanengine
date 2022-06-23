#include "../Interfaces/IScreenshot.h"


static Cmd* pCmd = 0;
static CmdPool* pCmdPool = 0;
static Renderer* pRendererRef = 0;
extern RendererApi gSelectedRendererApi;

void initScreenshotInterface(Renderer* pRenderer, Queue* pGraphicsQueue)
{
	ASSERT(pRenderer);
	ASSERT(pGraphicsQueue);

	pRendererRef = pRenderer;

	// Allocate a command buffer for the GPU work. We use the app's rendering queue to avoid additional sync.
	CmdPoolDesc cmdPoolDesc = {};
	cmdPoolDesc.pQueue = pGraphicsQueue;
	cmdPoolDesc.mTransient = true;
	vk_addCmdPool(pRenderer, &cmdPoolDesc, &pCmdPool);

	CmdDesc cmdDesc = {};
	cmdDesc.pPool = pCmdPool;
	vk_addCmd(pRenderer, &cmdDesc, &pCmd);
}