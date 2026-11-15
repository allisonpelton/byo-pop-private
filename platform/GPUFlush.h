// Sending a renderer's commands to the GPU early (framework/DDInterface.cpp's NoteUpload). A file of its own because it
// needs the real Windows headers, which the framework's own Windows types clash with.

#ifndef PLATFORM_GPUFLUSH_H
#define PLATFORM_GPUFLUSH_H

struct SDL_Renderer;

namespace Platform
{

// Sends the commands SDL's Direct3D 11 renderer has given its device to the GPU (the immediate context's Flush), without
// waiting for them to run; does nothing for any other renderer.
void FlushRendererCommands(SDL_Renderer* theRenderer);

}

#endif
