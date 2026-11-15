// modeprobe <variant>: does a window show its SDL surface after it has had a renderer? Night runs on the visible desktop only.
// Each stage fills the screen with a colour and prints the colour the desktop shows at the middle of the window's client area.
//  1 surface only (red)
//  2 renderer (green) -> destroy -> surface (red)
//  3 renderer (green) -> full screen -> window -> destroy -> surface (red)
//  4 as 3, but the surface is made while still full screen then leaves it
//  5 as 2, with SDL_HINT_RENDER_DRIVER=direct3d (D3D9)
//  6 as 2, with the window hidden and shown after the renderer is destroyed
//  7 as 2, with SetWindowPos SWP_FRAMECHANGED after the renderer is destroyed
//  8 as 2, with InvalidateRect + UpdateWindow
//  9 as 2, with the window's GDI DC released and got again before the surface
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static HWND hwnd;
static SDL_Window* win;

static unsigned shown(void)
{
	POINT pt = {400, 300}; ClientToScreen(hwnd, &pt);
	HDC dc = GetDC(NULL);
	COLORREF c = GetPixel(dc, pt.x, pt.y);
	ReleaseDC(NULL, dc);
	return ((unsigned)GetRValue(c) << 16) | ((unsigned)GetGValue(c) << 8) | GetBValue(c);
}
static void pump(int ms) { Uint32 t = SDL_GetTicks(); while (SDL_GetTicks() - t < (Uint32)ms) { SDL_Event e; while (SDL_PollEvent(&e)) {} SDL_Delay(5); } }
static void fill_surface(Uint32 rgb)
{
	SDL_Surface* s = SDL_GetWindowSurface(win);
	SDL_FillRect(s, NULL, rgb);
	SDL_UpdateWindowSurface(win);
	pump(300);
}
static void renderer_stage(void)
{
	SDL_Renderer* r = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_RendererInfo ri; SDL_GetRendererInfo(r, &ri);
	printf("  renderer %s\n", ri.name);
	for (int i = 0; i < 5; i++) { SDL_SetRenderDrawColor(r, 0, 255, 0, 255); SDL_RenderClear(r); SDL_RenderPresent(r); pump(30); }
	printf("  renderer shows %06x (green is 00ff00)\n", shown());
	if (getenv("MP_FULL"))
	{
		SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP); pump(600);
		for (int i = 0; i < 5; i++) { SDL_SetRenderDrawColor(r, 0, 255, 0, 255); SDL_RenderClear(r); SDL_RenderPresent(r); pump(30); }
		printf("  full screen shows %06x\n", shown());
		SDL_SetWindowFullscreen(win, 0); pump(600);
	}
	SDL_DestroyRenderer(r);
	pump(200);
}
int main(int argc, char** argv)
{
	int v = atoi(argv[1]);
	if (v == 3 || v == 4) _putenv("MP_FULL=1");
	if (v == 5) SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
	win = SDL_CreateWindow("modeprobe", 100, 60, 800, 600, 0);
	SDL_SysWMinfo info; SDL_VERSION(&info.version); SDL_GetWindowWMInfo(win, &info); hwnd = info.info.win.window;
	SDL_ShowWindow(win); SDL_RaiseWindow(win); SDL_SetWindowAlwaysOnTop(win, SDL_TRUE); SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); pump(500);
	if (v == 1) { fill_surface(0xff0000); printf("surface only: shows %06x (red is ff0000)\n", shown()); return 0; }
	renderer_stage();
	if (v == 6) { SDL_HideWindow(win); pump(100); SDL_ShowWindow(win); pump(300); }
	if (v == 7) { SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED); pump(200); }
	if (v == 10) { SDL_SetWindowSize(win, 801, 601); pump(200); SDL_SetWindowSize(win, 800, 600); pump(300); }
	if (v == 11) { SDL_MinimizeWindow(win); pump(400); SDL_RestoreWindow(win); pump(400); SDL_SetWindowAlwaysOnTop(win, SDL_TRUE); }
	if (v == 12) { SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP); pump(600); SDL_SetWindowFullscreen(win, 0); pump(600); }
	if (v == 13) { SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED); SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA); SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED); pump(300); }
	if (v == 8) { InvalidateRect(hwnd, NULL, TRUE); UpdateWindow(hwnd); pump(200); }
	fill_surface(0xff0000);
	printf("variant %d: after the renderer, the surface shows %06x (red is ff0000)\n", v, shown());
	if (v == 2 || v == 5) { SDL_DestroyWindowSurface(win); fill_surface(0xff0000); printf("  surface made again: %06x\n", shown()); }
	return 0;
}
