// scaletest <mode> <seconds>: 800x600 software frames, a rect of 15% of the screen (310x232) changed 40 times a second, shown in a
// window by each way that needs no renderer, to count what each costs (threadprof.py reads the cycles of every thread).
// Modes: 0 = SDL window surface, window 800x600 (rect copy + SDL_UpdateWindowSurfaceRects)
//        1 = SDL window surface, window 1600x1200, software 2x nearest of the rect
//        2 = GDI StretchBlt COLORONCOLOR of the rect from a DIB, window 1600x1200
//        3 = GDI StretchBlt HALFTONE, window 1600x1200
//        4/5 = as 2/3, window 1280x960 (1.6x)
//        6/7 = as 2/3, borderless window 3840x2160, the image letterboxed to 2880x2160 (3.6x)
//        8 = GDI BitBlt of the rect, window 800x600 (SDL's own surface blit is this call)
//        9 = SDL window surface, window 3200x2400 (4x) software nearest
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void copy_scaled(const uint32_t* src, int src_pitch_px, uint32_t* dst, int dst_pitch_px, int w, int h, int scale)
{
	for (int y = 0; y < h; y++)
	{
		const uint32_t* s = src + y * src_pitch_px;
		uint32_t* row = dst + y * scale * dst_pitch_px;
		if (scale == 1)
			memcpy(row, s, (size_t)w * 4);
		else if (scale == 2)
			for (int x = 0; x < w; x++) { row[2 * x] = s[x]; row[2 * x + 1] = s[x]; }
		else
			for (int x = 0; x < w; x++) for (int i = 0; i < scale; i++) row[x * scale + i] = s[x];
		for (int j = 1; j < scale; j++)
			memcpy(row + j * dst_pitch_px, row, (size_t)w * scale * 4);
	}
}

int main(int argc, char** argv)
{
	int mode = atoi(argv[1]), seconds = atoi(argv[2]);
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
	SDL_Init(SDL_INIT_VIDEO);
	int ww = 800, wh = 600, flags = 0;
	if (mode == 1 || mode == 2 || mode == 3) { ww = 1600; wh = 1200; }
	if (mode == 4 || mode == 5) { ww = 1280; wh = 960; }
	if (mode == 6 || mode == 7) { ww = 3840; wh = 2160; flags = SDL_WINDOW_BORDERLESS; }
	if (mode == 9) { ww = 3200; wh = 2400; }
	SDL_Window* win = SDL_CreateWindow("scaletest", (mode == 6 || mode == 7) ? 0 : 100, (mode == 6 || mode == 7) ? 0 : 20, ww, wh, flags);
	int pw = 0, ph = 0;
	SDL_GetWindowSizeInPixels(win, &pw, &ph);
	printf("mode %d: window %dx%d pixels\n", mode, pw, ph);
	fflush(stdout);

	uint32_t* px = (uint32_t*)malloc(800 * 600 * 4);
	for (int i = 0; i < 800 * 600; i++) px[i] = 0xff000000 | (i * 2654435761u >> 8);

	int gdi = (mode >= 2 && mode <= 8);
	HWND hwnd = NULL; HDC hdc = NULL, mem = NULL; HBITMAP dib = NULL; uint32_t* dibbits = NULL;
	SDL_Surface* surf = NULL;
	if (gdi)
	{
		SDL_SysWMinfo info; SDL_VERSION(&info.version); SDL_GetWindowWMInfo(win, &info);
		hwnd = info.info.win.window;
		hdc = GetDC(hwnd);
		BITMAPINFO bi; memset(&bi, 0, sizeof bi);
		bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = 800; bi.bmiHeader.biHeight = -600;
		bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
		mem = CreateCompatibleDC(hdc);
		dib = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (void**)&dibbits, NULL, 0);
		SelectObject(mem, dib);
		memcpy(dibbits, px, 800 * 600 * 4);
		SetStretchBltMode(hdc, (mode == 3 || mode == 5 || mode == 7) ? HALFTONE : COLORONCOLOR);
		if (mode == 3 || mode == 5 || mode == 7) SetBrushOrgEx(hdc, 0, 0, NULL);
	}
	else
	{
		surf = SDL_GetWindowSurface(win);
		printf("surface %dx%d\n", surf->w, surf->h); fflush(stdout);
	}

	// the whole window made once, so that every pixel holds a defined colour
	double scale = (double)pw / 800.0;
	int dw = 800, dh = 600, dx = 0, dy = 0;
	if (mode == 4 || mode == 5) { dw = 1280; dh = 960; }
	if (mode == 6 || mode == 7) { dw = 2880; dh = 2160; dx = (3840 - 2880) / 2; dy = 0; }
	if (mode == 2 || mode == 3) { dw = 1600; dh = 1200; }
	if (gdi)
		StretchBlt(hdc, dx, dy, dw, dh, mem, 0, 0, 800, 600, SRCCOPY);
	else
	{
		int sc = pw / 800;
		SDL_LockSurface(surf);
		copy_scaled(px, 800, (uint32_t*)surf->pixels, surf->pitch / 4, 800, 600, sc);
		SDL_UnlockSurface(surf);
		SDL_UpdateWindowSurface(win);
	}
	(void)scale;

	uint32_t start = SDL_GetTicks(), next = start;
	int frames = 0;
	while (SDL_GetTicks() - start < (uint32_t)seconds * 1000)
	{
		SDL_Event e; while (SDL_PollEvent(&e)) {}
		px[frames % 480000] ^= 0xffffff;
		SDL_Rect rc = { (frames * 7) % (800 - 310), (frames * 5) % (600 - 232), 310, 232 };
		if (gdi)
		{
			for (int y = rc.y; y < rc.y + rc.h; y++)
				memcpy(dibbits + y * 800 + rc.x, px + y * 800 + rc.x, (size_t)rc.w * 4);
			if (mode == 8)
				BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, mem, rc.x, rc.y, SRCCOPY);
			else
			{
				// the rect's scaled place in the window: the edges rounded outwards
				double fx = (double)dw / 800.0, fy = (double)dh / 600.0;
				int x0 = (int)(rc.x * fx), y0 = (int)(rc.y * fy);
				int x1 = (int)((rc.x + rc.w) * fx + 0.999), y1 = (int)((rc.y + rc.h) * fy + 0.999);
				StretchBlt(hdc, dx + x0, dy + y0, x1 - x0, y1 - y0, mem, rc.x, rc.y, rc.w, rc.h, SRCCOPY);
			}
		}
		else
		{
			int sc = pw / 800;
			SDL_LockSurface(surf);
			copy_scaled(px + rc.y * 800 + rc.x, 800, (uint32_t*)surf->pixels + rc.y * sc * (surf->pitch / 4) + rc.x * sc, surf->pitch / 4, rc.w, rc.h, sc);
			SDL_UnlockSurface(surf);
			SDL_Rect wr = { rc.x * sc, rc.y * sc, rc.w * sc, rc.h * sc };
			SDL_UpdateWindowSurfaceRects(win, &wr, 1);
		}
		frames++;
		next += 25;
		while (SDL_GetTicks() < next) SDL_Delay(1);
	}
	printf("frames %d\n", frames);
	return 0;
}
