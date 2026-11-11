// Brick: drawing of the plain brick (the base class) and of the parts every brick class shares: the light, the
// biohazard mark of the mutating brick, the sprocket and turbot overlays, the death strip, the particles and sparks.
// The original works out its positions on the x87 stack at 24 bits of precision (UpdateFrames sets it): every sum is a float
// before it is cut to a whole pixel, as _ftol does.

#include "Brick.h"
#include "AstroPopApp.h"
#include "Res.h"

using namespace Sexy;

// 0x4092c0 on an id the game can have: 0x214 + type for the standard bricks
Image* Brick::TypeImage(int theType, bool theSpecial)
{
	int anId = (gApp->mFlag6fa ? 0x21a : 0x214) + (theSpecial ? 0xd : 0) + theType;
	if (anId < 0 || anId >= RESOURCE_ID_MAX)
		return NULL;
	return GetImageById(anId);
}

// 0x404360 slot 14
void Brick::Draw(Graphics* g, bool theGlow)
{
	if (mGone)
		return;

	Image* anImage;
	if (mMutated == 1)
		anImage = IMAGE_BRICK_MUTATOR;
	else
		anImage = TypeImage(mType, false);

	if (mState == 2 && mImage != NULL)
	{
		if ((mType >= 0 && mType <= 5) || mSpecial)
			anImage = TypeImage(mType, mSpecial);
		else
			anImage = mImage;
	}

	if (mRainbow && mRainbowIndex > -10)
	{
		if (mRainbowIndex >= 0 && (size_t) mRainbowIndex < mRainbowImages.size())
			anImage = mRainbowImages[mRainbowIndex];
	}

	if (mState == 3 || mState == 4 || mState == 6 || mState == 7)
		DrawParticles(g);
	if (mTrail)
		DrawSparks(g);

	int aShakeX = ShakeOffsetX();
	int aShakeY = ShakeOffsetY();

	if (!mVisible)
		return;

	if (mState == 3 || mState == 4 || mState == 6 || mState == 7)
	{
		if (mDeathAnim.mStopped)
			return;
		if (mState != 3 && mState != 4)
			return;
		if (mMutated == 1)
		{
			DrawDeathStrip(g);
			return;
		}
		g->DrawImageCel(GetImageById(mType + 0x230), (int) ((float) mX - 15.0f), (int) ((float) mY - 40.0f), CelOf(mDeathAnim));
		return;
	}

	if (theGlow)
	{
		DrawGlow(g);
		return;
	}

	DrawBody(g, anImage, aShakeX, aShakeY);
}

// the brick itself and what goes on it; the end of the standard Draw functions
// port: split out of the standard Draw functions (Brick::Draw 0x404360)
void Brick::DrawBody(Graphics* g, Image* theImage, int theShakeX, int theShakeY)
{
	if (theImage == NULL)
		return;
	int aCel = mRainbow ? 0 : CelOf(mAnim);
	g->DrawImageCel(theImage, (int) ((float) mX + theShakeX), (int) ((float) mY + mOffsetY + theShakeY), aCel);
	DrawLightPass(g, theImage, aCel, theShakeX, theShakeY);
	DrawOverlays(g, theShakeX, theShakeY);
}

// biohazard, sprocket and turbot
// port: split out of the standard Draw functions (Brick::Draw 0x404360)
void Brick::DrawOverlays(Graphics* g, int theShakeX, int theShakeY)
{
	if (mMutated == 0)
		DrawBiohazard(g, (float) theShakeX + mX, (float) theShakeY + mY);
	if (mSprocketOn)
		DrawSprocket(g);
	if (mTurbotOn)
		DrawTurbot(g);
}

// the light of 0x404360: the brick drawn again, additive and tinted grey
void Brick::DrawLightPass(Graphics* g, Image* theImage, int theCel, int theShakeX, int theShakeY)
{
	if (mLight < 0 && !mFlag185 && !mFlag190 && !mFlag193)
		return;

	int aLight = mLight;
	if (mFlag190 || mFlag193)
		aLight = mLightValue;
	else if (mFlag185)
		aLight = mFade;

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(aLight, aLight, aLight));
	g->SetColorizeImages(true);
	g->DrawImageCel(theImage, (int) ((float) mX + theShakeX), (int) ((float) mY + mOffsetY + theShakeY), theCel);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
}

// 0x4030c0 slot 17: the brick drawn at a place of the caller's (the preview of the next brick, the stats screen)
void Brick::DrawAt(Graphics* g, bool theGlow, float theX, float theY)
{
	Image* anImage;
	if ((mType < 0 || mType > 5) && !mSpecial)
		anImage = mImage;
	else
		anImage = TypeImage(mType, mSpecial);

	bool is3D = gApp->Is3DAccelerated();
	int aCellWidth = anImage->GetWidth() / anImage->mNumCols;

	if (!theGlow)
	{
		int aCel = (mAnim.mNumFrames == 1) ? 0 : mAnim.mFrame;
		if (!is3D)
			g->DrawImageCel(anImage, (int) theX, (int) theY, aCel);
		else
			g->DrawImageF(anImage, theX, theY, Rect(aCel * aCellWidth, 0, aCellWidth, anImage->GetHeight()));
		return;
	}

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	g->SetColor(mColor);
	int aX = (int) ((float) theX - 1.0f);
	g->DrawImage(IMAGE_BRICK_GLOW, aX, (int) ((float) theY - 1.0f));
	g->SetColorizeImages(false);
	if (!mGlowAnim.mStopped)
	{
		if (mFlag194)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, mGlowAlpha));
		}
		int aCel = (mGlowAnim.mNumFrames == 1) ? 0 : mGlowAnim.mFrame;
		g->DrawImageCel(IMAGE_BRICK_BONUS_GLOW, aX, (int) ((float) theY + mOffsetY + 1.0f), aCel);
		g->SetColorizeImages(false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x402450 slot 13: the glow behind the brick (drawn in an additive pass before the bricks)
void Brick::DrawGlow(Graphics* g)
{
	int aShakeX = ShakeOffsetX();
	int aShakeY = ShakeOffsetY();

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->DrawImage(mGlowImage, (int) ((float) mX - 1.0f + aShakeX), (int) ((float) mY - 1.0f + mOffsetY + aShakeY));
	if (!mGlowAnim.mStopped)
	{
		if (mFlag194)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, mGlowAlpha));
		}
		int aCel = (mGlowAnim.mNumFrames == 1) ? 0 : mGlowAnim.mFrame;
		g->DrawImageCel(IMAGE_BRICK_BONUS_GLOW, (int) ((float) mX - 1.0f + aShakeX), (int) ((float) mOffsetY + mY + aShakeY + 1.0f), aCel);
		g->SetColorizeImages(false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x402090: the mutating brick's biohazard mark, pulsing in alpha (the shake is added here and by the caller)
void Brick::DrawBiohazard(Graphics* g, float theX, float theY)
{
	int aShakeX = ShakeOffsetX();
	int aShakeY = ShakeOffsetY();

	int aBrickWidth = IMAGE_BRICK_BLUE->GetWidth() / IMAGE_BRICK_BLUE->mNumCols;
	int aBrickHeight = IMAGE_BRICK_BLUE->GetHeight();
	int aMarkWidth = IMAGE_BIOHAZARD->GetWidth() / IMAGE_BIOHAZARD->mNumCols;
	int aMarkHeight = IMAGE_BIOHAZARD->GetHeight();

	float aX = (float) (aBrickWidth >> 1) + aShakeX + theX - (aMarkWidth >> 1);
	float aY = (float) ((aBrickHeight >> 1) - (aMarkHeight >> 1)) + mOffsetY + aShakeY + theY;

	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, mPulse));
	g->DrawImage(IMAGE_BIOHAZARD, (int) aX, (int) aY);
	g->SetColorizeImages(false);
}

// 0x402190 (sprocket) and 0x4022f0 (turbot): the overlay on the brick when a power-up lights it, drawn at its
// alpha and once more additive, grey, while its light is on
static void DrawOverlay(Graphics* g, Image* theOverlay, const Brick* theBrick, int theAlpha, int theLight, int theCel,
	int theShakeX, int theShakeY)
{
	int aBrickWidth = IMAGE_BRICK_BLUE->GetWidth() / IMAGE_BRICK_BLUE->mNumCols;
	int aBrickHeight = IMAGE_BRICK_BLUE->GetHeight();
	int anOverlayWidth = theOverlay->GetWidth() / theOverlay->mNumCols;
	int anOverlayHeight = theOverlay->GetHeight();

	float aX = (float) (aBrickWidth >> 1) + theBrick->mX + theShakeX - (anOverlayWidth >> 1);
	float aY = (float) ((aBrickHeight >> 1) - (anOverlayHeight >> 1)) + theBrick->mOffsetY + theBrick->mY + theShakeY;

	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, theAlpha));
	g->DrawImageCel(theOverlay, (int) aX, (int) aY, theCel);
	if (theLight > 0)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(theLight, theLight, theLight));
		g->DrawImageCel(theOverlay, (int) aX, (int) aY, theCel);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	g->SetColorizeImages(false);
}

void Brick::DrawSprocket(Graphics* g)
{
	DrawOverlay(g, IMAGE_SPROCKET_SUPA_OVERLAY, this, mSprocketAlpha, mSprocketLight, mSprocketCel, ShakeOffsetX(), ShakeOffsetY());
}

void Brick::DrawTurbot(Graphics* g)
{
	DrawOverlay(g, IMAGE_TURBOT_SUPA_OVERLAY, this, mTurbotAlpha, mTurbotLight, mTurbotCel, ShakeOffsetX(), ShakeOffsetY());
}

// 0x4032e0: the strip of the mutator's death
void Brick::DrawDeathStrip(Graphics* g)
{
	Image* anImage = IMAGE_BRICK_MUTATOR_DEATH;
	int aCel = (mDeathAnim.mNumFrames == 1) ? 0 : mDeathAnim.mFrame;
	if (!gApp->Is3DAccelerated())
	{
		g->DrawImageCel(anImage, (int) mX, (int) mY, aCel);
		return;
	}
	int aCellWidth = anImage->GetWidth() / anImage->mNumCols;
	g->DrawImageF(anImage, mX, mY, Rect(aCel * aCellWidth, 0, aCellWidth, anImage->GetHeight()));
}

// 0x404260: the fall particles, every fourth one unless the game draws in 3D
void Brick::DrawParticles(Graphics* g)
{
	bool is3D = gApp->Is3DAccelerated();
	if (is3D)
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (int i = 0; i < mNumParticles; i++)
	{
		if (i % (is3D ? 1 : 4) == 0)
			mParticles[i]->Draw(g);
	}
	if (is3D)
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x4042f0: the sparks of a brick that is up in the field, drawn additive in every mode
void Brick::DrawSparks(Graphics* g)
{
	if (mY >= 384.0f)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		for (size_t i = 0; i < mSparks.size(); i++)
			mSparks[i]->Draw(g);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}
