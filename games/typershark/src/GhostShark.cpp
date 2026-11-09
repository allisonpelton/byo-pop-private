#include "GhostShark.h"
#include "WinWSApp.h"
#include "Board.h"
#include "Graphics.h"
#include "Font.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// Not translated (vtordisp thunks and destructors): 0x411350 (deleting destructor, to 0x4113b0), 0x411360 and 0x411370
// (the two sizes), 0x411380 (SetY), 0x411390 (Draw, to 0x4037d0), 0x4113a0 (SetState) and 0x4113f0 (Update), which jump
// to Shark's or the Enemy's functions with the pointer adjusted

// 0x4111b0: Shark (0x4101d0) first, then the pictures the shark's movement code needs (the red one, and the mirrored
// swim picture of the app) and the colours of the word: the typed letters red (235, 0, 0), the rest black. The swim
// pictures themselves are drawn straight from the app's table in Draw
GhostShark::GhostShark(WinWSApp* theApp) :
	Shark(theApp)
{
	mImages[4] = mApp->mSharkImages[0][4];
	mImages[3] = mApp->mGhostSharkSwimDim;
	mTypedColor = Color(235, 0, 0);
	mTextColor = Color(0, 0, 0);
}

GhostShark::~GhostShark()
{
}

// the ghost drawn with its glow: first the glow picture, additive, on a frame 20 pixels wider and higher than the
// shark's (the frames lie mFrameWidth + 20 apart) and placed 10 up and to the left, then the body, the frame of
// mFrameOffset (mFrame * mFrameWidth) of its picture, both at the position mX, mY (not the last position). The
// original writes this out in each case of the switch of 0x4037d0
void GhostShark::DrawGlowed(Graphics* g, Image* theGlow, Image* theBody)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	Rect aGlowSrc(mFrame * (mFrameWidth + 20), 0, mFrameWidth + 20, mFrameHeight + 20);
	g->DrawImage(theGlow, (int)(mX - 10.0), (int)(mY - 10.0), aGlowSrc);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	Rect aBodySrc(mFrameOffset, 0, mFrameWidth, mFrameHeight);
	g->DrawImage(theBody, (int)mX, (int)mY, aBodySrc);
}

// 0x4037d0: nothing when not shown, else GameObject::Draw (the position history) and by the state (a jump table over
// 0 to 6; any other state draws nothing but the word): 0 swimming and 4 chased off, the ghost with its glow
// (IMAGE_SHARK_GHOST_SWIM and the glow IMAGE_SHARK_GHOST_GLOW_SWIM); 1 swimming back mirrored, with the app's mirrored
// pictures; 2 dead, nothing; 3 dying, the death pictures with the glow; 5 shocked, the shock picture while the
// Enemy flag mFlag30 is set, the ghost with its glow otherwise; 6 hit, the red picture (mImages[4]) while Shark's
// mFlashed is set, the ghost with its glow otherwise. Then the word, as Shark::Draw does but from the position mX, mY
void GhostShark::Draw(Graphics* g)
{
	if (!mVisible)
		return;

	GameObject::Draw(g);

	switch (mState)
	{
	case 0:
	case 4:
		DrawGlowed(g, mApp->mImages[IMAGE_SHARK_GHOST_GLOW_SWIM], mApp->mImages[IMAGE_SHARK_GHOST_SWIM]);
		break;
	case 1:
		DrawGlowed(g, mApp->mGhostSharkGlowDim, mApp->mGhostSharkSwimDim);
		break;
	case 3:
		DrawGlowed(g, mApp->mImages[IMAGE_SHARK_GHOST_GLOW_DEATH], mApp->mImages[IMAGE_SHARK_GHOST_DEATH]);
		break;
	case 5:
		if (mFlag30)
			g->DrawImage(mApp->mImages[IMAGE_SHARKSHOCK], (int)mX, (int)mY);
		else
			DrawGlowed(g, mApp->mImages[IMAGE_SHARK_GHOST_GLOW_SWIM], mApp->mImages[IMAGE_SHARK_GHOST_SWIM]);
		break;
	case 6:
		if (mFlashed)
		{
			Rect aSrc(mFrameOffset, 0, mFrameWidth, mFrameHeight);
			g->DrawImage(mImages[4], (int)mX, (int)mY, aSrc);
		}
		else
			DrawGlowed(g, mApp->mImages[IMAGE_SHARK_GHOST_GLOW_SWIM], mApp->mImages[IMAGE_SHARK_GHOST_SWIM]);
		break;
	}

	int aCenterX = (mFrameWidth >> 1) + mTextOffsetX;
	Board* aBoard = mApp->mBoard;
	if (aBoard != NULL && aBoard->mPaused)
		return;
	if (mState != 0 && mState != 4 && mState != 6)
		return;

	if (mNumTyped > 0)
	{
		g->SetFont(mTypedFont);
		g->SetColor(mTypedColor);
		int aY = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mY + 5.0);
		double aX = (double)aCenterX + mX;
		int aTypedX = (int)(aX - (double)(mTypedFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mTyped, aTypedX, aY);

		g->SetFont(mFont);
		g->SetColor(mTextColor);
		int aY2 = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mY + 5.0);
		double aTypedWidth = (double)mFont->StringWidth(mTyped);
		int aRemainingX = (int)((aTypedWidth + mX + (double)aCenterX) - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mRemaining, aRemainingX, aY2);
	}
	else
	{
		g->SetFont(mFont);
		g->SetColor(mTextColor);
		int aY = (int)((double)((mFrameHeight >> 1) + mTextOffsetY) + mY + 5.0);
		int aWordX = (int)(((double)aCenterX + mX) - (double)(mFont->StringWidth(mWord) >> 1) - 5.0);
		g->DrawString(mWord, aWordX, aY);
	}
}
