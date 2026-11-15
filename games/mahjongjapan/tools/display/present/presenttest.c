// presenttest <mode> <seconds>: 800x600 software frames presented at 40 per second, to count what each way of putting them on
// the screen costs. Modes: 0 = renderer, one static texture, UpdateTexture + RenderCopy + Present;
// 1 = the framework's path (Clear, copy to a target texture, Clear, copy to the back buffer, Present);
// 2 = the window's surface (memcpy, SDL_UpdateWindowSurface); 3 = renderer, streaming texture, Lock + memcpy;
// 4 = like 0, with SDL_HINT_RENDER_VSYNC off; 5 = software renderer (SDL_RENDERER_SOFTWARE), as 0.
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char** argv)
{
	int mode = atoi(argv[1]), seconds = atoi(argv[2]);
	SDL_Init(SDL_INIT_VIDEO);
	
	int big = mode >= 10;
	SDL_Window* w = SDL_CreateWindow("t", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, big ? 1600 : 800, big ? 1200 : 600, 0);
	SDL_Renderer* r = NULL;
	SDL_Texture *t = NULL, *screen = NULL;
	if (mode != 2 && (mode < 8 || mode == 10 || mode == 11))
	{
		r = SDL_CreateRenderer(w, -1, mode == 5 ? (SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE) : (SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | (mode == 4 ? 0 : SDL_RENDERER_PRESENTVSYNC)));
		SDL_RendererInfo ri; SDL_GetRendererInfo(r, &ri);
		printf("renderer %s\n", ri.name);
		t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, mode == 3 ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC, 800, 600);
		screen = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, big ? 1600 : 800, big ? 1200 : 600);
	}
	uint32_t* px = (uint32_t*)malloc(800 * 600 * 4);
	for (int i = 0; i < 800 * 600; i++) px[i] = 0xff000000 | (i * 2654435761u >> 8);
	// every pixel of the texture and the surface holds a defined colour before anything is shown (a texture is not cleared)
	if (t) SDL_UpdateTexture(t, NULL, px, 800 * 4);
	if (mode == 2 || mode == 8 || mode == 9 || mode >= 12)
	{
		SDL_Surface* s0 = SDL_GetWindowSurface(w);
		SDL_LockSurface(s0); for (int y = 0; y < s0->h; y++) memcpy((char*)s0->pixels + y * s0->pitch, px + (y % 600) * 800, (s0->w < 800 ? s0->w : 800) * 4); SDL_UnlockSurface(s0);
	}
	uint32_t start = SDL_GetTicks(), next = start;
	int frames = 0;
	while (SDL_GetTicks() - start < (uint32_t)seconds * 1000)
	{
		SDL_Event e; while (SDL_PollEvent(&e)) {}
		px[frames % 480000] ^= 0xffffff;
		if (mode == 10 || mode == 11)
		{
			// the game's chain at a 1600x1200 window: 10 = the whole frame uploaded, the target cleared (as now);
			// 11 = a 15% rect uploaded, no clear of the target (the frame texture is copied over all of it)
			SDL_Rect rc = { (frames * 7) % (800 - 310), (frames * 5) % (600 - 232), 310, 232 };
			SDL_SetRenderTarget(r, screen);
			if (mode == 10) { SDL_SetRenderDrawColor(r, 0, 0, 0, 255); SDL_RenderClear(r); SDL_UpdateTexture(t, NULL, px, 800 * 4); }
			else SDL_UpdateTexture(t, &rc, px + rc.y * 800 + rc.x, 800 * 4);
			SDL_SetTextureScaleMode(t, SDL_ScaleModeNearest);
			SDL_Rect dst = { 0, 0, 1600, 1200 };
			SDL_RenderCopy(r, t, NULL, &dst);
			SDL_SetRenderTarget(r, NULL);
			SDL_RenderClear(r);
			SDL_RenderCopy(r, screen, NULL, NULL);
			SDL_RenderPresent(r);
		}
		else if (mode == 12 || mode == 13)
		{
			// the window surface at 1600x1200: a rect of the 800x600 frame, 2x nearest, 12 = 15%, 13 = 3%
			int rw = mode == 12 ? 310 : 138, rh = mode == 12 ? 232 : 104;
			SDL_Rect rc = { (frames * 7) % (800 - rw), (frames * 5) % (600 - rh), rw, rh };
			SDL_Surface* s = SDL_GetWindowSurface(w);
			SDL_LockSurface(s);
			for (int y = 0; y < rh; y++)
			{
				uint32_t* d0 = (uint32_t*)((char*)s->pixels + (2 * (rc.y + y)) * s->pitch) + 2 * rc.x;
				const uint32_t* sp = px + (rc.y + y) * 800 + rc.x;
				for (int x = 0; x < rw; x++) { d0[2 * x] = sp[x]; d0[2 * x + 1] = sp[x]; }
				memcpy((char*)d0 + s->pitch, d0, (size_t)rw * 8);
			}
			SDL_UnlockSurface(s);
			SDL_Rect dr = { 2 * rc.x, 2 * rc.y, 2 * rw, 2 * rh };
			SDL_UpdateWindowSurfaceRects(w, &dr, 1);
		}
		else if (mode == 8 || mode == 9)
		{
			// the window's surface, only a rectangle of it written and updated: 8 = 15% of the area, 9 = 3%
			int rw = mode == 8 ? 310 : 138, rh = mode == 8 ? 232 : 104;
			SDL_Rect rc = { (frames * 7) % (800 - rw), (frames * 5) % (600 - rh), rw, rh };
			SDL_Surface* s = SDL_GetWindowSurface(w);
			SDL_LockSurface(s);
			for (int y = 0; y < rh; y++) memcpy((char*)s->pixels + (rc.y + y) * s->pitch + rc.x * 4, px + (rc.y + y) * 800 + rc.x, rw * 4);
			SDL_UnlockSurface(s);
			SDL_UpdateWindowSurfaceRects(w, &rc, 1);
		}
		else if (mode == 2)
		{
			SDL_Surface* s = SDL_GetWindowSurface(w);
			SDL_LockSurface(s); memcpy(s->pixels, px, 800 * 600 * 4); SDL_UnlockSurface(s);
			SDL_UpdateWindowSurface(w);
		}
		else if (mode == 3)
		{
			void* p; int pitch; SDL_LockTexture(t, NULL, &p, &pitch); memcpy(p, px, 800 * 600 * 4); SDL_UnlockTexture(t);
			SDL_RenderCopy(r, t, NULL, NULL); SDL_RenderPresent(r);
		}
		else if (mode == 1)
		{
			SDL_SetRenderTarget(r, screen);
			SDL_SetRenderDrawColor(r, 0, 0, 0, 255); SDL_RenderClear(r);
			SDL_UpdateTexture(t, NULL, px, 800 * 4);
			SDL_RenderCopy(r, t, NULL, NULL);
			SDL_SetRenderTarget(r, NULL);
			SDL_RenderClear(r);
			SDL_RenderCopy(r, screen, NULL, NULL);
			SDL_RenderPresent(r);
		}
		else if (mode >= 6 && mode <= 7)
		{
			// the framework's path again, but only a rectangle of the screen image goes up each frame: 6 = 15% of the area, 7 = 3%
			int rw = mode == 6 ? 310 : 138, rh = mode == 6 ? 232 : 104;
			SDL_Rect rc = { (frames * 7) % (800 - rw), (frames * 5) % (600 - rh), rw, rh };
			SDL_SetRenderTarget(r, screen);
			SDL_SetRenderDrawColor(r, 0, 0, 0, 255); SDL_RenderClear(r);
			SDL_UpdateTexture(t, &rc, px + rc.y * 800 + rc.x, 800 * 4);
			SDL_RenderCopy(r, t, NULL, NULL);
			SDL_SetRenderTarget(r, NULL);
			SDL_RenderClear(r);
			SDL_RenderCopy(r, screen, NULL, NULL);
			SDL_RenderPresent(r);
		}
		else
		{
			SDL_UpdateTexture(t, NULL, px, 800 * 4);
			SDL_RenderCopy(r, t, NULL, NULL); SDL_RenderPresent(r);
		}
		frames++;
		next += 25;
		int d = (int)(next - SDL_GetTicks());
		if (d > 0) SDL_Delay(d); else next = SDL_GetTicks();
	}
	printf("frames %d\n", frames);
	return 0;
}
