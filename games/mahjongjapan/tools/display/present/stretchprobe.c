// stretchprobe: does GDI's nearest-neighbour stretch (StretchBlt/StretchDIBits, COLORONCOLOR) of a rect of the 800x600 screen put the same pixels
// where the stretch of the whole screen does, and what does each way cost? The destination is a DIB section (no window), 2880x2160 (3.6x,
// the 3840x2160 desktop letterboxed) or 1600x1200 or 1280x960 (1.6x). For random rects it compares
//   A: the rect stretched alone into the dest rect that its edges make (the edges rounded outwards)
//   B: the whole screen stretched with the clip region set to that dest rect
// against C: the whole screen stretched at once, and prints the pixels that differ, and the cycles of the thread each call takes.
#define SDL_MAIN_HANDLED
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static uint64_t thread_cycles(void)
{
	ULONG64 c = 0;
	QueryThreadCycleTime(GetCurrentThread(), &c);
	return c;
}

int main(int argc, char** argv)
{
	int dw = (argc > 1) ? atoi(argv[1]) : 2880, dh = (argc > 2) ? atoi(argv[2]) : 2160;
	const int SW = 800, SH = 600;
	HDC screen = GetDC(NULL);
	BITMAPINFO bi;
	memset(&bi, 0, sizeof bi);
	bi.bmiHeader.biSize = sizeof bi.bmiHeader;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	uint32_t *srcbits, *whole, *test;
	bi.bmiHeader.biWidth = SW; bi.bmiHeader.biHeight = -SH;
	HDC srcdc = CreateCompatibleDC(screen);
	HBITMAP srcdib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, (void**)&srcbits, NULL, 0);
	SelectObject(srcdc, srcdib);
	bi.bmiHeader.biWidth = dw; bi.bmiHeader.biHeight = -dh;
	HDC wdc = CreateCompatibleDC(screen), tdc = CreateCompatibleDC(screen);
	HBITMAP wdib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, (void**)&whole, NULL, 0);
	HBITMAP tdib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, (void**)&test, NULL, 0);
	SelectObject(wdc, wdib);
	SelectObject(tdc, tdib);
	SetStretchBltMode(wdc, COLORONCOLOR);
	SetStretchBltMode(tdc, COLORONCOLOR);
	for (int i = 0; i < SW * SH; i++)
		srcbits[i] = (i * 2654435761u >> 8) & 0xffffff;
	StretchBlt(wdc, 0, 0, dw, dh, srcdc, 0, 0, SW, SH, SRCCOPY);
	GdiFlush();

	// the same bits again through StretchDIBits from a plain buffer
	uint32_t* plain = (uint32_t*)malloc(SW * SH * 4);
	memcpy(plain, srcbits, SW * SH * 4);

	srand(1);
	long diffA = 0, diffB = 0, diffD = 0, diffE = 0, totalA = 0, calls = 0;
	uint64_t cyclesA = 0, cyclesB = 0, cyclesD = 0, cyclesE = 0, cyclesFull = 0;
	for (int n = 0; n < 200; n++)
	{
		int rw = 1 + rand() % 400, rh = 1 + rand() % 300;
		int rx = rand() % (SW - rw + 1), ry = rand() % (SH - rh + 1);
		double fx = (double)dw / SW, fy = (double)dh / SH;
		int x0 = (int)(rx * fx), y0 = (int)(ry * fy);
		int x1 = (int)((rx + rw) * fx + 0.999), y1 = (int)((ry + rh) * fy + 0.999);
		if (x1 > dw) x1 = dw;
		if (y1 > dh) y1 = dh;
		// A: the rect alone
		memcpy(test, whole, (size_t)dw * dh * 4);
		uint64_t c0 = thread_cycles();
		StretchBlt(tdc, x0, y0, x1 - x0, y1 - y0, srcdc, rx, ry, rw, rh, SRCCOPY);
		GdiFlush();
		cyclesA += thread_cycles() - c0;
		for (int y = y0; y < y1; y++)
			for (int x = x0; x < x1; x++)
				if (test[(size_t)y * dw + x] != whole[(size_t)y * dw + x])
					diffA++;
		totalA += (long)(x1 - x0) * (y1 - y0);
		// B: the whole screen, clipped to the dest rect
		memset(test, 0, (size_t)dw * dh * 4);
		HRGN rgn = CreateRectRgn(x0, y0, x1, y1);
		SelectClipRgn(tdc, rgn);
		c0 = thread_cycles();
		StretchBlt(tdc, 0, 0, dw, dh, srcdc, 0, 0, SW, SH, SRCCOPY);
		GdiFlush();
		cyclesB += thread_cycles() - c0;
		SelectClipRgn(tdc, NULL);
		DeleteObject(rgn);
		for (int y = y0; y < y1; y++)
			for (int x = x0; x < x1; x++)
				if (test[(size_t)y * dw + x] != whole[(size_t)y * dw + x])
					diffB++;
		// D: StretchDIBits of the rect from the plain buffer (top-down DIB, the rect's offset in it)
		memcpy(test, whole, (size_t)dw * dh * 4);
		BITMAPINFO sb;
		memset(&sb, 0, sizeof sb);
		sb.bmiHeader.biSize = sizeof sb.bmiHeader;
		sb.bmiHeader.biWidth = SW; sb.bmiHeader.biHeight = -SH; sb.bmiHeader.biPlanes = 1; sb.bmiHeader.biBitCount = 32; sb.bmiHeader.biCompression = BI_RGB;
		c0 = thread_cycles();
		StretchDIBits(tdc, x0, y0, x1 - x0, y1 - y0, rx, ry, rw, rh, plain, &sb, DIB_RGB_COLORS, SRCCOPY);
		GdiFlush();
		cyclesD += thread_cycles() - c0;
		for (int y = y0; y < y1; y++)
			for (int x = x0; x < x1; x++)
				if ((test[(size_t)y * dw + x] & 0xffffff) != (whole[(size_t)y * dw + x] & 0xffffff))
					diffD++;
		// E: StretchDIBits of the whole buffer, clipped to the dest rect
		memcpy(test, whole, (size_t)dw * dh * 4);
		{
			HRGN rgn2 = CreateRectRgn(x0, y0, x1, y1);
			SelectClipRgn(tdc, rgn2);
			c0 = thread_cycles();
			StretchDIBits(tdc, 0, 0, dw, dh, 0, 0, SW, SH, plain, &sb, DIB_RGB_COLORS, SRCCOPY);
			GdiFlush();
			cyclesE += thread_cycles() - c0;
			SelectClipRgn(tdc, NULL);
			DeleteObject(rgn2);
		}
		for (int y = y0; y < y1; y++)
			for (int x = x0; x < x1; x++)
				if ((test[(size_t)y * dw + x] & 0xffffff) != (whole[(size_t)y * dw + x] & 0xffffff))
					diffE++;
		calls++;
	}
	uint64_t c0 = thread_cycles();
	for (int i = 0; i < 20; i++)
	{
		StretchBlt(tdc, 0, 0, dw, dh, srcdc, 0, 0, SW, SH, SRCCOPY);
		GdiFlush();
	}
	cyclesFull = (thread_cycles() - c0) / 20;
	printf("dest %dx%d, %ld rects, %ld dest pixels in all\n", dw, dh, calls, totalA);
	printf("A rect alone:           %ld pixels differ from the whole stretch, %llu cycles a call\n", diffA, (unsigned long long)(cyclesA / calls));
	printf("B whole, clipped:       %ld pixels differ, %llu cycles a call\n", diffB, (unsigned long long)(cyclesB / calls));
	printf("D StretchDIBits (rect): %ld pixels differ, %llu cycles a call\n", diffD, (unsigned long long)(cyclesD / calls));
	printf("E StretchDIBits (whole, clipped): %ld pixels differ, %llu cycles a call\n", diffE, (unsigned long long)(cyclesE / calls));
	printf("whole screen, no clip:  %llu cycles a call\n", (unsigned long long)cyclesFull);
	return 0;
}
