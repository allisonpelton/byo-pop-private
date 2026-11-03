#include "Animator.h"

#include "Common.h"
#include "Graphics.h"
#include "Image.h"

using namespace Sexy;

namespace JeffLib {

Animator::Animator()
{
	Init();
}

Animator::Animator(const Animator& theOther)
{
	Init();
	*this = theOther;
}

Animator::~Animator()
{
	delete mFade;
}

Animator& Animator::operator=(const Animator& theOther)
{
	if (this == &theOther)
		return *this;
	mImage = nullptr;
	if (theOther.mFade == nullptr)
		mFade = nullptr;  // port: as the original, which forgot its own (Init always makes one, so never seen)
	else
		*mFade = *theOther.mFade;
	mStopAfter = theOther.mStopAfter;
	mStopCounter = theOther.mStopCounter;
	mUpdateCount = theOther.mUpdateCount;
	mGoingBack = theOther.mGoingBack;
	mDone = theOther.mDone;
	mFrameChanged = theOther.mFrameChanged;
	mDelay = theOther.mDelay;
	SetNumFrames(theOther.mNumFrames);
	mStopped = theOther.mStopped;
	mPingPong = theOther.mPingPong;
	mOnce = theOther.mOnce;
	if (theOther.mImage != nullptr)
		SetImage(theOther.mImage);
	mMaxLoops = theOther.mMaxLoops;
	mLoopCount = theOther.mLoopCount;
	mRangeStart = theOther.mRangeStart;
	mRangeEnd = theOther.mRangeEnd;
	mDirection = theOther.mDirection;
	mStep = theOther.mStep;
	mUseRange = theOther.mUseRange;
	mX = theOther.mX;
	mY = theOther.mY;
	mUnknown30 = theOther.mUnknown30;
	mUnknown2c = theOther.mUnknown2c;
	mAdditive = theOther.mAdditive;
	mAdditiveColor = theOther.mAdditiveColor;
	return *this;
}

void Animator::Init()
{
	mDelay = 1;
	mY = 0;
	mNumFrames = 1;
	mX = 0;
	mDirection = 1;
	mGoingBack = true;
	mFrameChanged = true;
	mStep = 1;
	mUnknown2c = -1;
	mStopCounter = -1;
	mStopAfter = -1;
	mStopped = true;
	mFrame = 0;
	mUpdateCount = 0;
	mPingPong = false;
	mUseRange = false;
	mOnce = false;
	mDone = false;
	mMaxLoops = 0;
	mLoopCount = 0;
	mImage = nullptr;
	mRangeEnd = 0;
	mRangeStart = 0;
	mUnknown30 = 0;
	mUnknown99 = false;
	mUnknown98 = false;
	mAdditive = false;
	mColorize = false;
	mRandom = false;
	delete mFade;
	mFade = new Fade();
}

bool Animator::Update(bool theCountLoops)
{
	mUpdateCount++;
	mFrameChanged = false;
	if (mStopAfter > 0)
	{
		if (mStopped)
			goto end;
		if (!mDone && ++mStopCounter >= mStopAfter)
		{
			mStopped = true;
			mDone = true;
		}
	}
	if (mStopped || mDone)
		goto end;
	{
		// lo: the cel before the first (and the first again, going forwards); hi: the cel after the last
		int lo = mPingPong ? -(int)mGoingBack : (mDirection > 0) - 1;
		int hi = mNumFrames;
		int aDelay = mDelay;
		if (!mFrameDelays.empty() && hi >= 2 && mFrame >= 0 && mFrame < (int)mFrameDelays.size())
			aDelay = mFrameDelays[mFrame];
		bool aUseRange = mUseRange;
		if (aUseRange)
		{
			lo = mRangeStart - 1;
			hi = mRangeEnd + 1;
		}
		if (aDelay != 0 && mUpdateCount % aDelay == 0 && (mMaxLoops == 0 || mLoopCount <= mMaxLoops))
		{
			int aMax = mMaxLoops;
			mFrameChanged = true;
			if (mRandom)
			{
				if (mRandomOrder.empty())
				{
					if (theCountLoops)
						mLoopCount++;
					if (mOnce || (mLoopCount >= mMaxLoops && mMaxLoops > 0))
						mStopped = mDone = true;
					for (int i = 0; i < mNumFrames; i++)
						mRandomOrder.push_back(i);
				}
				// the original shows the drawn index, not the cel at it, so the cels do repeat
				int r = (int)((unsigned)Rand() % mRandomOrder.size());
				mFrame = r;
				mRandomOrder.erase(mRandomOrder.begin() + r);
			}
			else if (!mPingPong)
			{
				bool aWrapped = false;
				if (mDirection < 0)
				{
					mFrame -= mStep;
					if (mFrame <= lo)
					{
						if (aUseRange)
							hi = mRangeEnd;
						mFrame = hi - 1;
						aWrapped = true;
					}
				}
				else
				{
					mFrame += mStep;
					if (mFrame >= hi)
					{
						if (aUseRange)
							lo = mRangeStart;
						mFrame = lo;
						aWrapped = true;
					}
				}
				if (aWrapped)
				{
					if (theCountLoops)
						mLoopCount++;
					if (mOnce || (mLoopCount >= aMax && aMax > 0))
						mStopped = mDone = true;
				}
			}
			else if (!mGoingBack)
			{
				mFrame += mStep;
				if (mFrame >= hi)
				{
					if (theCountLoops)
						mLoopCount++;
					mFrame = hi - 2;
					if (mNumFrames == 1)
						mFrame = 0;
					mGoingBack = true;
					if (mLoopCount >= aMax && aMax > 0)
						mDone = mStopped = true;
				}
			}
			else
			{
				mFrame -= mStep;
				if (mFrame <= lo)
				{
					if (theCountLoops)
						mLoopCount++;
					if (!aUseRange)
						mFrame = lo >= 0 ? lo + 1 : lo + 2;
					else
						mFrame = lo + 2;
					if (mNumFrames == 1 && mFrame == 1)
						mFrame = 0;
					mGoingBack = false;
					if (mOnce || (mLoopCount >= aMax && aMax > 0))
					{
						mFrame = lo;
						mStopped = mDone = true;
					}
				}
			}
		}
		if (mFade->mState != 0)
			UpdateFade();
	}
end:
	if (mNumFrames == 1)
		mFrameChanged = false;
	return mFrameChanged;
}

void Animator::UpdateFade()
{
	Fade* f = mFade;
	if (f->mState == 2)
	{
		f->mAlpha += f->mUpSpeed;
		if (f->mAlpha >= f->mMax)
		{
			f->mAlpha = f->mMax;
			if (f->mCount > 0)
				f->mCount--;
			f->mState = 1;
		}
	}
	else
	{
		f->mAlpha -= f->mDownSpeed;
		if (f->mAlpha <= f->mMin)
		{
			f->mAlpha = f->mMin;
			if (f->mCount > 0)
				f->mCount--;
			f->mState = 2;
		}
	}
	if (f->mCount == 0)
	{
		f->mState = 0;
		if (f->mDoneWhenFinished)
			mDone = true;
	}
}

void Animator::Restart()
{
	mStopped = false;
	mFrame = mDirection < 0 ? mNumFrames - 1 : 0;
	mDone = false;
	mLoopCount = 0;
	mRangeStart = 0;
	mRangeEnd = mNumFrames;
	mUseRange = false;
	mStopCounter = 0;
	mUpdateCount = 0;
}

void Animator::SetRange(int theStart, int theEnd)
{
	mUseRange = true;
	mRangeStart = theStart;
	mRangeEnd = theEnd;
}

void Animator::SetDirection(int theDirection)
{
	mDirection = theDirection;
	if (theDirection < 0)
		mFrame = mNumFrames - 1;
}

void Animator::SetAdditiveColor(const Color& theColor)
{
	ClearColorize();
	mAdditiveColor = theColor;
	mAdditive = true;
}

void Animator::SetColorizeColor(const Color& theColor)
{
	ClearAdditive();
	mColorizeColor = theColor;
	mColorize = true;
}

void Animator::SetNumFrames(int theNumFrames)
{
	mNumFrames = theNumFrames;
	mFrame = 0;
	if (mDirection < 0)
		mFrame = theNumFrames - 1;
	mFrameDelays.assign(theNumFrames > 0 ? theNumFrames : 0, mDelay);
}

void Animator::SetImage(Image* theImage)
{
	mImage = nullptr;
	mFrame = 0;
	if (theImage != nullptr)
		SetNumFrames(theImage->mNumCols > theImage->mNumRows ? theImage->mNumCols : theImage->mNumRows);
	mImage = theImage;
}

void Animator::SetRandomOrder(bool theRandom)
{
	mRandom = theRandom;
	if (theRandom)
	{
		mRandomOrder.clear();
		for (int i = 0; i < mNumFrames; i++)
			mRandomOrder.push_back(i);
	}
}

void Animator::SetFrameDelay(int theDelay, int theFrame)
{
	if (theFrame >= 0 && theFrame < (int)mFrameDelays.size())
		mFrameDelays[theFrame] = theDelay;
	mDelay = theDelay;
}

void Animator::SetDelay(int theDelay)
{
	mDelay = theDelay;
	for (int& d : mFrameDelays)
		d = theDelay;
}

bool Animator::TakeFrameChanged()
{
	if (!mFrameChanged)
		return false;
	mFrameChanged = false;
	return true;
}

// The tint every draw starts with: the fade's alpha alone, or the additive or colourizing colour (with the fade's
// alpha while it runs). The original sets the tint again after drawing instead of clearing it, so a tinted draw
// leaves the Graphics additive or colourizing; only the plain fade is undone.
void Animator::SetDrawColor(Graphics* g) const
{
	if (!mAdditive && !mColorize && mFade->mState != 0)
	{
		g->SetColor(Color(255, 255, 255, mFade->mAlpha));
		g->SetColorizeImages(true);
	}
	if (mAdditive)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		const Color& c = mAdditiveColor;
		g->SetColor(mFade->mState == 0 ? c : Color(c.mRed, c.mGreen, c.mBlue, mFade->mAlpha));
		g->SetColorizeImages(true);
	}
	else if (mColorize)
	{
		g->SetColorizeImages(true);
		const Color& c = mColorizeColor;
		g->SetColor(mFade->mState == 0 ? c : Color(c.mRed, c.mGreen, c.mBlue, mFade->mAlpha));
	}
}

#define ANIMATOR_DRAWABLE() \
	(mImage != nullptr && !mStopped && !mDone && !(mMaxLoops > 0 && mLoopCount >= mMaxLoops))
#define ANIMATOR_DRAW_DONE(g) \
	do { if (!mAdditive && !mColorize && mFade->mState != 0) (g)->SetColorizeImages(false); } while (0)

// the cel's part of the image, for a strip of one row or one column
Rect Animator::CelSrcRect() const
{
	int x = 0, y = 0;
	if (mImage->mNumCols > mImage->mNumRows)
		x = mImage->GetCelWidth() * DrawCel();
	else if (mImage->mNumRows > mImage->mNumCols)
		y = mImage->GetCelHeight() * DrawCel();
	return Rect(x, y, mImage->GetCelWidth(), mImage->GetCelHeight());
}

void Animator::DrawF(Graphics* g, float theX, float theY, bool theFloat)
{
	if (!ANIMATOR_DRAWABLE())
		return;
	SetDrawColor(g);
	if (theFloat)
		g->DrawImageF(mImage, mX + theX, mY + theY, CelSrcRect());
	else
		g->DrawImageCel(mImage, (int)(mX + theX), (int)(mY + theY), DrawCel());
	ANIMATOR_DRAW_DONE(g);
}

void Animator::Draw(Graphics* g, int theX, int theY)
{
	if (!ANIMATOR_DRAWABLE())
		return;
	SetDrawColor(g);
	g->DrawImageCel(mImage, (int)(mX + theX), (int)(mY + theY), DrawCel());
	ANIMATOR_DRAW_DONE(g);
}

void Animator::DrawStretched(Graphics* g, float theX, float theY, int theWidth, int theHeight)
{
	if (!ANIMATOR_DRAWABLE())
		return;
	SetDrawColor(g);
	g->DrawImageCel(mImage, Rect((int)(mX + theX), (int)(mY + theY), theWidth, theHeight), DrawCel());
	ANIMATOR_DRAW_DONE(g);
}

void Animator::DrawScaled(Graphics* g, float theX, float theY, float theScale)
{
	if (!ANIMATOR_DRAWABLE())
		return;
	SetDrawColor(g);
	Rect aDest((int)(mX + theX), (int)(mY + theY), (int)((float)mImage->GetCelWidth() * theScale),
			   (int)(theScale * (float)mImage->GetCelHeight()));
	g->DrawImageCel(mImage, aDest, DrawCel());
	ANIMATOR_DRAW_DONE(g);
}

void Animator::DrawRotated(Graphics* g, float theX, float theY, float theRot, bool theFloat, float theCenterX,
						   float theCenterY)
{
	if (!ANIMATOR_DRAWABLE())
		return;
	SetDrawColor(g);
	if (theFloat)
	{
		Rect aSrc = CelSrcRect();
		g->DrawImageRotatedF(mImage, mX + theX, mY + theY, theRot, theCenterX, theCenterY, &aSrc);
	}
	else
	{
		Rect aSrc = mImage->GetCelRect(DrawCel());
		g->DrawImageRotated(mImage, (int)(mX + theX), (int)(mY + theY), theRot, (int)theCenterX, (int)theCenterY,
							&aSrc);
	}
	ANIMATOR_DRAW_DONE(g);
}

}  // namespace JeffLib
