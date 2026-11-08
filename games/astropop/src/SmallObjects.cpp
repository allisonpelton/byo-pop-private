// The small objects the Board keeps lists of: floating texts, hint boxes, the goal check.

#include "SmallObjects.h"
#include "Font.h"
#include "Graphics.h"
#include <SDL.h>
#include <math.h>

using namespace Sexy;

// 0x4107e0: a text at (theX, theY) in a font. The text moves up the screen unless it is given a height to move to
// (mTargetY) and lives 250 updates; theFlag false puts it 100 pixels lower. A text that would end past the right edge of
// the 640 pixel screen is centred instead.
FloatText::FloatText(const std::string& theText, Font* theFont, float theX, float theY, bool theFlag)
{
	m40 = -1.0f;
	mTargetY = -1.0f;
	mSpeed = -1.0f;
	mX = theX;
	m18 = 0;
	mNoFade = false;
	m50 = 0xff;
	m10 = 0;
	mLife = 0xfa;
	mHoldStart = 0;
	mHold = 500000000;
	mY = theY;
	if (!theFlag)
		mY = theY + 100.0f;
	mText = theText;
	mFont = theFont;
	mDone = false;
	mAge = 0;
	mWidth = theFont->StringWidth(theText);
	if (640.0f < (float) mWidth + mX)
		mX = (float) (0x140 - mWidth / 2);
}

// 0x4108e0: a text that rises from the bottom of the screen (y 480) to theTargetY at theSpeed, waits there theHold updates and
// goes on up (the clear bonus).
FloatText::FloatText(const std::string& theText, Font* theFont, float theX, float theTargetY, int theHold, float theSpeed)
{
	m18 = 0;
	m50 = 0xff;
	m10 = 0;
	mLife = 0xfa;
	mX = theX;
	mY = 480.0f;
	mText = theText;
	m40 = theX;
	mTargetY = theTargetY;
	mFont = theFont;
	mNoFade = false;
	mDone = false;
	mAge = 0;
	mHold = theHold;
	mHoldStart = 0;
	mSpeed = theSpeed;
	mWidth = theFont->StringWidth(mText);
}

// 0x40b600
LevelUpText::LevelUpText(Image* theImage, int theY)
{
	mImage = theImage;
	m18 = theY + 0xbe;
	m34 = 0.73f;
	m08 = 1;
	m30 = 0.0f;
	m20 = 1.0f;
	m24 = 0.0f;
	m1c = 0.0f;
	m0c = 0x1e;
	m14 = 0;
	m10 = 100;
	m28 = 1.0f;
	m2c = 0.025f;
	m04 = false;
}

// 0x40b6a0: one update of the picture that announces a level (or a minute): its position and size ease towards the middle
// of the screen, hold there and ease away; it is over when it has faded out or shrunk to nothing.
void LevelUpText::Update()
{
	if (m08 == 0)
		return;

	if (m1c < 1.0f)
		m1c = (float) ((double) m1c + (double) 0.04f);

	double aPull = (double) 0.975f - (double) m1c * (double) 0.12f;
	double aSpring = (double) m1c * (double) 0.01f + (double) 0.002f;
	double aVel = aPull * ((-(double) m20) * aSpring + (double) m24);
	m24 = (float) aVel;
	double aPos = aVel + (double) m20;
	m20 = (float) aPos;
	if (fabs(aPos) < 0.0010000000474974513 && fabs((double) m24) < 9.999999747378752e-05)
	{
		m20 = 0.0f;
		m24 = 0.0f;
	}

	if (m0c > 0)
		m0c--;
	else
	{
		m10--;
		m14--;
		double aForce;
		if (m14 > 0)
			aForce = ((double) 1.2f - (double) m28) * (double) 0.0015f;
		else if (m10 > 0)
			aForce = ((double) 1.75f - (double) m28) * (double) 0.0015f;
		else
			aForce = (-(double) m28) * (double) 0.002f;
		m2c = (float) (aForce + (double) m2c);
		m28 = (float) ((double) m28 + (double) m2c);
	}

	if (m18 > 0)
	{
		double aFade = (double) m30 + (double) 0.02f;
		m18--;
		m30 = (float) aFade;
		if (aFade > 1.0)
			m30 = 1.0f;
	}
	else
		m30 = (float) ((double) m30 - (double) 0.03f);

	if (m30 < 0.0f || m28 <= 0.0f)
		m08 = 0;
}

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
