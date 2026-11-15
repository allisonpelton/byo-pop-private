// Sending a renderer's commands to the GPU early (see GPUFlush.h).

#include "GPUFlush.h"

#ifdef _WIN32
#include <d3d11.h>
#endif
#include <SDL.h>

namespace Platform
{

void FlushRendererCommands(SDL_Renderer* theRenderer)
{
#ifdef _WIN32
	// NULL unless the renderer is SDL's Direct3D 11 one; the device comes with a reference, as does its context
	ID3D11Device* aDevice = SDL_RenderGetD3D11Device(theRenderer);
	if (aDevice == NULL)
		return;
	ID3D11DeviceContext* aContext = NULL;
	aDevice->GetImmediateContext(&aContext);
	if (aContext != NULL)
	{
		aContext->Flush();
		aContext->Release();
	}
	aDevice->Release();
#else
	(void) theRenderer;
#endif
}

}
