// presentbench.cpp <driver|-> <mode> <frames> [novsync]: what presenting a 640x480 software-drawn frame costs a process, per frame, in
// the cycles of its main thread and of its other threads (the Direct3D driver's), with no game in it (GAME_NOTES.md, TipTop's
// "Performance, second round"). A frame changes 3000 pixels of the picture, then:
//   mode 0  SDL_UpdateTexture of the whole picture, SDL_RenderClear, SDL_RenderCopy, SDL_RenderPresent (the framework's path)
//   mode 1  the same with a 1x1 SDL_UpdateTexture      mode 2  the same without an upload     mode 3  a 160x76 upload
//   mode 4  the picture through SDL_LockTexture/UnlockTexture of a streaming texture
//   mode 5  no renderer: the picture copied into SDL_GetWindowSurface's pixels and SDL_UpdateWindowSurface, paced by a 60 Hz timer
// <driver> is a render driver name for SDL_HINT_RENDER_DRIVER, or - for SDL's choice; novsync leaves SDL_RENDERER_PRESENTVSYNC out
// and paces by the timer too. Build (32-bit llvm-mingw, SDL2's mingw package), run under build/native/rundesk.exe:
//   i686-w64-mingw32-clang++ -O2 -std=gnu++17 presentbench.cpp -I<SDL2>/include/SDL2 -L<SDL2>/lib -lSDL2 -lwinmm -o presentbench.exe
#define SDL_MAIN_HANDLED
#define _WIN32_WINNT 0x0A00
#include <SDL.h>
#include <windows.h>
#include <mmsystem.h>
#include <realtimeapiset.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		printf("presentbench <driver|-> <mode 0-5> <frames> [novsync]\n");
		return 1;
	}
	const char* aDriver = argv[1];
	int aMode = atoi(argv[2]);
	int aFrames = atoi(argv[3]);
	bool aNoVsync = argc > 4;
	if (strcmp(aDriver, "-") != 0)
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, aDriver);
	SDL_Init(SDL_INIT_VIDEO);
	timeBeginPeriod(1);
	SDL_Window* aWindow = SDL_CreateWindow("presentbench", 100, 100, 640, 480, 0);
	std::vector<uint32_t> aPixels(640 * 480, 0xff203040);
	Uint64 aNext = SDL_GetPerformanceCounter();
	auto aPace = [&]()
	{
		aNext += SDL_GetPerformanceFrequency() / 60;
		Sint64 aWaitMs = (Sint64)((aNext - SDL_GetPerformanceCounter()) * 1000 / SDL_GetPerformanceFrequency());
		if (aWaitMs > 1)
			SDL_Delay((Uint32)(aWaitMs - 1));
	};
	auto aChange = [&](int theFrame)
	{
		for (int i = 0; i < 3000; i++)
			aPixels[(theFrame * 3001 + i * 97) % (640 * 480)] = 0xff000000 | (theFrame * 77 + i);
	};
	ULONG64 aMain0, aProc0, aMain1, aProc1;
	Uint64 aT0, aT1;
	const char* aName = "window surface";

	if (aMode == 5)
	{
		SDL_Surface* aSurface = SDL_GetWindowSurface(aWindow);
		if (aSurface == nullptr)
		{
			printf("no surface: %s\n", SDL_GetError());
			return 1;
		}
		QueryThreadCycleTime(GetCurrentThread(), &aMain0);
		QueryProcessCycleTime(GetCurrentProcess(), &aProc0);
		aT0 = SDL_GetPerformanceCounter();
		for (int f = 0; f < aFrames; f++)
		{
			aChange(f);
			SDL_LockSurface(aSurface);
			for (int y = 0; y < 480; y++)
				memcpy((char*)aSurface->pixels + y * aSurface->pitch, aPixels.data() + y * 640, 640 * 4);
			SDL_UnlockSurface(aSurface);
			SDL_UpdateWindowSurface(aWindow);
			aPace();
		}
		aT1 = SDL_GetPerformanceCounter();
	}
	else
	{
		SDL_Renderer* aRenderer = SDL_CreateRenderer(aWindow, -1,
			SDL_RENDERER_ACCELERATED | (aNoVsync ? 0 : SDL_RENDERER_PRESENTVSYNC) | SDL_RENDERER_TARGETTEXTURE);
		if (aRenderer == nullptr)
		{
			printf("no renderer: %s\n", SDL_GetError());
			return 1;
		}
		SDL_RendererInfo anInfo;
		SDL_GetRendererInfo(aRenderer, &anInfo);
		aName = anInfo.name;
		SDL_Texture* aTexture = SDL_CreateTexture(aRenderer, SDL_PIXELFORMAT_ARGB8888,
			aMode == 4 ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC, 640, 480);
		for (int i = 0; i < 20; i++)  // warm up
		{
			SDL_UpdateTexture(aTexture, nullptr, aPixels.data(), 640 * 4);
			SDL_RenderClear(aRenderer);
			SDL_RenderCopy(aRenderer, aTexture, nullptr, nullptr);
			SDL_RenderPresent(aRenderer);
		}
		aNext = SDL_GetPerformanceCounter();
		QueryThreadCycleTime(GetCurrentThread(), &aMain0);
		QueryProcessCycleTime(GetCurrentProcess(), &aProc0);
		aT0 = SDL_GetPerformanceCounter();
		for (int f = 0; f < aFrames; f++)
		{
			aChange(f);
			if (aMode == 0)
				SDL_UpdateTexture(aTexture, nullptr, aPixels.data(), 640 * 4);
			else if (aMode == 1)
			{
				SDL_Rect aRect = {0, 0, 1, 1};
				SDL_UpdateTexture(aTexture, &aRect, aPixels.data(), 640 * 4);
			}
			else if (aMode == 3)
			{
				SDL_Rect aRect = {400, 360, 160, 76};
				SDL_UpdateTexture(aTexture, &aRect, aPixels.data() + 360 * 640 + 400, 640 * 4);
			}
			else if (aMode == 4)
			{
				void* aLocked;
				int aPitch;
				SDL_LockTexture(aTexture, nullptr, &aLocked, &aPitch);
				for (int y = 0; y < 480; y++)
					memcpy((char*)aLocked + y * aPitch, aPixels.data() + y * 640, 640 * 4);
				SDL_UnlockTexture(aTexture);
			}
			SDL_RenderClear(aRenderer);
			SDL_RenderCopy(aRenderer, aTexture, nullptr, nullptr);
			SDL_RenderPresent(aRenderer);
			if (aNoVsync)
				aPace();
		}
		aT1 = SDL_GetPerformanceCounter();
	}
	QueryThreadCycleTime(GetCurrentThread(), &aMain1);
	QueryProcessCycleTime(GetCurrentProcess(), &aProc1);
	double aSecs = (double)(aT1 - aT0) / SDL_GetPerformanceFrequency();
	double aMain = (double)(aMain1 - aMain0) / aFrames, aAll = (double)(aProc1 - aProc0) / aFrames;
	printf("%s mode %d: %.1f fps, per frame: main %.0f k cycles, other threads %.0f k cycles\n", aName, aMode, aFrames / aSecs,
		aMain / 1000, (aAll - aMain) / 1000);
	return 0;
}
