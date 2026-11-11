// scaletest: what it costs to scale a game's finished frame up to the window without a 3D device, each way open to
// us, and what each way's picture looks like (docs/COORDINATION.md, "Memory broken down, and a software path to test").
//
//   build/native/rundesk.exe build/native/scaletest.exe <frame.bmp> <out dir> [frames]
//
// The frame is a game's picture at its own size (a port's "screenshot", converted to BMP). The tool shows it <frames>
// times (200) in a window of twice its size and then full screen at the desktop's size, each way in turn, and prints one
// JSON line a way: the milliseconds a frame takes to scale and to hand to the window, and the processor cycles a frame.
// It writes each way's scaled picture to <out dir> as BMP. The ways:
//
//   sdl-nearest, sdl-linear   SDL's software renderer scales the frame's texture into the window's surface (RenderCopy),
//                             then presents the surface (a BitBlt); "exact" fills the window keeping the frame's shape,
//                             "whole" uses the largest whole-number factor and leaves borders
//   sdl-surface               SDL_BlitScaled straight to the window's surface, with no renderer
//   gdi-stretch               one StretchDIBits call with nearest sampling (COLORONCOLOR): into a bitmap in memory,
//                             which is the processor's work alone, and to the window itself
//   gdi-halftone              the same with GDI's smoothing (HALFTONE)
//
// Under rundesk the window is on a desktop nobody sees, so the hand-over to the window ("present") costs what that
// desktop makes it cost; the scaling into memory is the same work everywhere. Windows only for the GDI ways.

#ifdef _WIN32
#define _WIN32_WINNT 0x0601    // QueryProcessCycleTime
#endif
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{

int gFrames = 200;
std::string gOutDir;

double Ms(Uint64 theStart, Uint64 theEnd)
{
	return (double) (theEnd - theStart) * 1000.0 / (double) SDL_GetPerformanceFrequency();
}

unsigned long long Cycles()
{
#ifdef _WIN32
	ULONG64 aCycles = 0;
	QueryProcessCycleTime(GetCurrentProcess(), &aCycles);
	return aCycles;
#else
	return 0;
#endif
}

void Report(const char* theWhere, const char* theWay, const char* theFit, const SDL_Rect& theRect, double theScaleMs, double thePresentMs,
	unsigned long long theCycles)
{
	printf("{\"where\":\"%s\",\"way\":\"%s\",\"fit\":\"%s\",\"width\":%d,\"height\":%d,\"scaleMs\":%.3f,\"presentMs\":%.3f,\"cyclesPerFrame\":%.0f}\n",
		theWhere, theWay, theFit, theRect.w, theRect.h, theScaleMs / gFrames, thePresentMs / gFrames, (double) theCycles / gFrames);
	fflush(stdout);
}

void Save(SDL_Surface* theSurface, const char* theWhere, const char* theWay, const char* theFit)
{
	std::string aName = gOutDir + "/" + theWhere + "-" + theWay + "-" + theFit + ".bmp";
	if (SDL_SaveBMP(theSurface, aName.c_str()) != 0)
		fprintf(stderr, "cannot write %s: %s\n", aName.c_str(), SDL_GetError());
}

// SDL's software renderer: the frame is a texture, RenderCopy scales it into the window's surface
void RunRenderer(SDL_Window* theWindow, SDL_Surface* theFrame, const char* theWhere, bool isLinear, const char* theFit, const SDL_Rect& theRect)
{
	SDL_Renderer* aRenderer = SDL_CreateRenderer(theWindow, -1, SDL_RENDERER_SOFTWARE);
	if (aRenderer == NULL)
	{
		fprintf(stderr, "no software renderer: %s\n", SDL_GetError());
		return;
	}
	SDL_Texture* aTexture = SDL_CreateTexture(aRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, theFrame->w, theFrame->h);
	SDL_SetTextureScaleMode(aTexture, isLinear ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
	SDL_SetRenderDrawColor(aRenderer, 0, 0, 0, 255);
	double aScaleMs = 0, aPresentMs = 0;
	unsigned long long aCycles = Cycles();
	for (int i = 0; i < gFrames; i++)
	{
		Uint64 aStart = SDL_GetPerformanceCounter();
		SDL_UpdateTexture(aTexture, NULL, theFrame->pixels, theFrame->pitch);
		SDL_RenderClear(aRenderer);
		SDL_RenderCopy(aRenderer, aTexture, NULL, &theRect);
		SDL_RenderFlush(aRenderer);
		Uint64 aMiddle = SDL_GetPerformanceCounter();
		SDL_RenderPresent(aRenderer);
		Uint64 anEnd = SDL_GetPerformanceCounter();
		aScaleMs += Ms(aStart, aMiddle);
		aPresentMs += Ms(aMiddle, anEnd);
	}
	aCycles = Cycles() - aCycles;
	Report(theWhere, isLinear ? "sdl-linear" : "sdl-nearest", theFit, theRect, aScaleMs, aPresentMs, aCycles);

	SDL_Surface* aShot = SDL_CreateRGBSurfaceWithFormat(0, theRect.w, theRect.h, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_RenderClear(aRenderer);
	SDL_RenderCopy(aRenderer, aTexture, NULL, &theRect);
	if (SDL_RenderReadPixels(aRenderer, &theRect, SDL_PIXELFORMAT_ARGB8888, aShot->pixels, aShot->pitch) == 0)
		Save(aShot, theWhere, isLinear ? "sdl-linear" : "sdl-nearest", theFit);
	SDL_FreeSurface(aShot);
	SDL_DestroyTexture(aTexture);
	SDL_DestroyRenderer(aRenderer);
}

// No renderer: the frame is blitted, scaled, to the window's own surface
void RunSurface(SDL_Window* theWindow, SDL_Surface* theFrame, const char* theWhere, const char* theFit, const SDL_Rect& theRect)
{
	SDL_Surface* aWindowSurface = SDL_GetWindowSurface(theWindow);
	if (aWindowSurface == NULL)
	{
		fprintf(stderr, "no window surface: %s\n", SDL_GetError());
		return;
	}
	SDL_FillRect(aWindowSurface, NULL, 0);
	double aScaleMs = 0, aPresentMs = 0;
	unsigned long long aCycles = Cycles();
	for (int i = 0; i < gFrames; i++)
	{
		SDL_Rect aRect = theRect;
		Uint64 aStart = SDL_GetPerformanceCounter();
		SDL_BlitScaled(theFrame, NULL, aWindowSurface, &aRect);
		Uint64 aMiddle = SDL_GetPerformanceCounter();
		SDL_UpdateWindowSurface(theWindow);
		Uint64 anEnd = SDL_GetPerformanceCounter();
		aScaleMs += Ms(aStart, aMiddle);
		aPresentMs += Ms(aMiddle, anEnd);
	}
	aCycles = Cycles() - aCycles;
	Report(theWhere, "sdl-surface", theFit, theRect, aScaleMs, aPresentMs, aCycles);
	SDL_DestroyWindowSurface(theWindow);
}

#ifdef _WIN32
// GDI: one StretchDIBits call, into a bitmap in memory (the scaling alone, and the picture) and to the window
void RunGdi(SDL_Window* theWindow, SDL_Surface* theFrame, const char* theWhere, bool isHalftone, const char* theFit, const SDL_Rect& theRect)
{
	SDL_SysWMinfo anInfo;
	SDL_VERSION(&anInfo.version);
	if (!SDL_GetWindowWMInfo(theWindow, &anInfo))
		return;
	HWND aHwnd = anInfo.info.win.window;

	BITMAPINFO aSource;
	memset(&aSource, 0, sizeof(aSource));
	aSource.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	aSource.bmiHeader.biWidth = theFrame->w;
	aSource.bmiHeader.biHeight = -theFrame->h;      // the first row is the top one
	aSource.bmiHeader.biPlanes = 1;
	aSource.bmiHeader.biBitCount = 32;
	aSource.bmiHeader.biCompression = BI_RGB;

	BITMAPINFO aTarget = aSource;
	aTarget.bmiHeader.biWidth = theRect.w;
	aTarget.bmiHeader.biHeight = -theRect.h;
	void* aBits = NULL;
	HDC aScreenDC = GetDC(NULL);
	HDC aMemoryDC = CreateCompatibleDC(aScreenDC);
	HBITMAP aBitmap = CreateDIBSection(aScreenDC, &aTarget, DIB_RGB_COLORS, &aBits, NULL, 0);
	ReleaseDC(NULL, aScreenDC);
	HGDIOBJ anOld = SelectObject(aMemoryDC, aBitmap);
	int aMode = isHalftone ? HALFTONE : COLORONCOLOR;
	const char* aWay = isHalftone ? "gdi-halftone" : "gdi-stretch";

	SetStretchBltMode(aMemoryDC, aMode);
	SetBrushOrgEx(aMemoryDC, 0, 0, NULL);
	double aMemoryMs = 0;
	unsigned long long aCycles = Cycles();
	for (int i = 0; i < gFrames; i++)
	{
		Uint64 aStart = SDL_GetPerformanceCounter();
		StretchDIBits(aMemoryDC, 0, 0, theRect.w, theRect.h, 0, 0, theFrame->w, theFrame->h, theFrame->pixels, &aSource, DIB_RGB_COLORS, SRCCOPY);
		GdiFlush();
		aMemoryMs += Ms(aStart, SDL_GetPerformanceCounter());
	}
	aCycles = Cycles() - aCycles;
	Report(theWhere, (std::string(aWay) + "-memory").c_str(), theFit, theRect, aMemoryMs, 0, aCycles);
	SDL_Surface* aShot = SDL_CreateRGBSurfaceWithFormatFrom(aBits, theRect.w, theRect.h, 32, theRect.w * 4, SDL_PIXELFORMAT_XRGB8888);
	Save(aShot, theWhere, aWay, theFit);
	SDL_FreeSurface(aShot);

	HDC aWindowDC = GetDC(aHwnd);
	SetStretchBltMode(aWindowDC, aMode);
	SetBrushOrgEx(aWindowDC, 0, 0, NULL);
	double aWindowMs = 0;
	aCycles = Cycles();
	for (int i = 0; i < gFrames; i++)
	{
		Uint64 aStart = SDL_GetPerformanceCounter();
		StretchDIBits(aWindowDC, theRect.x, theRect.y, theRect.w, theRect.h, 0, 0, theFrame->w, theFrame->h, theFrame->pixels, &aSource, DIB_RGB_COLORS,
			SRCCOPY);
		GdiFlush();
		aWindowMs += Ms(aStart, SDL_GetPerformanceCounter());
	}
	aCycles = Cycles() - aCycles;
	Report(theWhere, (std::string(aWay) + "-window").c_str(), theFit, theRect, 0, aWindowMs, aCycles);
	ReleaseDC(aHwnd, aWindowDC);
	SelectObject(aMemoryDC, anOld);
	DeleteObject(aBitmap);
	DeleteDC(aMemoryDC);
}
#endif

void RunAll(SDL_Window* theWindow, SDL_Surface* theFrame, const char* theWhere)
{
	int aWidth = 0, aHeight = 0;
	SDL_GetWindowSizeInPixels(theWindow, &aWidth, &aHeight);
	// "exact": the largest picture of the frame's shape that fits; "whole": the largest whole-number factor, with borders
	SDL_Rect anExact;
	if (aWidth * theFrame->h > aHeight * theFrame->w)
	{
		anExact.h = aHeight;
		anExact.w = aHeight * theFrame->w / theFrame->h;
	}
	else
	{
		anExact.w = aWidth;
		anExact.h = aWidth * theFrame->h / theFrame->w;
	}
	anExact.x = (aWidth - anExact.w) / 2;
	anExact.y = (aHeight - anExact.h) / 2;
	int aFactor = std::min(aWidth / theFrame->w, aHeight / theFrame->h);
	SDL_Rect aWhole = {(aWidth - theFrame->w * aFactor) / 2, (aHeight - theFrame->h * aFactor) / 2, theFrame->w * aFactor, theFrame->h * aFactor};
	bool isSame = (aWhole.w == anExact.w) && (aWhole.h == anExact.h);
	printf("{\"where\":\"%s\",\"window\":[%d,%d],\"exact\":[%d,%d],\"whole\":[%d,%d],\"factor\":%d}\n", theWhere, aWidth, aHeight, anExact.w, anExact.h,
		aWhole.w, aWhole.h, aFactor);

	for (int aFit = 0; aFit < (isSame ? 1 : 2); aFit++)
	{
		const SDL_Rect& aRect = (aFit == 0) ? anExact : aWhole;
		const char* aFitName = (aFit == 0) ? "exact" : "whole";
		RunRenderer(theWindow, theFrame, theWhere, false, aFitName, aRect);
		RunRenderer(theWindow, theFrame, theWhere, true, aFitName, aRect);
		RunSurface(theWindow, theFrame, theWhere, aFitName, aRect);
#ifdef _WIN32
		RunGdi(theWindow, theFrame, theWhere, false, aFitName, aRect);
		RunGdi(theWindow, theFrame, theWhere, true, aFitName, aRect);
#endif
	}
}

} // namespace

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "usage: scaletest <frame.bmp> <out dir> [frames]\n");
		return 2;
	}
	gOutDir = argv[2];
	if (argc > 3)
		gFrames = std::max(1, atoi(argv[3]));

	// as the framework's window: real pixels on a scaled display, and no 3D device behind the window's surface
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	SDL_Surface* aLoaded = SDL_LoadBMP(argv[1]);
	if (aLoaded == NULL)
	{
		fprintf(stderr, "cannot read %s: %s\n", argv[1], SDL_GetError());
		return 1;
	}
	SDL_Surface* aFrame = SDL_ConvertSurfaceFormat(aLoaded, SDL_PIXELFORMAT_XRGB8888, 0);
	SDL_FreeSurface(aLoaded);

	SDL_Window* aWindow = SDL_CreateWindow("scaletest", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, aFrame->w * 2, aFrame->h * 2, 0);
	if (aWindow == NULL)
	{
		fprintf(stderr, "no window: %s\n", SDL_GetError());
		return 1;
	}
	RunAll(aWindow, aFrame, "window2x");
	SDL_DestroyWindow(aWindow);

	aWindow = SDL_CreateWindow("scaletest", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, aFrame->w, aFrame->h, SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (aWindow == NULL)
	{
		fprintf(stderr, "no full-screen window: %s\n", SDL_GetError());
		return 1;
	}
	RunAll(aWindow, aFrame, "fullscreen");
	SDL_DestroyWindow(aWindow);
	SDL_FreeSurface(aFrame);
	SDL_Quit();
	return 0;
}
