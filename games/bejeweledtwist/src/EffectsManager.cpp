// Sexy::Effect and Sexy::EffectsManager (EffectsManager.h).
//
// port: the parts every effect shares and the types the board uses so far (0, 5, 6, 7, 10, 11, 12, 13, 14, 16, 17, 23, 24,
// 26, 29, 30, 38, 39 and 41). The others (the rest of the board's explosions, fire, lightning and the shows' effects) come with
// their parts of the game; until then an effect of one of them logs once and only moves and fades.
#include "EffectsManager.h"

#include "Board.h"
#include "Piece.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "GraphicsRecorder.h"
#include "Image.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <SDL.h>

#include <cmath>
#include <set>

using namespace Sexy;

namespace {

void LogUnportedType(const char* theWhere, int theType)
{
	static std::set<int> aLogged;
	if (aLogged.insert(theType).second)
		SDL_Log("EffectsManager: effect type %d is not ported yet (%s)", theType, theWhere);
}

// Clamped away from zero: a shard seen edge on still shows a quarter of its face
float ShardSide(float theAngle)
{
	float aSide = sinf(theAngle);
	if (0.0f < aSide && aSide < 0.25f)
		aSide = 0.25f;
	if (aSide < 0.0f && -0.25f < aSide)
		aSide = -0.25f;
	return aSide;
}

// 0x6f6451's type 16: a shard tumbling about two axes, its outline glinting as it faces the screen, with a sparkle
void DrawShard(Graphics* g, Effect* theEffect, int theUpdateCnt)
{
	float aFade = theEffect->mAlpha;
	if (0.0f < aFade)
		aFade = sqrtf(aFade);
	Color aColor = theEffect->mColor;
	aColor.mAlpha = Ftol((double)aFade * 64.0);
	g->SetColorizeImages(true);
	g->SetColor(aColor);
	float aScale = theEffect->mScale;
	Transform aTransform;
	aTransform.RotateRad(theEffect->mAngle);
	float aSideX = ShardSide(theEffect->mBlend);
	float aSideY = ShardSide(theEffect->mRateFrom);
	aTransform.Scale((float)((double)aSideX * 1.4 * (double)aScale), (float)((double)aScale * (double)aSideY * 1.4));
	g->SetColorizeImages(true);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	float aGrow = (float)((double)theEffect->mScale * 1.15);
	aTransform.Scale(aGrow, aGrow);
	aColor.mAlpha = Ftol((double)aFade * 255.0);
	g->SetColor(aColor);
	// port: the original picks the shard's cel from the manager's update count times the effect's address, so the
	// shapes flicker differently here (a deviation without effect on play)
	int aCel = (int)((unsigned int)theUpdateCnt * (unsigned int)(uintptr_t)theEffect) & 7;
	int aWidth = IMAGE_SM_SHARDS->GetCelWidth();
	Rect aSrc(aWidth * aCel, 0, aWidth, IMAGE_SM_SHARDS->GetCelHeight());
	gGraphicsRecorder->DrawImageTransform(g, IMAGE_SM_SHARDS, aTransform, aSrc, (float)Ftol(theEffect->mX), (float)Ftol(theEffect->mY), false);
	float aFacing = sinf(theEffect->mRateFrom);
	aFacing = fabsf(sinf(theEffect->mBlend) * aFacing);
	float aDim = 2.0f - theEffect->mScale;
	if (0.2f <= aDim) {
		if (1.0f < aDim)
			aDim = 1.0f;
	} else {
		aDim = 0.2f;
	}
	g->SetColor(Color(0xffffff, Ftol((double)aDim * 255.0 * std::pow((double)aFacing, 5.0) * (double)aFade)));  // 0x670a49
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	aWidth = IMAGE_SM_SHARDS_OUTLINE->GetCelWidth();
	aSrc = Rect(aWidth * aCel, 0, aWidth, IMAGE_SM_SHARDS_OUTLINE->GetCelHeight());
	gGraphicsRecorder->DrawImageTransform(g, IMAGE_SM_SHARDS_OUTLINE, aTransform, aSrc, (float)Ftol(theEffect->mX),
		(float)Ftol(theEffect->mY), false);
	g->SetColor(Color(0xffffff, Ftol(std::pow((double)aFacing, 7.5) * 255.0 * (double)aFade)));
	int aHeight = Ftol((double)IMAGE_SPARKLE->GetCelHeight() * (double)aScale);
	aWidth = Ftol((double)IMAGE_SPARKLE->GetCelWidth() * (double)aScale);
	int aY = Ftol((double)Ftol(theEffect->mY) - (double)(IMAGE_SPARKLE->GetCelHeight() / 2) * (double)aScale);
	int aX = Ftol((double)Ftol(theEffect->mX) - (double)(IMAGE_SPARKLE->GetCelWidth() / 2) * (double)aScale);
	gGraphicsRecorder->DrawImageCel(g, IMAGE_SPARKLE, Rect(aX, aY, aWidth, aHeight), aCel);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
}

}  // namespace

Effect::Effect(int theType) : mType(theType)
{
	mColor = Color(255, 255, 255);
	switch (mType) {
	case 0:
	case 23:  // a bomb's bubbles (Piece::Update)
	case 41:  // the cursor's trail of rings (Board::UpdateCursor)
	case 43:  // the nova of coal breaking (Board::BreakSpecial)
		break;
	case 10:  // a wisp of steam turning as it goes (Piece::AddShards, the credits' sparks)
		// 0x6f4a05 has no case for it. It does not set +0x9c, +0xa0 and +0xa4 either, so they hold what the heap had;
		// AddShards sets them and the credits (0x6e903c) do not (port: 0, see NOTES, "Oddities")
		break;
	case 40:  // a spark of a geode (Board::UpdateGame)
		mImage = IMAGE_GEM_FRUIT_SPARK;
		mIsAdditive = true;
		mGravity = (float)((double)gApp->mScale * 0.01);
		mAlpha = 1.0f;
		mDAlpha = 0;
		mScale = 0.5f;
		mDScale = -0.005f;
		break;
	case 11:  // the gates' sparks
	case 12:  // the sparks where the points' smoke reaches the tube (0x74f457)
		mGravity = (float)(gApp->mScale * 0.004999999888241291);
		mDX = (float)(RandSigned() * gApp->mScale);
		mDY = (float)(RandSigned() * gApp->mScale);
		mScale = 0.2f;
		mAlpha = 1.0f;
		mDAlpha = -0.005f;
		// (Is3DAccelerated 0x4a7900) With Direct3D a random angle, the draw after the two of the velocity
		if (gApp->Is3DAccelerated())
			mAngle = (float)((double)RandSigned() * 3.1415927410125732);
		else
			mAngle = 0;
		break;
	case 16:  // a gem's shards (Board::BlowUpPiece)
		mFrame = (int)(Rand() % 40);
		mAlpha = 1.0f;
		mUpdateDiv = 0;
		mDAlpha = (float)(-0.004999999888241291 - (double)fabsf(RandSigned()) * 0.009999999776482582);
		mVelocityDamping = 1.0f;
		break;
	case 26:  // smoke (Board::BlowUpPiece)
		mImage = IMAGE_FX_STEAM;
		mGravity = (float)((double)gApp->mScale * -0.005);
		mAngle = (float)((double)RandSigned() * 3.1415927410125732);
		mDAngle = (float)((double)RandSigned() * 0.03999999910593033);
		mAlpha = 0.85f;
		mDAlpha = 0;
		mBlend = 0.5f;
		mRateFrom = -0.02f;
		mRateTo = 0.95f;
		break;
	case 24:  // smoke (the countdown shards' trail)
		mImage = IMAGE_SMOKE;
		mGravity = (float)((double)gApp->mScale * -0.005);
		mAlpha = 0.5f;
		mDAlpha = -0.005f;
		break;
	case 39: {  // a bomb's shard (Board::BreakSpecial)
		int aTurning = (int)(Rand() % 2);
		int aSide = (int)(Rand() % 2);
		float aHalf = (float)((double)aSide * 3.1415927410125732);
		float aDir = (float)((double)RandSigned() * 3.1415927410125732 * 0.30000001192092896 + (double)aHalf);
		float aSpeed = (float)(((double)fabsf(RandSigned()) * 0.05000000074505806 + 0.019999999552965164) *
							   (double)gGemWidth);
		if (aTurning != 0) {
			mAngle = aDir;
			mImage = IMAGE_COUNTDOWNSHARDS;
			mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.009999999776482582);
		} else {
			mImage = IMAGE_GEMSHARD;
			mAngle = 0;
		}
		// With Direct3D (gD3DInterface != 0) a random angle and turn, drawn after the turning shard's own and before
		// the frame
		if (gD3DInterface != nullptr) {
			mAngle = (float)((double)RandSigned() * 3.1415927410125732);
			mDAngle = (float)((double)RandSigned() * 0.20000000298023224);
		}
		mFrame = (int)Rand() % (mImage->mNumCols * mImage->mNumRows);
		mColor = Color::White;
		mDX = (float)((double)(cosf(aDir) * aSpeed) * 1.25);
		mDY = (float)((double)-fabsf(sinf(aDir) * aSpeed) * 1.5);
		mGravity = (float)((double)gApp->mScale * 0.15000000596046448);
		float aRand = fabsf(RandSigned());
		mDAlpha = (float)((double)aRand * -0.0 + -0.0);
		mAlpha = 1.0f;
		mScale = (float)((double)fabsf(RandSigned()) * 0.20000000298023224 + 0.5);
		mDScale = (float)(-0.0 - (double)fabsf(RandSigned()) * 0.009999999776482582);
		mSizeJitter = (float)((double)fabsf(RandSigned()) * 0.75 + 0.5);
		break;
	}
	case 29:  // steam that trails smoke (AddBombFlash)
		mImage = IMAGE_FX_STEAM;
		mGravity = (float)((double)gApp->mScale * -0.005);
		mAngle = (float)((double)RandSigned() * 3.1415927410125732);
		mDAngle = (float)((double)RandSigned() * 0.03999999910593033);
		mAlpha = 0.85f;
		mDAlpha = 0;
		break;
	case 30:  // a drip (AddBombFlash)
		mImage = IMAGE_DRIP;
		mDAlpha = 0;
		mGravity = (float)((double)gApp->mScale * 0.05);
		break;
	case 5:  // a power gem's flames (Piece::UpdatePowerSparks)
	case 6: {
		mImage = IMAGE_FIREPARTICLE;
		int aBlue = (int)(Rand() % 32) + 30;
		int aGreen = (int)(Rand() % 64) + 64;
		mColor = Color(255, aGreen, aBlue);
		mGravity = -0.0f;
		mScale = 0.75f;
		mDScale = 0.005f;
		mAngle = (float)((double)RandSigned() * 3.1415927410125732);
		mDAngle = 0.01f;
		if (mType == 6) {
			mPhase = 0;
			mAlpha = 0.01f;
			mDAlpha = 0.02f;
		}
		break;
	}
	case 7:  // smoke rising from a breaking gem lock's glowing links (type 14)
		mImage = IMAGE_GEM_FIRE_SMOKE;
		mGravity = (float)((double)gApp->mScale * -0.005);
		mDAlpha = -0.01f;
		break;
	case 13:  // a gem lock's link flying off (Board::BreakSpecial); 14 glows and smokes
	case 14: {
		mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.05000000074505806);
		float aSpread = (float)((double)gApp->mScale * 0.02500000037252903);
		mDX = (float)((double)RandSigned() * (double)gGemWidth * (double)aSpread);
		float aScale = gApp->mScale;
		mDY = (float)((double)RandSigned() * (double)aScale);
		float aGravity = (float)((double)gApp->mScale * 0.05000000074505806);
		mGravity = aGravity;
		mAlpha = 1.0f;
		break;
	}
	case 38: {  // a shard of a doom gem (Board::BreakSpecial)
		float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
		float aSpeed = (float)((double)gApp->mScale * 5.0 * (double)fabsf(RandSigned()));
		mDX = (float)((double)cosf(anAngle) * (double)aSpeed);
		mDY = (float)((double)sinf(anAngle) * (double)aSpeed);
		mDScale = 0.01f;
		mScale = (float)((double)fabsf(RandSigned()) * 0.10000000149011612 + 0.25);
		mColor = Color(32, 32, 32);
		mAlpha = 1.0f;
		mDAlpha = 0;
		mImage = IMAGE_SHARD1;
		mAngle = anAngle;
		mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.019999999552965164);
		mDelay = (float)((double)fabsf(RandSigned()) + 0.75);
		break;
	}
	case 17:  // the hint's sparkles (Board::UpdateHint)
		mUpdateDiv = (int)(Rand() % 4) + 3;
		mDX = (float)((double)gApp->mScale * ((double)(float)(int)(Rand() % 20) * 0.1 - 1.0));
		mDY = (float)((double)gApp->mScale * (double)(float)(int)(Rand() % 50) * 0.1);
		break;
	default:
		LogUnportedType("constructor", mType);
		break;
	}
	mDeleteMe = false;
}

Effect::~Effect()
{
	if (mOwnsImage && mImage != nullptr)
		delete mImage;
}

EffectsManager::EffectsManager()
{
	mHasAlpha = true;
	mX = 0;
	mY = 0;
	mClip = false;
	mMouseVisible = false;
}

EffectsManager::~EffectsManager()
{
	for (auto& anEntry : mEffects)
		delete anEntry.second;
	mEffects.clear();
}

void EffectsManager::Clear()
{
	while (!mEffects.empty()) {
		auto anIt = mEffects.begin();
		delete anIt->second;
		mEffects.erase(anIt);
	}
}

Effect* EffectsManager::AllocEffect(int theType)
{
	return new Effect(theType);
}

void EffectsManager::AddEffect(Effect* theEffect)
{
	mEffects.insert(std::make_pair(theEffect->mType, theEffect));
	theEffect->mFXManager = this;
}

void EffectsManager::Update()
{
	Widget::Update();
	mIs3D = gApp->Is3DAccelerated();
	mWidth = gApp->mWidth;
	mHeight = gApp->mHeight;
	// The heat waves of the list at +0x94 grow and go (added in 2D mode too, only drawn in 3D)
	if (!mHeatWaves.empty()) {
		auto aWaveIt = mHeatWaves.begin();
		while (aWaveIt != mHeatWaves.end()) {
			float aProgress = aWaveIt->mGrowth + aWaveIt->mProgress;
			aWaveIt->mProgress = aProgress;
			if (1.0 < (double)aProgress)
				aWaveIt = mHeatWaves.erase(aWaveIt);
			else
				++aWaveIt;
		}
	}

	auto anIt = mEffects.begin();
	while (anIt != mEffects.end()) {
		Effect* anEffect = anIt->second;
		int aType = anEffect->mType;
		if (aType == 44) {
			anEffect->Update();
			++anIt;
			continue;
		}
		int aDiv = anEffect->mUpdateDiv != 0 ? anEffect->mUpdateDiv : 1;
		float aDivF = (float)aDiv;
		anEffect->mX = anEffect->mDX / aDivF + anEffect->mX;
		anEffect->mY = anEffect->mDY / aDivF + anEffect->mY;
		if (mUpdateCnt % aDiv == 0) {
			anEffect->mDX = anEffect->mDXScalar * anEffect->mDX;
			float aDY = anEffect->mGravity + anEffect->mDY;
			anEffect->mDY = aDY * anEffect->mDYScalar;
			if ((anEffect->mFlags & Effect::FLAG_DELAY) == 0 || anEffect->mDelay <= 0.0f) {
				anEffect->mAlpha = anEffect->mDAlpha + anEffect->mAlpha;
				anEffect->mScale = anEffect->mDScale + anEffect->mScale;
			} else {
				float aDelay = (float)(anEffect->mDelay - 0.009999999776482582);
				anEffect->mDelay = aDelay;
				if (aDelay <= 0.0f)
					anEffect->mDelay = 0;
			}
		}

		switch (aType) {
		case 0:
			if (anEffect->mUpdateDiv != 0 && mUpdateCnt % anEffect->mUpdateDiv != 0)
				break;
			if (anEffect->mIsCyclingColor) {
				anEffect->mCurHue = (anEffect->mCurHue + 8) % 256;
				anEffect->mColor = Color((int)gApp->HSLToRGB(anEffect->mCurHue, 255, 128));
			}
			if (anEffect->mY < 0.0f || (float)mHeight <= anEffect->mY || anEffect->mX < 0.0f ||
				(float)mWidth <= anEffect->mX)
				anEffect->mDeleteMe = true;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			break;
		case 23:
			break;
		case 7:
			anEffect->mDX = (float)((double)anEffect->mDX * 0.949999988079071);
			if (0.5f < anEffect->mAlpha) {
				anEffect->mAlpha = 0.5f;
				anEffect->mDAlpha = -0.01f;
			}
			break;
		case 14: {
			// a glowing link leaves smoke behind it
			Effect* aSmoke = AllocEffect(7);
			aSmoke->mX = anEffect->mX;
			aSmoke->mY = anEffect->mY;
			aSmoke->mDX = 0;
			float aRand = RandSigned();
			aSmoke->mDY = (float)(-0.10000000149011612 - (double)fabsf(aRand) * 0.5);
			aSmoke->mGravity = 0;
			aSmoke->mScale = 0.25f;
			AddEffect(aSmoke);
		}
			// fall through
		case 13:
			// it fades, ends below the screen and moves across a second time
			anEffect->mAlpha = (float)((double)anEffect->mAlpha - 0.0020000000949949026);
			if ((float)mHeight < anEffect->mY)
				anEffect->mDeleteMe = true;
			anEffect->mX = anEffect->mDX + anEffect->mX;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			break;
		case 38: {
			// it waits out its time, turning and fading, and leaves steam every tenth update
			float aDelay = (float)((double)anEffect->mDelay - 0.009999999776482582);
			anEffect->mDelay = aDelay;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			anEffect->mAlpha = (float)((double)anEffect->mAlpha - 0.009999999776482582);
			if (!(0.0f < aDelay)) {
				anEffect->mDeleteMe = true;
				break;
			}
			if (mUpdateCnt % 10 != 0)
				break;
			Effect* aSteam = AllocEffect(0);
			aSteam->mImage = IMAGE_FX_STEAM;
			aSteam->mScale = (float)((double)fabsf(RandSigned()) * 0.20000000298023224 + 0.5);
			aSteam->mDScale = 0.01f;
			aSteam->mX = anEffect->mX;
			aSteam->mY = anEffect->mY;
			aSteam->mColor = Color(96, 96, 96);
			aSteam->mAlpha = 0.5f;
			aSteam->mDAlpha = -0.003f;
			aSteam->mDY = -0.1f;
			aSteam->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
			aSteam->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.10000000149011612);
			AddEffect(aSteam);
			break;
		}
		case 40:
			// it flickers
			anEffect->mAlpha = Rand() % 5 == 0 ? 1.0f : 0.25f;
			break;
		case 43: {
			// the coal's nova: it grows, stays hidden for 40 updates, and fades from the 60th
			float aScale = anEffect->mDScale + anEffect->mScale;
			if (1.0f <= aScale)
				aScale = 1.0f;
			anEffect->mScale = aScale;
			if (anEffect->mFrame < 40)
				anEffect->mScale = 0;
			anEffect->mFrame++;
			if (anEffect->mFrame == 60)
				anEffect->mDAlpha = -0.02f;
			break;
		}
		case 24:
			anEffect->mDX = (float)((double)anEffect->mDX * 0.95);
			break;
		case 39: {
			// it trails smoke (effect 24), and ends once small
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			Effect* aSmoke = AllocEffect(24);
			float aRand = fabsf(RandSigned());
			aSmoke->mScale = (float)(((double)aRand * 0.44999998807907104 + 0.0) * (double)anEffect->mScale *
									 (double)aSmoke->mScale);
			aRand = fabsf(RandSigned());
			float anAlpha = (float)(((double)aRand * 0.05000000074505806 + 0.30000001192092896) * (double)aSmoke->mAlpha);
			aSmoke->mAlpha = anAlpha;
			aSmoke->mAlpha = anEffect->mAlpha * anAlpha;
			aSmoke->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
			aSmoke->mIsAdditive = false;
			aSmoke->mDAngle = (float)((double)RandSigned() * 3.1415927410125732 * 0.10000000149011612);
			aSmoke->mGravity = 0;
			aSmoke->mDY = (float)((double)gApp->mScale * -1.0);
			aSmoke->mX = anEffect->mX;
			aSmoke->mY = anEffect->mY;
			AddEffect(aSmoke);
			if (anEffect->mScale < 0.02f)
				anEffect->mDeleteMe = true;
			break;
		}
		case 29:
			if (anEffect->mScale < 0.1f) {
				anEffect->mAlpha = 0;
				break;
			}
			if (mUpdateCnt % 10 == 9) {
				anEffect->mDX = (float)((double)RandSigned() * 1.2999999523162842 + (double)anEffect->mDX);
				anEffect->mDY = (float)((double)RandSigned() * 1.2999999523162842 + (double)anEffect->mDY);
			}
			if (mUpdateCnt % 2 != 0) {
				// a puff of smoke left where it is
				Effect* aSmoke = AllocEffect(26);
				aSmoke->mDX = 0;
				aSmoke->mDY = 0;
				aSmoke->mX = anEffect->mX;
				aSmoke->mY = anEffect->mY;
				aSmoke->mIsAdditive = false;
				aSmoke->mScale = anEffect->mScale;
				aSmoke->mDScale = 0;
				AddEffect(aSmoke);
			}
			break;
		case 30:
			// it turns to its direction of fall
			if (anEffect->mScale < 0.0f)
				anEffect->mAlpha = 0;
			anEffect->mAngle = (float)((double)(float)std::atan2((double)anEffect->mDX, (double)anEffect->mDY) + 0.0);
			break;
		case 41: {
			// it fades the faster the further it is from the cursor
			Board* aBoard = gApp->mBoard;
			if (aBoard == nullptr) {
				anEffect->mDeleteMe = true;
				break;
			}
			float aDX = anEffect->mX - (float)(aBoard->mCursorX + gGemWidth);
			float aDY = anEffect->mY - (float)(aBoard->mCursorY + gGemHeight);
			float aDist = sqrtf(aDX * aDX + aDY * aDY);
			float anAlpha = (float)((double)anEffect->mAlpha - (double)aDist * 0.0010000000474974513);
			anEffect->mAlpha = anAlpha;
			if (anAlpha < 0.0f)
				anEffect->mDeleteMe = true;
			break;
		}
		case 10: {
			// it fades and turns by its fall; its velocity turns by a rate that moves from +0xa0 to +0xa4 as +0x9c runs
			// from 0 to 1, and then a new rate is drawn
			anEffect->mAlpha = (float)((double)anEffect->mAlpha - 0.0024999999441206455);
			anEffect->mAngle = (float)((double)anEffect->mDY * 0.10000000149011612 + (double)anEffect->mAngle);
			float aRate = (float)((1.0 - (double)anEffect->mBlend) * (double)anEffect->mRateFrom +
								  (double)anEffect->mRateTo * (double)anEffect->mBlend);
			float aTurn = (float)((double)aRate * 0.05000000074505806);
			float anOldDX = anEffect->mDX;
			float aCos = cosf(aTurn);
			float aSin = sinf(aTurn);
			anEffect->mDX = (float)((double)anOldDX * (double)aCos + (double)aSin * (double)anEffect->mDY);
			aCos = cosf(aTurn);
			aSin = sinf(aTurn);
			anEffect->mDY = (float)((double)aCos * (double)anEffect->mDY - (double)anOldDX * (double)aSin);
			float aPos = (float)((double)anEffect->mBlend + 0.009999999776482582);
			anEffect->mBlend = aPos;
			if (1.0f < aPos) {
				anEffect->mBlend = 0;
				anEffect->mRateFrom = anEffect->mRateTo;
				anEffect->mRateTo = RandSigned();
			}
			break;
		}
		case 11:
			if (anEffect->mScale > 0.85f)
				anEffect->mScale = 0.85f;
			break;
		case 12: {
			anEffect->mDY = anEffect->mGravity + anEffect->mDY;
			// With Direct3D (Is3DAccelerated) the angle also turns by mDY * 0.02
			float aTurn = 0.0f;
			if (gApp->Is3DAccelerated())
				aTurn = (float)((double)anEffect->mDY * 0.02);
			anEffect->mAngle = aTurn + anEffect->mAngle;
			float aScale = (float)((double)anEffect->mScale + 0.05);
			anEffect->mScale = aScale;
			if (0.5f < aScale)
				anEffect->mScale = 0.5f;
			break;
		}
		case 16:
			if (mUpdateCnt % 2 == 0)
				anEffect->mFrame = (anEffect->mFrame + 1) % 40;
			anEffect->mDX = anEffect->mDX * anEffect->mVelocityDamping;
			anEffect->mDY = anEffect->mVelocityDamping * anEffect->mDY;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			anEffect->mBlend = anEffect->mRateTo + anEffect->mBlend;
			anEffect->mRateFrom = anEffect->mTumbleTurn + anEffect->mRateFrom;
			break;
		case 26: {
			// it slows, and once slow it fades
			float aSpeed = sqrtf(anEffect->mDX * anEffect->mDX + anEffect->mDY * anEffect->mDY);
			anEffect->mDX = anEffect->mDX * anEffect->mRateTo;
			if (anEffect->mBlend < aSpeed)
				anEffect->mDY = anEffect->mRateTo * anEffect->mDY;
			else
				anEffect->mDAlpha = anEffect->mRateFrom;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			anEffect->mScale = anEffect->mDScale + anEffect->mScale;
			break;
		}
		case 5:
		case 6: {
			float aScale = anEffect->mDScale + anEffect->mScale;
			anEffect->mScale = aScale;
			anEffect->mAngle = anEffect->mDAngle + anEffect->mAngle;
			if (aType == 6) {
				// it grows brighter through its cels and ends at full brightness (its fade is added twice)
				float anAlpha = anEffect->mDAlpha + anEffect->mAlpha;
				anEffect->mAlpha = anAlpha;
				if (1.0f <= anAlpha)
					anEffect->mDeleteMe = true;
				anEffect->mFrame = Ftol((double)anAlpha * 12.0);
			} else if (anEffect->mColor.mRed + anEffect->mColor.mGreen == 0) {
				// (a black flame, which none is, grows faster and fades)
				anEffect->mScale = (float)((double)aScale + 0.019999999552965164);
				float anAlpha = (float)((double)anEffect->mAlpha - 0.009999999776482582);
				anEffect->mAlpha = anAlpha;
				if (!(0.0f < anAlpha))
					anEffect->mDeleteMe = true;
			}
			break;
		}
		case 17:
			anEffect->mDX = (float)((double)anEffect->mDX * 0.98);
			anEffect->mDY = (float)((double)anEffect->mDY * 0.98);
			if (mUpdateCnt % anEffect->mUpdateDiv == 0)
				anEffect->mFrame = (anEffect->mFrame + 1) % 40;
			if (anEffect->mFrame == 14)
				anEffect->mDeleteMe = true;
			if (anEffect->mY <= (float)mHeight && 0.0f <= anEffect->mY && anEffect->mX <= (float)mWidth &&
				0.0f <= anEffect->mX) {
				if (!(0.0f < anEffect->mAlpha))
					anEffect->mDeleteMe = true;
			} else {
				anEffect->mDeleteMe = true;
			}
			break;
		default:
			LogUnportedType("Update", aType);
			break;
		}

		if (anEffect->mMinScale <= anEffect->mScale) {
			if (anEffect->mMaxScale < anEffect->mScale) {
				anEffect->mScale = anEffect->mMaxScale;
				if ((anEffect->mFlags & Effect::FLAG_SCALE_BOUNCE) != 0)
					anEffect->mDScale = -anEffect->mDScale;
			}
		} else {
			anEffect->mDeleteMe = true;
			anEffect->mScale = anEffect->mMinScale;
		}
		if (anEffect->mAlpha <= anEffect->mMaxAlpha) {
			if (anEffect->mAlpha <= 0.0f)
				anEffect->mDeleteMe = true;
		} else {
			anEffect->mAlpha = anEffect->mMaxAlpha;
			if ((anEffect->mFlags & Effect::FLAG_ALPHA_BOUNCE) != 0)
				anEffect->mDAlpha = -anEffect->mDAlpha;
			else
				anEffect->mDeleteMe = true;
		}

		if (!anEffect->mDeleteMe) {
			++anIt;
		} else {
			delete anEffect;
			anIt = mEffects.erase(anIt);
		}
	}
}

void EffectsManager::DrawEffects(Graphics* g, bool theOverlay)
{
	// With the manager's +0xa1 set and Direct3D, the depth test goes off first
	if (mDepthTestOffFirst && gD3DInterface != nullptr)
		gD3DInterface->SetDepthTestMode(0);
	for (auto anIt = mEffects.begin(); anIt != mEffects.end(); ++anIt) {
		Effect* anEffect = anIt->second;
		if (anEffect->mOverlay != theOverlay)
			continue;
		// An effect on a piece is drawn at the piece's place plus its own (the board's piece, app +0x7e4), and not at all
		// without a board or once the piece is gone
		Piece* aPiece = nullptr;
		if (anEffect->mPieceId != -1) {
			if (gApp->mBoard == nullptr || (aPiece = gApp->mBoard->GetPieceById(anEffect->mPieceId)) == nullptr)
				continue;
			anEffect->mX = aPiece->mX + anEffect->mX;
			anEffect->mY = aPiece->mY + anEffect->mY;
		}
		if (anEffect->mType == 44) {
			anEffect->Draw(g);
			continue;
		}
		float anAlpha = anEffect->mAlpha <= 1.0f ? anEffect->mAlpha : 1.0f;
		float anOpacity = mAlpha * anAlpha;
		switch (anEffect->mType) {
		case 0:
		case 38:
		case 41: {
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(aColor);
			g->SetDrawMode(anEffect->mIsAdditive ? Graphics::DRAWMODE_ADDITIVE : Graphics::DRAWMODE_NORMAL);
			if (anEffect->mImage != nullptr) {
				DrawImageCentered(g, anEffect->mImage, anEffect->mImage->GetCelRect(anEffect->mFrame), anEffect->mX,
					anEffect->mY, (double)anEffect->mAngle, anEffect->mScale, anEffect->mScale);
				g->SetColorizeImages(false);
			}
			break;
		}
		case 10: {
			// five copies of the steam, smaller each, back along a path that turns the other way: a trail
			float aDX = anEffect->mDX;
			float aDY = anEffect->mDY;
			float aX = anEffect->mX;
			float aY = anEffect->mY;
			float aSize = 1.0f;
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(125, 145, 255, Ftol((double)anOpacity * 105.0)));  // the effect's own colour is not used
			float aRate = (float)((1.0 - (double)anEffect->mBlend) * (double)anEffect->mRateFrom +
								  (double)anEffect->mRateTo * (double)anEffect->mBlend);
			float aTurn = (float)((double)aRate * -0.30000001192092896);
			int aCount = 5;
			do {
				float aScale = anEffect->mScale * aSize;
				DrawImageCentered(g, IMAGE_FX_STEAM, aX, aY, (double)anEffect->mAngle, aScale, aScale);
				aX = (float)((double)aX - (double)aDX * 6.0);
				aY = (float)((double)aY - (double)aDY * 6.0);
				float anOldDX = aDX;
				float aCos = cosf(aTurn);
				aDX = (float)((double)sinf(aTurn) * (double)aDY + (double)aCos * (double)aDX);
				aCos = cosf(aTurn);
				double aCosDY = (double)aCos * (double)aDY;
				aDY = (float)(aCosDY - (double)sinf(aTurn) * (double)anOldDX);
				aSize = (float)((double)aSize - 0.11999999731779099);
			} while (--aCount != 0);
			// the draw mode and colorizing are left set, as the original leaves them
			break;
		}
		case 11: {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(aColor);
			float aScale = (float)std::pow((double)anOpacity, (double)0.65f) * anEffect->mScale;  // 0x670a26
			DrawImageCentered(g, IMAGE_GATEPARTICLE, anEffect->mX, anEffect->mY, (double)anEffect->mAngle, aScale,
				aScale);
			break;
		}
		case 12: {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, Ftol(anOpacity * 255.0)));
			float aScale = anEffect->mScale * anOpacity;
			DrawImageCentered(g, IMAGE_GEM_FRUIT_SPARK, anEffect->mX, anEffect->mY, (double)anEffect->mAngle, aScale,
				aScale);
			break;
		}
		case 16:
			DrawShard(g, anEffect, mUpdateCnt);
			break;
		case 14:
			// a red glow under the link
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(255, 0, 0));
			DrawImageCentered(g, IMAGE_GEM_LOCK_LINKS_GLOW, IMAGE_GEM_LOCK_LINKS_GLOW->GetCelRect(anEffect->mFrame),
				anEffect->mX, anEffect->mY, (double)anEffect->mAngle, 1.0f, 1.0f);
			// fall through
		case 13:
			// the link (IMAGE_GEM_LOCK_LINK1 or 2 by the cel)
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, Ftol(anOpacity * 255.0)));
			if (anEffect->mType == 13)
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			DrawImageCentered(g, GetImageById(IMAGE_GEM_LOCK_LINK1_ID + anEffect->mFrame), anEffect->mX, anEffect->mY,
				(double)anEffect->mAngle, 1.0f, 1.0f);
			break;
		case 7:
		case 24:
		case 26:
		case 29: {
			g->SetDrawMode(anEffect->mIsAdditive ? Graphics::DRAWMODE_ADDITIVE : Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(aColor);
			DrawImageCentered(g, anEffect->mImage, anEffect->mImage->GetCelRect(anEffect->mFrame), anEffect->mX,
				anEffect->mY, (double)anEffect->mAngle, anEffect->mScale, anEffect->mScale);
			break;
		}
		case 39: {
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(aColor);
			g->SetDrawMode(anEffect->mIsAdditive ? Graphics::DRAWMODE_ADDITIVE : Graphics::DRAWMODE_NORMAL);
			if (anEffect->mImage != nullptr) {
				// (the cel counted along the first row, 0x4237c0)
				int aWidth = anEffect->mImage->GetCelWidth();
				Rect aSrc(aWidth * anEffect->mFrame, 0, aWidth, anEffect->mImage->GetCelHeight());
				DrawImageCentered(g, anEffect->mImage, aSrc, anEffect->mX, anEffect->mY, (double)anEffect->mAngle,
					anEffect->mScale, anEffect->mScale);
				g->SetColorizeImages(false);
			}
			break;
		}
		case 30: {
			g->SetDrawMode(anEffect->mIsAdditive ? Graphics::DRAWMODE_ADDITIVE : Graphics::DRAWMODE_NORMAL);
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(aColor);
			float aScale = sqrtf(anEffect->mScale);
			DrawImageCentered(g, anEffect->mImage, anEffect->mImage->GetCelRect(0), anEffect->mX, anEffect->mY,
				(double)anEffect->mAngle, aScale, aScale);
			break;
		}
		case 23: {
			Color aColor = anEffect->mColor;
			aColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(aColor);
			g->SetColorizeImages(true);
			gGraphicsRecorder->DrawImage(g, IMAGE_SPARK_BUBBLE, Ftol(anEffect->mX), Ftol(anEffect->mY), 4, 4);
			g->SetColorizeImages(false);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			break;
		}
		case 5:
		case 6: {
			g->SetColorizeImages(true);
			Color aColor = anEffect->mColor;
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			aColor.mAlpha = Ftol(anOpacity * 200.0);
			g->SetColor(aColor);
			if (anEffect->mType != 6) {
				DrawImageCentered(g, anEffect->mImage, anEffect->mX, anEffect->mY, (double)anEffect->mAngle,
					anEffect->mScale, anEffect->mScale);
				break;
			}
			// The cel of its brightness, at the colour's own alpha; with Direct3D (gD3DInterface) scaled by mScale
			// and turned by mAngle (when it is not 0) and drawn as floats (0x70e2d5), otherwise unscaled (0x70e26a)
			int aCel = Ftol((double)anEffect->mAlpha * 64.0);
			if (63 < aCel)
				aCel = 63;
			Transform aTransform;
			if (gD3DInterface != nullptr) {
				aTransform.Scale(anEffect->mScale, anEffect->mScale);
				if (anEffect->mAngle != 0.0f)
					aTransform.RotateRad(anEffect->mAngle);
			}
			g->SetColor(anEffect->mColor);
			gGraphicsRecorder->DrawImageTransform(g, anEffect->mImage, aTransform, anEffect->mImage->GetCelRect(aCel),
				anEffect->mX, anEffect->mY, gD3DInterface != nullptr);
			break;
		}
		case 40: {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 128, Ftol(anOpacity * 255.0)));
			float aScale = anEffect->mScale * anOpacity;
			DrawImageCentered(g, IMAGE_GEM_FRUIT_SPARK, anEffect->mX, anEffect->mY, (double)anEffect->mAngle, aScale,
				aScale);
			break;
		}
		case 43: {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, Ftol(anOpacity * 255.0)));
			Transform aTransform;
			aTransform.RotateRad((float)((double)anEffect->mFrame * -0.10000000149011612));
			aTransform.Scale(anEffect->mScale, anEffect->mScale);
			g->DrawImageTransform(IMAGE_COALNOVA, aTransform, anEffect->mX, anEffect->mY);
			g->SetColorizeImages(false);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			break;
		}
		case 17: {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			anEffect->mColor.mAlpha = Ftol(anOpacity * 255.0);
			g->SetColor(anEffect->mColor);
			int aWidth = Ftol((double)IMAGE_SPARKLE->GetCelWidth() * (double)anEffect->mScale);
			int aHeight = Ftol((double)IMAGE_SPARKLE->GetCelHeight() * (double)anEffect->mScale);
			Rect aRect(Ftol(anEffect->mX) - aWidth / 2, Ftol(anEffect->mY) - aHeight / 2, aWidth, aHeight);
			gGraphicsRecorder->DrawImageCel(g, IMAGE_SPARKLE, aRect, anEffect->mFrame);
			g->SetColorizeImages(false);
			break;
		}
		default:
			LogUnportedType("Draw", anEffect->mType);
			break;
		}
		if (aPiece != nullptr) {
			anEffect->mX = anEffect->mX - aPiece->mX;
			anEffect->mY = anEffect->mY - aPiece->mY;
		}
	}
}

void EffectsManager::Draw(Graphics* g)
{
	// With the manager's +0xa0 set, a board and Direct3D, the board's transform (+0x29e8) is pushed around the drawing
	bool aBoardTransform = mDrawnUnderBoardTransform && gBoard != nullptr && gD3DInterface != nullptr;
	if (aBoardTransform)
		gD3DInterface->PushTransform(gBoard->mTransform, true);  // slot 0x14 (0x6f7a45)
	DrawEffects(g, false);
	if (aBoardTransform)
		gD3DInterface->PopTransform();
	if (mWidgetManager != nullptr)
		DeferOverlay(0x31);
}

void EffectsManager::DrawOverlay(Graphics* g)
{
	DrawEffects(g, true);
	// The heat waves and the distortion pass (0x6f80a4): with a heat wave in the list, or the bombs' haze marked (+0xb5)
	// while the widget manager's image is the one drawn to, the distortion drawing (0x6f7ab4) runs, then the height
	// image goes grey again (0x6f55b0) and +0xb5 clear. Both need the 3D interface, as the height image is made only
	// with one.
	mHeightImageUsed = mHeightImageUsed || !mHeatWaves.empty();
	if (!mHeatWaves.empty() ||
		(gD3DInterface != nullptr && mHazeMarked && mWidgetManager != nullptr &&
			(Image*)mWidgetManager->mImage == g->mDestImage)) {
		if (gD3DInterface != nullptr) {
			DrawDistortion(g);
			ClearHeightImage(g);
		}
		mHazeMarked = false;
	}
}

void EffectsManager::MakeHeightImage()
{
	// 0x6f5ea6: a half-size pooled target (flags 8, and the pool's 0x10), grey; port: the hold is the manager's
	if (mHeightImage == nullptr && gD3DInterface != nullptr) {
		mHeightImage = gD3DInterface->HoldTarget(&mHeightHold, gApp->mWidth / 2, gApp->mHeight / 2, 8, "HeightImage");
		ClearHeightImage(nullptr);
	}
}

DDImage* EffectsManager::GetHeightImage()
{
	// 0x6f5efc
	if (mHeightImage == nullptr)
		MakeHeightImage();
	mHeightImageUsed = true;
	return mHeightImage;
}

void EffectsManager::ClearHeightImage(Graphics* g)
{
	// 0x6f55b0: the height image is cleared to grey (128, 128, 128, 255), then drawing goes on in g's image (the screen
	// for none). port: the screen's own image is the device's null target
	if (mHeightImage != nullptr && gD3DInterface != nullptr) {
		mHeightImageUsed = false;
		if (gD3DInterface->SetRenderTarget(mHeightImage))
			gD3DInterface->Clear(0x80, 0x80, 0x80, 0xff);
		Image* aTarget = g != nullptr ? g->mDestImage : nullptr;
		if (aTarget != nullptr && mWidgetManager != nullptr && aTarget == (Image*)mWidgetManager->mImage)
			aTarget = nullptr;
		gD3DInterface->SetRenderTarget(aTarget);
	}
}

// 0x6f49c9: a vertex of FVF 0x2c4 (pre-transformed, diffuse, specular, two texture sets)
struct DistortVertex {
	float x, y, z, rhw;
	unsigned int mDiffuse, mSpecular;
	float u0, v0, u1, v1;
};

// port: one vertex of the distortion's mesh, whose fields the original sets in place
static DistortVertex MakeDistortVertex(float theX, float theY, float theU0, float theV0, float theU1, float theV1)
{
	DistortVertex aVertex;
	aVertex.x = theX;
	aVertex.y = theY;
	aVertex.z = 0;
	aVertex.rhw = 1.0f;
	aVertex.mDiffuse = 0;
	aVertex.mSpecular = 0;
	aVertex.u0 = theU0;
	aVertex.v0 = theV0;
	aVertex.u1 = theU1;
	aVertex.v1 = theV1;
	return aVertex;
}

// 0x6f5aaf: theRect (x, y, width, height) drawn as a strip with theImage on stage 0 and the whole of stage 1's picture
// spread over the app's size (times theScale) on its second texture coordinates
static void DrawDistortRect(Image* theImage, const double theRect[4], const Color& theColor, float theScale)
{
	float aWidth = (float)gApp->mWidth;
	float aHeight = (float)gApp->mHeight;
	float aX = (float)theRect[0];
	float aU1 = theScale * (aX / aWidth);
	float aRight = (float)((double)theRect[2] + (double)theRect[0]);
	float aU2 = theScale * (aRight / aWidth);
	float aV1 = theScale * ((float)theRect[1] / aHeight);
	float aV2 = ((float)((double)theRect[3] + (double)theRect[1]) / aHeight) * theScale;
	DistortVertex aVertices[4];
	float aLeftEdge = aX - 0.5f;
	aVertices[0] = MakeDistortVertex(aLeftEdge, (float)theRect[1] - 0.5f, 0, 0, aU1, aV1);
	aRight = aRight - 0.5f;
	aVertices[1] = MakeDistortVertex(aRight, (float)theRect[1] - 0.5f, 1.0f, 0, aU2, aV1);
	float aBottom = (float)((double)theRect[3] + (double)theRect[1]) - 0.5f;
	aVertices[2] = MakeDistortVertex(aLeftEdge, aBottom, 0, 1.0f, aU1, aV2);
	aVertices[3] = MakeDistortVertex(aRight, aBottom, 1.0f, 1.0f, aU2, aV2);
	gD3DInterface->SetTexture(0, theImage);  // slot 0xd4
	gD3DInterface->DrawPrimitiveEx(0x2c4, GPUPT_TRIANGLESTRIP, aVertices, 2, theColor, 0, 0.0f, 0.0f, true, nullptr);  // slot 0x7c
}

// 0x6f5c7d: like 0x6f5aaf, but theSource (x, y, width, height of the manager's size) picks the part of theImage
static void DrawDistortSource(EffectsManager* theManager, Image* theImage, const double theRect[4],
	const double theSource[4], const Color& theColor, float theScale)
{
	float aSourceW = (float)theManager->mWidth;
	float aSourceH = (float)theManager->mHeight;
	float aU1 = (float)theSource[0] / aSourceW;
	float aU2 = (float)((double)theSource[2] + (double)theSource[0]) / aSourceW;
	float aV1 = (float)theSource[1] / aSourceH;
	float aV2 = (float)((double)theSource[3] + (double)theSource[1]) / aSourceH;
	float aWidth = (float)gApp->mWidth;
	float aHeight = (float)gApp->mHeight;
	float aT1 = theScale * ((float)theRect[0] / aWidth);
	float aRight = (float)((double)theRect[2] + (double)theRect[0]);
	float aT2 = theScale * (aRight / aWidth);
	float aS1 = theScale * ((float)theRect[1] / aHeight);
	float aS2 = ((float)((double)theRect[1] + (double)theRect[3]) / aHeight) * theScale;
	DistortVertex aVertices[4];
	float aLeftEdge = (float)theRect[0] - 0.5f;
	aVertices[0] = MakeDistortVertex(aLeftEdge, (float)theRect[1] - 0.5f, aU1, aV1, aT1, aS1);
	float aRightEdge = aRight - 0.5f;
	aVertices[1] = MakeDistortVertex(aRightEdge, (float)theRect[1] - 0.5f, aU2, aV1, aT2, aS1);
	float aBottom = (float)((double)theRect[1] + (double)theRect[3]) - 0.5f;
	aVertices[2] = MakeDistortVertex(aLeftEdge, aBottom, aU1, aV2, aT1, aS2);
	aVertices[3] = MakeDistortVertex(aRightEdge, aBottom, aU2, aV2, aT2, aS2);
	gD3DInterface->SetTexture(0, theImage);  // slot 0xd4
	gD3DInterface->DrawPrimitiveEx(0x2c4, GPUPT_TRIANGLESTRIP, aVertices, 2, theColor, 0, 0.0f, 0.0f, true, nullptr);  // slot 0x7c
}

// The tent of width 1/3 around a point (0x401ad0 is |x|): 1 - 3 |distance| where positive, else 0
static float HeatTent(double theDistance)
{
	double aTent = 0.0;
	if (0.0 < 1.0 - std::fabs(theDistance) * 3.0)
		aTent = 1.0 - std::fabs(theDistance) * 3.0;
	return (float)aTent;
}

void EffectsManager::DrawDistortion(Graphics* g)
{
	(void)g;
	// 0x6f7ab4: the waves into the height image, then the screen's copy over the rectangles (or all of it) through the
	// screen_distort shader with the height image on stage 1
	DDImage* aHeight = GetHeightImage();
	if (aHeight == nullptr || (gD3DInterface->GetCaps(0) & 2) == 0)
		return;
	gD3DInterface->SetSamplerState(1, 6, 2);
	gD3DInterface->SetSamplerState(1, 5, 2);
	gD3DInterface->SetSamplerState(1, 7, 2);
	gD3DInterface->SetRenderTarget(aHeight);
	gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_WAVE]);  // slot 0xd8, 0x85e8f8
	auto aWaveIt = mHeatWaves.begin();
	while (aWaveIt != mHeatWaves.end()) {
		HeatWave& aWave = *aWaveIt;
		float aProgress = aWave.mProgress;
		float aFade = (float)(1.1 - (double)aProgress * 1.1);
		float aCap = aFade;
		if (1.0 <= aFade)
			aCap = 1.0f;
		float aStrength = (float)((double)aProgress * 5.0);
		if (aCap < aStrength) {
			aStrength = 1.0f;
			if (aFade < 1.0f)
				aStrength = aFade;
		}
		aStrength = aStrength * aWave.mOpacity;
		float aConstants[4];
		aConstants[0] = HeatTent((double)aProgress) * aStrength;
		aConstants[1] = HeatTent((double)aProgress - 0.333) * aStrength;
		aConstants[2] = HeatTent((double)aProgress - 0.667) * aStrength;
		aConstants[3] = HeatTent((double)aProgress - 1.0) * aStrength;
		float aSize = (float)((double)aProgress * (double)aWave.mEndSize + (1.0 - (double)aProgress) * (double)aWave.mStartSize);
		double aRect[4];
		aRect[0] = (double)((((float)((double)aProgress * (double)(float)aWave.mEndX) + (float)aWave.mX) - aSize) * 0.5f);
		aRect[1] = (double)((((float)((double)(float)aWave.mEndY * (double)aProgress) + (float)aWave.mY) - aSize) * 0.5f);
		aRect[2] = (double)aSize;
		aRect[3] = (double)aSize;
		gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_WAVE], 0, aConstants);  // slot 0x108
		((MemoryImage*)IMAGE_HEATWAVE)->mD3DFlags = 8;  // +0x44
		DrawDistortRect(IMAGE_HEATWAVE, aRect, Color(255, 255, 255, 255), 2.0f);
		++aWaveIt;
	}
	// The slot 0xd8 call's argument is lost in the export; screen_distort is the shader whose constant follows
	gD3DInterface->SetPixelShader(gTwistShaders[TWISTSHADER_SCREEN_DISTORT]);
	float aDistort[4] = {0.02f, 0.02f, 0.0f, 0.0f};
	gD3DInterface->SetShaderConstant(gTwistShaders[TWISTSHADER_SCREEN_DISTORT], 0, aDistort);  // slot 0x108, 0x85e7d0
	TargetHold aHold;
	if (mHeatWaves.empty()) {
		// The rectangles over the bombs' cells (the board's +0x14cc): the screen so far drawn back over each through
		// the shader (DistortQuadA), then the result over each again plain (DistortQuadB)
		DDImage* aScreen = gD3DInterface->HoldScreen(&aHold, "DistortQuadA");
		gD3DInterface->SetTexture(1, aHeight);
		int aCount = 0;
		if (gBoard != nullptr)
			aCount = (int)gBoard->mHazeRects.size();
		int i = 0;
		if (0 < aCount) {
			do {
				const Board::HazeRect& aHaze = gBoard->mHazeRects[i];
				double aRect[4] = {(double)aHaze.mLeft, (double)aHaze.mTop, (double)(aHaze.mRight - aHaze.mLeft),
					(double)(aHaze.mBottom - aHaze.mTop)};
				DrawDistortSource(this, aScreen, aRect, aRect, Color(255, 255, 255, 64), 1.0f);
				i++;
			} while (i < aCount);
		}
		gD3DInterface->ReleaseTarget(&aHold);
		aScreen = gD3DInterface->HoldScreen(&aHold, "DistortQuadB");
		i = 0;
		gD3DInterface->SetPixelShader(nullptr);
		if (0 < aCount) {
			do {
				const Board::HazeRect& aHaze = gBoard->mHazeRects[i];
				double aRect[4] = {(double)aHaze.mLeft, (double)aHaze.mTop, (double)(aHaze.mRight - aHaze.mLeft),
					(double)(aHaze.mBottom - aHaze.mTop)};
				DrawDistortSource(this, aScreen, aRect, aRect, Color(255, 255, 255, 255), 1.0f);
				i++;
			} while (i < aCount);
		}
		gD3DInterface->SetTexture(1, nullptr);
		gD3DInterface->ReleaseTarget(&aHold);
	} else {
		DDImage* aScreen = gD3DInterface->HoldScreen(&aHold, "DistortFull");
		gD3DInterface->SetTexture(1, aHeight);
		double aRect[4] = {0.0, 0.0, (double)mWidth, (double)mHeight};
		DrawDistortRect(aScreen, aRect, Color(255, 255, 255, 64), 1.0f);
		gD3DInterface->SetTexture(1, nullptr);
		gD3DInterface->ReleaseTarget(&aHold);
	}
	gD3DInterface->SetPixelShader(nullptr);
}

HeatWave* EffectsManager::AddHeatWave(float theX, float theY, float theStrength)
{
	// 0x6f5a46 makes it; the end size is the app's scale times 400
	HeatWave aWave;
	aWave.mKind = 0;
	aWave.mGrowth = 0.025f;
	aWave.mOpacity = 1.0f;
	aWave.mStartSize = 0;
	aWave.mEndSize = (float)((double)gApp->mScale * 400.0);
	aWave.mProgress = 0;
	aWave.mX = (double)theX;
	aWave.mY = (double)theY;
	aWave.mEndSize = aWave.mEndSize * theStrength;
	aWave.mGrowth = (float)((double)aWave.mGrowth * 0.5);
	mHeatWaves.push_back(aWave);
	return &mHeatWaves.back();
}

void EffectsManager::AddBombFlash(float theX, float theY, const Color& theColor)
{
	// Smoke flying out
	int aCount = 30;
	do {
		Effect* anEffect = AllocEffect(26);
		float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
		float aBase = (float)((double)gApp->mScale * 0.0);
		float aRange = (float)((double)gApp->mScale * 4.0);
		float aSpeed = fabsf(RandSigned()) * aRange + aBase;
		anEffect->mDX = cosf(anAngle) * aSpeed;
		anEffect->mDY = sinf(anAngle) * aSpeed;
		anEffect->mX = (float)((double)cosf(anAngle) * 25.0 + (double)theX);
		anEffect->mY = (float)((double)sinf(anAngle) * 25.0 + (double)theY);
		anEffect->mIsAdditive = false;
		anEffect->mScale = (float)((double)fabsf(RandSigned()) + 0.10000000149011612);
		anEffect->mDScale = 0.02f;
		AddEffect(anEffect);
		aCount--;
	} while (aCount != 0);
	// Drips thrown up
	aCount = 30;
	do {
		Effect* anEffect = AllocEffect(30);
		float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
		float aBase = gApp->mScale;
		float aRange = (float)((double)gApp->mScale * 5.0);
		float aSpeed = fabsf(RandSigned()) * aRange + aBase;
		anEffect->mDX = cosf(anAngle) * aSpeed;
		anEffect->mDY = (float)((double)(sinf(anAngle) * aSpeed) - 1.5);
		anEffect->mX = (float)((double)cosf(anAngle) * 25.0 + (double)theX);
		anEffect->mY = (float)((double)sinf(anAngle) * 25.0 + (double)theY);
		anEffect->mIsAdditive = false;
		anEffect->mScale = (float)((double)fabsf(RandSigned()) * 0.20000000298023224 + 0.6000000238418579);
		anEffect->mDScale = -0.01f;
		anEffect->mAlpha = 0.6f;
		anEffect->mColor = Color(0xaaaacc);
		AddEffect(anEffect);
		aCount--;
	} while (aCount != 0);
	// Six puffs of steam, one each sixth of the way round
	for (int i = 0; i < 6; i++) {
		Effect* anEffect = AllocEffect(29);
		float anAngle = (float)((double)RandSigned() * 0.20000000298023224 + (double)i * 3.1415927410125732 / 3.0);
		float aBase = (float)((double)gApp->mScale * 6.0);
		float aRange = gApp->mScale + gApp->mScale;
		float aSpeed = fabsf(RandSigned()) * aRange + aBase;
		anEffect->mBlend = theX;
		anEffect->mRateFrom = theY;
		anEffect->mDX = cosf(anAngle) * aSpeed;
		anEffect->mDY = sinf(anAngle) * aSpeed;
		anEffect->mX = (float)((double)cosf(anAngle) * 25.0 + (double)theX);
		anEffect->mY = (float)((double)sinf(anAngle) * 25.0 + (double)theY);
		anEffect->mIsAdditive = false;
		anEffect->mScale = (float)((double)fabsf(RandSigned()) * 0.10000000149011612 + 2.5);
		anEffect->mDScale = -0.08f;
		AddEffect(anEffect);
	}
	// Shards of the gem's colour
	aCount = 50;
	do {
		Effect* anEffect = AllocEffect(16);
		anEffect->mColor = theColor;
		Rand();  // (both draws are thrown away)
		fabsf(RandSigned());
		double aDir = (double)RandSigned() * 3.14159;
		int aReach = Ftol((double)gApp->mScale * 48.0);
		int aDist = Ftol((double)RandSigned() * (double)aReach);
		float aDistF = (float)aDist;
		anEffect->mX = (float)((double)Ftol(std::cos(aDir) * (double)aDistF) + (double)theX);
		float aY = (float)((double)Ftol(std::sin(aDir) * (double)aDistF) + (double)theY);
		anEffect->mY = aY;
		float anOut = (float)std::atan2((double)(float)(aY - theY), (double)(float)(anEffect->mX - theX));  // 0x4c1b70
		double anAngle = (double)RandSigned() * 0.3 + (double)anOut;
		float aBase = (float)((double)gApp->mScale * 4.5);
		float aRand = fabsf(RandSigned());
		float aRange = (float)((double)gApp->mScale * 1.5);
		float aSpeed = aRange * aRand + aBase;
		anEffect->mDX = (float)(std::cos(anAngle) * (double)aSpeed);
		float aLift = (float)((double)gApp->mScale * -2.0);
		anEffect->mDY = (float)(std::sin(anAngle) * (double)aSpeed + (double)aLift);
		anEffect->mVelocityDamping = (float)((double)RandSigned() * 0.005 + 0.99);
		anEffect->mAngle = (float)anAngle;
		anEffect->mDAngle = (float)((double)RandSigned() * 0.05000000074505806);
		anEffect->mGravity = 0.06f;
		anEffect->mBlend = (float)((double)RandSigned() * 6.2831854820251465);
		anEffect->mRateFrom = (float)((double)RandSigned() * 6.2831854820251465);
		anEffect->mRateTo = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
		anEffect->mTumbleTurn = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
		float aFade = fabsf(RandSigned());
		anEffect->mDAlpha = (float)(((double)(aFade + aFade) + 4.0) * -0.0025);
		AddEffect(anEffect);
		aCount--;
	} while (aCount != 0);
	// With no hyperspace behind the board (+0x200c) a heat wave of half the strength, its growth (+0x28) doubled
	// (no Direct3D test: 2D mode adds it too, and only ages it)
	if (gBoard->mHyperspace == nullptr) {
		HeatWave* aWave = AddHeatWave(theX, theY, 0.5f);
		aWave->mGrowth = aWave->mGrowth + aWave->mGrowth;
	}
}
