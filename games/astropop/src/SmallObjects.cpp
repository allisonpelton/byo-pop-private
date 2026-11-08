// The small objects the Board keeps lists of: floating texts, hint boxes, the goal check.

#include "SmallObjects.h"
#include "Font.h"
#include "Graphics.h"
#include <SDL.h>

using namespace Sexy;

// 0x408890: one update of a floating text. It rises one pixel an update (-1 in the three fields: nothing else was
// asked for), or moves to its height at its speed, waits there for its hold time and then moves on up; it is done when
// it is above the screen. Unless it keeps its colour it cycles its colour phase and ends when its life has run out.
void FloatText::Update()
{
	mAge++;
	if (mDone)
		return;

	if (mSpeed == -1.0f && m40 == -1.0f && mTargetY == -1.0f)
		mY = (float) ((double) mY - 1.0);
	else if (mY > mTargetY)
	{
		double aNew = (double) mY - (double) mSpeed;
		mY = (float) aNew;
		if (aNew <= (double) mTargetY)
		{
			mY = mTargetY;
			mHoldStart = mAge;
		}
	}
	else if (mY == mTargetY)
	{
		if (mAge - mHoldStart >= mHold)
			mY = (float) ((double) mY - (double) mSpeed);
	}
	else
		mY = (float) ((double) mY - (double) mSpeed);

	if ((double) -mFont->mHeight > (double) mY)
		mDone = true;

	if (!mNoFade)
	{
		m10 = (m10 + 7) % 0xff;
		mLife--;
		if (mLife < 1)
		{
			mLife = 0;
			mDone = true;
		}
	}
	if (mHold == 500000000)
	{
		m18--;
		if (m18 < 1)
		{
			m50 -= 2;
			if (m50 < 0)
				m50 = 0;
		}
	}
}

// 0x456660
HintBox::HintBox()
{
	mFont = NULL;
	m38 = Color(0x28, 0x41, 0x2d);
	m48 = Color(0xd7, 0xff, 0x0a);
	mShowTime = 100;
	mHideTime = 200;
	m21 = true;
	m1c = true;
	m1e = true;
	m1f = true;
	m1d = false;
	m20 = false;
	m04 = 0;
	m64 = 0;
	m60 = 0;
	mY = 0;
	mX = 0;
	m08 = 0;
	mHeight = 0;
	mWidth = 0;
}

// 0x435a40: the box counts; once it is shown it waits mShowTime updates, slides in, stays mHideTime updates and goes
void HintBox::Update()
{
	m04++;
	if (m1c)
		m08++;

	if (m21 && !m20)
	{
		if (!m1d)
		{
			if (mShowTime <= m08)
			{
				m1d = true;
				m20 = true;
				m08 = 0;
				return;
			}
			return;
		}
	}
	else if (!m1d)
		return;

	if (m20 && mHideTime <= m08)
	{
		m1d = false;
		m20 = false;
		m1c = false;
		m08 = 0;
	}
}

// 0x43c5d0: the panel (a frame of one colour round a fill of another) and its lines of text
void HintBox::Draw(Graphics* g)
{
	if (m1d && m1f && !m1e)
	{
		g->SetFont(mFont);
		g->SetColor(Color(0x69, 0xaa, 0x3c));
		g->FillRect(Rect(mX, mY, mWidth, mHeight));
		g->SetColor(m38);
		g->FillRect(Rect(mX + 1, mY + 1, mWidth - 2, mHeight - 2));
		g->SetColor(m48);
		for (unsigned int i = 0; i < mLines.size(); i++)
			g->DrawString(mLines[i], mX + 3, mFont->GetAscent() * (i + 1) + i * 2 + mY);
	}
}

// 0x4359f0: the box is asked to show or hide itself
void HintBox::SetShown(bool theShow)
{
	if (!theShow)
	{
		if (m21)
		{
			m08 = 0;
			m1c = false;
			m21 = false;
			m1f = false;
			m1e = true;
			m1d = false;
		}
	}
	else if (!m21)
	{
		m1c = true;
		m21 = true;
		m20 = false;
		m1f = true;
		m1e = false;
		m1d = false;
	}
}

// 0x43d990
void HintBox::SetFont(Font* theFont)
{
	mFont = theFont;
	mWidth = ComputeWidth();
	mHeight = ComputeHeight();
}

// 0x43c700: the widest line and 8
int HintBox::ComputeWidth()
{
	if (mFont == NULL)
		return 0;
	int aWidest = 0;
	for (unsigned int i = 0; i < mLines.size(); i++)
	{
		int aWidth = mFont->StringWidth(mLines[i]);
		if (aWidth > aWidest)
			aWidest = aWidth;
	}
	return aWidest + 8;
}

// 0x436490: the lines' heights (ascent each, and 2 between) and 4
int HintBox::ComputeHeight()
{
	if (mFont == NULL)
		return 0;
	int aLines = (int) mLines.size();
	return mFont->GetAscent() * aLines + 4 + aLines * 2;
}

// 0x460a70
void HintBox::AddLine(const std::string& theLine)
{
	mLines.push_back(theLine);
	mWidth = ComputeWidth();
	mHeight = ComputeHeight();
}

// 0x4264a0: counts the update and runs the check the kind asks for (0x426370, 0x40b340, 0x40b430, 0x40b540)
bool GoalChecker::Check(int theValue)
{
	mCounter++;
	SDL_Log("AstroPop: GoalChecker::Check(%d) kind %d not translated yet", theValue, mKind);
	return false;
}
