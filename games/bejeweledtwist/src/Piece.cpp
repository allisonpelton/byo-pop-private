// Sexy::Piece (0x728c19-0x72fc8e): a gem on the board
#include "Piece.h"

#include "Board.h"
#include "Buffer.h"
#include "EffectsManager.h"
#include "Font.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "GraphicsRecorder.h"
#include "Image.h"
#include "Profile.h"
#include "Res.h"
#include "Shockwave.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "TriVertex.h"
#include "X86Math.h"

#include <cmath>
#include <cstring>

using namespace Sexy;

int Sexy::gNextPieceId = 0;

namespace {

// 0x84f678
const float kTwoPi = 6.2831855f;

// 0x7e7dc8: for each colour, a gem's outline radius at 256 angles (read from the exe: data of the game)
float GemRadius(int theColor, int theIndex)
{
	int aBits = GameInt(GTB(0x7e7dc8, 7168) + (theColor * 256 + theIndex) * 4);
	float aRadius;
	std::memcpy(&aRadius, &aBits, sizeof(aRadius));
	return aRadius;
}

}  // namespace

Piece::Piece()
{
	mFadeColors[0] = mFadeColors[1] = mFadeColors[2] = mFadeColors[3] = Color(0, 0, 0, 0);
	mFadeTargets[0] = mFadeTargets[1] = mFadeTargets[2] = mFadeTargets[3] = Color(0, 0, 0, 0);
	if (gNextPieceId != -1) {
		mId = gNextPieceId;
		gNextPieceId++;
	}
	gApp->mBoard->AddPieceId(mId, this);
	for (int i = 0; i < 4; i++)
		mHaloAngles[i] = (float)(Rand() % 629) * 0.01f;
	mUnknown4E8 = RandSigned() * 2.0f;
	mColorCycle.mColors.clear();
	mColorCycle.mColors.push_back(Color(255, 0, 0));
	mColorCycle.mColors.push_back(Color(255, 0, 255));
	mColorCycle.mColors.push_back(Color(255, 255, 255));
	mColorCycle.mColors.push_back(Color(255, 0, 255));
	// 2, or 8 with Direct3D
	mUnknown4F8 = 2;
}

Piece::~Piece()
{
	gBoard->RemovePieceId(mId);
	delete mCountImage;
	delete mCountImage2;
	if (mUnknown528 != nullptr)  // never reached: +0x528 is only written 0 (0x72c1fa, 0x72c67b) or copied (0x6bc62e)
		NotPorted("Piece +0x528's object (deleted through its slot 1)");
}

void Piece::SetFlag(int theFlag, bool on)
{
	unsigned int aBit = 1u << theFlag;
	if (!on) {
		mFlags &= ~aBit;
	} else {
		mFlags |= aBit;
		if (theFlag == FLAG_ELECTRO || theFlag == FLAG_POWER) {
			mUnknown3A4[0] = 0;
			mUnknown3A4[1] = 0;
		}
	}
}

void Piece::ClearSpecial()
{
	mUnknownD0 = 0;
	mUnknown29C = 0;
	mFlags = 0;
	mBombCount = 0;
	mCanMove = true;
	if (mColor < 0)
		mColor = mOrigColor;
	mUnknown4FC = 0;
}

bool Piece::CanMove() const
{
	return mCanMove && !mBreaking && mPendingKind == 0;
}

void Piece::SetColor(int theColor)
{
	mUnknown3A4[1] = -1;
	mUnknown3A4[0] = -1;
	mColor = theColor;
}

bool Piece::IsSpecial()
{
	return IsFlagSet(FLAG_POWER) || IsFlagSet(FLAG_ELECTRO) || IsFlagSet(FLAG_7) || IsFlagSet(FLAG_6) ||
		IsFlagSet(FLAG_FRUIT);
}

Effect* Piece::AddSpark(bool theBig, float theSpeed, float theAngle)
{
	if (gBoard->IsSparkBlocked())
		return nullptr;
	Effect* anEffect = gBoard->mFXManager->AllocEffect(26);
	float aScale = gApp->mScale;
	float aSpeed = (std::fabs(RandSigned()) * (aScale * 0.05f) + aScale * 0.25f) * theSpeed;
	if (IsFlagSet(FLAG_DEVIL)) {
		// port: the original leaves the effect allocated (docs/NOTES.md, deviations)
		delete anEffect;
		return nullptr;
	}
	float anAngle = theAngle;
	anEffect->mX = (float)((double)mOffsetX + mX + (double)(gGemWidth / 2) +
		(double)gGemWidth * (double)std::cos(theAngle) * ((double)RandSigned() * 0.1f + 0.6f));
	anEffect->mIsAdditive = false;
	anEffect->mY = (float)((double)mOffsetY + mY + (double)(gGemHeight / 2) +
		(double)gGemHeight * (double)std::sin(theAngle) * ((double)RandSigned() * 0.1f + 0.6f));
	if (!theBig) {
		anEffect->mScale = 0.1f;
		anEffect->mDScale = 0.03f;
		anEffect->mMaxScale = 2.0f;
	} else {
		anEffect->mType = 0;
		anEffect->mAlpha = 0.1f * theSpeed;
		anEffect->mDAlpha = -0.01f;
		anEffect->mDScale = 0.05f;
		anEffect->mScale = 1.5f;
		anEffect->mMaxScale = 1.5f;
		anEffect->mUnknown9C = 0.25f;
		anEffect->mUnknownA0 = -0.01f;
		anEffect->mUnknownA4 = 0.95f;
		anEffect->mColor = (Rand() % 2 == 0) ? Color(0, 0, 0) : Color(255, 0, 0);
		anAngle = RandSigned() * 0.3f + theAngle;
		aSpeed *= 4.0f;
	}
	anEffect->mDX = std::cos(anAngle) * aSpeed;
	anEffect->mDY = std::sin(anAngle) * aSpeed;
	if (!theBig)
		gBoard->mFXManager->AddEffect(anEffect);
	// Without Direct3D the effects move with the board's shake
	anEffect->mX = gBoard->mShakeX + anEffect->mX;
	anEffect->mY = gBoard->mShakeY + anEffect->mY;
	return anEffect;
}

void Piece::AddShards(float theSpeed, bool theSecondManager)
{
	EffectsManager* aManager = theSecondManager ? gBoard->mFXManager2 : gBoard->mFXManager;
	Effect* anEffect = aManager->AllocEffect(10);
	float anAngle = std::fabs(RandSigned() * kTwoPi);
	float aDist = std::fabs((float)((double)RandSigned() * (double)gGemWidth / 3.5));
	float aYOffset = 0.0f;
	if (mColor == 4)
		aYOffset = 15.0f;
	else if (mColor == 6)
		aYOffset = -12.0f;
	anEffect->mX =
		(float)((double)(gGemWidth / 2) + (double)mX + (double)cosf(anAngle) * (double)theSpeed * (double)aDist);
	anEffect->mY = (float)((double)(gGemHeight / 2) + (double)mY +
						   (double)sinf(anAngle) * (double)theSpeed * (double)aDist + (double)aYOffset);
	double aPush = (double)theSpeed * (double)theSpeed * 0.02500000037252903;
	anEffect->mDX = (float)((double)cosf(anAngle) * aPush * (double)aDist);
	anEffect->mDY = (float)((double)sinf(anAngle) * aPush * (double)aDist);
	anEffect->mScale = (float)((double)theSpeed * 0.5);
	anEffect->mGravity = 0;
	anEffect->mDAlpha = -0.005f;
	anEffect->mUnknown9C = 0;
	anEffect->mUnknownA0 = RandSigned();
	anEffect->mUnknownA4 = RandSigned();
	aManager->AddEffect(anEffect);
}

void Piece::AddDust()
{
	Effect* anEffect = gBoard->mFXManager->AllocEffect(12);
	float anAngle = std::fabs(RandSigned() * kTwoPi);
	float aDist = std::fabs((float)((double)RandSigned() * (double)gGemWidth * 0.5));
	anEffect->mX = std::cos(anAngle) * aDist + (float)(gGemWidth / 2) + mX;
	anEffect->mY = std::sin(anAngle) * aDist + (float)(gGemHeight / 2) + mY;
	anEffect->mDX = (float)((double)std::cos(anAngle) * (double)(gApp->mScale * 0.1f) * aDist);
	anEffect->mDY = (float)((double)std::sin(anAngle) * (double)(gApp->mScale * 0.1f) * aDist);
	anEffect->mScale = 0.75f;
	anEffect->mGravity = gApp->mScale * 0.01f;
	anEffect->mAlpha = 0.5f;
	anEffect->mDAlpha = -0.0025f;
	gBoard->mFXManager->AddEffect(anEffect);
}

void Piece::GetCounterPos(int* theX, int* theY, float theRise)
{
	int aHeight = FONT_COUNTER->GetHeight();
	int anAscent = FONT_COUNTER->GetAscent();
	*theX = Ftol((double)gGemWidth * 0.5 + mX);
	*theY = Ftol(((double)gGemHeight * 0.800000011920929 + mY) - (1.0 - theRise) * (double)(aHeight - anAscent));
}

float Piece::GetRadius(float theAngle)
{
	if (theAngle > kTwoPi) {
		do {
			theAngle = theAngle - kTwoPi;
		} while (kTwoPi <= theAngle);
	}
	for (; theAngle < 0.0f; theAngle = theAngle + kTwoPi) {
	}
	float aPos = (theAngle * 256.0f) / kTwoPi;
	int anIndex = Ftol(aPos);
	if (anIndex < 0)
		return GemRadius(mColor, 0);
	if (anIndex < 255) {
		aPos = aPos - (float)anIndex;
		return GemRadius(mColor, anIndex + 1) * aPos + (1.0f - aPos) * GemRadius(mColor, anIndex);
	}
	return GemRadius(mColor, 255);
}

void Piece::Load(Buffer* theBuffer)
{
	int aColor = theBuffer->ReadShort();
	SetColor(aColor);
	if (aColor == -1)
		return;
	mOrigColor = theBuffer->ReadShort();
	unsigned int aFlags = (unsigned int)theBuffer->ReadLong();
	unsigned int aBit = 1;
	int aFlag = 0;
	do {
		if ((aFlags & aBit) != 0)
			SetFlag(aFlag, true);
		aBit = aBit * 2;
		aFlag++;
	} while (aBit != 0);
	mBombCount = theBuffer->ReadShort();
	mLockCount = theBuffer->ReadShort();
	mUnknown51D = theBuffer->ReadByte() != 0;
	mUnknown4FC = theBuffer->ReadShort();
	mUnknown2E0 = theBuffer->ReadByte() != 0;
	mCanMove = theBuffer->ReadByte() != 0;
	gApp->mBoard->AddPieceId(mId, this);
	mUnknown116 = theBuffer->ReadByte() != 0;
	mBreaking = theBuffer->ReadByte() != 0;
	mUnknownD0 = (float)theBuffer->ReadShort();
	mUnknownD4 = (float)theBuffer->ReadShort();
}

void Piece::Save(Buffer* theBuffer)
{
	theBuffer->WriteShort((short)mColor);
	theBuffer->WriteShort((short)mOrigColor);
	theBuffer->WriteLong((long)mFlags);
	theBuffer->WriteShort((short)mBombCount);
	theBuffer->WriteShort((short)mLockCount);
	theBuffer->WriteByte(mUnknown51D);
	theBuffer->WriteShort((short)mUnknown4FC);
	theBuffer->WriteByte(mUnknown2E0);
	theBuffer->WriteByte(mCanMove);
	theBuffer->WriteByte(mUnknown116);
	theBuffer->WriteByte(mBreaking);
	theBuffer->WriteShort((short)Ftol(mUnknownD0));
	theBuffer->WriteShort((short)Ftol(mUnknownD4));
}

void Piece::MakeElectro(float theDelay)
{
	if (theDelay == 0.0f) {
		bool wasSet = IsFlagSet(FLAG_1);
		ClearSpecial();
		SetFlag(FLAG_ELECTRO, true);
		if (wasSet)
			SetFlag(FLAG_1, true);
		if (gBoard->mUnknown25A4 && mX != 0.0f) {
			int i = 10;
			do {
				Effect* anEffect = gBoard->mFXManager->AllocEffect(17);
				anEffect->mX = (float)(gGemWidth / 2) + mX;
				anEffect->mY = (float)(gGemHeight / 2) + mY;
				gBoard->mFXManager->AddEffect(anEffect);
				i--;
			} while (i != 0);
			if (!gBoard->mUnknown37F8)
				gBoard->PlayPieceSound(this, SOUND_ELECTROGEM_CREATED, 0, 0, 1);
		}
		if (!gBoard->mUnknown37F8)
			mJustMade = 2;
	} else {
		mPendingKind = 1 << FLAG_ELECTRO;
		mPendingTime = theDelay;
		mPendingDY = 0;
		mPendingDX = 0;
	}
	mUnknown4FC = 0;
}

void Piece::Make7(float theDelay)
{
	if (theDelay == 0.0f) {
		ClearSpecial();
		SetFlag(FLAG_7, true);
		mJustMade = 2;
	} else {
		mPendingKind = 1 << FLAG_7;
		mPendingTime = theDelay;
		mPendingDY = 0;
		mPendingDX = 0;
	}
	mUnknown4FC = 0;
}

void Piece::MakePower(float theDelay)
{
	if (theDelay == 0.0f) {
		bool wasSet = IsFlagSet(FLAG_1);
		ClearSpecial();
		SetFlag(FLAG_POWER, true);
		if (wasSet)
			SetFlag(FLAG_1, true);
		mColor = mColor % 7;
		if (!gBoard->mUnknown37F8) {
			gBoard->PlayPieceSound(this, SOUND_POWERGEM_CREATED, 0, 0, 1);
			mJustMade = 2;
		}
	} else {
		mPendingKind = 1 << FLAG_POWER;
		mPendingTime = theDelay;
		mPendingDY = 0;
		mPendingDX = 0;
	}
	mUnknown4FC = 0;
}

void Piece::Make6(float theDelay)
{
	if (theDelay == 0.0f) {
		bool wasSet = IsFlagSet(FLAG_1);
		ClearSpecial();
		SetFlag(FLAG_6, true);
		if (wasSet)
			SetFlag(FLAG_1, true);
		mJustMade = 2;
	} else {
		mPendingKind = 1 << FLAG_6;
		mPendingTime = theDelay;
		mPendingDY = 0;
		mPendingDX = 0;
	}
	mUnknown4FC = 0;
}

// One step of a corner's colour towards its target (Draw)
static void StepColor(int* theValue, int theGoal)
{
	int aValue = *theValue;
	if (aValue < theGoal) {
		aValue += 12;
		if (theGoal < aValue)
			aValue = theGoal;
		*theValue = aValue;
	} else if (theGoal < aValue) {
		aValue -= 12;
		if (aValue < theGoal)
			aValue = theGoal;
		*theValue = aValue;
	}
}

void Piece::Draw(Graphics* g, bool theForce)
{
	if (!((float)-gGemHeight < mY))
		return;
	Board* aBoard = gApp->mBoard;
	float aBoardAlpha = aBoard->mUnknown3924 * aBoard->mPieceAlpha;
	int aSrcX = 0;
	int aSrcY = 0;
	float anAlpha;
	if (0.0f <= mAlpha)
		anAlpha = mAlpha;
	else
		anAlpha = 0.0f;
	if (anAlpha <= 1.0f) {
		if (0.0f <= mAlpha)
			anAlpha = mAlpha;
		else
			anAlpha = 0.0f;
	} else {
		anAlpha = 1.0f;
	}
	mAlpha = anAlpha;
	if (!theForce && mUnknown14 == 0.0f) {
		if (!mDimmed)
			DrawBack(g);
		return;
	}
	if (aBoard->mGameMode == Board::MODE_PUZZLE && aBoard->mPuzzleImage != nullptr)
		return;

	bool isDevil = IsFlagSet(FLAG_DEVIL);
	if (!isDevil && mBreaking && mUnknown110 == 0 && mColor < 7 && !IsFlagSet(FLAG_COAL)) {
		// A gem breaking up: it shrinks through the gems drawn ahead and fades
		if (!IsFlagSet(FLAG_1) && !IsFlagSet(FLAG_COAL) && mBreakFrame < 7) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255, 255, 255 - (mBreakFrame * 255) / 7));
			int aQuarter = gGemWidth / 4;
			int aY = Ftol(mY) - aQuarter;
			int aX = Ftol(mX) - aQuarter;
			gGraphicsRecorder->DrawImage(g, gApp->mGemImages[mColor][mBreakFrame], aX, aY);
			g->SetColorizeImages(false);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		}
	} else {
		// port: the original reads the draw mode, colour and colorizing here and keeps none of them
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(aBoardAlpha * 255.0)));
		float aWidth = (float)gGemWidth;
		float aHeight = (float)gGemHeight;
		float aScale = mDrawScale;
		float aX = mShakeX + mX;
		float aY = mShakeY + mY;
		Color aColor(255, 255, 255);
		if (mDimmed)
			aColor = Color(128, 128, 128);
		bool isCoal = IsFlagSet(FLAG_COAL);
		if (!IsFlagSet(FLAG_1)) {
			Image* anImage;
			if (isCoal) {
				aColor = Color::White;
				if (mBreaking) {
					anImage = IMAGE_COAL_BREAK;
					int aFrame = mBreakFrame - 30;
					if (aFrame <= 0)
						aFrame = 0;
					aFrame = aFrame / 4;
					if (anImage->mNumCols * anImage->mNumRows - 1 < aFrame)
						aFrame = anImage->mNumCols * anImage->mNumRows - 1;
					aSrcX = IMAGE_COAL_BREAK->GetCelWidth() * aFrame;
					aWidth = (float)IMAGE_COAL_BREAK->GetCelWidth();
					aHeight = (float)IMAGE_COAL_BREAK->GetCelHeight();
				} else {
					anImage = gBoard->mUnknown1BF2 ? IMAGE_SNACKERS : IMAGE_COAL;
				}
			} else if (isDevil) {
				anImage = IMAGE_DEVILGEM;
				aWidth = (float)anImage->mWidth;
				aHeight = (float)anImage->mHeight;
				aY = (float)Ftol(gApp->mScale * 6.0) + aY;
			} else if (IsFlagSet(FLAG_BOMB)) {
				anImage = IMAGE_BOMBGEMS;
				aWidth = (float)anImage->GetCelWidth();
				aHeight = (float)anImage->GetCelHeight();
				aSrcX = Ftol((double)mColor * aWidth);
				aScale = 0.5f;
				aY = (float)(aY - aHeight * 0.0);
				// The bomb's glow, pulsing every 200 updates
				float anAngle = (float)((double)((unsigned int)mUpdateCnt % 200) / 200.0 * 3.1415927410125732);
				float aSin = sinf(anAngle);
				Color anOldColor = g->GetColor();
				g->SetColor(Color(255, 255, 255, Ftol(aSin * 255.0 * mAlpha)));
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
				int aGlowX = Ftol((double)mOffsetX + mX + mShakeX - Ftol(gApp->mScale + gApp->mScale));
				int aGlowY = Ftol((double)mOffsetY + mY + mShakeY + Ftol(gApp->mScale * 0.0));
				Image* aGlow = IMAGE_BOMBGLOWS_GLOW;
				int aGlowWidth = Ftol(aGlow->GetCelWidth() * 0.5);
				int aGlowHeight = Ftol(aGlow->GetCelHeight() * 0.5);
				Rect aRect(gGemWidth / 2 - aGlowWidth / 2 + aGlowX, gGemHeight / 2 - aGlowHeight / 2 + aGlowY,
					aGlowWidth, aGlowHeight);
				g->DrawImageCel(aGlow, aRect, mColor);
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColor(anOldColor);
				// port: without Direct3D the bomb drawn ahead at its size (with it, IMAGE_BOMBGEMS at half size while
				// the board's +0x268c is above 1.002)
				anImage = gApp->mBombImages[mColor];
				aWidth = (float)anImage->mWidth;
				aSrcX = 0;
				aSrcY = 0;
				aHeight = (float)anImage->mHeight;
				aScale = 1.0f;
			} else {
				anImage = IMAGE_GEMS;
				if (IsFlagSet(FLAG_FRUIT)) {
					anImage = IMAGE_FRUITS;
					aHeight = (float)anImage->GetCelHeight();
					aWidth = (float)anImage->GetCelWidth();
					aSrcX = Ftol((double)mColor * aWidth);
					if (mScale288 < 1.0f) {
						aScale = (float)(2.0 - mScale288);
						aScale = aScale * aScale;
					}
				} else {
					aScale = (float)(aScale * 0.5);
					// port: with Direct3D, times the board's +0x268c
					float aDrawnScale = aScale;
					if (!(0.501f < aDrawnScale)) {
						// The gem drawn ahead at the cell's size, its cell cut from the middle of the image
						aWidth = (float)gGemWidth;
						anImage = gApp->mGemImages[mColor][3];
						aHeight = (float)gGemHeight;
						aScale = aScale + aScale;
						aSrcX = gGemWidth / 4;
						aSrcY = gGemHeight / 4;
					} else {
						aHeight = (float)anImage->GetCelHeight();
						aWidth = (float)anImage->GetCelWidth();
						aSrcX = Ftol((double)mColor * aWidth);
					}
				}
			}

			if (anImage != nullptr && -1.0f < mUnknown14) {
				bool isFruit = IsFlagSet(FLAG_FRUIT);
				while (aScale <= 3.0f || 1.0f <= mScale288) {
					aX = mOffsetX + aX;
					aY = mOffsetY + aY;
					// A fruit growing in is drawn shrinking from larger
					if (mScale288 < 1.0f && isFruit) {
						aScale = (float)(aScale * 0.8500000238418579);
						if (aScale < 1.05f)
							aScale = 1.0f;
					}
					g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
					if (aBoard->mGameMode == Board::MODE_PUZZLE && !isCoal) {
						aSrcY = 0;
						aSrcX = 0;
						aColor = gGemColors[mColor];
						aWidth = (float)anImage->mWidth;
						aHeight = (float)anImage->mHeight;
					}
					aColor.mAlpha = Ftol(aColor.mAlpha * mAlpha);
					if (aScale == 1.0f) {
						g->SetColorizeImages(true);
						g->SetColor(aColor);
						Rect aSrcRect(aSrcX, aSrcY, Ftol(aWidth), Ftol(aHeight));
						int aDrawY = Ftol(aY - (aHeight - gGemHeight) * 0.5);
						int aDrawX = Ftol(aX - (aWidth - gGemWidth) * 0.5);
						gGraphicsRecorder->DrawImage(g, anImage, aDrawX, aDrawY, aSrcRect);
					} else {
						g->SetColorizeImages(true);
						g->SetColor(aColor);
						int aDrawWidth = Ftol(aScale * aWidth) & ~1;
						int aDrawHeight = Ftol(aScale * aHeight) & ~1;
						if (0.0f < gBoard->mUnknown23EC && aDrawWidth > gGemWidth) {
							// A gem grown over its neighbours: drawn once additively at half alpha, then again clipped
							// away from each neighbour that is grown too, and wrapped once up and once left
							Graphics aClipG(*g);
							Color aHalfColor = aColor;
							aHalfColor.mAlpha = Ftol(aColor.mAlpha * 0.5);
							g->SetColor(aHalfColor);
							g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
							Rect aSrcRect(aSrcX, aSrcY, Ftol(aWidth), Ftol(aHeight));
							int aDrawY = Ftol(aY - (double)((aDrawHeight - gGemHeight) / 2));
							int aDrawX = Ftol(aX - (double)((aDrawWidth - gGemWidth) / 2));
							gGraphicsRecorder->DrawImage(g, anImage, Rect(aDrawX, aDrawY, aDrawWidth, aDrawHeight),
								aSrcRect);
							bool aWrapUp = false;
							bool aWrapLeft = false;
							Piece* aPiece = gBoard->GetPieceAt(mRow - 1, mCol);
							if (aPiece != nullptr && 1.0f < aPiece->mDrawScale) {
								aClipG.ClipRect(Ftol(mX - (double)gGemWidth), Ftol(mY), gGemWidth * 3, gGemHeight * 2);
								aWrapUp = true;
							}
							aPiece = gBoard->GetPieceAt(mRow + 1, mCol);
							if (aPiece != nullptr && 1.0f < aPiece->mDrawScale)
								aClipG.ClipRect(Ftol(mX - (double)gGemWidth), Ftol(mY - (double)gGemHeight), gGemWidth * 3,
									gGemHeight * 2);
							aPiece = gBoard->GetPieceAt(mRow, mCol - 1);
							if (aPiece != nullptr && 1.0f < aPiece->mDrawScale) {
								aClipG.ClipRect(Ftol(mX), Ftol(mY - (double)gGemHeight), gGemWidth * 2, gGemHeight * 3);
								aWrapLeft = true;
							}
							aPiece = gBoard->GetPieceAt(mRow, mCol + 1);
							if (aPiece != nullptr && 1.0f < aPiece->mDrawScale)
								aClipG.ClipRect(Ftol(mX - (double)gGemWidth), Ftol(mY - (double)gGemHeight), gGemWidth * 2,
									gGemHeight * 3);
							aDrawY = Ftol(aY - (double)((aDrawHeight - gGemHeight) / 2));
							aDrawX = Ftol(aX - (double)((aDrawWidth - gGemWidth) / 2));
							gGraphicsRecorder->DrawImage(&aClipG, anImage, Rect(aDrawX, aDrawY, aDrawWidth, aDrawHeight),
								aSrcRect);
							aClipG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
							aClipG.SetColor(aHalfColor);
							if (aWrapUp) {
								aDrawY = Ftol(aY - (double)((aDrawHeight - gGemHeight) / 2) - gGemHeight);
								aDrawX = Ftol(aX - (double)((aDrawWidth - gGemWidth) / 2));
								gGraphicsRecorder->DrawImage(&aClipG, anImage,
									Rect(aDrawX, aDrawY, aDrawWidth, aDrawHeight), aSrcRect);
							}
							if (aWrapLeft) {
								aDrawY = Ftol(aY - (double)((aDrawHeight - gGemHeight) / 2));
								aDrawX = Ftol(aX - (double)((aDrawWidth - gGemWidth) / 2) - gGemWidth);
								gGraphicsRecorder->DrawImage(&aClipG, anImage,
									Rect(aDrawX, aDrawY, aDrawWidth, aDrawHeight), aSrcRect);
							}
						} else {
							Rect aSrcRect(aSrcX, aSrcY, Ftol(aWidth), Ftol(aHeight));
							int aDrawX = Ftol(aX - (double)((aDrawWidth - gGemWidth) / 2));
							int aDrawY = Ftol(aY - (double)((aDrawHeight - gGemHeight) / 2));
							gGraphicsRecorder->DrawImage(g, anImage, Rect(aDrawX, aDrawY, aDrawWidth, aDrawHeight),
								aSrcRect);
						}
					}

					// The glow over a gem
					float aGlow = mGlow < mGlow2 ? mGlow2 : mGlow;
					bool aGlowing = 1.0f < aGlow;
					if (!aGlowing) {
						aGlow = mGlow < mGlow2 ? mGlow2 : mGlow;
						aGlowing = 0.0f < aGlow;
					}
					if (aGlowing) {
						// port: the original also makes an orange colour here while mGlow2 is above 0, and drops it
						if (!isCoal) {
							g->SetColorizeImages(true);
							g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
							Rect aCelRect = IMAGE_GEM_GLOWS->GetCelRect(mColor);
							int aDrawY = Ftol(aY - (double)((aCelRect.mHeight - gGemHeight) / 2));
							int aDrawX = Ftol(aX - (double)((aCelRect.mWidth - gGemWidth) / 2));
							gGraphicsRecorder->DrawImageCel(g, IMAGE_GEM_GLOWS, aDrawX, aDrawY, mColor);
						}
					}
					if (!isFruit || mScale288 == 1.0f || aScale == 1.0f)
						break;
				}
			}
		}

		// The colours of the gem's four corners fade 12 a draw towards their targets
		if (!IsFlagSet(FLAG_7)) {
			for (int i = 0; i < 4; i++) {
				StepColor(&mFadeColors[i].mRed, mFadeTargets[i].mRed);
				StepColor(&mFadeColors[i].mGreen, mFadeTargets[i].mGreen);
				StepColor(&mFadeColors[i].mBlue, mFadeTargets[i].mBlue);
				StepColor(&mFadeColors[i].mAlpha, mFadeTargets[i].mAlpha);
			}
		}
	}

	if ((!mBreaking || 0 < mUnknown110) && mUnknown14 == 0.0f)
		DrawFront(g);

	// A bomb's (or a devil gem's) counter, turning while +0xd0 is outside 5 to 15
	if ((IsFlagSet(FLAG_BOMB) || (IsFlagSet(FLAG_DEVIL) && !mBreaking)) &&
		(mUnknownD0 < 5.0f || 15.0 < mUnknownD0) && mCountImage != nullptr) {
		float anAngle = (float)(mUnknownD0 * kTwoPi / 20.0);
		bool isDevilCount = IsFlagSet(FLAG_DEVIL);
		float aSize = (float)((mCurve1A0.GetOutVal() + 1.0) * (mDrawScale * 0.5));
		int aWidth = Ftol(mCountImage->mWidth * aSize);
		if (!isDevilCount)
			aWidth = Ftol(cosf(anAngle) * aWidth);
		int aHeight = Ftol(mCountImage->mHeight * aSize);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(true);
		int aX = Ftol((double)mOffsetX + mX + mShakeX + (gGemWidth / 2) - (aWidth / 2) +
			Ftol(gApp->mScale * 0.0) * mDrawScale);
		int aY = Ftol((double)mOffsetY + mY + mShakeY + (gGemHeight / 2) - aHeight * 0.44999998807907104);
		int anAlpha255 = Ftol(mAlpha * 255.0);
		g->SetColor(Color(255, 255, 255, Ftol(255.0 * gBoard->mUnknown21B0)));
		float aSin = sinf(anAngle);
		aX = Ftol(gGemWidth * aSin * 0.5 + aX);
		if (isDevil)
			aY += Ftol(gApp->mScale * -7.0);
		int aPulse = Ftol(mCountPulse.GetOutVal() * anAlpha255);
		Image* aDangerGlow = IMAGE_BOMBGLOWS_DANGERGLOW;
		int aGlowWidth = Ftol(aDangerGlow->GetCelWidth() * aSize * 2.0);
		int aGlowHeight = Ftol(aDangerGlow->GetCelHeight() * aSize * 2.0);
		int aGlowX = Ftol((double)(gGemWidth / 2) + mX - (aGlowWidth / 2));
		int aGlowY = Ftol((double)(gGemHeight / 2) + mY - (aGlowHeight / 2));
		if (!isDevil) {
			g->SetColor(Color(255, 255, 255, Ftol(aPulse * mAlpha)));
			g->DrawImageCel(aDangerGlow, Rect(aGlowX, aGlowY, aGlowWidth, aGlowHeight), mColor);
		}
		g->SetColor(Color(255, 255, 255, Ftol(aPulse * mAlpha)));
		g->DrawImage(mCountImage2, aX, aY, aWidth, aHeight);
		g->SetColor(Color(255, 255, 255, Ftol((255 - aPulse) * mAlpha)));
		g->DrawImage(mCountImage, aX, aY, aWidth, aHeight);
	}
}

void Piece::DrawBack(Graphics* g)
{
	Board* aBoard = gApp->mBoard;
	if (gBoard->mGameMode == Board::MODE_PUZZLE && gBoard->mPuzzleImage != nullptr) {
		// The puzzle's picture: the piece shows the part of it its id was cut for, lit (by the board's +0x2038) while
		// it is back in that place
		Image* aPuzzle = aBoard->mPuzzleImage;
		int aCellWidth = aPuzzle->mWidth / aBoard->mCols;
		int aCellHeight = aPuzzle->mHeight / aBoard->mRows;
		int aRow = mId / aBoard->mCols;
		int aCol = mId % aBoard->mCols;
		Rect aSrcRect(aCol * aCellWidth, aRow * aCellHeight, aCellWidth, aCellHeight);
		Rect aDestRect(Ftol(mX), Ftol(mY), gGemWidth, gGemHeight);
		gGraphicsRecorder->DrawImage(g, aPuzzle, aDestRect, aSrcRect);
		if (mCol == aCol && mRow == aRow) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColor(Color(255, 255, 255, Ftol(gBoard->mUnknown2038 * 128.0)));
			int aWidth = gGemWidth;
			int aHeight = gGemHeight;
			int aY = Ftol(mY);
			int aX = Ftol(mX);
			g->FillRect(aX, aY, aWidth, aHeight);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			g->SetColor(Color::White);
		}
		return;
	}

	if (IsFlagSet(FLAG_LOCKED)) {
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mAlpha * 255.0)));
		int anOffsetY = Ftol(gApp->mScale * (double)GetImageOffsetY(IMAGE_GEMLOCK_BACKFACE_ID));
		int anOffsetX = GetImageOffsetX(IMAGE_GEMLOCK_BACKFACE_ID);
		int aY = Ftol(mY + (double)anOffsetY);
		anOffsetX = Ftol(gApp->mScale * (double)anOffsetX);
		int aX = Ftol((double)anOffsetX + mX);
		gGraphicsRecorder->DrawImage(g, IMAGE_GEMLOCK_BACKFACE, aX, aY);
	}
	if (IsFlagSet(FLAG_BOMB)) {
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, Ftol(mAlpha * 128.0)));
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		// port: with Direct3D two black halos (IMAGE_BLACKHALO_2) turn behind a bomb at mHaloAngles[0] and [1]
		g->SetColor(Color(255, 255, 255, 255));
		if (0.0f < mFruitGlow) {
			// The bomb's shockwave image of its colour, growing as it fades
			Image* aWave = GetImageById(IMAGE_GEM_SHOCKWAVE_0_ID + mColor);
			float aScale = 2.0f - mFruitGlow;
			Color aColor(128, 0, 128, Ftol(mFruitGlow * 255.0));
			g->SetColor(aColor);
			DrawImageCentered(g, aWave, mShakeX + mX + (float)(gGemWidth / 2), mShakeY + mY + (float)(gGemHeight / 2), 0,
				aScale, aScale);
		}
	}
	if (!IsFlagSet(FLAG_1) && !IsFlagSet(FLAG_7) && IsFlagSet(FLAG_DEVIL)) {
		int anAlpha = Ftol((double)mAlpha * mDevilGlow * 255.0);
		Image* aGlow = IMAGE_DEVILGEM_GLOW;
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColor(Color(255, 255, 255, anAlpha));
		int aY = Ftol((double)(gGemHeight / 2) + mY - (double)(aGlow->mHeight / 2));
		int aX = Ftol((double)(gGemWidth / 2) + mX - (double)(aGlow->mWidth / 2));
		gGraphicsRecorder->DrawImage(g, aGlow, aX, aY);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x85d9b0 and 0x85d930: an electro gem's arcs by its colour, plain and (for an arc with its +0x10 set) bright
static const Color kArcColors[8] = {Color(255, 96, 96), Color(255, 255, 255), Color(128, 255, 64), Color(225, 225, 128),
	Color(225, 128, 225), Color(255, 160, 64), Color(48, 160, 255), Color(0, 0, 0)};
static const Color kArcBrightColors[8] = {Color(255, 200, 200), Color(255, 250, 255), Color(220, 255, 128),
	Color(255, 255, 200), Color(255, 128, 255), Color(255, 160, 64), Color(80, 200, 255), Color(0, 0, 0)};

// 0x72b40b: a textured beam theWidth wide from (theX0, theY0) to (theX1, theY1), additively in the colour halfway
// between theColor0 and theColor1: a strip of six triangles, the texture's left half on the start's cap and the
// segment, its right half on the end's cap
static void DrawBeam(Graphics* g, Image* theImage, float theX0, float theY0, float theX1, float theY1, float theWidth,
	const Color& theColor0, const Color& theColor1)
{
	float aDX = theX1 - theX0;
	float aDY = theY1 - theY0;
	// port: the original also takes the length (sqrtf) and the angle's sine here and keeps neither
	float anAngle = (float)atan2((double)aDY, (double)aDX);
	float aCos = cosf(anAngle);
	float aNormal = (float)(anAngle + 1.5707963705062866);
	float aNormalCos = cosf(aNormal);
	float aNormalSin = sinf(aNormal);
	float aStartX = theX0 - aCos * theWidth;
	float anEndX = aCos * theWidth + theX1;
	double anOffsetY = aNormalSin * theWidth;
	float aStartY = (float)(theY0 + anOffsetY);
	double anOffsetX = aNormalCos * theWidth;
	// The strip's eight corners (x, y, u, v); the original also gives each its colour for Direct3D
	float aVerts[8][4] = {
		{(float)(aStartX + anOffsetX), aStartY, 0, 0},
		{(float)(aStartX - anOffsetX), (float)(theY0 - anOffsetY), 0, 1},
		{(float)(theX0 + anOffsetX), aStartY, 0.5f, 0},
		{(float)(theX0 - anOffsetX), (float)(theY0 - anOffsetY), 0.5f, 1},
		{(float)(theX1 + anOffsetX), (float)(theY1 + anOffsetY), 0.5f, 0},
		{(float)(theX1 - anOffsetX), (float)(theY1 - anOffsetY), 0.5f, 1},
		{(float)(anEndX + anOffsetX), (float)(theY1 + anOffsetY), 1, 0},
		{(float)(anEndX - anOffsetX), (float)(theY1 - anOffsetY), 1, 1},
	};
	// port: Direct3D draws the strip at once with the corners' colours
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	int aTriangle = 6;
	float (*aVert)[4] = aVerts;
	do {
		g->SetColor(Color((theColor0.mRed + theColor1.mRed) / 2, (theColor0.mGreen + theColor1.mGreen) / 2,
			(theColor0.mBlue + theColor1.mBlue) / 2, (theColor0.mAlpha + theColor1.mAlpha) / 2));
		TriVertex aV1(aVert[0][0], aVert[0][1], aVert[0][2], aVert[0][3]);
		TriVertex aV2(aVert[1][0], aVert[1][1], aVert[1][2], aVert[1][3]);
		TriVertex aV3(aVert[2][0], aVert[2][1], aVert[2][2], aVert[2][3]);
		g->DrawTriangleTex(theImage, aV1, aV2, aV3);
		aTriangle--;
		aVert++;
	} while (aTriangle != 0);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

void Piece::DrawFront(Graphics* g)
{
	Board* aBoard = gApp->mBoard;
	float aPieceAlpha = aBoard->mPieceAlpha;
	if ((!mBreaking || 0 < mUnknown110) && !mBlasted) {
		// The board's shine sweeping over the gems
		if (0.0f < aBoard->mShineAmount && mColor <= 6 && !mBreaking && mUnknownD0 == 0.0f && !mDimmed &&
			!IsFlagSet(FLAG_COAL) && !IsFlagSet(FLAG_DEVIL) && !IsFlagSet(FLAG_FRUIT)) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			// port: the original leaves the image unset when FLAG_1 is set; the port draws nothing then
			Image* aShineImage = nullptr;
			int aShine = 0;
			do {
				// port: with Direct3D each of the nine shines (+0x50 to +0x70) is drawn; without it their sum, once, as
				// the last
				float anAmount = 0.0f;
				for (int i = 0; i < 8; i++)
					anAmount = (float)(mUnknown50[i] * 0.10000000149011612 + anAmount);
				anAmount = (float)(mUnknown70 * 0.5 + anAmount);
				if (anAmount < 0.05)
					anAmount = 0.0f;
				aShine = 8;
				if (0.01 < anAmount) {
					if (1.0 < anAmount)
						anAmount = 1.0f;
					double aLevel = (double)aBoard->mShineAmount * aPieceAlpha * anAmount * 255.0;
					if (255.0 <= aLevel)
						aLevel = 255.0;
					int aGrey = Ftol(aLevel);
					g->SetColor(Color(aGrey, aGrey, aGrey, Ftol(255.0 * mAlpha)));
					int aWidth = gGemWidth;
					int aHeight = gGemHeight;
					float aX = (float)(mShakeX + mX + (gGemWidth / 2));
					float aY = (float)(mShakeY + mY + (gGemHeight / 2));
					if (gBoard->mGameMode == Board::MODE_PUZZLE) {
						aShineImage = nullptr;
					} else {
						if (!IsFlagSet(FLAG_1))
							aShineImage = gBoard->mShineImage;
						if (aShineImage != nullptr) {
							Rect aSrcRect = aShineImage->GetCelRect(aShine, mColor);
							aWidth = Ftol(aWidth * mDrawScale);
							aHeight = Ftol(aHeight * mDrawScale);
							int aDrawY = Ftol(aY) - gGemHeight / 2;
							int aDrawX = Ftol(aX) - gGemWidth / 2;
							gGraphicsRecorder->DrawImage(g, aShineImage, Rect(aDrawX, aDrawY, aWidth, aHeight), aSrcRect);
						}
					}
				}
				aShine++;
			} while (aShine < 9);
		}

		if (IsFlagSet(FLAG_BOMB) && mBombCount <= 8 && !gBoard->GetUnknown23F5()) {
			// A bomb about to go off is drawn later, over the board (its +0x14bc)
			gBoard->mBombList.push_back(this);
		} else {
			if (mUnknown4FC < 1) {
				// A gem lock: its face, bolts and chains over the gem, and the padlock, shaking while +0x508 runs
				if (IsFlagSet(FLAG_LOCKED) && !mBreaking && !mBlasted) {
					float anArtScale = (float)(gApp->mArtRes / 1200.0);
					float aLockScale = (float)((50 - mLockCount) * 2 * 0.019999999552965164 + 1.0);
					g->PushState();
					g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
					g->SetColorizeImages(true);
					if (mLockShake < 1) {
						g->SetColor(Color(255, 255, 255, Ftol(mAlpha * 255.0)));
					} else {
						aLockScale = (float)(sinf((float)(mLockShake * 0.10000000149011612)) * 0.10000000149011612 + 1.0);
						float aShake = fabsf(sinf((float)(mLockShake * 0.20000000298023224)));
						if (aShake < 0.0f)
							aShake = 0.0f;
						else
							aShake = fabsf(sinf((float)(mLockShake * 0.20000000298023224)));
						double aRed = (1.0 - aShake) * 191.0;
						int aBlue = Ftol(mAlpha * aRed + 64.0);
						int aGreen = Ftol(aRed + 64.0);
						g->SetColor(Color(255, aGreen, aBlue));
						g->Translate(Ftol(mOffsetX), Ftol(mOffsetY));
					}
					int anOffsetY = Ftol(gApp->mScale * GetImageOffsetY(IMAGE_GEMLOCK_FRONTFACE_ID));
					int anOffsetX = GetImageOffsetX(IMAGE_GEMLOCK_FRONTFACE_ID);
					int aY = Ftol(anOffsetY + (double)mY);
					anOffsetX = Ftol(gApp->mScale * anOffsetX);
					gGraphicsRecorder->DrawImage(g, IMAGE_GEMLOCK_FRONTFACE, Ftol(anOffsetX + (double)mX), aY);
					// port: the original also works out how far each of the two chains' links (IMAGE_GEM_LOCK_LINK1
					// and 2) shows by the lock's strength, and keeps it nowhere
					for (int anId = IMAGE_GEMLOCK_BOLTBL_ID; anId < IMAGE_GEMLOCK_BOLTBL_ID + 4; anId++) {
						aY = Ftol(GetImageOffsetY(anId) * anArtScale + (double)mY);
						int aX = Ftol(GetImageOffsetX(anId) * anArtScale + (double)mX);
						gGraphicsRecorder->DrawImage(g, GetImageById(anId), aX, aY);
					}
					int anId = IMAGE_GEMLOCK_THINLINK1_ID;
					int aLink;
					do {
						aY = Ftol(GetImageOffsetY(anId - 0xe) * anArtScale + (double)mY);
						int aX = Ftol(GetImageOffsetX(anId - 0xe) * anArtScale + (double)mX);
						gGraphicsRecorder->DrawImage(g, GetImageById(anId - 0xe), aX, aY);
						if (anId - IMAGE_GEMLOCK_THINLINK1_ID < 10) {
							aY = Ftol(GetImageOffsetY(anId) * anArtScale + (double)mY);
							aX = Ftol(GetImageOffsetX(anId) * anArtScale + (double)mX);
							gGraphicsRecorder->DrawImage(g, GetImageById(anId), aX, aY);
						}
						aLink = anId - (IMAGE_GEMLOCK_THINLINK1_ID - 1);
						anId++;
					} while (aLink < 12);
					int aTurn = 100;
					if (mLockCount * 2 <= 100)
						aTurn = mLockCount * 2;
					float aRot = (float)(aTurn * 0.01 * kTwoPi);
					if (aLockScale < 1.0f)
						aLockScale = 1.0f;
					if (IsFlagSet(FLAG_12)) {
						float aDevilScale = (float)((2.5 - aLockScale) * 0.81);
						if (aDevilScale < 0.0f)
							aDevilScale = 0.0f;
						int aRise = Ftol(gApp->mScale * 8.0);
						DrawImageScaledCentered(g, IMAGE_DEVILGEM, (float)(gGemWidth / 2 + (double)mX),
							(float)((gGemHeight / 2 + (double)mY) + aRise), aDevilScale, aDevilScale);
					}
					g->SetColor(Color(255, 255, 255, Ftol(mAlpha * 255.0)));
					DrawImageCentered(g, IMAGE_GEMLOCK_LOCK, (float)(gGemWidth / 2 + (double)mX),
						(float)(gGemHeight / 2 + (double)mY), aRot, aLockScale, aLockScale);
					g->PopState();
				}
			} else {
				// A lock that is opening (+0x4fc 1) or shut: the padlock, or a doom lock for FLAG_12
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				g->SetColorizeImages(true);
				Image* aLockImage;
				float aY;
				float aCos;
				float aX;
				if (!IsFlagSet(FLAG_12)) {
					if (mUnknown4FC != 1) {
						float aTurn = (float)(mLockTick * 0.07999999821186066);
						g->SetColor(Color(120, 120, 120, Ftol(mAlpha * 180.0)));
						aCos = cosf(aTurn);
						float aSin = sinf((float)(aTurn * 0.75));
						aY = (float)((float)(gApp->mScale * 8.0) * aSin + (gGemHeight / 2 + (double)mY));
						aX = (float)(gGemWidth / 2 + (double)mX);
						aLockImage = IMAGE_GEMLOCK_LOCK;
						DrawImageCentered(g, aLockImage, aX, aY, (double)(float)(aCos * 0.8), 1.0f, 1.0f);
					} else {
						float aPop = (float)(sinf((float)(mLockFlash * 3.1415927410125732)) * 0.75 + 1.0);
						float aTurn = (float)(mLockTick * 0.07999999821186066);
						g->SetColor(Color(200, 200, 200, Ftol(mAlpha * 255.0)));
						aCos = cosf(aTurn);
						float aSin = sinf((float)(aTurn * 0.75));
						aX = (float)(gGemWidth / 2 + (double)mX);
						aY = (float)((float)(gApp->mScale * 8.0) * aSin + (gGemHeight / 2 + (double)mY));
						DrawImageCentered(g, IMAGE_GEMLOCK_LOCK, aX, aY, (double)(float)(aCos * 0.8), aPop, aPop);
					}
				} else if (mUnknown4FC == 1) {
					float aPop = (float)(sinf((float)(mLockFlash * 3.1415927410125732)) * 0.75 + 1.0);
					float aTurn = (float)(mLockTick * 0.05999999865889549);
					g->SetColor(Color(200, 200, 200, Ftol(mAlpha * 255.0)));
					aCos = cosf(aTurn);
					float aBob = (float)(aTurn * 0.6499999761581421);
					float aSin = sinf(aBob);
					aX = (float)(gGemWidth / 2 + (double)mX);
					aY = (float)((gGemHeight / 2 + (double)mY) + (float)(gApp->mScale * 8.0) * aSin);
					DrawImageCentered(g, IMAGE_DOOMLOCK, aX, aY, (double)(float)(aCos * 0.8), aPop, aPop);
					g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					float aGlow = sinf((float)(mLockTick * 0.10000000149011612));
					if (aGlow < 0.0)
						aGlow = 0.0f;
					else
						aGlow = sinf((float)(mLockTick * 0.10000000149011612));
					g->SetColor(Color(200, 0, 180, Ftol(aGlow * 255.0 * mAlpha)));
					aCos = cosf(aTurn);
					aSin = sinf(aBob);
					aX = (float)(gGemWidth / 2 + (double)mX);
					aY = (float)((gGemHeight / 2 + (double)mY) + (float)(gApp->mScale * 8.0) * aSin);
					DrawImageCentered(g, IMAGE_DOOMLOCK_GLOW, aX, aY, (double)(float)(aCos * 0.8), aPop, aPop);
					g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
				} else {
					float aTurn = (float)(mLockTick * 0.07999999821186066);
					g->SetColor(Color(120, 120, 120, Ftol(mAlpha * 180.0)));
					aCos = cosf(aTurn);
					float aSin = sinf((float)(aTurn * 0.75));
					aY = (float)((float)(gApp->mScale * 8.0) * aSin + (gGemHeight / 2 + (double)mY));
					aX = (float)(gGemWidth / 2 + (double)mX);
					DrawImageCentered(g, IMAGE_DOOMLOCK, aX, aY, (double)(float)(aCos * 0.8), 1.0f, 1.0f);
				}
				g->SetColorizeImages(false);
			}
		}

		// An electro gem: its outline, brighter the more arcs it has, and its arcs, each a chain of segments from
		// one point of its outline to another (or to the piece +0x14 names), drawn twice: the arc and its core
		if (!IsFlagSet(FLAG_1) && IsFlagSet(FLAG_ELECTRO) && mUnknown535) {
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			g->SetColorizeImages(true);
			double anOutline = (double)(unsigned int)mArcs.size() * 6.0 + 64.0;
			if (anOutline >= 255.0)
				anOutline = 255.0;
			else
				anOutline = (double)(unsigned int)mArcs.size() * 6.0 + 64.0;
			g->SetColor(Color(255, 255, 255, Ftol(anOutline * mAlpha)));
			int anOutlineY = Ftol((double)mShakeY + mY);
			int anOutlineX = Ftol((double)mShakeX + mX);
			gGraphicsRecorder->DrawImageCel(g, IMAGE_GEMOUTLINES, Rect(anOutlineX, anOutlineY, gGemWidth, gGemHeight),
				mColor);
			g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			for (int anArcIdx = 0; anArcIdx < (int)mArcs.size(); anArcIdx++) {
				Arc& anArc = mArcs[anArcIdx];
				Color anArcColor = anArc.mBright ? kArcBrightColors[mColor] : kArcColors[mColor];
				anArcColor.mAlpha = Ftol(mAlpha * 255.0);
				g->SetColor(anArcColor);
				float aPrevJitter = 0.0f;
				float aRadius = GetRadius(anArc.mStartAngle);
				float aPrevX = (float)(cosf(anArc.mStartAngle) * aRadius);
				float aPrevY = (float)(sinf(anArc.mStartAngle) * aRadius);
				float aStartX = aPrevX;
				float aStartY = aPrevY;
				aRadius = GetRadius(anArc.mEndAngle);
				float anEndX = (float)(cosf(anArc.mEndAngle) * aRadius);
				float anEndY = (float)(sinf(anArc.mEndAngle) * aRadius);
				if (anArc.mToPiece) {
					Piece* aTarget = gApp->mBoard->GetPieceById(anArc.mTargetId);
					if (aTarget != nullptr) {
						// To the outline of the other piece, in this one's unscaled coordinates
						aRadius = aTarget->GetRadius(anArc.mEndAngle);
						float aScale = gApp->mScale;
						float aCosEnd = cosf(anArc.mEndAngle);
						anEndX = (float)((aTarget->mX - mX) / aScale + aRadius * aCosEnd);
						aScale = gApp->mScale;
						float aSinEnd = sinf(anArc.mEndAngle);
						anEndY = (float)((aTarget->mY - mY) / aScale + aRadius * aSinEnd);
					}
				}
				int aSegment = 0;
				while (aSegment < anArc.mSegments + 1) {
					int aNext = aSegment + 1;
					float aT = (float)aNext / (float)(anArc.mSegments + 1);
					double aU = 1.0 - aT;
					float anAngle = (float)(aT * anArc.mEndAngle + anArc.mStartAngle * aU);
					float aJitter = 0.0f;
					if (aSegment < anArc.mSegments)
						aJitter = anArc.mOffsets[aSegment];
					float aSegRadius = (float)(GetRadius(anAngle) + aJitter);
					float aX = (float)(cosf(anAngle) * aSegRadius);
					float aY = (float)(sinf(anAngle) * aSegRadius);
					if (anArc.mBright || anArc.mToPiece) {
						// Straight across, jittered sideways
						float aDir = (float)atan2((double)(float)(anEndY - aPrevY), (double)(float)(anEndX - aPrevX));
						aX = (float)(aT * anEndX + aStartX * aU);
						aY = (float)(aT * anEndY + aStartY * aU);
						if (aSegment < anArc.mSegments) {
							aX = (float)(anArc.mOffsets[aSegment] * sinf(aDir) + aX);
							aY = (float)(anArc.mOffsets[aSegment] * cosf(aDir) + aY);
						}
					}
					Color aColor0 = anArcColor;
					Color aColor1 = anArcColor;
					if (!anArc.mBright && !anArc.mToPiece) {
						double anAlpha = (1.0 - aPrevJitter * 0.029999999329447746) * 255.0;
						if (anAlpha < 2.0)
							anAlpha = 2.0;
						aColor0.mAlpha = Ftol(anAlpha);
						anAlpha = (1.0 - aJitter * 0.029999999329447746) * 255.0;
						if (anAlpha < 2.0)
							anAlpha = 2.0;
						aColor1.mAlpha = Ftol(anAlpha);
					}
					float aWidth;
					if (anArc.mToPiece)
						aWidth = (float)(gApp->mScale * 8.0);
					else if (anArc.mBright)
						aWidth = (float)(gApp->mScale * 9.0);
					else
						aWidth = (float)(gApp->mScale * 6.0);
					double aCenterY = gGemHeight / 2 + (double)mY;
					double aCenterX = gGemWidth / 2 + (double)mX;
					float aScale = gApp->mScale;
					DrawBeam(g, IMAGE_ELECTROTEX, (float)((float)(aPrevX * aScale) + aCenterX),
						(float)(aCenterY + (float)(aPrevY * aScale)), (float)((float)(aX * aScale) + aCenterX),
						(float)((float)(aY * aScale) + aCenterY), aWidth, aColor0, aColor1);
					Color aCore0 = Color::White;
					Color aCore1 = Color::White;
					if (!anArc.mBright) {
						if (!anArc.mToPiece) {
							double anAlpha = (0.8500000238418579 - aPrevJitter * 0.03999999910593033) * 255.0;
							if (anAlpha < 2.0)
								anAlpha = 2.0;
							aCore0.mAlpha = Ftol(anAlpha);
							anAlpha = (0.8500000238418579 - aJitter * 0.03999999910593033) * 255.0;
							if (anAlpha < 2.0)
								anAlpha = 2.0;
							aCore1.mAlpha = Ftol(anAlpha);
						}
					} else {
						aCore0.mAlpha = Ftol(aCore0.mAlpha * 0.5);
						aCore1.mAlpha = Ftol(0.5 * aCore1.mAlpha);
					}
					float aCoreWidth = (anArc.mToPiece || anArc.mBright) ? 8.0f : 6.0f;
					aCenterY = gGemHeight / 2 + (double)mY;
					aCenterX = gGemWidth / 2 + (double)mX;
					DrawBeam(g, IMAGE_ELECTROTEX_CENTER, (float)((float)(aPrevX * gApp->mScale) + aCenterX),
						(float)(aCenterY + (float)(aPrevY * gApp->mScale)), (float)((float)(aX * gApp->mScale) + aCenterX),
						(float)((float)(aY * gApp->mScale) + aCenterY), aCoreWidth, aCore0, aCore1);
					aPrevX = aX;
					aPrevY = aY;
					aPrevJitter = aJitter;
					aSegment = aNext;
				}
			}
		}
		// port: while +0xfc is above 0 the original sets a colour by it, between saving and restoring the draw mode
		// and colorizing, and draws nothing
		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	}

	// The shockwave of the gem's colour, shrinking from four times its size as +0x2e4 falls (half that without
	// Direct3D)
	if ((gBoard->mGameMode != Board::MODE_PUZZLE || !gBoard->mUnknown1324) && 0.0f < mDevilGlow &&
		!IsFlagSet(FLAG_DEVIL)) {
		g->SetColorizeImages(true);
		Image* aWave = GetImageById(IMAGE_GEM_SHOCKWAVE_0_ID + mColor);
		Color aColor = gGemColors[mColor];
		if (aWave != nullptr) {
			aColor.mAlpha = Ftol(mDevilGlow * 255.0 * mAlpha);
			float aScale = (float)((1.0 - mDevilGlow) * 3.0 + 1.0);
			aScale = (float)(aScale * 0.5);
			int aWidth = Ftol(aWave->mWidth * aScale);
			int aHeight = Ftol(aWave->mHeight * aScale);
			g->SetColor(aColor);
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			int aY = Ftol((gGemHeight - aHeight) / 2 + (double)mY);
			int aX = Ftol((gGemWidth - aWidth) / 2 + (double)mX);
			gGraphicsRecorder->DrawImage(g, aWave, aX, aY, aWidth, aHeight);
			aY = Ftol((gGemHeight - aHeight) / 2 + (double)mY);
			aX = Ftol((gGemWidth - aWidth) / 2 + (double)mX);
			gGraphicsRecorder->DrawImage(g, aWave, aX, aY, aWidth, aHeight);
		}
	}

	// Coal glowing as it smokes (+0x2f8): two of its cels added over it; the smoke itself is drawn only with
	// Direct3D (a fan of triangles over 32 of +0x3e8's values for each cel)
	if (IsFlagSet(FLAG_COAL) && !mBreaking && !gBoard->mUnknown1BF2 && mUnknown535) {
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		float aGlow = 1.0f;
		if (mUnknown2F8 <= 0.0f)
			aGlow = 0.0f;
		else if (mUnknown2F8 <= 1.0f)
			aGlow = mUnknown2F8;
		int aCel = 0;
		do {
			if (aGlow != 0.0f) {
				g->SetColor(Color(255, 255, 255, Ftol(aGlow * 50.0 * mAlpha)));
				int aY = Ftol(mY);
				int aX = Ftol(mX);
				gGraphicsRecorder->DrawImageCel(g, IMAGE_COAL, aX, aY, aCel + 1);
			}
			aCel++;
		} while (aCel <= 1);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	}

	// port: with the board's +0x21db each piece also shows its row and column and its id; nothing sets it (the
	// board's constructor only clears it)
}

// The purple sparkles of a doom gem or a devil gem's smoke, in three layers (Piece::Update)
static void SetPurpleSparkle(Effect* theEffect, int theLayer, int theDarkest, float theScale)
{
	Rand();
	if (theLayer == 0) {
		theEffect->mColor = Color(theDarkest, 0, theDarkest);
		theEffect->mScale = (float)((double)theScale * 3.01);
		theEffect->mAlpha = 1.0f;
		theEffect->mDAlpha = -0.005f;
	} else if (theLayer == 1) {
		theEffect->mColor = Color(100, 0, 100);
		theEffect->mScale = (float)((double)theScale * 1.1);
		theEffect->mAlpha = 0.8f;
		theEffect->mDAlpha = -0.005f;
	} else if (theLayer == 2) {
		theEffect->mColor = Color(200, 250, 200);
		theEffect->mScale = theScale;
		theEffect->mAlpha = 0.5f;
		theEffect->mDAlpha = -0.004f;
	}
}

// Part of Piece::Update (0x72d427-0x72d90d): ten sparks fly off a power gem's outline, half of them rising, the
// others hanging on; most stay with the gem
void Piece::UpdatePowerSparks()
{
	for (int i = 10; i != 0; i--) {
		bool aRising = Rand() % 2 == 0;
		EffectsManager* aManager;
		Effect* anEffect;
		if (!aRising) {
			aManager = Rand() % 2 == 0 ? gApp->mBoard->mFXManager : gApp->mBoard->mFXManager2;
			anEffect = aManager->AllocEffect(5);
			anEffect->mAlpha = 1.0f;
			// port: with Direct3D the spark is drawn at twice the size, from IMAGE_SPARKLET
			anEffect->mFrame = 0;
			anEffect->mDScale = -0.01f;
			anEffect->mImage = IMAGE_SPARKLET_FAT;
			float aBase = (float)((double)gApp->mScale * -0.4);
			float aRange = (float)((double)gApp->mScale * 0.15);
			anEffect->mDY = (float)((double)RandSigned() * (double)aRange + (double)aBase);
			int aBlue = Rand() % 24 + 24;
			int aGreen = Rand() % 32;
			anEffect->mColor = Color(128, aGreen + 48, aBlue);
			if (mColor == 0)
				anEffect->mColor = Color(240, 128, 64);
			else if (mColor == 3)
				anEffect->mColor = Color(255, 0, 0);
		} else {
			bool aFirst = IsFlagSet(FLAG_ELECTRO) ? Rand() % 8 == 0 : Rand() % 32 == 0;
			aManager = aFirst ? gApp->mBoard->mFXManager : gApp->mBoard->mFXManager2;
			anEffect = aManager->AllocEffect(6);
			anEffect->mAngle = 0.0f;
			anEffect->mDAngle = 0.0f;
			anEffect->mAlpha = 0.0f;
			anEffect->mDAlpha = (float)((double)RandSigned() * 0.001500000013038516 + 0.007499999832361937);
			// port: with Direct3D its scale (0.12 +- 0.035) and its growth (0.01 +- 0.005) are random
			float aBase = (float)((double)gApp->mScale * -0.12);
			float aRange = (float)((double)gApp->mScale * -0.05);
			anEffect->mDY = (float)((double)RandSigned() * (double)aRange + (double)aBase);
			if (mColor == 3)
				anEffect->mColor = Color(255, 128, 128);
			else
				anEffect->mColor = Color(255, 255, 255);
		}
		float aTurn = (float)((double)RandSigned() * 3.1415927410125732 * 2);
		float anAngle = (float)((double)std::fabs(aTurn) + 3.1415927410125732);
		float aRadius = (float)((double)GetRadius(anAngle) * (double)gApp->mScale);
		if (IsFlagSet(FLAG_ELECTRO)) {
			float anOut = (float)((double)gApp->mScale * 7.0);
			aRadius = anOut + aRadius;
		}
		float aCos = std::cos(anAngle);
		float aSin = std::sin(anAngle);
		if (aRising && 0.0f < aSin && Rand() % 2 != 0) {
			// On the lower half it may also follow the outline: the tangent from two points just along it
			float aStep = 0.0f <= aCos ? -0.001f : 0.001f;
			float anAngle1 = aStep + anAngle;
			float aCos1 = std::cos(anAngle1);
			float anAngle2 = aStep + aStep + anAngle;
			float aMidX = (float)(((double)std::cos(anAngle2) + (double)aCos1) * 0.5);
			float aSin1 = std::sin(anAngle1);
			float aMidY = (float)(((double)std::sin(anAngle2) + (double)aSin1) * 0.5);
			float aDX = aMidX - aCos;
			float aDY = aMidY - aSin;
			float aTangent = atan2f(aDY, aDX);
			float aSpread = (float)((double)gApp->mScale * 0.05);
			float aMin = (float)((double)gApp->mScale * 0.12);
			float aSpeed = (float)((double)RandSigned() * (double)aSpread + (double)aMin);
			float aVX = (float)((double)std::cos(aTangent) * (double)aSpeed);
			float aVY = (float)((double)std::sin(aTangent) * (double)aSpeed);
			anEffect->mDX = (float)(((double)anEffect->mDX + (double)aVX) * 0.5);
			anEffect->mDY = (float)(((double)aVY + (double)anEffect->mDY) * 0.5);
		}
		anEffect->mX = (float)((double)(gGemWidth / 2) + (double)mX + (double)std::cos(anAngle) * (double)aRadius);
		float aLift = gApp->mScale + gApp->mScale;
		anEffect->mY = (float)((double)(gGemHeight / 2) + (double)mY + (double)std::sin(anAngle) * (double)aRadius +
			(double)aLift);
		if (Rand() % 4 != 0) {
			anEffect->mX = anEffect->mX - mX;
			anEffect->mY = anEffect->mY - mY;
			anEffect->mPieceId = mId;
		}
		aManager->AddEffect(anEffect);
	}
}

// Part of Piece::Update (0x72dbd9-0x72df41): a devil gem breathes three puffs of purple smoke to one side, and while
// its counter shows (mUnknownD4) sparks rise from it
void Piece::UpdateDevilSmoke()
{
	float aScale = (float)((double)std::fabs(RandSigned()) * 0.10000000149011612 + 1.5);
	int aSide = Ftol(((double)RandSigned() * 0.05000000074505806 + 0.30000001192092896) *
		(double)IMAGE_DEVILGEM->mWidth);
	float aRand = std::fabs(RandSigned());
	float aDX = (float)((double)gApp->mScale * 0.75 * (double)aRand);
	int aX = Ftol((double)(gGemWidth / 2) + (double)mX + (double)mOffsetX);
	int aY = Ftol(((double)RandSigned() * 15.0 + 2.0) * (double)gApp->mScale + (double)mY + (double)mOffsetY);
	float aSpread = (float)((double)gApp->mScale * 0.05);
	float aBase = (float)((double)gApp->mScale * -0.3);
	float aDY = (float)((double)RandSigned() * (double)aSpread + (double)aBase);
	if (Rand() % 2 != 0) {
		aSide = -aSide;
		aDX = -aDX;
	}
	float aPuffX = (float)aX + (float)aSide;
	float aPuffY = (float)aY;
	for (int i = 0; i < 3; i++) {
		EffectsManager* aManager = gApp->mBoard->mFXManager2;
		Effect* anEffect = aManager->AllocEffect(0);
		anEffect->mImage = IMAGE_SPARKLET;
		SetPurpleSparkle(anEffect, i, 40, aScale);
		anEffect->mIsAdditive = true;
		anEffect->mDScale = -0.008f;
		anEffect->mDXScalar = 0.95f;
		anEffect->mX = aPuffX;
		anEffect->mDX = aDX;
		anEffect->mY = aPuffY;
		anEffect->mDY = aDY;
		anEffect->mGravity = (float)((double)gApp->mScale * -0.005);
		aManager->AddEffect(anEffect);
	}
	if (mUnknownD4 != 0.0f) {
		int aCountX, aCountY;
		GetCounterPos(&aCountX, &aCountY, mUnknownD0);
		float aSparkY = (float)aCountY;
		float aSparkX = (float)aCountX;
		for (int i = 40; i != 0; i--) {
			EffectsManager* aManager = gApp->mBoard->mFXManager;
			Effect* anEffect = aManager->AllocEffect(6);
			anEffect->mY = (float)((double)RandSigned() * 5.0 + (double)aSparkY);
			float anOffset = RandSigned();
			anEffect->mFrame = 0;
			anEffect->mX = (float)((double)anOffset * 0.20000000298023224 * (double)IMAGE_DEVILGEM->mWidth +
				(double)aSparkX);
			anEffect->mAlpha = 1.0f;
			anEffect->mScale = 1.75f;
			anEffect->mDScale = -0.01f;
			anEffect->mImage = IMAGE_SPARKLET;
			float aMin = (float)((double)gApp->mScale * -0.1);
			float aRise = std::fabs(RandSigned());
			float aRange = (float)((double)gApp->mScale * -0.6);
			anEffect->mDY = aRange * aRise + aMin;
			int aBlue = Rand() % 24 + 24;
			int aGreen = Rand() % 32;
			anEffect->mColor = Color(128, aGreen + 48, aBlue);
			aManager->AddEffect(anEffect);
		}
	}
}

void Piece::Update()
{
	mUpdateCnt++;
	mCurve1A0.IncInVal();
	mCountPulse.IncInVal();
	// port: the original drops mUnknown528 here once its slot 9 says it is done; it never sets it

	if (0.0f < mLockFlash) {
		float aFlash = (float)((double)mLockFlash - 0.027);
		if (aFlash <= 0.0f)
			aFlash = 0.0f;
		mLockFlash = aFlash;
	}
	mLockTick++;
	if (mLockShake != 0) {
		int aShake = ++mLockShake;
		if (aShake < 13 || 25 < aShake)
			mUnknown29C = 0.0f;
		else
			mUnknown29C = 0.5f;
		if (aShake == 32)
			mLockShake = 0;
	}

	if (0.0f < mPendingTime) {
		float aTime = (float)((double)mPendingTime - 0.01);
		mPendingTime = aTime;
		mX = mPendingDX + mX;
		mY = mPendingDY + mY;
		if (0.0f < aTime) {
			if (mPendingDX != 0.0f || mPendingDY != 0.0f) {
				// The gem flies to where it turns, trailing sparklets in the colour of what it becomes
				Image* anImage = nullptr;
				Color aColor = Color::White;
				int aKind = mPendingKind;
				if (aKind == 3) {
					anImage = IMAGE_SPARKLET;
				} else if (aKind == 4) {
					anImage = IMAGE_SPARKLET;
					int aGreen = Rand() % 128;
					int aRed = Rand() % 128 + 128;
					aColor = Color(aRed, aGreen, 0);
				} else if ((unsigned int)(aKind - 6) <= 1) {
					anImage = IMAGE_SPARKLET;
					aColor = Color(128, 128, 255);
				}
				for (int i = 10; i != 0; i--) {
					Effect* anEffect = gBoard->mFXManager->AllocEffect(0);
					float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
					float aDist = (float)((double)RandSigned() * gGemWidth / 3.0);
					float aCos = std::cos(anAngle);
					anEffect->mX = (float)((double)aCos * aDist + (double)(gGemWidth / 2) + mX);
					float aSin = std::sin(anAngle);
					anEffect->mY = (float)(((double)(gGemHeight / 2) + mY) - (double)aSin * aDist);
					float aRand = std::fabs(RandSigned());
					float aSpeed = (float)((double)gApp->mScale * 0.1);
					anEffect->mDX = (float)(-(double)mPendingDX * aRand * aSpeed);
					aRand = std::fabs(RandSigned());
					aSpeed = (float)((double)gApp->mScale * 0.1);
					anEffect->mIsAdditive = true;
					anEffect->mDY = (float)(-(double)mPendingDY * aRand * aSpeed);
					anEffect->mColor = aColor;
					anEffect->mDAlpha = -0.02f;
					if (anImage != nullptr)
						anEffect->mImage = anImage;
					gBoard->mFXManager->AddEffect(anEffect);
				}
			}
		} else {
			ClearSpecial();
			mPendingTime = 0.0f;
			if (mPendingDX == 0.0f && mPendingDY == 0.0f) {
				int aKind = mPendingKind;
				if (aKind == 8) {
					MakeElectro(0.0f);
				} else if (aKind == 0x10) {
					MakePower(0.0f);
				} else if (aKind == 0x18) {
					MakeElectro(0.0f);
					SetFlag(FLAG_POWER, true);
				} else if (aKind == 0x40) {
					Make6(0.0f);
				} else if (aKind == 0x80) {
					Make7(0.0f);
				}
				mPendingKind = 0;
			} else {
				mPendingKind = 0;
				mPendingDY = 0.0f;
				mPendingDX = 0.0f;
				mUnknown536 = false;
				gBoard->BreakPiece(this);
			}
		}
	}

	if (mShakeX != 0.0f) {
		mShakeX = (float)((double)mShakeX - (double)mShakeX * 0.1);
		if (std::fabs(mShakeX) < 1.0f)
			mShakeX = 0.0f;
	}
	if (mShakeY != 0.0f) {
		mShakeY = (float)((double)mShakeY - (double)mShakeY * 0.1);
		if (std::fabs(mShakeY) < 1.0f)
			mShakeY = 0.0f;
	}
	if (0.0f < mGlow) {
		float aGlow = mUnknown29C <= mGlow ? mGlow : mUnknown29C;
		mUnknown29C = aGlow <= 1.0f ? aGlow : 1.0f;
	}
	if (0.0f < mUnknown298) {
		float aTime = (float)((double)mUnknown298 - 0.01);
		mUnknown298 = aTime;
		if (aTime <= 0.0f)
			mUnknown29C = 0.0f;
	}

	if (!IsFlagSet(FLAG_LOCKED)) {
		if (!IsFlagSet(FLAG_POWER)) {
			if (IsFlagSet(FLAG_COAL)) {
				// Coal smoulders: its 64 embers (two rows of 32) brighten and go out, lighting the coal as they
				// burn, quicker while the mouse is over it or the board points at it
				bool aHot = false;
				float aMouseX = (float)gBoard->mUnknown15B4;
				if (mX <= aMouseX && aMouseX < (float)gGemWidth + mX) {
					float aMouseY = (float)gBoard->mUnknown15B8;
					if (mY <= aMouseY && aMouseY < (float)gGemHeight + mY)
						aHot = true;
				}
				if (gBoard->mUnknown2584 != nullptr && gBoard->GetPointedPieceId() == mId)
					aHot = true;
				unsigned int aPeriod = aHot ? 1 : 20;
				float* anEmber = mUnknown3E8;
				for (int aRow = 0;;) {
					for (int i = 32; i != 0; i--) {
						if (0.0f < *anEmber) {
							float aValue = (float)((double)*anEmber + 0.01);
							*anEmber = aValue;
							if (aValue < 1.0f) {
								double aCoal = (double)mUnknown2F8 + 0.019999999552965164;
								mUnknown2F8 = (float)(aCoal <= 1.0 ? aCoal : 1.0);
							} else {
								*anEmber = 0.0f;
							}
						}
						anEmber++;
					}
					if (0.0f < mUnknown4E8 && (unsigned int)mUpdateCnt % aPeriod == 0) {
						float* aNew = &mUnknown3E8[Rand() % 32 + aRow];
						if (*aNew == 0.0f)
							*aNew = 0.01f;
					}
					aRow += 32;
					if (aRow > 63)
						break;
				}
				if (mUnknown4E8 <= 0.0f) {
					float aTime = (float)((double)mUnknown124 - 0.01);
					mUnknown124 = aTime;
					if (aTime > 0.0f) {
						double aCoal;
						if (1.0 <= aTime) {
							aCoal = (double)mUnknown2F8 - 0.02500000037252903;
							if (aCoal < 0.0)
								aCoal = 0.0;
						} else {
							aCoal = 1.0 - aTime;
						}
						mUnknown2F8 = (float)aCoal;
					} else {
						mUnknown124 = 0.0f;
						mUnknown4E8 = (float)((double)std::fabs(RandSigned()) + 1.0);
					}
				} else {
					float aTime = (float)((double)mUnknown4E8 - 0.01);
					if (aTime < 0.0f)
						aTime = 0.0f;
					mUnknown4E8 = aTime;
					if (aTime <= 0.0f)
						mUnknown124 = (float)((double)std::fabs((float)((double)RandSigned() * 4.0)) + 5.0);
				}
				if (aHot)
					mUnknown4E8 = 0.1f;
			}
		} else if (mUnknown2FC <= mUnknown2F8) {
			// A power gem's glow swings between 0 and 1, turning at a random point above or below the middle
			double aGlow = (double)mUnknown2F8 - 0.03;
			if (aGlow < 0.0)
				aGlow = 0.0;
			mUnknown2F8 = (float)aGlow;
			if (mUnknown2F8 <= mUnknown2FC)
				mUnknown2FC = (float)((double)std::fabs((float)((double)RandSigned() * 0.5)) + 0.5);
		} else {
			double aGlow = 0.03 + (double)mUnknown2F8;
			if (1.0 < aGlow)
				aGlow = 1.0;
			mUnknown2F8 = (float)aGlow;
			if (mUnknown2FC < mUnknown2F8)
				mUnknown2FC = (float)((double)std::fabs((float)((double)RandSigned() * 0.5)) + 0.5);
		}
	} else {
		// A gem lock counts to 100 after it is made; from 50 its glow fades, and at 50 a doom lock becomes a devil
		// gem with a bomb's counter
		int aCount = mLockCount;
		if (aCount < 100) {
			mLockCount = ++aCount;
			if (49 < aCount) {
				float aLeft = (float)(100.0 - (double)mUnknown29C);
				double aGlow = (double)aLeft / 100.0 * 0.5;
				if (1.0 < aGlow)
					aGlow = 1.0;
				mUnknown29C = (float)aGlow;
				if (aCount == 50) {
					gApp->PlaySample(SOUND_GEM_LOCK_END);
					if (IsFlagSet(FLAG_12)) {
						ClearSpecial();
						SetFlag(FLAG_DEVIL, true);
						mCanMove = false;
						int aBombCount = 1;
						if (0 < gBoard->mDevil.GetValue())
							aBombCount = gBoard->mDevil.GetValue();
						mBombCount = aBombCount;
						mScale288 = 0.01f;
						gApp->PlaySample(SOUND_DOOMGEM_COUNTDOWN);
						MakeCountImage();
						mCountPulse.SetCurve(GTS(0x7e9a70));
						mJustMade = 2;
					}
				} else if (aCount == 100) {
					mUnknown29C = 0.0f;
				}
			}
		}
	}

	if (IsFlagSet(FLAG_BOMB))
		mColorCycle.Update();
	if (0 < mJustMade) {
		float aGlow = (float)((double)mDevilGlow - 0.02);
		mDevilGlow = aGlow;
		if (aGlow <= 0.0f) {
			mJustMade--;
			mDevilGlow = mJustMade != 0 ? 1.0f : 0.0f;
		}
	}

	if (IsFlagSet(FLAG_12) && mUnknown535) {
		// A doom gem gives off three purple sparkles from its outline
		for (int i = 0; i < 3; i++) {
			float aScale = (float)((double)std::fabs(RandSigned()) * 0.10000000149011612 + 1.2999999523162842);
			float aRand = std::fabs(RandSigned());
			float aSpeed = (float)((double)gApp->mScale * 0.75) * aRand;
			int aX = Ftol((double)(gGemWidth / 2) + (double)mX);
			int aY = Ftol((double)(gGemHeight / 2) + (double)mY);
			RandSigned();
			EffectsManager* aManager = gApp->mBoard->mFXManager;
			Effect* anEffect = aManager->AllocEffect(0);
			anEffect->mImage = IMAGE_SPARKLET;
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			anEffect->mDX = (float)((double)std::cos(anAngle) * (double)aSpeed);
			anEffect->mDY = (float)((double)std::sin(anAngle) * (double)aSpeed);
			float aRadius = (float)((double)GetRadius(anAngle) * (double)gApp->mScale);
			aX = Ftol((double)std::cos(anAngle) * (double)aRadius + (double)aX);
			aY = Ftol((double)std::sin(anAngle) * (double)aRadius + (double)aY);
			SetPurpleSparkle(anEffect, i, 20, aScale);
			anEffect->mIsAdditive = true;
			anEffect->mX = (float)aX;
			anEffect->mY = (float)aY;
			anEffect->mDScale = -0.008f;
			anEffect->mDXScalar = 0.95f;
			anEffect->mGravity = (float)((double)gApp->mScale * -0.005);
			aManager->AddEffect(anEffect);
		}
	}

	if ((!IsFlagSet(FLAG_POWER) && !IsFlagSet(FLAG_ELECTRO) && !IsFlagSet(FLAG_FRUIT) && !IsFlagSet(FLAG_6) &&
		!IsFlagSet(FLAG_DEVIL)) || mDrawScale != 1.0f || !mUnknown535) {
		if (IsFlagSet(FLAG_7) && !gBoard->IsBusy() && mUnknown535) {
			// Bubbles rise from the gem, and its four sparkles (mFadeColors) come and go
			if (gBoard->mUpdateCnt % 12 == 0) {
				EffectsManager* aManager = gApp->mBoard->mFXManager;
				Effect* anEffect = aManager->AllocEffect(0);
				anEffect->mX = (float)((double)(gGemWidth / 2) + (double)mX + (double)RandSigned() * 0.3499999940395355 *
					(double)gGemHeight);
				anEffect->mY = (float)((double)(gGemHeight / 2) + (double)mY + (double)RandSigned() * 0.3499999940395355 *
					(double)gGemWidth);
				anEffect->mDX = 0.0f;
				float aSpeed = (float)((double)gApp->mScale * -0.15);
				anEffect->mIsAdditive = true;
				anEffect->mDY = aSpeed;
				anEffect->mScale = 0.25f;
				anEffect->mDScale = 0.02f;
				anEffect->mGravity = 0.0f;
				anEffect->mAlpha = 1.0f;
				anEffect->mDAlpha = -0.01f;
				aManager->AddEffect(anEffect);
			}
			if (gBoard->mUpdateCnt % 20 == 0) {
				for (int i = 0; i < 4; i++) {
					if (mFadeColors[i].mAlpha == 0 && mFadeTargets[i].mAlpha == 0) {
						mFadeColors[i] = Color(255, 255, 255, 0);
						mFadeTargets[i] = Color(255, 255, 255, 255);
						mUnknown304[i] = (float)((double)RandSigned() * 3.1415927410125732);
						break;
					}
				}
			}
			for (int i = 0; i < 4; i++) {
				int anAlpha = mFadeColors[i].mAlpha;
				int aTarget = mFadeTargets[i].mAlpha;
				if (anAlpha != aTarget) {
					if (anAlpha < aTarget) {
						mFadeColors[i].mAlpha = anAlpha + 3;
						if (aTarget <= anAlpha + 3) {
							mFadeTargets[i].mAlpha = 0;
							mFadeColors[i].mAlpha = aTarget;
						}
					} else {
						mFadeColors[i].mAlpha = anAlpha - 3;
						if (anAlpha - 3 < 1)
							mFadeColors[i].mAlpha = 0;
					}
				}
			}
		}
	} else {
		if (gBoard->mUpdateCnt % 33 == 0) {
			mUnknown11C++;
			if (mUnknown11C == 16)
				mUnknown11C = 0;
			else if (mUnknown11C == 1 || mUnknown11C == 4)
				mFruitGlow = 1.0f;
		}
		float aTime = (float)((double)mUnknown124 - 0.01);
		mUnknown124 = aTime;
		if (aTime <= 0.0f) {
			mUnknown124 = (float)((double)std::fabs((float)((double)RandSigned() * 0.10000000149011612)) +
				0.019999999552965164);
			if (IsFlagSet(FLAG_POWER)) {
				UpdatePowerSparks();
			} else if (IsFlagSet(FLAG_FRUIT)) {
				// A fruit lets fall a seed (effect 12)
				Effect* anEffect = gApp->mBoard->mFXManager->AllocEffect(12);
				float anAngle = std::fabs((float)((double)RandSigned() * kTwoPi));
				float aDist = std::fabs((float)((double)RandSigned() * (double)gGemWidth * 0.25));
				float aCos = std::cos(anAngle);
				anEffect->mX = (float)((double)aCos * (double)aDist + (double)(gGemWidth / 2) + (double)mX);
				float aSin = std::sin(anAngle);
				anEffect->mY = (float)((double)aSin * (double)aDist + (double)(gGemHeight / 2) + (double)mY);
				anEffect->mDX = 0.0f;
				float aSpeed = (float)((double)std::fabs((float)((double)RandSigned() * 0.10000000149011612)) + 0.05);
				anEffect->mDY = aSpeed * gApp->mScale;
				anEffect->mScale = 0.1f;
				gApp->mBoard->mFXManager->AddEffect(anEffect);
			} else if (IsFlagSet(FLAG_6)) {
				for (int i = 5; i != 0; i--)
					AddShards(1.0f, true);
				if ((gBoard->mUpdateCnt & 3) == 0) {
					Effect* anEffect = gApp->mBoard->mFXManager->AllocEffect(0);
					anEffect->mImage = IMAGE_SPARKLET;
					float anAngle = std::fabs((float)((double)RandSigned() * kTwoPi));
					float aDist = std::fabs((float)((double)RandSigned() * (double)gGemWidth * 0.5));
					float aCos = std::cos(anAngle);
					anEffect->mX = (float)((double)aCos * (double)aDist + (double)(gGemWidth / 2) + (double)mX);
					float aSin = std::sin(anAngle);
					anEffect->mY = (float)((double)aSin * (double)aDist + (double)(gGemHeight / 2) + (double)mY);
					anEffect->mDX = 0.0f;
					anEffect->mDY = 0.1f;
					anEffect->mScale = 1.5f;
					anEffect->mGravity = 0.0f;
					anEffect->mAlpha = 1.0f;
					anEffect->mDAlpha = -0.002f;
					gApp->mBoard->mFXManager->AddEffect(anEffect);
				}
			} else if (IsFlagSet(FLAG_DEVIL)) {
				mUnknown124 = 0.0f;
				if (!gBoard->IsSparkBlocked())
					UpdateDevilSmoke();
			}
		}
	}

	if (mUnknownE8 <= 0.0f) {
		if (0.0f <= mUnknownE4) {
			// A ring of sparks (effect 17) goes round the gem one and a half times
			float aTurn = (float)((double)kTwoPi * 0.02 + (double)mUnknownE4);
			mUnknownE4 = aTurn;
			if (9.424778f <= aTurn) {
				mUnknownE4 = -1.0f;
			} else {
				Effect* anEffect = gApp->mBoard->mFXManager->AllocEffect(17);
				float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
				anEffect->mDAlpha = -0.02f;
				float aCos = std::cos(anAngle);
				anEffect->mX = (float)((double)gGemWidth * (double)aCos * 0.5 + (double)(gGemWidth / 2) + (double)mX);
				float aSin = std::sin(anAngle);
				anEffect->mY = (float)((double)gGemHeight * (double)aSin * 0.5 + (double)(gGemHeight / 2) + (double)mY);
				anEffect->mScale = 1.0f;
				float aRise = std::fabs(RandSigned());
				float aSpeed = (float)((double)gApp->mScale * 0.3);
				anEffect->mDY = aSpeed * aRise;
				anEffect->mIsAdditive = true;
				anEffect->mAngle = (float)((double)RandSigned() * 3.1415927410125732);
				gApp->mBoard->mFXManager->AddEffect(anEffect);
			}
		}
	} else {
		float aTime = (float)((double)mUnknownE8 - 0.01);
		mUnknownE8 = aTime;
		if (aTime <= 0.0f)
			mUnknownE4 = 0.0f;
	}
	if (0.0f < mFruitGlow) {
		float aGlow = (float)((double)mFruitGlow - 0.01);
		mFruitGlow = aGlow;
		if (aGlow <= 0.0f)
			mFruitGlow = 0.0f;
	}
	if (0.0f < mUnknownFC) {
		float aValue = (float)((double)mUnknownFC - 0.005);
		if (aValue < 0.0f)
			aValue = 0.0f;
		mUnknownFC = aValue;
	}

	if (mScale288 < 1.0f) {
		// A gem that is made or broken grows to its size; a devil gem being made rumbles and throws up flames
		bool aKept = IsFlagSet(FLAG_1);
		if (!mBreaking) {
			if (aKept || mPendingKind == 0)
				mScale288 = (float)((double)mScale288 + 0.02);
		} else if (aKept) {
			if ((gBoard->mUpdateCnt & 5) == 0) {
				mUnknown88 = 1000;
				mUnknown29C = (float)((double)mScale288 * mScale288 * mScale288);
			}
			mScale288 = (float)((double)mScale288 + 0.05);
		} else if (IsFlagSet(FLAG_DEVIL)) {
			mUnknown88 = 1000;
			if ((gBoard->mUpdateCnt & 5) == 0) {
				mUnknown29C = (float)((double)mScale288 * mScale288 * mScale288);
				mUnknown2A0 = (float)((double)RandSigned() * 3.1415927410125732);
				mUnknown118 = Ftol((double)mScale288 * 8.0);
			}
			if (gBoard->mUpdateCnt % 2 == 0 && mUnknown535 && 2 <= mUnknown118) {
				Effect* anEffect = gBoard->mFXManager->AllocEffect(24);
				anEffect->mScale = 0.4f;
				float aRise = std::fabs(RandSigned());
				float aSpeed = gApp->mScale + gApp->mScale;
				anEffect->mDY = -aRise * aSpeed;
				float aSide = (float)((double)RandSigned() * (double)gApp->mScale);
				anEffect->mDX = aSide;
				anEffect->mX = (float)((double)(gGemWidth / 2) + (double)mX + (double)aSide * 10.0);
				anEffect->mY = (float)((double)RandSigned() * 3.0 + ((double)gGemHeight * 0.6000000238418579 + (double)mY));
				anEffect->mIsAdditive = true;
				anEffect->mGravity = (float)((double)gApp->mScale * -0.01);
				anEffect->mColor = Color(255, Rand() % 255, 0);
				gBoard->mFXManager->AddEffect(anEffect);
			}
			float aScale = (float)((double)mScale288 + 0.005);
			mScale288 = aScale;
			if (1.0f <= aScale) {
				mScale288 = 1.0f;
				mBlasted = true;
			}
		} else {
			mScale288 = (float)((double)mScale288 + 0.01);
		}
		if (1.0f <= mScale288) {
			mScale288 = 1.0f;
			if (aKept && mBreaking) {
				mJustMade = 0;
				mDevilGlow = 0.0f;
				mUnknown88 = 0;
				mBlasted = true;
			}
		}
	}

	bool aDevil = IsFlagSet(FLAG_DEVIL);
	if (IsFlagSet(FLAG_BOMB) || aDevil) {
		// A bomb's or devil gem's counter: its halos turn, and below 9 it glows and flickers faster as it runs out,
		// a bomb's sparks flying more often
		if (mCountImage == nullptr)
			MakeCountImage();
		if (mUnknownD0 == 0.0f) {
			mHaloAngles[0] = (float)((double)kTwoPi / 800.0 + (double)mHaloAngles[0]);
			mHaloAngles[1] = (float)((double)mHaloAngles[1] - (double)kTwoPi / 1000.0);
			if (mBombCount < 9 && !gBoard->GetUnknown23F5() && mUnknown2F6) {
				int aDanger = 8 - mBombCount;
				if (aDanger < 1)
					aDanger = 1;
				float aDangerF = (float)aDanger;
				if (mBombCount < 3) {
					float aTime = (float)((double)mUnknown4E8 - 0.01);
					mUnknown4E8 = aTime;
					if (aTime <= 0.0f) {
						mUnknown4E8 = 0.0f;
						// port: the original also tells mUnknown528 here, which it never sets
						if (mUnknown4EC < 0) {
							mUnknown4EC = Rand() % 4;
							double aWait = 0.5;
							if (0.5 <= (double)std::fabs(RandSigned()) * 0.30000001192092896 * (double)aDangerF)
								aWait = (double)std::fabs(RandSigned()) * 0.30000001192092896 * (double)aDangerF;
							mUnknown4E8 = (float)aWait;
						} else {
							mUnknown4EC = -1;
							float aRand = std::fabs(RandSigned());
							float aWait = 0.5f;
							if (0.5f <= (aRand + aRand) * (float)mBombCount) {
								aRand = std::fabs(RandSigned());
								aWait = (aRand + aRand) * (float)mBombCount;
							}
							mUnknown4E8 = aWait;
						}
					}
				}
				mUnknown108 -= 2;
				if (mBombCount < 3)
					mUnknown108 -= 1;
				if (mUnknown108 < 0)
					mUnknown108 = 255;
				mUnknown29C = (float)((double)aDangerF * 0.15);
				if (!aDevil) {
					int aQuarter = Rand() % 4;
					int aPeriod = 2;
					if (1 < mBombCount)
						aPeriod = mBombCount;
					int aSparks = 0;
					if (1 < mBombCount) {
						int aSpeed = gBoard->mUnknown1F78;
						if (aSpeed < 3)
							aSpeed = 3;
						aPeriod = (aSpeed * aPeriod) / 3;
					}
					if (aPeriod < 20 && gBoard->mUpdateCnt % aPeriod == 0)
						aSparks = 1;
					if (!gBoard->IsBusy() && !mBreaking && !gBoard->GetUnknown23F5() && aSparks != 0) {
						float anAngle = (float)((double)aQuarter * 3.1415927410125732 * 0.5 + 0.7853981633974483);
						do {
							aSparks--;
							AddSpark(false, aDangerF, anAngle);
						} while (aSparks != 0);
					}
				}
			} else {
				mUnknown108 = 0;
				if (mLockShake == 0)
					mUnknown29C = 0.0f;
			}
		}
	}

	if (mUnknownEC && !mBreaking) {
		if (mUnknownF4 == 0) {
			if (mUnknownF0 < 1.0f) {
				float aValue = (float)((double)mUnknownF0 + 0.05);
				mUnknownF0 = aValue;
				if (1.0f <= aValue)
					mUnknownF0 = 1.0f;
			}
		} else {
			mUnknownF4--;
		}
	}

	if (IsFlagSet(FLAG_ELECTRO) && !mBreaking && !mBlasted && mUnknown535)
		UpdateElectro();

	if (mUnknownD4 == 0.0f) {
		if (mUnknownB1) {
			mUnknownB8 = mUnknownB8 - 0.012;
			if (mUnknownB8 < 0.0)
				mUnknownB1 = false;
		}
		if ((IsFlagSet(FLAG_ELECTRO) || IsFlagSet(FLAG_POWER)) && mUnknown535) {
			mUnknownB8 = mUnknownB8 + 0.045;
			if (1.0 < mUnknownB8)
				mUnknownB8 = 1.0;
		}
		if (mUnknownB1) {
			float aValue = (float)((double)mUnknownC0 + 0.0625);
			mUnknownC0 = aValue;
			if (10.0f <= aValue)
				mUnknownC0 = (float)((double)aValue - 10.0);
		}
		if (mBreaking && mUnknown110 == 0 && !IsFlagSet(FLAG_1) && !IsFlagSet(FLAG_DEVIL)) {
			if (IsFlagSet(FLAG_COAL)) {
				// Coal burns up: it glows brighter frame by frame until it is gone
				mBreakFrame++;
				mUnknown29C = (float)((double)mBreakFrame * 0.02 + 0.2);
				int aBurnt = mBreakFrame - 30;
				if (aBurnt <= 0)
					aBurnt = 0;
				if (9 < aBurnt / 4)
					mBlasted = true;
			} else if (mGlow == 0.0f) {
				// A gem breaks up through its frames, scattering dust over its shape (effect 12) and a shard (23)
				mUnknownCC++;
				if (mUnknownCC % 3 == 0) {
					mBreakFrame++;
					if (6 < mBreakFrame)
						mBlasted = true;
				}
				if (gBoard->mUpdateCnt % 3 == 0) {
					for (int i = 3; i != 0; i--) {
						Effect* anEffect = gApp->mBoard->mFXManager->AllocEffect(12);
						AddBreakDust(anEffect);
						gApp->mBoard->mFXManager->AddEffect(anEffect);
					}
				}
				Effect* anEffect = gApp->mBoard->mFXManager->AllocEffect(23);
				anEffect->mX = (float)((double)(gGemWidth / 2) + (double)mX + (double)RandSigned() * (double)gGemWidth * 0.25);
				anEffect->mY = (float)((double)(gGemHeight / 2) + (double)mY + (double)RandSigned() * (double)gGemHeight * 0.25);
				float aFall = (float)((double)std::fabs(RandSigned()) * 0.02 + 0.01);
				anEffect->mGravity = aFall * gApp->mScale;
				gApp->mBoard->mFXManager->AddEffect(anEffect);
			}
		}
	} else {
		// The counter turns over (mUnknownD0 runs to 20 at mUnknownD4 an update); between 5 and 10 it ticks down
		float aTurn = mUnknownD4 + mUnknownD0;
		mUnknownD0 = aTurn;
		if (0.0f <= aTurn) {
			if (20.0f <= aTurn) {
				mUnknownD0 = 0.0f;
				mUnknownD4 = 0.0f;
			}
		} else {
			mUnknownD0 = (float)((double)aTurn + 20.0);
		}
		if ((IsFlagSet(FLAG_BOMB) || IsFlagSet(FLAG_DEVIL)) && mUnknownD4 != 0.0f && 5.0f < mUnknownD0 &&
			mUnknownD0 <= 10.0f) {
			if (IsFlagSet(FLAG_DEVIL))
				gApp->PlaySample(SOUND_DOOMGEM_COUNTDOWN);
			mBombCount--;
			MakeCountImage();
			mUnknownD0 = 16.0f;
			if (mBombCount == 0 && !mBreaking && !mBlasted) {
				mUnknownD4 = 0.0f;
				mOrigColor = mColor;
				mUnknownD0 = 0.0f;
			}
		}
	}

	if (0 < mUnknown110 && gBoard->mUnknown149C.size() == 0)
		mUnknown110--;
	if (mUnknown29C <= 0.0f) {
		mOffsetY = 0.0f;
		mOffsetX = 0.0f;
	} else {
		// The gem shakes by mUnknown29C
		float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
		mUnknown2A0 = anAngle;
		mOffsetX = (float)((double)mUnknown29C * (double)std::cos(anAngle) * (double)gGemWidth / 20.0);
		mOffsetY = (float)((double)mUnknown29C * (double)std::sin(mUnknown2A0) * (double)gGemHeight / 20.0);
	}
}

// Part of Piece::Update (0x72f7c3-0x72fb0d): where a breaking gem's dust starts, over the shape of its colour
void Piece::AddBreakDust(Effect* theEffect)
{
	float aWidth = (float)(gGemWidth - 20);
	float aHeight = (float)(gGemHeight - 20);
	int aShape = mColor;
	if (aShape == 0) {
		// A square
		theEffect->mX = (float)(Rand() % Ftol(aWidth));
		theEffect->mY = (float)(Rand() % Ftol(aHeight));
	} else if (0 < aShape) {
		if (aShape < 3 || aShape == 5) {
			// A round gem
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			float aDist = (float)((double)std::fabs(RandSigned()) * (double)aWidth * 0.5);
			theEffect->mX = (float)((double)aWidth * 0.5 + (double)std::cos(anAngle) * (double)aDist);
			theEffect->mY = (float)((double)aHeight * 0.5 + (double)std::sin(anAngle) * (double)aDist);
		} else if (aShape == 3) {
			// A diamond: narrower away from the middle
			float aY = (float)((double)std::fabs(RandSigned()) * (double)Ftol(aHeight));
			theEffect->mY = aY;
			double aHalf = (double)(float)((double)aHeight * 0.5);
			float aWide = (float)(1.0 - (double)std::fabs((float)(aHalf - (double)aY)) / aHalf);
			theEffect->mX = (float)((double)RandSigned() * (double)(float)((double)aWidth * 0.5 * (double)aWide) +
				(double)(gGemWidth / 2));
		} else if (aShape == 4) {
			// A triangle, wider to the bottom
			float aY = (float)((double)std::fabs(RandSigned()) * (double)Ftol(aHeight));
			theEffect->mY = aY;
			theEffect->mX = (float)(0.5 * (double)aWidth + ((double)theEffect->mY / (double)aHeight) * (double)aWidth *
				(double)RandSigned() * 0.5);
		} else if (aShape == 6) {
			// Round at the top, a point at the bottom
			float aY = (float)((double)std::fabs(RandSigned()) * (double)Ftol(aHeight));
			theEffect->mY = aY;
			float aHalfF = (float)((double)aHeight * 0.5);
			double aHalf = (double)aHalfF;
			if (aY < aHalfF) {
				float anAngle = (float)((double)std::fabs(RandSigned()) * 3.1415927410125732);
				float aDist = (float)((double)std::fabs(RandSigned()) * (double)aWidth * 0.5);
				theEffect->mX = (float)((double)aWidth * 0.5 + (double)std::cos(anAngle) * (double)aDist);
				theEffect->mY = (float)(aHalf - (double)std::sin(anAngle) * (double)aDist);
			} else {
				float aWide = (float)(1.0 - (double)std::fabs((float)(aHalf - (double)aY)) / aHalf);
				theEffect->mX = (float)((double)RandSigned() * (double)(float)((double)aWidth * 0.5 * (double)aWide) +
					(double)(gGemWidth / 2));
			}
		}
	}
	theEffect->mY = theEffect->mY + mY + 10.0f;
	theEffect->mX = mX + 10.0f + theEffect->mX;
	theEffect->mDY = 0.0f;
	theEffect->mDX = 0.0f;
	theEffect->mGravity = 0.0f;
	theEffect->mDAlpha = -0.03f;
}

// Part of Piece::Update (0x72e8f5-0x72f50b): an electro gem gives off sparks and keeps up to six arcs (any number, now
// and then), which wander and snap
void Piece::UpdateElectro()
{
	if (gBoard->mUpdateCnt % 5 == 0 && 0 < mBreakFrame)
		mBreakFrame--;
	bool aPower = IsFlagSet(FLAG_POWER);
	if (RandFloat() < 0.2f) {
		EffectsManager* aManager = gApp->mBoard->mFXManager2;
		Effect* anEffect = aManager->AllocEffect(5);
		float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
		float aRadius = (float)((double)GetRadius(anAngle) * (double)gApp->mScale);
		float aSpeed = (float)((double)RandFloat() * 0.10000000149011612 + 0.3499999940395355);
		anEffect->mFrame = 0;
		aSpeed = gApp->mScale * aSpeed;
		anEffect->mAlpha = 1.0f;
		anEffect->mScale = 1.0f;
		anEffect->mDScale = -0.0f;
		anEffect->mDAlpha = -0.012f;
		anEffect->mImage = IMAGE_SPARKLET;
		anEffect->mX = aRadius * std::cos(anAngle) + (float)(gGemWidth / 2);
		anEffect->mY = aRadius * std::sin(anAngle) + (float)(gGemHeight / 2);
		anEffect->mDX = (float)((double)std::cos(anAngle) * (double)aSpeed);
		anEffect->mDY = (float)((double)std::sin(anAngle) * (double)aSpeed);
		anEffect->mColor = kArcColors[mColor];
		anEffect->mPieceId = mId;
		aManager->AddEffect(anEffect);
	}

	bool aMore = RandFloat() < 0.2f;
	// port: with Direct3D the draw above decides; without it, as here, this one does
	aMore = RandFloat() < 0.001f;
	if ((int)mArcs.size() < 6 || aMore) {
		// A new arc: now and then one to a neighbouring piece, or one bright arc right round the gem; otherwise a
		// loop out from the outline and back
		Arc anArc;
		float aRoll = RandFloat();
		if (aPower)
			anArc.mToPiece = aRoll < 0.25f;
		else
			anArc.mToPiece = aRoll < 0.075f;
		bool aRound = false;
		if (!anArc.mToPiece) {
			if (aPower)
				aRound = RandFloat() < 0.1f;
			else
				aRound = RandFloat() < 0.02f;
		}
		if (aRound) {
			anArc.mBright = true;
			anArc.mStartAngle = (float)((double)std::fabs(RandSigned()) * 3.1415927410125732 * 2);
			anArc.mEndAngle = anArc.mStartAngle;
			float aSpeed = (float)((double)RandSigned() * 0.02);
			double aStartSpeed;
			if (0.0f <= aSpeed)
				aStartSpeed = 0.02 + (double)aSpeed;
			else
				aStartSpeed = (double)aSpeed - 0.02;
			anArc.mStartSpeed = (float)aStartSpeed;
			anArc.mEndSpeed = (float)((double)RandSigned() * 0.019999999552965164 - (double)anArc.mStartSpeed);
		} else {
			anArc.mBright = false;
			if (anArc.mToPiece) {
				anArc.mStartAngle = (float)((double)std::fabs(RandSigned()) * 3.1415927410125732 * 2);
				float aSin = std::sin(anArc.mStartAngle);
				float aCos = std::cos(anArc.mStartAngle);
				int aY = Ftol((double)(gGemHeight / 2) + (double)mY + (double)gGemHeight * (double)aSin * 0.6000000238418579);
				int aX = Ftol((double)(gGemWidth / 2) + (double)mX + (double)gGemWidth * (double)aCos * 0.6000000238418579);
				Piece* aPiece = gApp->mBoard->GetPieceAtXY(aX, aY);
				if (aPiece == nullptr || aPiece == this)
					anArc.mToPiece = false;
				else
					anArc.mTargetId = aPiece->mId;
			}
			if (anArc.mToPiece) {
				anArc.mEndAngle = (float)((double)anArc.mStartAngle + 3.1415927410125732 +
					(double)std::fabs(RandSigned()) * 0.5);
				anArc.mStartSpeed = (float)((double)RandSigned() * 0.029999999329447746);
				anArc.mEndSpeed = (float)((double)RandSigned() * 0.029999999329447746);
			} else {
				anArc.mStartAngle = (float)((double)std::fabs(RandSigned()) * 3.1415927410125732 * 2);
				anArc.mEndAngle = (float)(0.5 + (double)anArc.mStartAngle + (double)std::fabs(RandSigned()) * 0.5);
				anArc.mStartSpeed = (float)((double)RandSigned() * 0.007499999832361937);
				anArc.mEndSpeed = (float)((double)RandSigned() * 0.0020000000949949026 + (double)anArc.mStartSpeed);
			}
		}
		// port: with Direct3D an arc has 2 to 6 segments
		anArc.mSegments = Rand() % 2 + 3;
		for (int i = 0; i < anArc.mSegments; i++) {
			anArc.mOffsets[i] = (float)((double)RandSigned() * 10.0);
			anArc.mSpeeds[i] = (float)((double)RandSigned() * 0.20000000298023224);
		}
		mArcs.push_back(anArc);
	}

	for (int anArcIdx = 0; anArcIdx < (int)mArcs.size(); anArcIdx++) {
		Arc& anArc = mArcs[anArcIdx];
		bool aSnap = false;
		anArc.mStartAngle = anArc.mStartSpeed + anArc.mStartAngle;
		anArc.mEndAngle = anArc.mEndSpeed + anArc.mEndAngle;
		if (0 < anArc.mSegments) {
			int aSeg = 1;
			bool aMore;
			do {
				float& anOffset = anArc.mOffsets[aSeg - 1];
				float& aSpeed = anArc.mSpeeds[aSeg - 1];
				anOffset = aSpeed + anOffset;
				if (!anArc.mToPiece && !anArc.mBright) {
					// A loop's segments drift back towards the outline, settle and pull on each other
					if (0.0f < anOffset) {
						if (RandFloat() < 0.05f) {
							float aPull = (float)((4.0 - (double)anOffset) * 0.1);
							aSpeed = (float)((double)RandSigned() + (double)aPull);
						} else if (RandFloat() < 0.025f) {
							anOffset = (float)((double)RandFloat() * 18.0);
						} else if (RandFloat() < 0.04f) {
							aSpeed = (float)((double)RandSigned() * 2.5 + (double)aSpeed);
						}
					} else {
						anOffset = 0.0f;
						aSpeed = (float)((double)RandFloat() * 0.10000000149011612);
					}
					if (RandFloat() < 0.1f) {
						float aPrev = 0.0f;
						float aNext = 0.0f;
						if (-1 < aSeg - 2)
							aPrev = anArc.mOffsets[aSeg - 2];
						if (aSeg < anArc.mSegments)
							aNext = anArc.mOffsets[aSeg];
						anOffset = (float)(((double)aPrev + (double)anOffset + (double)aNext) / 3.0);
					}
					if (RandFloat() < 0.2f) {
						int anOther = Rand() % 3 - 2 + aSeg;
						if (-1 < anOther && anOther < anArc.mSegments) {
							float aGap = anArc.mOffsets[anOther] - anOffset;
							aSpeed = (float)((double)aGap * 0.2 + (double)aSpeed);
						}
					}
					if (RandFloat() < 0.1f) {
						float aPrev = 0.0f;
						float aNext = 0.0f;
						if (-1 < aSeg - 2)
							aPrev = anArc.mSpeeds[aSeg - 2];
						if (aSeg < anArc.mSegments)
							aNext = anArc.mSpeeds[aSeg];
						aSpeed = (float)(((double)aNext + (double)aPrev) * 0.5);
					}
				} else {
					// An arc to a piece or a bright arc jumps about more, a bright one more often
					float aChance = anArc.mToPiece ? 0.05f : 0.1f;
					if (25.0f <= std::fabs(anOffset)) {
						aSpeed = (float)((double)aSpeed * -0.6499999761581421);
					} else if (RandFloat() < 0.2f) {
						anOffset = (float)((double)RandSigned() * 15.0);
					} else if (RandFloat() < aChance) {
						aSpeed = (float)((double)RandSigned() * 1.5 + (double)aSpeed);
					} else if (RandFloat() < aChance) {
						aSpeed = (float)((double)RandSigned() * 1.5);
					}
				}
				if (32.0f < anOffset)
					aSnap = true;
				aMore = aSeg < anArc.mSegments;
				aSeg++;
			} while (aMore);
		}
		if (!anArc.mToPiece) {
			if (!anArc.mBright) {
				if (RandFloat() < 0.005f)
					aSnap = true;
			} else {
				if (RandFloat() < 0.001f)
					aSnap = true;
				if (6.2831854820251465 <= std::fabs(anArc.mStartAngle - anArc.mEndAngle))
					aSnap = true;
			}
		} else {
			// An arc to a piece snaps once the piece is gone, too far, or no longer in the arc's way
			float aRadius = (float)((double)GetRadius(anArc.mStartAngle) + 0.0);
			float aStartX = (float)((double)std::cos(anArc.mStartAngle) * (double)aRadius);
			float aStartY = (float)((double)std::sin(anArc.mStartAngle) * (double)aRadius);
			Piece* aPiece = gApp->mBoard->GetPieceById(anArc.mTargetId);
			if (aPiece == nullptr) {
				aSnap = true;
			} else {
				float anEndRadius = GetRadius(anArc.mEndAngle);
				float aScale = gApp->mScale;
				float anEndX = anEndRadius * std::cos(anArc.mEndAngle) + (aPiece->mX - mX) / aScale;
				float aScaleY = gApp->mScale;
				float aDX = aStartX - anEndX;
				float aDY = aStartY - (anEndRadius * std::sin(anArc.mEndAngle) + (aPiece->mY - mY) / aScaleY);
				if (90.0f < std::sqrt(aDX * aDX + aDY * aDY))
					aSnap = true;
				float aDir = atan2f(aPiece->mY - mY, aPiece->mX - mX);
				float aStartCos = std::cos(anArc.mStartAngle);
				float aDirCos = std::cos(aDir);
				float aStartSin = std::sin(anArc.mStartAngle);
				if ((float)((double)aDirCos * (double)aStartCos + (double)std::sin(aDir) * (double)aStartSin) < 0.75f)
					aSnap = true;
				float anEndCos = std::cos(anArc.mEndAngle);
				float aBack = aDir + 3.1415927f;
				float aBackCos = std::cos(aBack);
				float anEndSin = std::sin(anArc.mEndAngle);
				if ((float)((double)aBackCos * (double)anEndCos + (double)std::sin(aBack) * (double)anEndSin) < 0.75f)
					aSnap = true;
			}
			if (RandFloat() < 0.001f)
				aSnap = true;
		}
		if (aSnap) {
			mArcs.erase(mArcs.begin() + anArcIdx);
			anArcIdx--;
		}
	}

	if (aPower && (unsigned int)mUpdateCnt % 50 == 0) {
		EffectsManager* aManager = gBoard->mFXManager2;
		Effect* anEffect = aManager->AllocEffect(0);
		anEffect->mX = (float)(gGemWidth / 2) + mX;
		anEffect->mY = (float)(gGemHeight / 2) + mY;
		anEffect->mImage = IMAGE_BURNNOVA;
		anEffect->mScale = 0.5f;
		anEffect->mIsAdditive = true;
		anEffect->mDScale = 0.005f;
		anEffect->mAlpha = 1.0f;
		anEffect->mDAlpha = -0.01f;
		aManager->AddEffect(anEffect);
	}
}

void Piece::MakeCountImage()
{
	SexyString aText = StrFormat(_S("%d"), mBombCount);
	Font* aFont = FONT_COUNTER;
	int aWidth = Ftol((double)aFont->StringWidth(aText) * 1.2999999523162842);
	int aFontHeight = aFont->mHeight - aFont->mAscent;
	int aHeight = gGemHeight;
	if (gGemHeight <= aFontHeight)
		aHeight = aFontHeight;
	aHeight = aHeight - Ftol((double)gApp->mScale * -18.0);
	// port: the original also sets each image's +0x44 to 0x80, a Direct3D flag
	if (mCountImage == nullptr) {
		mCountImage = new MemoryImage(gApp);
		mCountImage->SetImageMode(true, true);
	}
	if (mCountImage2 == nullptr) {
		mCountImage2 = new MemoryImage(gApp);
		mCountImage2->SetImageMode(true, true);
	}
	if (mCountImage->mWidth < aWidth || mCountImage->mHeight < aHeight) {
		mCountImage->Create(aWidth, aHeight);
		mCountImage2->Create(aWidth, aHeight);
	}
	mCountImage->Clear();
	mCountImage2->Clear();

	// The number in the gem's colour at half brightness (a devil gem's purple), then with a red outline
	Graphics g(mCountImage);
	g.SetColorizeImages(true);
	Color aLayerColor;
	if (!IsFlagSet(FLAG_DEVIL)) {
		const Color& aGemColor = gGemColors[mColor];
		aLayerColor = Color(aGemColor.mRed / 2, aGemColor.mGreen / 2, aGemColor.mBlue / 2, aGemColor.mAlpha);
	} else {
		aLayerColor = Color(0x600020);
	}
	SetFontLayerColor(aFont, 0, Color::Black);
	SetFontLayerColor(aFont, 1, aLayerColor);
	SetFontLayerColor(aFont, 2, Color::White);
	SetFontLayerColor(aFont, 3, Color::White);
	g.SetFont(aFont);
	int aLeft = Ftol((double)gApp->mScale * 0.0);
	int aCount = mBombCount;
	int aX = (mCountImage->mWidth - g.StringWidth(aText)) / 2 + aLeft;
	if (9 < aCount && aCount < 20)
		aX = aX + Ftol((double)gApp->mScale * -15.0);
	if (aCount == 1)
		aX = aX + Ftol((double)gApp->mScale * -11.0);
	int aDown = Ftol((double)gApp->mScale * 30.0);
	int aY = g.GetFont()->mHeight + (mCountImage->mHeight / 2 - g.GetFont()->mAscent) + aDown;
	g.SetColor(Color(255, 255, 255));
	g.DrawString(aText, aX, aY);
	SetFontLayerColor(aFont, 0, Color::Black);
	SetFontLayerColor(aFont, 1, Color::White);
	SetFontLayerColor(aFont, 2, Color(255, 0, 0));
	SetFontLayerColor(aFont, 3, Color(255, 0, 0));
	Graphics g2(mCountImage2);
	g2.SetColorizeImages(true);
	g2.SetFont(aFont);
	g2.SetColor(Color(255, 255, 255));
	g2.DrawString(aText, aX, aY);

	if (mBombCount < 9) {
		if (mCountPulse.mIncRate == 0.0) {
			mCountPulse.SetCurve(GTS(0x7e99f0));
			mCountPulse.mMode = 1;
		}
	} else {
		mCountPulse.mInVal = 0.0;
		mCountPulse.mIncRate = 0.0;
	}
}

void Piece::BreakLock(bool theType)
{
	gBoard->BreakSpecial(this, theType);
	gBoard->AddStat(0x48, 1);
	ClearFlag(FLAG_LOCKED);
}

void Piece::Blast()
{
	// 0x729363: a power gem goes off: a blast one gem round, the gems above jump, and 50 points
	if (mBlasted || !IsFlagSet(FLAG_POWER))
		return;
	gApp->PlaySample(SOUND_PREBLAST);
	mBlasted = true;
	if (!gApp->mProfile->GetFlag1E4(0x10)) {
		// the first power gem's tip
		gApp->mBoard->mUnknown3904 = true;
		gApp->mProfile->SetFlag1E4(0x10, true);
	}
	gApp->mBoard->mUnknown1660.push_back(new BlastShockwave(this, 1, 1));  // 0x6bc3cd
	gApp->mBoard->LiftAround(this, 1.0f);
	// (unless the board is +0x37c4's, the original adds a heat wave here, 0x6f9efe: Direct3D only)
	gApp->mBoard->PlayPieceSound(this, SOUND_GEM_SHATTERS, 0, 0, true);
	gApp->mBoard->AddPoints(Ftol((double)(gGemWidth / 2) + (double)mX),
		Ftol((double)(gGemHeight / 2) + (double)mY), 50, gGemColors[mColor], mUnknown2EC, true, true, false, true);
}
