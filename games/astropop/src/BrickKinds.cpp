// The four classes made from the plain brick: KillAllBrick, SpecialBrick, FuelCell and PowerBrick. Functions that
// the original runs on the listener part (OnSettle, OnDie) have the listener's offsets in the decompilation, 0xc
// below the object's.

#include "BrickKinds.h"
#include "Board.h"
#include "AstroPopApp.h"
#include "Res.h"

using namespace Sexy;

Image* Brick::SafeImage(int theId)
{
	if (theId < 0 || theId >= RESOURCE_ID_MAX)
		return NULL;
	return GetImageById(theId);
}

static bool IsDeathState(int theState)
{
	return theState == 3 || theState == 4 || theState == 6 || theState == 7;
}

//
// KillAllBrick
//

// 0x4073f0
KillAllBrick::KillAllBrick(Board* theBoard, EventBus* theBus, int theColumn) :
	Brick(theBoard, theBus, theColumn)
{
	mSpecial = true;
	mField234 = 0xff;
	mDeathAnim.SetNumFrames(IMAGE_BRICK_KILLALL_RED_DEATH->mNumCols);
	mNum1B0 = 7;
	mField238 = false;
}

// 0x404850
void KillAllBrick::SetType(int theType)
{
	Brick::SetType(theType);
	mImage = GetImageById(mType + 0x221);
}

// 0x404c60 (listener)
void KillAllBrick::OnDie()
{
	Brick::OnDie();
	if (!mBoard->mFlag8E9 && !mBoard->mFlag8E8 && mMutated != 1)
		PlayBoardSound(SOUND_KILLALL_EXPLODE);
}

// 0x402750: the death strip and the effect strip have both played
void KillAllBrick::UpdateState3()
{
	mDeathAnim.Update();
	mFxAnim.Update();
	if (mDeathAnim.mStopped && mFxAnim.mStopped)
	{
		SetState(4);
		if (mMutated != 1)
			Fire(0x1f, mType);
		else
			Fire(6);
	}
}

// 0x404880
void KillAllBrick::Draw(Graphics* g, bool theGlow)
{
	Image* anImage;
	if (mMutated == 1)
		anImage = IMAGE_BRICK_MUTATOR;
	else
		anImage = SafeImage((gApp->mFlag6fa ? 0x227 : 0x221) + mType);

	if (mState == 2 && mImage != NULL)
		anImage = TypeImage(mType, mSpecial);

	if (mRainbow && mRainbowIndex > -10 && mRainbowIndex >= 0 && (size_t) mRainbowIndex < mRainbowImages.size())
		anImage = mRainbowImages[mRainbowIndex];

	if (IsDeathState(mState))
		DrawParticles(g);

	int aShakeX = ShakeOffsetX();
	int aShakeY = ShakeOffsetY();

	if (!mVisible)
		return;
	if (mTrail)
		DrawSparks(g);

	if (IsDeathState(mState))
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
		Image* aDeathImage = GetImageById(mType + 0x237);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImageCel(aDeathImage, (int) ((double) mX - 54.0), (int) ((double) mY - 73.0), CelOf(mDeathAnim));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		return;
	}

	if (theGlow)
	{
		DrawGlow(g);
		return;
	}
	DrawBody(g, anImage, aShakeX, aShakeY);
}

//
// SpecialBrick
//

// 0x407480
SpecialBrick::SpecialBrick(Board* theBoard, EventBus* theBus, int theColumn) :
	Brick(theBoard, theBus, theColumn)
{
	mBlastArg = 100;
	mStage = 100;
}

// 0x4063e0 slot 26
void SpecialBrick::StartMutatorCountdown()
{
	Brick::StartMutatorCountdown();
	mStage = 1;
}

// 0x403420 (listener): a bomb that has landed starts its activated strip
void SpecialBrick::OnSettle()
{
	Brick::OnSettle();
	Image* anImage = IMAGE_BRICK_ROW_NUKE_ACTIVATED;
	if (mType == 0xb || (anImage = IMAGE_BRICK_RADIAL_ACTIVATED, mType == 6))
	{
		mImage = anImage;
		mAnim.Reset();
		mAnim.SetNumFrames(mImage->mNumCols);
		mAnim.SetDelay(0xb);
		mAnim.SetStopped(false);
		mAnim.mHoldLast = false;
		mAnim.mPingPong = false;
	}
}

// 0x405140 (listener)
void SpecialBrick::OnDie()
{
	StopListeningToAll();
	Listen(4);
	for (int i = 0; i < mNumParticles; i++)
	{
		mParticles[i]->SetX((float) (AstroRand() % 0xf) + mX);
		mParticles[i]->SetY((float) (AstroRand() % 0xf) + mY);
		if (mType == 9)
			mParticles[i]->SetFadeBlue();
	}
	if (mMutated == 1)
	{
		mDeathAnim.SetNumFrames(IMAGE_BRICK_MUTATOR_DEATH->mNumCols);
		mDeathAnim.mPingPong = false;
		mDeathAnim.mHoldLast = true;
		gApp->PlaySample(SOUND_MUTATOR_DIE);
	}
	if (mType != 0xb)
		mDeathAnim.SetDelay(5);
	mDeathAnim.SetStopped(false);
	mShake = false;
	SetState(3);

	if (!mBoard->mFlag8E9 && !mBoard->mFlag8E8 && mMutated != 1)
	{
		if (mType == 6)
			PlayBoardSound(SOUND_STAR_EXPLODE);
		else if (mType == 0xb)
			PlayBoardSound(SOUND_ROW_NUKE);
	}

	if (!mVisible)
	{
		if (mMutated != 1)
		{
			mFlag17B = true;
			return;
		}
	}
	else if (mMutated != 1)
	{
		if (mType == 0xb)
			mFxAnim.SetStopped(false);
		mFlag17B = false;
		Fire(0x26, mBlastArg);
		return;
	}
	Fire(0x1a);
}

// 0x4027b0: a hit takes one off the stage; the first hit of a whole steel brick plays its sound (if asked to)
void SpecialBrick::Crack(bool thePlaySound)
{
	if (mType == 9 && mStage == 2 && thePlaySound)
		PlayBoardSound(SOUND_STEEL);
	mStage--;
	if (mStage > 0)
		mDeathAnim.SetStopped(true);
}

// 0x4074c0 slot 16
void SpecialBrick::SetType(int theType)
{
	mType = theType;
	mStage = 1;
	switch (theType)
	{
	case 6:			// radial bomb
		mColor = Color(0xff, 0xaa, 0);
		mColor2 = Color(0xff, 0xaa, 0, 0x80);
		mBlastArg = 1;
		mDeathAnim.SetNumFrames(IMAGE_BRICK_RADIAL_DEATH->mNumCols);
		mImage = IMAGE_BRICK_RADIAL;
		mNumFrames = IMAGE_BRICK_RADIAL->mNumCols;
		mAnim.SetNumFrames(IMAGE_BRICK_RADIAL->mNumCols);
		mAnim.SetDelay(10);
		mAnim.SetStopped(false);
		break;
	case 7:			// stone
		mColor = Color(0x96, 0x96, 0x96);
		mColor2 = Color(0xff, 0xff, 0xff, 0x80);
		mBlastArg = 0;
		mNumFrames = IMAGE_BRICK_STONE->mNumCols;
		mDeathAnim.SetNumFrames(IMAGE_BRICK_ROCK_DEATH->mNumCols);
		mImage = NULL;
		break;
	case 9:			// steel
		mNumParticles = 0x14;
		mColor = Color(0x96, 0x96, 0x96);
		mColor2 = Color(0xff, 0xff, 0xff, 0x80);
		mBlastArg = -1;
		mDeathAnim.SetNumFrames(IMAGE_BRICK_STEEL_DEATH->mNumCols);
		mNumFrames = IMAGE_BRICK_STEEL_1->mNumCols;
		mStage = 2;
		mImage = NULL;
		break;
	case 0xb:		// row nuke
		mColor = Color(0xff, 0xff, 0);
		mColor2 = Color(0xff, 0xff, 0, 0x80);
		mBlastArg = 0x13;
		mDeathAnim.SetNumFrames(0);
		mImage = IMAGE_BRICK_ROW_NUKE;
		mNumFrames = IMAGE_BRICK_ROW_NUKE->mNumCols;
		mAnim.SetNumFrames(IMAGE_BRICK_ROW_NUKE->mNumCols);
		mAnim.SetDelay(10);
		mAnim.SetStopped(false);
		mDeathAnim.SetNumFrames(IMAGE_BRICK_ROW_NUKE_DEATH->mNumCols);
		mDeathAnim.SetDelay(7);
		mDeathAnim.SetStopped(true);
		mFxAnim.SetNumFrames(IMAGE_BRICK_ROW_NUKE_BLAST->mNumRows);
		mFxAnim.SetDelay(8);
		mFxAnim.SetStopped(true);
		mFxAnim.mHoldLast = true;
		break;
	default:
		break;
	}

	if (mType == 9)
	{
		// the steel brick's shards: every particle is made afresh, thrown up, the later ones falling in turn
		mParticles.resize(mNumParticles + 0x23, NULL);
		int aCount = mNumParticles;
		int aSum = (int) (AstroRand() % 10) + aCount;
		float aStep = 10.0f / (float) aSum;
		float aVX = -3.0f;
		int aDelay = 0;
		for (int i = 0; i < aCount + 0x23; i++)
		{
			float aSpeed = (float) (AstroRand() % 1000) * 0.001f - 40.0f;
			if (i <= aCount)
				aVX = aVX + aStep;
			float aVY = -5.0f - (float) (AstroRand() % 0x5dc) * 0.001f;
			float aGravity = (float) (AstroRand() % 0x259) * 0.001f + 0.7f;
			float anAngle = (float) (AstroRand() % 0x50) + 10.0f;
			delete mParticles[i];
			int aMiddle = (aCount + 0x23) / 2 - 2;
			if (aMiddle <= i && i <= aMiddle)
				anAngle = 90.0f;
			float aHeight = (float) (AstroRand() % 100 + 0x19);
			mParticles[i] = new Particle(aSpeed, aVX, aVY, (int) anAngle, aGravity, aHeight, gSinTable, gCosTable);
			int aParticleDelay = aDelay;
			if ((aCount + 0x23) / 2 <= i)
				aParticleDelay = ((aCount - i) + 0x23) * 5;
			mParticles[i]->mDelay = aParticleDelay;
			mParticles[i]->SetColors(Color(0xff, 0xff, 0xff, 0xff), Color(0xff, 0, 0, 0xff));
			aDelay += 5;
		}
	}

	for (int i = 0; i < mNumParticles + 0x23; i++)
	{
		if (mType == 0xb)
		{
			mParticles[i]->SetImage(IMAGE_PARTICLE_LIGHTNING, 5);
			mParticles[i]->mAdditive = true;
		}
		else if (mType == 6 || mType == 9)
		{
			mParticles[i]->SetImage(mType == 6 ? IMAGE_PARTICLE_RADIAL : IMAGE_PARTICLE_FIREBALL, 5);
			mParticles[i]->mAdditive = true;
		}
		else
			mParticles[i]->SetImage(IMAGE_PARTICLE_WHITE, 5);
	}
	RebuildGlow();
}

// 0x404cb0
void SpecialBrick::Draw(Graphics* g, bool theGlow)
{
	if (IsDeathState(mState))
		DrawParticles(g);

	int aShakeX = ShakeOffsetX();
	int aShakeY = ShakeOffsetY();

	if (!mVisible)
		return;
	if (mTrail)
		DrawSparks(g);

	// the image of the brick, the image of its death and where that is drawn, by type
	Image* aTypeImage = NULL;
	Image* aDeathImage = NULL;
	int aDeathX = 0;
	int aDeathY = 0;
	switch (mType)
	{
	case 6:
		aTypeImage = IMAGE_BRICK_RADIAL;
		aDeathImage = IMAGE_BRICK_RADIAL_DEATH;
		aDeathX = (int) ((double) mX - 53.0);
		aDeathY = (int) ((double) mY - 63.0);
		break;
	case 7:
		aTypeImage = IMAGE_BRICK_STONE;
		aDeathImage = IMAGE_BRICK_ROCK_DEATH;
		aDeathX = (int) ((double) mX - 12.0);
		aDeathY = (int) ((double) mY - 23.0);
		break;
	case 9:
		aTypeImage = (mStage == 2) ? IMAGE_BRICK_STEEL_1 : IMAGE_BRICK_STEEL_2;
		aDeathImage = IMAGE_BRICK_STEEL_DEATH;
		aDeathX = (int) mX;
		aDeathY = (int) mY;
		break;
	case 0xb:
		aTypeImage = IMAGE_BRICK_ROW_NUKE;
		aDeathImage = IMAGE_BRICK_ROW_NUKE_DEATH;
		aDeathX = (int) ((double) mX - 54.0);
		aDeathY = (int) ((double) mY - 73.0);
		break;
	default:
		break;
	}

	Image* anImage;
	if (mState == 2 && mImage != NULL)
		anImage = mImage;
	else if (mMutated == 1)
		anImage = IMAGE_BRICK_MUTATOR;
	else
		anImage = aTypeImage;
	if (mRainbow && mRainbowIndex > -10 && mRainbowIndex >= 0 && (size_t) mRainbowIndex < mRainbowImages.size())
		anImage = mRainbowImages[mRainbowIndex];

	if (IsDeathState(mState))
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
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->DrawImageCel(aDeathImage, aDeathX, aDeathY, CelOf(mDeathAnim));
		if (mType == 0xb && !mFxAnim.mStopped)
			g->DrawImageCel(IMAGE_BRICK_ROW_NUKE_BLAST, 0, (int) ((double) mY - 17.0), CelOf(mFxAnim));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		return;
	}

	if (theGlow)
	{
		if ((mType == 9 || mType == 7) && mMutated != 1)
			return;
		DrawGlow(g);
		return;
	}
	DrawBody(g, anImage, aShakeX, aShakeY);
}

//
// FuelCell
//

// 0x407bd0
FuelCell::FuelCell(Board* theBoard, EventBus* theBus) :
	Brick(theBoard, theBus, -1),
	mTint(0, 0, 0)
{
	mColumn = -1;
	mRow = -1;
	mAnim.SetDelay(10);
	mAnim.SetNumFrames(IMAGE_FUELCELL->mNumCols);
	mAnim.SetStopped(false);
	mColorStep = 2;
	mFlag240 = false;
	mDeathAnim.SetDelay(4);
	mDeathAnim.SetNumFrames(IMAGE_FUELCELL_DEATH->mNumCols);
	mDeathAnim.mHoldLast = true;
	mColorPhase = 0;
	mTint = Color(0xff, 0, 0);
}

// 0x402a90 slot 16
void FuelCell::SetType(int theType)
{
	mType = theType;
	mColor = Color(0xb8, 0xb8, 0);
	mColor2 = Color(0xb8, 0xb8, 0, 0x80);
	mImage = IMAGE_BRICK_BONUS_ACTIVATED;
	mNumFrames = IMAGE_FUELCELL->mNumCols;
	RebuildGlow();
}

// 0x403860 (listener)
void FuelCell::OnSettle()
{
	Brick::OnSettle();
	mColorStep = 4;
	mAnim.Reset();
	mAnim.SetNumFrames(mImage->mNumCols);
	mAnim.SetDelay(0xb);
	mAnim.SetStopped(false);
	mAnim.mHoldLast = false;
	mAnim.mPingPong = false;
}

// 0x4056e0 (listener)
void FuelCell::OnDie()
{
	Brick::OnDie();
	if (mMutated != 1)
		PlayBoardSound(SOUND_BONUS_EXPLODE);
	Fire(0x1c);
}

// 0x4036f0 slot 15: the tint walks round the colour wheel, then the brick updates
bool FuelCell::Update()
{
	switch (mColorPhase)
	{
	case 0:
		mTint.mGreen += mColorStep;
		if (mTint.mGreen > 0xff)
		{
			mTint.mGreen = 0xff;
			mColorPhase = 1;
		}
		break;
	case 1:
		mTint.mRed -= mColorStep;
		if (mTint.mRed < 1)
		{
			mTint.mRed = 0;
			mColorPhase = 2;
		}
		break;
	case 2:
		mTint.mBlue += mColorStep;
		if (mTint.mBlue > 0xfe)
		{
			mTint.mBlue = 0xff;
			mColorPhase = 3;
		}
		break;
	case 3:
		mTint.mGreen -= mColorStep;
		if (mTint.mGreen < 1)
		{
			mTint.mGreen = 0;
			mColorPhase = 4;
		}
		break;
	case 4:
		mTint.mRed += mColorStep;
		if (mTint.mRed > 0xfe)
		{
			mTint.mRed = 0xff;
			mColorPhase = 5;
		}
		break;
	case 5:
		mTint.mBlue -= mColorStep;
		if (mTint.mBlue < 1)
		{
			mTint.mBlue = 0;
			mColorPhase = 0;
		}
		break;
	}
	return Brick::Update();
}

// 0x402b30 slot 17
void FuelCell::DrawAt(Graphics* g, bool theGlow, float theX, float theY)
{
	if (!theGlow)
	{
		g->DrawImageCel(mImage, (int) theX, (int) theY, (mAnim.mNumFrames == 1) ? 0 : mAnim.mFrame);
		return;
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(mTint);
	g->SetColorizeImages(true);
	g->DrawImage(IMAGE_BRICK_GLOW, (int) theX, (int) theY);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
}

void FuelCell::SetPlace(int theColumn, int theRow)
{
	mColumn = theColumn;
	mRow = theRow;
}

void FuelCell::SetFlag240()
{
	mFlag240 = true;
}

int FuelCell::GetColumn()
{
	return mColumn;
}

int FuelCell::GetRow()
{
	return mRow;
}

bool FuelCell::GetFlag240()
{
	return mFlag240;
}

// 0x405340
void FuelCell::Draw(Graphics* g, bool theGlow)
{
	Image* anImage = (mMutated == 1) ? IMAGE_BRICK_MUTATOR : IMAGE_FUELCELL;
	if (mState == 2 && mImage != NULL)
		anImage = mImage;
	if (mRainbow && mRainbowIndex > -10 && mRainbowIndex >= 0 && (size_t) mRainbowIndex < mRainbowImages.size())
		anImage = mRainbowImages[mRainbowIndex];

	int aShakeX = ShakeOffsetX();
	int aShakeY = ShakeOffsetY();

	if (!mVisible)
		return;
	if (mTrail)
		DrawSparks(g);

	if (IsDeathState(mState))
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
		g->DrawImageCel(IMAGE_FUELCELL_DEATH, (int) ((double) mX - 53.0), (int) ((double) mY - 62.0), CelOf(mDeathAnim));
		return;
	}

	if (theGlow)
	{
		DrawGlow(g);
		return;
	}

	int aCel = mRainbow ? 0 : CelOf(mAnim);
	int aX = (int) ((double) mX + aShakeX);
	int aY = (int) ((double) mY + mOffsetY + aShakeY);
	g->DrawImageCel(anImage, aX, aY, aCel);

	if (mMutated != 1)
	{
		// the star over the cell, in the tint
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		if (mColorPhase != 6)
		{
			g->SetColorizeImages(true);
			g->SetColor(mTint);
		}
		if (!mRainbow)
			g->DrawImage(IMAGE_BRICK_BONUS_FX, aX, aY);
		if (mColorPhase != 6)
			g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	DrawLightPass(g, anImage, CelOf(mAnim), aShakeX, aShakeY);
	DrawOverlays(g, aShakeX, aShakeY);
}

//
// PowerBrick
//

// 0x407d80
PowerBrick::PowerBrick(Board* theBoard, EventBus* theBus, int theColumn) :
	Brick(theBoard, theBus, theColumn)
{
	mBoosterTime0 = 1000;
	mBoosterTime = 1000;
	mDeathAnim.SetNumFrames(1);
	mActive = false;
	mNukeY = 0;
	mBoosterY = 0;
	mBoosterOff = false;
	mBoosterTick = 100;
}

// 0x4039c0 (listener) event 0
void PowerBrick::OnSpawn()
{
	Brick::OnSpawn();
	if (mType == 0xe && !IsListening(0x2a))
		Listen(0x2a);
}

// 0x403930 (listener)
void PowerBrick::OnSettle()
{
	Brick::OnSettle();
	int aFrames = 1;
	if (mType == 10)
		aFrames = IMAGE_BRICK_FREEZE_ACTIVATED->mNumCols;
	else if (mType == 0xd)
		aFrames = IMAGE_BRICK_COL_NUKE_ACTIVATED->mNumCols;
	else if (mType == 0xe)
		aFrames = IMAGE_BRICK_BOOSTER_ACTIVATED->mNumCols;
	mAnim.Reset();
	mAnim.SetNumFrames(aFrames);
	mAnim.SetDelay(0xb);
	mAnim.SetStopped(false);
	mAnim.mHoldLast = false;
	mAnim.mPingPong = false;
}

// 0x402d60 (listener) event 0x2a: the booster pauses and goes on
void PowerBrick::OnEvent2A(int theArg)
{
	if (mType == 0xe)
		mBoosterOff = theArg != 0;
}

// 0x402be0 slot 2: a booster that is up in the field counts its time down and whooshes
void PowerBrick::UpdateState0()
{
	Brick::UpdateState0();
	if (!(mY < 29.0f) && mType == 0xe && !mBoosterOff)
	{
		mBoosterTick--;
		if (mBoosterTick < 1)
		{
			gApp->PlaySampleAt(SOUND_HYPER_BRICK, ((int) mX - 0xa6) / 44);
			if (mBoosterTime <= 200)
				mBoosterTick = 12;
			else if (mBoosterTime <= 400)
				mBoosterTick = 25;
			else
				mBoosterTick = ((((mBoosterTime > 600) ? 1 : 0) - 1) & -50) + 100;
		}
		mBoosterTime--;
		if (mBoosterTime < 1)
		{
			mBoosterTime = 0;
			Fire(0x13);
			return;
		}
		int aDelay = mBoosterTime / 100;
		if (aDelay < mAnim.mDelay && aDelay >= 1)
			mAnim.SetDelay(aDelay);
	}
}

// 0x4038c0 slot 15
bool PowerBrick::Update()
{
	if (!mActive)
		return Brick::Update();
	if (mState == 7)
		return false;
	mDeathAnim.Update();
	mFxAnim.Update();
	if (mDeathAnim.mStopped && mFxAnim.mStopped)
	{
		SetState(7);
		mActive = false;
		Fire(0xe);
	}
	return true;
}

// 0x407e80 slot 16
void PowerBrick::SetType(int theType)
{
	mType = theType;
	if (theType == 10)
	{
		mColor = Color(0, 0xf0, 0xff);
		mColor2 = Color(0, 0xf0, 0xff, 200);
		mImage = IMAGE_BRICK_FREEZE_ACTIVATED;
		mNumFrames = IMAGE_BRICK_FREEZE->mNumCols;
		mDeathAnim.SetNumFrames(IMAGE_BRICK_FREEZE_DEATH->mNumCols);
		mAnim.SetNumFrames(IMAGE_BRICK_FREEZE->mNumCols);
		mAnim.SetDelay(10);
		mAnim.SetStopped(false);
		for (int i = 0; i < mNumParticles + 0x23; i++)
		{
			mParticles[i]->SetImage(IMAGE_PARTICLE_SNOWFLAKE, 4);
			mParticles[i]->mAdditive = true;
		}
	}
	else if (theType == 0xd)
	{
		mColor = Color(0xff, 0, 0);
		mColor2 = Color(0xff, 0, 0, 0x80);
		mImage = IMAGE_BRICK_COL_NUKE_ACTIVATED;
		mNumFrames = IMAGE_BRICK_COL_NUKE->mNumCols;
		mDeathAnim.SetNumFrames(1);
		mAnim.SetNumFrames(IMAGE_BRICK_COL_NUKE->mNumCols);
		mAnim.SetDelay(10);
		mAnim.SetStopped(false);
		mFxAnim.SetNumFrames(IMAGE_BRICK_COL_NUKE_FX->mNumCols);
		mFxAnim.SetDelay(5);
		mFxAnim.SetStopped(true);
		mFxAnim.mHoldLast = true;
		mFxAnim.mPingPong = false;
		for (int i = 0; i < mNumParticles + 0x23; i++)
		{
			mParticles[i]->SetImage(IMAGE_PARTICLE_FIREBALL, 4);
			mParticles[i]->mAdditive = true;
			mParticles[i]->mHidden = true;
		}
	}
	else if (theType == 0xe)
	{
		if (!IsListening(0x2a))
			Listen(0x2a);
		mColor = Color(0, 0xf0, 0xff);
		mColor2 = Color(0, 0xf0, 0xff, 200);
		mImage = IMAGE_BRICK_BOOSTER_ACTIVATED;
		mNumFrames = IMAGE_BRICK_BOOSTER->mNumCols;
		mDeathAnim.SetNumFrames(IMAGE_BRICK_BOOSTER_DEATH->mNumCols);
		mAnim.SetNumFrames(IMAGE_BRICK_BOOSTER->mNumCols);
		mAnim.SetDelay(10);
		mAnim.mPingPong = true;
		mAnim.SetStopped(false);
		mAnim.mHoldLast = false;
		mNumParticles = 0x46;
		for (size_t i = 0; i < mParticles.size(); i++)
			delete mParticles[i];
		mParticles.clear();
		mParticles.resize(mNumParticles + 0x23, NULL);
		mBoosterTime0 = mBoosterTime0 + (mBoard->mField193C / 10) * 100;
		mBoosterTime = mBoosterTime0;
		for (int i = 0; i < mNumParticles + 0x23; i++)
		{
			mParticles[i] = new Particle(0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, gSinTable, gCosTable);
			mParticles[i]->SetImage(IMAGE_PARTICLE_WHITE, 4);
			mParticles[i]->mAdditive = true;
		}
	}
	mDeathAnim.SetDelay(5);
	mDeathAnim.mHoldLast = true;
	mDeathAnim.mPingPong = false;
	RebuildGlow();
}

// 0x408340 (listener)
void PowerBrick::OnDie()
{
	Brick::OnDie();
	if (mMutated == 1)
		return;

	if (mType == 10)
	{
		mFxAnim.SetDelay(1);
		mFxAnim.SetStopped(true);
		Fire(0x1c);
	}
	else if (mType == 0xe)
	{
		mFxAnim.SetDelay(1);
		mFxAnim.SetStopped(true);
		Fire(0x12);

		// the booster's flames: every particle is made afresh, a column rising to the row it flies to and a shower after it
		for (size_t i = 0; i < mParticles.size(); i++)
			delete mParticles[i];
		mParticles.clear();
		mParticles.resize(mNumParticles + 0x23, NULL);

		int aXStep = (int) (((double) 600.0f - (double) mX) / (double) (mNumParticles - 0x23));
		AstroRand();
		float aYStep = (float) (((double) mBoosterY - (double) mY) / (double) (mNumParticles - 0xf));
		int aXAcc = 0;
		int aDelayAcc = 0;
		int aLastDelay = 0;
		for (int i = 0; i < mNumParticles + 0x23; i++)
		{
			if (i < mNumParticles - 0x23)
			{
				float aGravity = (float) (AstroRand() % 0x1f5) * 0.001f + 0.2f;
				int aHeight = (int) ((double) 480.0f - (double) mY);
				mParticles[i] = new Particle(0.0f, 0.0f, 0.0f, 0xb4, aGravity, (float) aHeight, gSinTable, gCosTable);
				int aRandom = (int) (AstroRand() % 10) - 5;
				mParticles[i]->SetY((float) ((double) i * aYStep + aRandom + mY));
				mParticles[i]->SetX((float) aXAcc + mX);
				aLastDelay = aDelayAcc;
				mParticles[i]->mDelay = aDelayAcc;
			}
			else
			{
				float aSpeed = (float) (AstroRand() % 100) * 0.001f - 0.5f;
				float aVX = (float) (AstroRand() % 300) * 0.01f - 4.0f;
				float aVY = -0.5f - (float) (AstroRand() % 0x5dc) * 0.001f;
				float aGravity = (float) (AstroRand() % 0x12d) * 0.001f + 0.35f;
				float anAngle = (float) (AstroRand() % 0x32) + 30.0f;
				int aHeight = (int) ((double) 480.0f - (double) mY);
				mParticles[i] = new Particle(aSpeed, aVX, aVY, (int) anAngle, aGravity, (float) aHeight, gSinTable, gCosTable);
				AstroRand();
				mParticles[i]->SetY((float) mBoosterY);
				mParticles[i]->SetX(600.0f);
				mParticles[i]->mDelay = aLastDelay;
			}
			mParticles[i]->SetImage(IMAGE_PARTICLE_WHITE, 4);
			mParticles[i]->mAdditive = true;
			mParticles[i]->SetColors(Color(0xff, 0xff, 0), Color(0xff, 0xff, 0));
			aXAcc += aXStep;
			aDelayAcc += 2;
		}
	}
	else
	{
		mFxAnim.SetStopped(false);
		Fire(0xd);
	}

	if (!mBoard->mFlag8E9 && !mBoard->mFlag8E8)
	{
		if (mType == 10)
			PlayBoardSound(SOUND_FREEZE);
		else
		{
			if (mType == 0xe)
				PlayBoardSound(SOUND_GALAXY);
			PlayBoardSound(SOUND_COLUMN_KILLER);
		}
	}
}

// 0x405720
void PowerBrick::Draw(Graphics* g, bool theGlow)
{
	if (mHidden)
		return;

	if (mState == 3 || mState == 4 || mState == 6 || (mState == 7 && mType != 0xd))
		DrawParticles(g);
	if (mTrail)
		DrawSparks(g);

	int aShakeX = ShakeOffsetX();
	int aShakeY = ShakeOffsetY();

	if (!mVisible)
		return;

	if (IsDeathState(mState))
	{
		if (mDeathAnim.mStopped && mFxAnim.mStopped)
			return;
		if (mState != 3 && mState != 4)
			return;
		if (mMutated == 1)
		{
			DrawDeathStrip(g);
			return;
		}
		Image* aDeathImage = NULL;
		if (mType == 10)
			aDeathImage = IMAGE_BRICK_FREEZE_DEATH;
		if (mType == 0xe)
			aDeathImage = IMAGE_BRICK_BOOSTER_DEATH;
		int aDX = -15;
		int aDY = -40;
		if (mType == 10 || mType == 0xe)
		{
			aDX = -53;
			aDY = -74;
		}
		if (!mDeathAnim.mStopped && mType != 0xd && aDeathImage != NULL)
			g->DrawImageCel(aDeathImage, (int) ((double) aDX + mX), (int) ((double) aDY + mY), CelOf(mDeathAnim));
		if (mType == 0xd && !mFxAnim.mStopped)
		{
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->DrawImageCel(IMAGE_BRICK_COL_NUKE_FX, (int) ((double) aDX + mX + 14.0), (int) (-(double) mNukeY), CelOf(mFxAnim));
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
		return;
	}

	if (theGlow)
	{
		DrawGlow(g);
		return;
	}

	Image* anImage = NULL;
	if (mMutated == 1)
		anImage = IMAGE_BRICK_MUTATOR;
	else if (mType == 10)
		anImage = IMAGE_BRICK_FREEZE;
	else if (mType == 0xd)
		anImage = IMAGE_BRICK_COL_NUKE;
	else if (mType == 0xe)
		anImage = IMAGE_BRICK_BOOSTER;
	if (mState == 2 && mImage != NULL)
		anImage = mImage;
	if (mRainbow && mRainbowIndex > -10 && mRainbowIndex >= 0 && (size_t) mRainbowIndex < mRainbowImages.size())
		anImage = mRainbowImages[mRainbowIndex];

	DrawBody(g, anImage, aShakeX, aShakeY);
}
