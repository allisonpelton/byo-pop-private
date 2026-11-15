#include "Bubbles.h"
#include "PixelusApp.h"
#include "Res.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "Compat2004.h"

using namespace Sexy;

HintBubbles::HintBubbles(PixelusApp* theApp)
{
	mApp = theApp;
	mNextId = 1;
	mMouseVisible = false;
}

// 0x40ece0: the first bubble is 1; one that a click made (theFlagged) shows at once
int HintBubbles::Add(int theX, int theY, int theCorner, const std::string& theText, bool theFlagged)
{
	Bubble aBubble;
	aBubble.mText = theText;
	aBubble.mAlpha = 0;
	aBubble.mRemoving = false;
	aBubble.mCorner = theCorner;
	aBubble.mX = theX;
	aBubble.mY = theY;
	aBubble.mId = mNextId++;
	aBubble.mDirty = false;
	aBubble.mFlagged = theFlagged;
	aBubble.mDelay = theFlagged ? 0 : 0x32;
	aBubble.mSoundDelay = 0x32;
	mBubbles.push_back(aBubble);
	return aBubble.mId;
}

// 0x40e550: the bubble fades out
void HintBubbles::Remove(int theId)
{
	for (size_t i = 0; i < mBubbles.size(); i++)
		if (mBubbles[i].mId == theId)
		{
			mBubbles[i].mRemoving = true;
			return;
		}
}

// 0x40e590
void HintBubbles::RemoveAll()
{
	for (size_t i = 0; i < mBubbles.size(); i++)
		mBubbles[i].mRemoving = true;
}

// 0x40e5c0: the bubbles a click made fade out; true when there was one
bool HintBubbles::RemoveFlagged()
{
	bool aFound = false;
	for (size_t i = 0; i < mBubbles.size(); i++)
		if (mBubbles[i].mFlagged)
		{
			mBubbles[i].mRemoving = true;
			aFound = true;
		}
	return aFound;
}

// 0x40ede0
void HintBubbles::Update()
{
	bool aChanged = false;
	for (size_t i = 0; i < mBubbles.size(); i++)
	{
		Bubble& aBubble = mBubbles[i];
		if (aBubble.mSoundDelay > 0 && --aBubble.mSoundDelay == 0)
			mApp->PlaySample(SOUND_HINT_POPUP_01);
		int aMax = aBubble.mX > 200 ? 200 : 255;
		if (aBubble.mDirty)
		{
			aBubble.mDirty = false;
			aChanged = true;
		}
		if (aBubble.mDelay < 1)
		{
			if (!aBubble.mRemoving)
			{
				if (aBubble.mAlpha < aMax)
				{
					aBubble.mAlpha += 5;
					aChanged = true;
				}
				if (aBubble.mAlpha >= aMax)
					aBubble.mAlpha = aMax;
			}
			else
			{
				if (aBubble.mAlpha > 0)
				{
					aBubble.mAlpha -= 0x50;
					aChanged = true;
				}
				if (aBubble.mAlpha < 0)
					aBubble.mAlpha = 0;
			}
		}
		else
			aBubble.mDelay--;
	}
	for (size_t i = 0; i < mBubbles.size();)
	{
		if (mBubbles[i].mRemoving && mBubbles[i].mAlpha < 1)
			mBubbles.erase(mBubbles.begin() + i);	// 0x40ec40
		else
			i++;
	}
	if (aChanged)
		MarkDirty();
}

// 0x431c70: the lines the text takes at the width (a '^' starts a colour code of 8 characters, as in the game's other
// texts; '^^' is one '^')
int HintBubbles::CountLines(Graphics* g, int theWidth, const std::string& theText)
{
	Font* aFont = g->GetFont();
	int aLines = 0;
	int aLastSpace = -1;
	int aWidth = 0;
	int aLineStart = 0;
	char aPrev = 0;
	int aLength = (int) theText.size();
	int aPos = 0;
	while (aPos < aLength)
	{
		char aChar = theText[aPos];
		int aNext = aPos;
		int aSpace = aLastSpace;
		bool aMeasure = true;
		if (aChar == '^')
		{
			if (aPos + 1 < aLength)
			{
				if (theText[aPos + 1] == '^')
					aNext = aPos + 1;
				else
				{
					aNext = aPos + 8;
					aMeasure = false;
				}
			}
		}
		else
		{
			aSpace = aPos;
			if (aChar != ' ')
			{
				aSpace = aLastSpace;
				if (aChar == '\n')
				{
					aWidth = theWidth + 1;
					aSpace = aPos;
					aNext = aPos + 1;
				}
			}
		}
		if (aMeasure)
		{
			aLastSpace = aSpace;
			aWidth += aFont->CharWidthKern(aChar, aPrev);
			aPrev = aChar;
			if (aWidth > theWidth)
			{
				if (aLastSpace == -1)
				{
					if (aNext < aLineStart + 1)
						aNext++;
				}
				else
					aNext = aLastSpace + 1;
				aLastSpace = -1;
				aWidth = 0;
				aLines++;
				aPrev = 0;
				aLineStart = aNext;
			}
			else
				aNext++;
		}
		aPos = aNext;
	}
	if (aLineStart < aLength)
		aLines++;
	return aLines;
}

// 0x40e5f0
void HintBubbles::Draw(Graphics* g)
{
	Graphics aGraphics(*g);
	aGraphics.SetColorizeImages(true);
	Font* aFont = mApp->mFonts[FONT_GARAMOND14];
	g->SetFont(aFont);
	g->SetColor(Color(255, 255, 255, 255));
	for (size_t i = 0; i < mBubbles.size(); i++)
	{
		const Bubble& aBubble = mBubbles[i];
		if (aBubble.mDelay > 0)
			continue;
		int aSpacing = aFont->GetLineSpacing();
		int aTextOffset = 0x2d;
		Image* anImage = NULL;
		int aX = 0, aY = 0;
		switch (aBubble.mCorner)
		{
		case 0:
			anImage = mApp->mImages[IMAGE_HINTBUBBLE_NW];
			aX = aBubble.mX - anImage->GetWidth() / 2 - 0x1c;
			aY = aBubble.mY - anImage->GetHeight();
			break;
		case 1:
			anImage = mApp->mImages[IMAGE_HINTBUBBLE_NE];
			aX = aBubble.mX - anImage->GetWidth() / 2 + 0x1c;
			aY = aBubble.mY - anImage->GetHeight();
			break;
		case 2:
			anImage = mApp->mImages[IMAGE_HINTBUBBLE_SE];
			aX = aBubble.mX - anImage->GetWidth() / 2 + 0x1c;
			aY = aBubble.mY;
			aTextOffset = 0x55;
			break;
		default:
			anImage = mApp->mImages[IMAGE_HINTBUBBLE_SW];
			aX = aBubble.mX - anImage->GetWidth() / 2 - 0x1c;
			aY = aBubble.mY;
			aTextOffset = 0x55;
			break;
		}
		aGraphics.SetColor(Color(255, 255, 255, aBubble.mAlpha));
		aGraphics.DrawImage(anImage, aX, aY);
		int aHeight = GetWordWrappedHeight2004(g, 0x82, aBubble.mText, aSpacing);
		Rect aRect(aX + 10, aY - aHeight / 2 + aTextOffset, 0x82, aHeight);
		if (CountLines(g, 0x82, aBubble.mText) > 3)
		{
			aSpacing -= 2;
			aHeight = GetWordWrappedHeight2004(g, 0x82, aBubble.mText, aSpacing);
			aRect = Rect(aX + 10, aY - aHeight / 2 + aTextOffset, 0x82, aHeight);
		}
		WriteWordWrapped2004(g, aRect, aBubble.mText, aSpacing, 0);
	}
}
