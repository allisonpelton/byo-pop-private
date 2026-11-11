// Board: the beam under the pointer (0x45ea50). It lights the column the held pieces will be fired up: the plain beam, the
// wide one of the fuel cell and the freeze bomb, the beam of a nuke with its ghost brick at the top of the stack, and
// the sparks that drift up it.

#include "Board.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"

using namespace Sexy;

// 0x45ea50
void Board::DrawBeam(Graphics* g)
{
	bool is3D = mApp->Is3DAccelerated();
	int aTick = (mPauseCount > 0) ? m1af0 : mUpdateCnt;

	Image* aParticle = NULL;        // the spark picture, if there are sparks
	Image* aGhost = NULL;           // the brick drawn at the top of the stack
	int aGhostDX = 0;
	int aGhostDY = 0;
	bool aIsFreeze = FirstPieceIsType(10);
	bool aIsFuel = FirstPieceIsType(8);
	Image* aBeam = IMAGE_BEAM;

	if (mClearCountRunning)
		return;
	if (mWarpActive || mWarpCharging || mWarpFlight || mGameOver)
	{
		if (!mShipInWarp)
			return;
		if (mGameOver)
			return;
		if (m1a60 != 1)
			return;
	}
	if (m1a34 == 1)
	{
		if (mMode == 3)
			return;
		if (mMode == 1)
			return;
	}

	Color aColor;
	bool aHasColor = false;         // the beam is drawn in its own colour (else in the backdrop's tint)
	if (mMode == 0 && m1a34 == 1)
	{
		aBeam = IMAGE_BEAM_SUPA;
		aHasColor = true;
	}
	else if (FirstPieceIsType(0xd))
	{
		aParticle = IMAGE_PARTICLE_FIREBALL;
		aGhost = IMAGE_BRICK_COL_NUKE_GHOST;
		aGhostDY = -0x22;
		aGhostDX = -5;
		aBeam = IMAGE_BEAM_COL_NUKE;
		aHasColor = true;
	}
	else if (FirstPieceIsType(0xb))
	{
		aParticle = IMAGE_PARTICLE_LIGHTNING;
		aGhost = IMAGE_BRICK_HORIZ_GHOST;
		aGhostDX = -0x32;
		aGhostDY = 0x15;
		aBeam = IMAGE_BEAM_ROW_NUKE;
		aHasColor = true;
	}
	else if (FirstPieceIsType(6))
	{
		aGhost = IMAGE_BRICK_RADIAL_GHOST;
		aParticle = IMAGE_PARTICLE_RADIAL;
		aGhostDX = -0x2e;
		aBeam = IMAGE_BEAM_RADIAL;
		aHasColor = true;
	}
	else if (aIsFuel)
	{
		aParticle = IMAGE_PARTICLE_RED;
		g->SetColorizeImages(true);
		aBeam = IMAGE_BEAM_WIDE;
		aHasColor = true;
	}
	else if (aIsFreeze)
	{
		g->SetColorizeImages(true);
		aParticle = IMAGE_PARTICLE_SNOWFLAKE;
		aBeam = IMAGE_BEAM_WIDE;
		aHasColor = true;
	}
	else if (mBackdropTint != Color(100, 100, 100))
	{
		g->SetColorizeImages(true);
		aBeam = IMAGE_BEAM_OPAQUE;
	}

	if (!aHasColor)
		g->SetColor(mBackdropTint);
	else if (aIsFreeze)
		g->SetColor(Color(0, 0xf0, 0xfa, 200));
	else if (aIsFuel)
	{
		int aPhase = aTick % 0x1fe;
		if (aPhase > 0xff)
			aPhase = 0x1fe - aPhase;
		g->SetColor(Color(0xff, aPhase, aPhase / 2, 200));
	}
	else
		g->SetColor(mBackdropTint);

	// the wide beams stand on the ship; the others hang from the top (and are not drawn on the warp screens)
	if (aIsFreeze || aIsFuel)
		g->DrawImage(aBeam, mShipX - aBeam->GetWidth() / 2 + 0x1a, 0x1ca - aBeam->mHeight);
	else if (!mShipInWarp)
		g->DrawImage(aBeam, mShipX, 0);

	if (mBackdropTint != Color(100, 100, 100) || aIsFreeze || aIsFuel)
		g->SetColorizeImages(false);

	if (aParticle != NULL)
	{
		// the sparks drift up the beam (one in four with the plain 2D drawing)
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aStep = is3D ? 1 : 4;
		for (int i = 0; i < 40; i++)
		{
			BlinkRec& aRec = mBeamSparks[i];
			int aX = aRec.mX;
			int aY = aRec.mCounter;
			int anAlpha = aY / 2;
			if (anAlpha > 0xff)
				anAlpha = 0xff;
			else if (anAlpha < 0)
				continue;
			if (i % aStep == 0)
			{
				g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
				DrawCel(g, aParticle, mShipX + aX, aY, (aRec.mAnim.mNumFrames == 1) ? 0 : aRec.mAnim.mFrame);
			}
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	if (aGhost != NULL)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		int aRow = FindBottomRow(mShipX, true);
		if (aRow == -1)
			aRow = 1;
		if (!aIsFreeze && !aIsFuel)
		{
			int aY = (aRow > 0) ? aRow * 0x19 + 9 : 4;
			g->DrawImage(aGhost, mShipX + aGhostDX, aY + aGhostDY);
		}
		else
		{
			// (the ghost blinks; only a nuke has one, so this part is never reached)
			int aPhase = aTick % 0x32;
			if (aPhase >= 0x19)
				aPhase = 0x32 - aPhase;
			if ((aTick & 1) == 0)
				aPhase--;
			if (aPhase < 0)
				aPhase = 0;
			g->DrawImage(aGhost, mShipX + aGhostDX, 0x140 - aPhase);
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}
