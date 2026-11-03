#ifndef __ATOMICA_SUPPORT_H__
#define __ATOMICA_SUPPORT_H__

// The small helper classes Atomica Deluxe's code is built on (Atomica.exe 0x401000-0x401570, 0x4259a0-0x425f70,
// 0x429470-0x429f40): the pointer list, the game's own random number generator, and the image holders ("sprites")
// that carry a source rectangle. Behaviour notes: docs/GAME_NOTES.md section 2.

#include "Common.h"
#include "Rect.h"
#include "Color.h"
#include <vector>
#include <string>

namespace Sexy
{

class Image;
class MemoryImage;
class Graphics;
class SexyAppBase;
class AtomicaApp;

// 0x4259a0: the original's list of pointers (an array that grows by one), with an iteration cursor that
// Reset() (0x425c00) rewinds and Next() (0x425c10) advances; Next() returns NULL at the end. Remove() keeps the
// cursor on the element it was on: removing an element at or before the cursor moves it back by one. With a sort
// key set (0x425a10), Add() inserts in order instead of appending.
template<class T>
class PtrList
{
public:
	typedef int (*KeyFunc)(const T*);

	std::vector<T*>		mItems;
	int					mCursor;		// +8
	KeyFunc				mKey;			// the int field the sorted list orders by (+0x10: its offset), NULL: not sorted
	int					mSortMode;		// +0x14: 1 ascending, otherwise descending

	PtrList() : mCursor(0), mKey(NULL), mSortMode(0) {}

	int		Size() const { return (int) mItems.size(); }
	void	Reset() { mCursor = 0; }					// 0x425c00
	T*		Next()										// 0x425c10
	{
		if (mCursor >= (int) mItems.size())
			return NULL;
		return mItems[mCursor++];
	}
	// 0x425cf0: the element after thePtr; the first one for a list of one or for NULL; after the last, the first if theWrap
	T*		NextAfter(const T* thePtr, bool theWrap) const
	{
		if (mItems.empty())
			return NULL;
		if ((mItems.size() == 1) || (thePtr == NULL))
			return mItems[0];
		for (size_t i = 0; i < mItems.size(); i++)
		{
			if (mItems[i] == thePtr)
			{
				if (i + 1 < mItems.size())
					return mItems[i + 1];
				return theWrap ? mItems[0] : NULL;
			}
		}
		return NULL;
	}
	T*		Get(int theIndex) const						// 0x425cc0
	{
		if (theIndex < 0 || theIndex >= (int) mItems.size())
			return NULL;
		return mItems[theIndex];
	}
	void	Clear() { mItems.clear(); }					// 0x4259f0: forgets the elements, frees none of them
	bool	Contains(const T* thePtr) const				// 0x425bc0
	{
		for (size_t i = 0; i < mItems.size(); i++)
			if (mItems[i] == thePtr)
				return true;
		return false;
	}
	void	Add(T* thePtr)								// 0x425da0, sorted: 0x425a10
	{
		if (mKey == NULL)
		{
			mItems.push_back(thePtr);
			return;
		}
		if (mItems.empty())
		{
			mItems.push_back(thePtr);
			mCursor = 0;
			return;
		}
		int aNewKey = mKey(thePtr);
		size_t i = 0;
		for (; i < mItems.size(); i++)
		{
			int aKey = mKey(mItems[i]);
			if (mSortMode == 1 ? (aNewKey < aKey) : (aKey < aNewKey))
				break;
		}
		mItems.insert(mItems.begin() + i, thePtr);
		if ((int) i < mCursor)
			mCursor++;
	}
	bool	Remove(const T* thePtr)						// 0x425b00
	{
		bool aFound = false;
		for (size_t i = 0; i < mItems.size();)
		{
			if (mItems[i] == thePtr)
			{
				if ((int) i <= mCursor)
					mCursor--;
				mItems.erase(mItems.begin() + i);
				aFound = true;
			}
			else
				i++;
		}
		if (mCursor < 0)
			mCursor = 0;
		return aFound;
	}
	void	DeleteAll()									// the loop "Reset, Next, delete" that closes many destructors
	{
		Reset();
		T* anItem = Next();
		while (anItem != NULL)
		{
			delete anItem;
			anItem = Next();
		}
		Clear();
	}
};

// 0x425e30-0x425f10: the game's random numbers, a lagged Fibonacci generator over 55 words of 30 bits (tap 24
// apart), seeded from the tick count by the original and from the app's -seed by the port.
class GameRand
{
public:
	GameRand();
	void	Seed(unsigned int theSeed);			// 0x425e30
	int		Next();								// 0x425e80: 24 random bits
	int		Get(int theRange, bool theSigned = false); // 0x425f10: 0 .. theRange-1, negated half the time if signed
	int		J() const { return mJ; }		// test hooks (testhooks/Trace.cpp): where the generator is
	int		K() const { return mK; }
private:
	int		mJ;			// +4
	int		mK;			// +8
	unsigned int mState[55]; // +0xc
};

// 0x429680: an image with the rectangle of it that a draw takes, made by the image manager
class Sprite
{
public:
	Image*		mImage;			// +8
	bool		mOwnsImage;		// +0xc
	int			mWidth;			// +0x10 size of a draw
	int			mHeight;		// +0x14
	Rect		mSrc;			// +0x18 the part of the image a draw takes

	Sprite() : mImage(NULL), mOwnsImage(true), mWidth(0), mHeight(0) {}
	~Sprite();
	void		SetImage(Image* theImage);	// 0x429610: the whole image
	unsigned int* GetBits();				// 0x429650
	void		BitsChanged();				// 0x429660

	// 0x4014c0: top left at x, y
	void		Draw(Graphics* g, int theX, int theY);
	// 0x4014f0: centred on x, y
	void		DrawCentered(Graphics* g, int theX, int theY);
	// 0x401570: centred on the float position x, y (0x4015.. adds one half before it truncates)
	void		DrawCenteredF(Graphics* g, float theX, float theY);
	// 0x4296e0: the sprite cut short to a fraction (theModeX/theModeY 1: from the left/top, -1: from the right/bottom)
	void		DrawPartial(Graphics* g, int theX, int theY, float theFraction, int theModeX, int theModeY);
};

// 0x429830: owns the sprites it made (and the images of those that own theirs) and loads images by name
class SpriteMgr
{
public:
	AtomicaApp*				mApp;			// +4
	std::vector<Sprite*>	mSprites;		// +8
	bool					mLoadFailed;	// +0x24
	Graphics*				mGraphics;		// +0x20 (set by 0x429960, which no draw ever reads)

	SpriteMgr(AtomicaApp* theApp) : mApp(theApp), mLoadFailed(false), mGraphics(NULL) {}
	~SpriteMgr();

	Sprite*		Load(const std::string& theName);						// 0x429c60: images\<name>, or a popup and NULL
	Sprite*		Wrap(Image* theImage);									// 0x429bc0
	Sprite*		SubSprite(Sprite* theSheet, int theX, int theY, int theWidth, int theHeight); // 0x429970
	Sprite*		Cell(Sprite* theSheet, int theCellWidth, int theCellHeight, int theIndex);  // 0x429a10
	Sprite*		Blank(int theWidth, int theHeight);						// 0x429ab0: a clear image to draw on
	void		Free(Sprite* theSprite);								// 0x429920
};

}

#endif
