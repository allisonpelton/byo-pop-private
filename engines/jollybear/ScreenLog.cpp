#include "ScreenLog.h"

#include "Image.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace jb
{

namespace screenlog
{

namespace
{

// the most commands by which two frames' lists may differ for the match to be looked for (Myers's greedy search takes
// time and memory in proportion to it); beyond it every command in between counts as changed
const int kMaxEdits = 96;
// the most rects a frame's changed area is kept as: beyond it the area is one rect
const size_t kMaxRects = 6;

bool sInited = false;
bool sEnabled = true;
bool sVerify = false;
bool sRecording = false;
bool sAborted = false;
bool sPrevValid = false;
uint64_t* sWhy = nullptr;  // the counter of what made sPrevValid false
uint32_t sSerial = 0;
std::vector<ScreenCommand> sCur;
std::vector<ScreenCommand> sPrev;
std::vector<uint8_t> sShadow;
Stats sStats;

// the working storage of the match, kept from frame to frame
std::vector<char> sMatchedPrev;
std::vector<char> sMatchedCur;
std::vector<int> sTraceV;
std::vector<std::vector<int>> sTrace;

void Init()
{
	if (sInited)
		return;
	sInited = true;
	const char* aVerify = getenv("JB_VERIFY_FRAMES");
	sVerify = aVerify != nullptr && aVerify[0] != 0 && aVerify[0] != '0';
	const char* aOff = getenv("JB_NO_SCREENLOG");
	sEnabled = !(aOff != nullptr && aOff[0] != 0 && aOff[0] != '0');
}

uint64_t Mix(uint64_t theHash, uint64_t theValue)
{
	return theHash ^ (theValue + 0x9e3779b97f4a7c15ull + (theHash << 6) + (theHash >> 2));
}

uint64_t HashOf(const ScreenCommand& theCommand)
{
	uint64_t aHash = Mix(0, (uint64_t)theCommand.mKind | ((uint64_t)theCommand.mKeyed << 8));
	aHash = Mix(aHash, ((uint64_t)(uint32_t)theCommand.mX << 32) | (uint32_t)theCommand.mY);
	aHash = Mix(aHash, ((uint64_t)(uint32_t)theCommand.mRect.left << 32) | (uint32_t)theCommand.mRect.top);
	aHash = Mix(aHash, ((uint64_t)(uint32_t)theCommand.mRect.right << 32) | (uint32_t)theCommand.mRect.bottom);
	aHash = Mix(aHash, ((uint64_t)(uint32_t)theCommand.mScaleX << 32) | (uint32_t)theCommand.mScaleY);
	aHash = Mix(aHash, ((uint64_t)(uint32_t)theCommand.mAlpha << 32) | theCommand.mColour);
	return Mix(aHash, theCommand.mStamp);
}

bool SameRect(const Rect& theA, const Rect& theB)
{
	return theA.left == theB.left && theA.top == theB.top && theA.right == theB.right && theA.bottom == theB.bottom;
}

// the same pixels drawn the same way (the image itself is not looked at: its stamp stands for its pixels)
bool Same(const ScreenCommand& theA, const ScreenCommand& theB)
{
	return theA.mHash == theB.mHash && theA.mKind == theB.mKind && theA.mKeyed == theB.mKeyed && theA.mX == theB.mX &&
		theA.mY == theB.mY && SameRect(theA.mRect, theB.mRect) && theA.mScaleX == theB.mScaleX &&
		theA.mScaleY == theB.mScaleY && theA.mAlpha == theB.mAlpha && theA.mColour == theB.mColour &&
		theA.mStamp == theB.mStamp;
}

void Add(const ScreenCommand& theCommand)
{
	sCur.push_back(theCommand);
	sCur.back().mHash = HashOf(sCur.back());
}

Rect ScreenRect();

// true when the command overwrites every pixel of its bounds with its own, whatever was there
bool Overwrites(const ScreenCommand& theCommand)
{
	if (theCommand.mKind == ScreenCommand::FILL)
		return true;
	if (theCommand.mKind == ScreenCommand::FILL_ALPHA)
		return false;
	// the unkeyed 24-bit image at full opacity is copied; the others are blended over what is there
	return theCommand.mImage->mBitCount == 0x18 && !theCommand.mKeyed && theCommand.mAlpha == 0xff;
}

// theRects with theCover taken out of them
void Subtract(std::vector<Rect>& theRects, const Rect& theCover)
{
	std::vector<Rect> aResult;
	for (const Rect& aRect : theRects)
	{
		if (theCover.right <= aRect.left || aRect.right <= theCover.left || theCover.bottom <= aRect.top ||
			aRect.bottom <= theCover.top)
		{
			aResult.push_back(aRect);
			continue;
		}
		Rect aPiece;
		if (aRect.top < theCover.top)
		{
			SetRect(aPiece, aRect.left, aRect.top, aRect.right, theCover.top);
			aResult.push_back(aPiece);
		}
		if (theCover.bottom < aRect.bottom)
		{
			SetRect(aPiece, aRect.left, theCover.bottom, aRect.right, aRect.bottom);
			aResult.push_back(aPiece);
		}
		int aTop = aRect.top < theCover.top ? theCover.top : aRect.top;
		int aBottom = theCover.bottom < aRect.bottom ? theCover.bottom : aRect.bottom;
		if (aRect.left < theCover.left)
		{
			SetRect(aPiece, aRect.left, aTop, theCover.left, aBottom);
			aResult.push_back(aPiece);
		}
		if (theCover.right < aRect.right)
		{
			SetRect(aPiece, theCover.right, aTop, aRect.right, aBottom);
			aResult.push_back(aPiece);
		}
	}
	theRects.swap(aResult);
}

// true when the commands that overwrite their pixels cover every pixel of the screen between them: each pixel then
// holds what the commands drew, and nothing of the frame before (so a frame like the last one, in the commands that draw
// a pixel, is the same at that pixel)
bool CoversScreen(const std::vector<ScreenCommand>& theList)
{
	std::vector<Rect> aLeft;
	aLeft.push_back(ScreenRect());
	for (const ScreenCommand& aCommand : theList)
	{
		if (!Overwrites(aCommand))
			continue;
		Subtract(aLeft, aCommand.mBounds);
		if (aLeft.empty())
			return true;
		if (32 < aLeft.size())
			return false;
	}
	return false;
}

// every command of theList, each clipped to theClip (the screen, or a rect inside it), drawn to the screen in order
void Execute(const std::vector<ScreenCommand>& theList, const Rect& theClip)
{
	for (const ScreenCommand& aCommand : theList)
	{
		Rect aRect;
		if (!IntersectRect(aRect, aCommand.mBounds, theClip))
			continue;
		switch (aCommand.mKind)
		{
		case ScreenCommand::DRAW_IMAGE:
			aCommand.mImage->Blit(aCommand.mX, aCommand.mY, aCommand.mRect, aCommand.mScaleX, aCommand.mScaleY,
				aCommand.mAlpha, gScreen, gScreenPitch, &aRect, aCommand.mKeyed ? 1 : 0);
			break;
		case ScreenCommand::FILL:
			ScreenFillRectDirect(&aRect, aCommand.mColour);
			break;
		case ScreenCommand::FILL_ALPHA:
			ScreenFillRectAlphaDirect(aRect, aCommand.mAlpha, aCommand.mColour);
			break;
		}
	}
}

Rect ScreenRect()
{
	Rect aScreen;
	SetRect(aScreen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	return aScreen;
}

// The match of theA[0, theCountA) and theB[0, theCountB) in order, the longest one (Myers, "An O(ND) difference
// algorithm and its variations": D the number of commands in one list and not the other): the matched commands are
// marked in theMatchedA and theMatchedB. False when D would be over kMaxEdits.
bool Match(const ScreenCommand* theA, int theCountA, const ScreenCommand* theB, int theCountB, char* theMatchedA,
	char* theMatchedB)
{
	int aMaxEdits = theCountA + theCountB < kMaxEdits ? theCountA + theCountB : kMaxEdits;
	// the furthest position in theA reached on each diagonal (position in theA minus position in theB), by diagonal +
	// anOffset, after each number of edits
	int anOffset = aMaxEdits + 1;
	sTraceV.assign((size_t)(2 * aMaxEdits + 3), 0);
	sTrace.clear();
	for (int anEdits = 0; anEdits <= aMaxEdits; anEdits++)
	{
		sTrace.push_back(sTraceV);
		for (int aDiagonal = -anEdits; aDiagonal <= anEdits; aDiagonal += 2)
		{
			int aPosA;
			if (aDiagonal == -anEdits ||
				(aDiagonal != anEdits && sTraceV[anOffset + aDiagonal - 1] < sTraceV[anOffset + aDiagonal + 1]))
				aPosA = sTraceV[anOffset + aDiagonal + 1];
			else
				aPosA = sTraceV[anOffset + aDiagonal - 1] + 1;
			int aPosB = aPosA - aDiagonal;
			while (aPosA < theCountA && aPosB < theCountB && Same(theA[aPosA], theB[aPosB]))
			{
				aPosA++;
				aPosB++;
			}
			sTraceV[anOffset + aDiagonal] = aPosA;
			if (theCountA <= aPosA && theCountB <= aPosB)
			{
				// back along the path, marking the commands of the diagonal runs
				aPosA = theCountA;
				aPosB = theCountB;
				for (int anEdit = anEdits; 0 <= anEdit; anEdit--)
				{
					const std::vector<int>& aBefore = sTrace[anEdit];
					int aCurrentDiagonal = aPosA - aPosB;
					int aPrevDiagonal;
					if (aCurrentDiagonal == -anEdit ||
						(aCurrentDiagonal != anEdit &&
							aBefore[anOffset + aCurrentDiagonal - 1] < aBefore[anOffset + aCurrentDiagonal + 1]))
						aPrevDiagonal = aCurrentDiagonal + 1;
					else
						aPrevDiagonal = aCurrentDiagonal - 1;
					int aPrevPosA = aBefore[anOffset + aPrevDiagonal];
					int aPrevPosB = aPrevPosA - aPrevDiagonal;
					while (aPrevPosA < aPosA && aPrevPosB < aPosB)
					{
						aPosA--;
						aPosB--;
						theMatchedA[aPosA] = 1;
						theMatchedB[aPosB] = 1;
					}
					aPosA = aPrevPosA;
					aPosB = aPrevPosB;
				}
				return true;
			}
		}
	}
	return false;
}

bool Touch(const Rect& theA, const Rect& theB)
{
	return theA.left <= theB.right && theB.left <= theA.right && theA.top <= theB.bottom && theB.top <= theA.bottom;
}

// theRect joins the rects: with those it touches into one
void AddRect(std::vector<Rect>& theRects, Rect theRect)
{
	bool aMerged = true;
	while (aMerged)
	{
		aMerged = false;
		for (size_t i = 0; i < theRects.size(); i++)
		{
			if (Touch(theRects[i], theRect))
			{
				UnionRect(theRect, theRect, theRects[i]);
				theRects.erase(theRects.begin() + i);
				aMerged = true;
				break;
			}
		}
	}
	theRects.push_back(theRect);
}

// the rects of the screen that the two frames' commands do not both draw the same way
void ChangedRects(const std::vector<ScreenCommand>& thePrev, const std::vector<ScreenCommand>& theCur,
	std::vector<Rect>& theRects)
{
	int aCountA = (int)thePrev.size();
	int aCountB = (int)theCur.size();
	int aFirst = 0;
	while (aFirst < aCountA && aFirst < aCountB && Same(thePrev[aFirst], theCur[aFirst]))
		aFirst++;
	int aLast = 0;
	while (aLast < aCountA - aFirst && aLast < aCountB - aFirst &&
		Same(thePrev[aCountA - 1 - aLast], theCur[aCountB - 1 - aLast]))
		aLast++;
	int aMiddleA = aCountA - aFirst - aLast;
	int aMiddleB = aCountB - aFirst - aLast;
	if (aMiddleA == 0 && aMiddleB == 0)
		return;
	sMatchedPrev.assign((size_t)aMiddleA, 0);
	sMatchedCur.assign((size_t)aMiddleB, 0);
	// (a failed match leaves the flags that were set: they are cleared, the middle all counts as changed)
	if (!Match(thePrev.data() + aFirst, aMiddleA, theCur.data() + aFirst, aMiddleB, sMatchedPrev.data(),
		sMatchedCur.data()))
	{
		sMatchedPrev.assign((size_t)aMiddleA, 0);
		sMatchedCur.assign((size_t)aMiddleB, 0);
	}
	for (int i = 0; i < aMiddleA; i++)
	{
		if (!sMatchedPrev[i])
			AddRect(theRects, thePrev[aFirst + i].mBounds);
	}
	for (int i = 0; i < aMiddleB; i++)
	{
		if (!sMatchedCur[i])
			AddRect(theRects, theCur[aFirst + i].mBounds);
	}
	if (kMaxRects < theRects.size())
	{
		Rect aAll = theRects[0];
		for (size_t i = 1; i < theRects.size(); i++)
			UnionRect(aAll, aAll, theRects[i]);
		theRects.clear();
		theRects.push_back(aAll);
	}
}

// draws what waits, in full, and leaves the rest of the frame to be drawn as it comes
void Flush()
{
	Execute(sCur, ScreenRect());
	sCur.clear();
	sRecording = false;
	sAborted = true;
	sPrevValid = false;
}

// theCur drawn in full over the screen and compared with the screen as it is: the process ends at a difference
void Verify(const std::vector<ScreenCommand>& theCur, const std::vector<Rect>& theDirty)
{
	size_t aSize = (size_t)gScreenPitch * SCREEN_HEIGHT;
	sShadow.assign(gScreen, gScreen + aSize);
	Execute(theCur, ScreenRect());
	for (int y = 0; y < SCREEN_HEIGHT; y++)
	{
		const uint32_t* aOld = (const uint32_t*)(sShadow.data() + (size_t)y * gScreenPitch);
		const uint32_t* aNew = (const uint32_t*)(gScreen + (size_t)y * gScreenPitch);
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			if (((aOld[x] ^ aNew[x]) & 0xffffff) == 0)
				continue;
			fprintf(stderr, "screenlog: frame %llu differs from its full redraw at (%d, %d): %06x, redrawn %06x\n",
				(unsigned long long)sStats.mFrames, x, y, aOld[x] & 0xffffff, aNew[x] & 0xffffff);
			for (const Rect& aRect : theDirty)
				fprintf(stderr, "  changed rect %d,%d - %d,%d\n", aRect.left, aRect.top, aRect.right, aRect.bottom);
			for (size_t i = 0; i < theCur.size(); i++)
			{
				const ScreenCommand& aCommand = theCur[i];
				if (aCommand.mBounds.left <= x && x < aCommand.mBounds.right && aCommand.mBounds.top <= y &&
					y < aCommand.mBounds.bottom)
					fprintf(stderr, "  command %zu kind %d stamp %u at %d,%d bounds %d,%d - %d,%d scale %d,%d alpha %d\n",
						i, (int)aCommand.mKind, aCommand.mStamp, aCommand.mX, aCommand.mY, aCommand.mBounds.left,
						aCommand.mBounds.top, aCommand.mBounds.right, aCommand.mBounds.bottom, aCommand.mScaleX,
						aCommand.mScaleY, aCommand.mAlpha);
			}
			fflush(stderr);
			abort();
		}
	}
	sStats.mVerified++;
}

} // namespace

bool Recording()
{
	return sRecording;
}

bool Verifying()
{
	Init();
	return sVerify;
}

const Stats& GetStats()
{
	return sStats;
}

void Begin()
{
	Init();
	// (a frame that starts inside another one draws what the outer one has waiting first, as the original's draws were)
	if (sRecording)
		Flush();
	sCur.clear();
	sAborted = false;
	sRecording = sEnabled;
	if (sRecording)
		sSerial++;
}

bool RecordDraw(Image* theImage, int theX, int theY, const Rect& theSrc, int theScaleX, int theScaleY, int theAlpha)
{
	int aWidth = theSrc.right - theSrc.left;
	int aHeight = theSrc.bottom - theSrc.top;
	if (theScaleX != 100 || theScaleY != 100)
	{
		aWidth = MulDiv(aWidth, theScaleX, 100);
		aHeight = MulDiv(aHeight, theScaleY, 100);
	}
	Rect aDest;
	SetRect(aDest, theX, theY, theX + aWidth, theY + aHeight);
	ScreenCommand aCommand;
	if (!IntersectRect(aCommand.mBounds, aDest, ScreenRect()))
		return true;
	aCommand.mKind = ScreenCommand::DRAW_IMAGE;
	aCommand.mKeyed = theImage->mKeyed;
	aCommand.mX = theX;
	aCommand.mY = theY;
	aCommand.mRect = theSrc;
	aCommand.mScaleX = theScaleX;
	aCommand.mScaleY = theScaleY;
	aCommand.mAlpha = theAlpha;
	aCommand.mStamp = theImage->mStamp;
	aCommand.mImage = theImage;
	theImage->mRecordSerial = sSerial;
	Add(aCommand);
	return true;
}

void RecordFill(const Rect& theRect, uint32_t theColour)
{
	ScreenCommand aCommand;
	aCommand.mKind = ScreenCommand::FILL;
	aCommand.mRect = theRect;
	aCommand.mBounds = theRect;
	aCommand.mColour = theColour;
	Add(aCommand);
}

void RecordFillAlpha(const Rect& theRect, int theAlpha, uint32_t theColour)
{
	ScreenCommand aCommand;
	aCommand.mKind = ScreenCommand::FILL_ALPHA;
	aCommand.mRect = theRect;
	aCommand.mBounds = theRect;
	aCommand.mAlpha = theAlpha;
	aCommand.mColour = theColour;
	Add(aCommand);
}

void NoteDirectDraw()
{
	sPrevValid = false;
	sWhy = &sStats.mAfterDirect;
}

void BeforePixelsFreed(Image* theImage)
{
	if (sRecording && theImage->mRecordSerial == sSerial)
		Flush();
}

void Invalidate()
{
	sPrevValid = false;
	sWhy = &sStats.mAfterInvalidate;
}

void End(std::vector<Rect>& theDirty)
{
	theDirty.clear();
	Rect aScreen = ScreenRect();
	sStats.mFrames++;
	if (!sRecording)
	{
		// nothing was kept (JB_NO_SCREENLOG), or the frame was flushed part of the way
		sStats.mFull++;
		if (sAborted)
			sStats.mAborted++;
		sPrevValid = false;
		theDirty.push_back(aScreen);
		return;
	}
	sRecording = false;
	if (sCur.empty())
	{
		// no draw: the screen is what it was
		if (sPrevValid)
		{
			sStats.mIncremental++;
			sStats.mIdentical++;
		}
		else
		{
			sStats.mFull++;
			theDirty.push_back(aScreen);
		}
		return;
	}
	if (!CoversScreen(sCur))
	{
		Execute(sCur, aScreen);
		sCur.clear();
		sPrev.clear();
		sPrevValid = false;
		sWhy = &sStats.mAfterNoCover;
		sStats.mFull++;
		sStats.mNoCover++;
		theDirty.push_back(aScreen);
		return;
	}
	sStats.mCommands += sCur.size();
	if (!sPrevValid)
	{
		Execute(sCur, aScreen);
		sStats.mFull++;
		if (sWhy != nullptr)
			(*sWhy)++;
		theDirty.push_back(aScreen);
	}
	else
	{
		ChangedRects(sPrev, sCur, theDirty);
		for (const Rect& aRect : theDirty)
		{
			Execute(sCur, aRect);
			sStats.mDirtyPixels += (uint64_t)(aRect.right - aRect.left) * (aRect.bottom - aRect.top);
		}
		sStats.mIncremental++;
		if (theDirty.empty())
			sStats.mIdentical++;
		if (sVerify)
			Verify(sCur, theDirty);
	}
	sPrev.swap(sCur);
	sCur.clear();
	sPrevValid = true;
}

void Shutdown()
{
	if (sVerify)
		fprintf(stderr,
			"screenlog: %llu frames, %llu incremental (%llu without a change), %llu in full (%llu flushed, %llu with no "
			"cover, %llu after one, %llu after a draw outside a frame, %llu after a reset), %llu commands, "
			"%llu pixels changed, %llu compared with a full redraw\n",
			(unsigned long long)sStats.mFrames, (unsigned long long)sStats.mIncremental,
			(unsigned long long)sStats.mIdentical, (unsigned long long)sStats.mFull,
			(unsigned long long)sStats.mAborted, (unsigned long long)sStats.mNoCover,
			(unsigned long long)sStats.mAfterNoCover, (unsigned long long)sStats.mAfterDirect,
			(unsigned long long)sStats.mAfterInvalidate, (unsigned long long)sStats.mCommands,
			(unsigned long long)sStats.mDirtyPixels, (unsigned long long)sStats.mVerified);
	sRecording = false;
	sPrevValid = false;
	sCur.clear();
	sPrev.clear();
}

} // namespace screenlog

} // namespace jb
