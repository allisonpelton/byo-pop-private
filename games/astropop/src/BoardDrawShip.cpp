// Board: the player's ship at the bottom of the screen, the pieces it holds and the lightning of the second game type
// (0x438210).

#include "Board.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "SexyAppBase.h"

using namespace Sexy;

// the tables of the ship's cels (static data at 0x59d650, indexed by the tilt of the ship and by the flame's frame; the
// original reads past the end of the first two into the next, so one array is kept)
static const int kShipCels[] = {3, 4, 4, 3, 1, 0, 0, 1, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0};
static const int kShipCelsLeft = 4;     // the table used when the ship does not turn to the right (0x59d660)
static const int kFlameCels = 8;        // the flame's cels (0x59d670)

// 0x438210
void Board::DrawShip(Graphics* g)
{
	int aX = m198c - 4;
	int aY = (int) (m1bf8 + 423.0f);
	Image* aShip = GetImageById(mMode + 0x173);
	Image* aFlame = GetImageById(mMode + 0x177);
	Image* aWarp = GetImageById(mMode + 0x17b);
	Image* aWarpJet = GetImageById(mMode + 0x17f);

	bool aShipVisible = (!m8eb && !mFlag8E8 && !m8ed && !m8ec && !m8ea) || (m8ea && m1954 == 2) || m8f0;
	if (aShipVisible)
	{
		int anOffset = 0;
		if (m8f0 || m917)
			anOffset = m199c;
		if (m8f0)
			anOffset += 0xd;

		int aCel;
		if (m194c == 0)
			aCel = kShipCels[kShipCelsLeft + m1954];
		else
			aCel = kShipCels[m1954];
		if (m194c == 2)
			aCel = 2;

		float aShipY = m8f0 ? m938 : (float) aY;
		int aShipX = anOffset + aX;
		DrawCel(g, aShip, aShipX, (int) aShipY, aCel);

		if (m1a34 == 1 && mShipShimmerHeight > 0)
		{
			// the charge of the weapon, over the ship
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, m8ea ? m1b38 : 0xff));
			Rect aSrc(aShip->mWidth / aShip->mNumCols * aCel + mShipShimmerX, mShipShimmerY, mShipShimmerWidth, mShipShimmerHeight);
			g->DrawImage(aShip, mShipShimmerX + anOffset + aX, (int) ((float) mShipShimmerY + (float) aShipY), aSrc);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(false);
		}

		// the flame
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		if (m8ea)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, m1b38));
		}
		float aFlameY = m8f0 ? m938 : (float) aY;
		DrawCel(g, aFlame, aShipX + 3, (int) (aFlameY + 36.0f), kShipCels[kFlameCels + m1950]);
		if (m8ea)
			g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	else
	{
		if (m8ec && !mFlag8E8)
		{
			// the ship leaves through the warp: its jet and flame flash
			DrawCel(g, aWarp, m199c + aX, aY - m19a0 - 0x15c, 0);
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, m19a4));
			DrawCel(g, aFlame, m199c + aX + 2, aY - m19a0 + 0x20, kShipCels[kFlameCels + m1950]);
			DrawCel(g, aWarpJet, m199c + aX + 2, aY - m19a0 - 0x15c, 0);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(false);
		}
		else if (!m8ed && !mFlag8E8)
		{
			bool aDraw = true;
			if (!m8ea)
			{
				if (m1954 >= 8)
					aDraw = false;
			}
			else if (m1954 >= 10 || m19b0 < 0)
				aDraw = false;
			if (aDraw)
			{
				if (m8ea)
					aY += 0x64;
				DrawCel(g, aWarp, aX, aY - m19a0 - 0x15c, m1954);
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0xff, 0xff, m19a8));
				DrawCel(g, aWarp, aX, aY - m19a0 - 0x15c, m1954);
				g->SetColor(Color(0xff, 0xff, 0xff, m19a4));
				DrawCel(g, aWarpJet, aX, aY - m19a0 - 0x15c, m1954);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColorizeImages(false);
			}
		}
	}

	if (!mTurbotPowerupAnim.mStopped)
	{
		// the turbot's power-up on the ship, and the lightning that jumps from it (3D only)
		g->SetColor(Color(0xff, 0xff, 0xff, m1aa0));
		g->SetColorizeImages(true);
		DrawCel(g, IMAGE_TURBOT_POWERUP, m198c - 0x26, aY - 0x25, (mTurbotPowerupAnim.mNumFrames == 1) ? 0 : mTurbotPowerupAnim.mFrame);
		g->SetColorizeImages(false);
		if (mApp->Is3DAccelerated())
		{
			for (size_t i = 0; i < mTurbotBeams.size(); i++)
			{
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				g->SetColorizeImages(true);
				g->SetColor(Color(0xff, 0xff, 0xff, mTurbotBeams[i].mLife));
				g->DrawImageRotatedF(IMAGE_TURBOT_LIGHTNING, (float) (aX - 0x68), (float) (aY + 0xd), (double) mTurbotBeams[i].mAngle,
					(float) 0x81, (float) 0x1e);
				g->SetColorizeImages(false);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			}
		}
	}

	// the pieces the ship holds, over each other; the pieces that are tools are drawn in one pass
	if (!m916)
	{
		bool aGlow = false;
		for (int aPass = 0; aPass < 2; aPass++)
		{
			bool aToolsOnly = false;
			if (!mHeldBricks.empty())
			{
				Brick* aFirst = mHeldBricks.front();
				int aKind = aFirst->mType;
				aToolsOnly = aFirst->mMutated != 1 &&
					(aKind == 6 || aKind == 8 || aKind == 10 || aKind == 0xe || aKind == 0xb || aKind == 0xd);
			}
			int aPieceY = aY;
			for (std::list<Brick*>::iterator aNode = mHeldBricks.begin(); aNode != mHeldBricks.end(); ++aNode)
			{
				aPieceY -= 0x19;
				Brick* aBrick = *aNode;
				if (!aBrick->mVisible)
					aBrick->DrawAt(g, aGlow, (float) (aX + 5), (float) aPieceY);
				else
					aBrick->Draw(g, aGlow);
			}
			if (aToolsOnly)
				return;
			aGlow = true;
		}
	}
}
