"""statspatch.py apply|revert (from the worktree root): temporary statistics in the framework, for GAME_NOTES.md "Performance" (never commit what it
writes; revert, then rebuild, before any check): build/native/bejeweled2.exe built with it counts the draw calls by kind (DRAWSTATS=1 in the
environment) and the cycles of the commands file's poll (perf.py reads both lines from game.log).
 - framework/DDInterface.cpp: every SDL draw call of the 2D path counted and timed by kind, printed at exit when DRAWSTATS is set;
 - framework/SexyAppBase.cpp: the cycles spent in PollLiveCommands (the -commands file's stat() each update), printed at exit."""
import subprocess
import sys

DD = 'framework/DDInterface.cpp'
SA = 'framework/SexyAppBase.cpp'
if sys.argv[1] == 'revert':
    subprocess.run(['git', 'checkout', '--', DD, SA], check=True)
    sys.exit(0)


def load(p):
    s = open(p, encoding='utf-8', newline='').read()
    return s.replace('\r\n', '\n'), '\r\n' in s


def save(p, s, crlf):
    open(p, 'w', encoding='utf-8', newline='').write(s.replace('\n', '\r\n') if crlf else s)


s, crlf = load(DD)


def rep(a, b):
    global s
    assert s.count(a) == 1, (a[:60], s.count(a))
    s = s.replace(a, b)


head = r'''
// DRAWSTATS (temporary)
#include <stdio.h>
#include <stdlib.h>
struct DrawKind { const char* mName; long long mCalls; unsigned long long mTicks; double mPixels; };
static DrawKind gDrawKinds[] = { {"copy opaque", 0, 0, 0}, {"copy alpha", 0, 0, 0}, {"copy additive", 0, 0, 0}, {"copy mirrored", 0, 0, 0},
	{"geometry quad", 0, 0, 0}, {"geometry quad additive", 0, 0, 0}, {"geometry piece", 0, 0, 0}, {"triangles", 0, 0, 0},
	{"fill rects", 0, 0, 0}, {"polygon", 0, 0, 0}, {"fill rect clear", 0, 0, 0}, {"present", 0, 0, 0}, {"screen copy", 0, 0, 0}, {"clear", 0, 0, 0} };
static bool gDrawStatsOn = getenv("DRAWSTATS") != NULL;
struct DrawStatsPrinter
{
	~DrawStatsPrinter()
	{
		if (!gDrawStatsOn)
			return;
		double aFreq = (double) SDL_GetPerformanceFrequency();
		double aTotal = 0;
		fprintf(stderr, "DRAWSTATS kind calls ms avg_us Mpixels\n");
		for (size_t i = 0; i < sizeof(gDrawKinds) / sizeof(gDrawKinds[0]); i++)
		{
			DrawKind& k = gDrawKinds[i];
			double t = 1000.0 * k.mTicks / aFreq;
			aTotal += t;
			fprintf(stderr, "DRAWSTATS %s %lld %.1f %.2f %.1f\n", k.mName, k.mCalls, t, k.mCalls ? 1000.0 * t / k.mCalls : 0.0, k.mPixels / 1e6);
		}
		fprintf(stderr, "DRAWSTATS total %.1f ms\n", aTotal);
		fflush(stderr);
	}
} gDrawStatsPrinter;
struct DrawTimer
{
	DrawKind& mKind;
	unsigned long long mStart;
	DrawTimer(int k, double px) : mKind(gDrawKinds[k]), mStart(0)
	{
		if (!gDrawStatsOn)
			return;
		mKind.mCalls++;
		mKind.mPixels += px;
		mStart = SDL_GetPerformanceCounter();
	}
	~DrawTimer()
	{
		if (gDrawStatsOn)
			mKind.mTicks += SDL_GetPerformanceCounter() - mStart;
	}
};
'''
i = s.index('using namespace Sexy;')
s = s[:i] + head.lstrip('\n') + '\n' + s[i:]

rep('''		SDL_RenderCopyExF(mRenderer, aTexture, &aSrcRect, &aDestRect, 0, NULL, mirror ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
		return;''', '''		{
			DrawTimer aT(mirror ? 3 : (theDrawMode == Graphics::DRAWMODE_ADDITIVE ? 2 : (aColor.a < 255 ? 1 : 0)), aDestRect.w * aDestRect.h);
			SDL_RenderCopyExF(mRenderer, aTexture, &aSrcRect, &aDestRect, 0, NULL, mirror ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
		}
		return;''')
rep('''				SDL_RenderGeometry(mRenderer, aTexture, aPiece, 4, aPieceIndices, 6);''', '''				DrawTimer aT(6, (aC.mEnd - aC.mStart) * (aR.mEnd - aR.mStart) * aScale * aScale);
				SDL_RenderGeometry(mRenderer, aTexture, aPiece, 4, aPieceIndices, 6);''')
rep('''	SDL_RenderGeometry(mRenderer, aTexture, aVertices, 4, anIndices, 6);
}''', '''	float aMinX = std::min(std::min(theX[0], theX[1]), std::min(theX[2], theX[3])), aMaxX = std::max(std::max(theX[0], theX[1]), std::max(theX[2], theX[3]));
	float aMinY = std::min(std::min(theY[0], theY[1]), std::min(theY[2], theY[3])), aMaxY = std::max(std::max(theY[0], theY[1]), std::max(theY[2], theY[3]));
	DrawTimer aT(theDrawMode == Graphics::DRAWMODE_ADDITIVE ? 5 : 4, (aMaxX - aMinX) * (aMaxY - aMinY) * aScale * aScale);
	SDL_RenderGeometry(mRenderer, aTexture, aVertices, 4, anIndices, 6);
}''')
rep('''	SDL_RenderGeometry(mRenderer, aTexture, &gVertices[0], theNumTriangles * 3, NULL, 0);''', '''	DrawTimer aT(7, 0);
	SDL_RenderGeometry(mRenderer, aTexture, &gVertices[0], theNumTriangles * 3, NULL, 0);''')
rep('''		SDL_RenderFillRectsF(mRenderer, &gRects[0], (int) gRects.size());''', '''		double aPx = 0;
		for (size_t q = 0; q < gRects.size(); q++)
			aPx += gRects[q].w * gRects[q].h;
		DrawTimer aT(8, aPx);
		SDL_RenderFillRectsF(mRenderer, &gRects[0], (int) gRects.size());''')
rep('''	SDL_RenderGeometry(mRenderer, NULL, &gVertices[0], theNumVertices, &gIndices[0], (theNumVertices - 2) * 3);''', '''	DrawTimer aT(9, 0);
	SDL_RenderGeometry(mRenderer, NULL, &gVertices[0], theNumVertices, &gIndices[0], (theNumVertices - 2) * 3);''')
rep('''	SDL_RenderFillRect(mRenderer, &aRect);
}''', '''	DrawTimer aT(10, aRect.w * (double) aRect.h);
	SDL_RenderFillRect(mRenderer, &aRect);
}''')
rep('''		SDL_RenderCopy(mRenderer, mScreenTexture, NULL, &aDestRect);
	}

	DrawCursor();

	SDL_RenderPresent(mRenderer);''', '''		DrawTimer aT(12, aDestRect.w * (double) aDestRect.h);
		SDL_RenderCopy(mRenderer, mScreenTexture, NULL, &aDestRect);
	}

	DrawCursor();

	{
		DrawTimer aT(11, 0);
		SDL_RenderPresent(mRenderer);
	}''')
rep('''	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mRenderer);
}''', '''	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	DrawTimer aT(13, 0);
	SDL_RenderClear(mRenderer);
}''')
save(DD, s, crlf)

# the commands file's poll: its cycles are counted, to be taken off the port's processor time
s, crlf = load(SA)
block = r'''// POLLSTATS (temporary)
#include <x86intrin.h>
static unsigned long long gPollCalls = 0, gPollCycles = 0;
struct PollStatsPrinter
{
	~PollStatsPrinter()
	{
		fprintf(stderr, "POLLSTATS calls %llu cycles %llu\n", gPollCalls, gPollCycles);
		fflush(stderr);
	}
} gPollStatsPrinter;
struct PollTimer
{
	unsigned long long mStart;
	PollTimer() : mStart(__rdtsc()) {}
	~PollTimer()
	{
		gPollCalls++;
		gPollCycles += __rdtsc() - mStart;
	}
};

'''
i = s.index("// -commands: the file's new whole lines")
s = s[:i] + block + s[i:]
rep('void SexyAppBase::PollLiveCommands()\n{\n', 'void SexyAppBase::PollLiveCommands()\n{\n\tPollTimer aPollTimer;\n')
save(SA, s, crlf)
print('applied')
