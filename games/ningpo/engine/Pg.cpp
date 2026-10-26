// The framework's utilities: the host, logging, strings, the dirty rectangles (see Pg.h).

#include "Pg.h"

#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace pg
{

static Host* gHost = nullptr;

void SetHost(Host* theHost)
{
	gHost = theHost;
}

Host* GetHost()
{
	return gHost;
}

void PGLog(const char* theFormat, ...)
{
	char aBuf[1024];
	va_list anArgs;
	va_start(anArgs, theFormat);
	vsnprintf(aBuf, sizeof(aBuf), theFormat, anArgs);
	va_end(anArgs);
	size_t aLen = strlen(aBuf);
	if (aLen > 0 && aBuf[aLen - 1] == '\n')
		aBuf[aLen - 1] = 0;
	if (gHost != nullptr)
		gHost->Log(aBuf);
}

void PGAlert(const char* theFormat, ...)
{
	char aBuf[1024];
	va_list anArgs;
	va_start(anArgs, theFormat);
	vsnprintf(aBuf, sizeof(aBuf), theFormat, anArgs);
	va_end(anArgs);
	if (gHost != nullptr)
		gHost->ShowMessage("Alert", aBuf);
}

void PGMessageBox(unsigned int theFlags, const char* theCaption, const char* theFormat, ...)
{
	char aBuf[1024];
	va_list anArgs;
	va_start(anArgs, theFormat);
	vsnprintf(aBuf, sizeof(aBuf), theFormat, anArgs);
	va_end(anArgs);
	if (gHost != nullptr)
		gHost->ShowMessage(theCaption, aBuf);
}

uint32_t HTimeGetTime()
{
	return gHost->TimeMs();
}

void HSleep(uint32_t theMs)
{
	gHost->Sleep((int)theMs);
}

void HStrncpy(char* theDest, const char* theSrc, int theSize)
{
	// unlike strncpy, the rest of theDest is left as it is (the best times copy a name with a size larger than
	// the name's field, 0x408010)
	int i = 0;
	if (*theSrc != '\0')
	{
		do
		{
			if (theSize - 1 <= i)
				break;
			theDest[i] = *theSrc;
			i++;
			theSrc++;
		} while (*theSrc != '\0');
	}
	theDest[i] = 0;
}

int HStricmp(const char* a, const char* b)
{
	for (;; a++, b++)
	{
		int ca = tolower((unsigned char)*a);
		int cb = tolower((unsigned char)*b);
		if (ca != cb || ca == 0)
			return ca - cb;
	}
}

int HSnprintf(char* theBuf, int theSize, const char* theFormat, ...)
{
	va_list anArgs;
	va_start(anArgs, theFormat);
	int aLen = vsnprintf(theBuf, theSize, theFormat, anArgs);
	va_end(anArgs);
	return aLen;
}

double HSin(float theDegrees)
{
	// 0x40d3d0: pi * x / 180, with the two constants as floats (0x463044, 0x463048)
	return sin((double)3.14159274f * (double)theDegrees * (double)0.00555555569f);
}

// 0x40d45c
int PyTimeDelta(PyTime a, PyTime b)
{
	int64_t aDelta = a - b;
	if ((aDelta >> 32) != 0)
		return 0;
	return (int)aDelta;
}

// 0x413eed
bool RectsTouch(const PGRect& a, const PGRect& b)
{
	if (b.right == b.left || b.bottom == b.top || a.left == a.right || a.bottom == a.top)
		return false;
	bool aX = !(((b.left < a.left) || (a.right < b.left)) && ((a.right < b.right) || (b.right < a.left)));
	bool aY = !(((b.top < a.top) || (a.bottom < b.top)) && ((a.bottom < b.bottom) || (b.bottom < a.top)));
	if (!aX && (b.left <= a.left) && (a.right <= b.right))
		aX = true;
	if (!aY && (b.top <= a.top) && (a.bottom <= b.bottom))
		aY = true;
	return aX && aY;
}

// 0x413d3f: a rectangle inside one already listed is dropped; otherwise it takes in every listed one it touches
// (again after each growth), replaces those it now contains and is appended
void HRectList::add(int theTop, int theLeft, int theBottom, int theRight)
{
	if (mCount >= 0x200)
		return;
	int aTop = theTop, aBottom = theBottom;
	if (theBottom < theTop)
	{
		aTop = theBottom;
		aBottom = theTop;
	}
	int aLeft = theLeft, aRight = theRight;
	if (theRight < theLeft)
	{
		aLeft = theRight;
		aRight = theLeft;
	}
	if (aRight == aLeft || aBottom == aTop)
		return;
	for (int i = 0; i < mCount; i++)
	{
		PGRect& r = mRects[i];
		if (r.left <= aLeft && aRight <= r.right && r.top <= aTop && aBottom <= r.bottom)
			return;
	}
	PGRect aNew = {aLeft, aTop, aRight, aBottom};
	for (int i = 0; i < mCount; i++)
	{
		PGRect& r = mRects[i];
		if (RectsTouch(r, aNew))
		{
			bool aGrew = false;
			if (r.left < aNew.left) { aNew.left = r.left; aGrew = true; }
			if (r.top < aNew.top) { aNew.top = r.top; aGrew = true; }
			if (aNew.right < r.right) { aNew.right = r.right; aGrew = true; }
			if (aNew.bottom < r.bottom) { aNew.bottom = r.bottom; aGrew = true; }
			if (aGrew)
				i = -1;
		}
	}
	for (int i = 0; i < mCount; i++)
	{
		PGRect& r = mRects[i];
		if (aNew.left <= r.left && r.right <= aNew.right && aNew.top <= r.top && r.bottom <= aNew.bottom)
		{
			memmove(&mRects[i], &mRects[i + 1], (mCount - i - 1) * sizeof(PGRect));
			mCount--;
			i--;
		}
	}
	mRects[mCount++] = aNew;
}

int HRectList::area() const
{
	int anArea = 0;
	for (int i = 0; i < mCount; i++)
		anArea += (mRects[i].right - mRects[i].left) * (mRects[i].bottom - mRects[i].top);
	return anArea;
}

} // namespace pg
