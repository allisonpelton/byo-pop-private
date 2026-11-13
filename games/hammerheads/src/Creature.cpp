#include "Creature.h"

#include <cmath>
#include <map>

#include "Application.h"
#include "LevelRules.h"
#include "HammerDisplay.h"
#include "Common.h"
#include "Cubby.h"
#include "DDImage.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Gameplay.h"
#include "GnomeBomb.h"
#include "GnomeBonus.h"
#include "GnomeBubbleHead.h"
#include "GnomeChallenge.h"
#include "GnomeChest.h"
#include "GnomeClover.h"
#include "GnomeCoin.h"
#include "GnomeCupido.h"
#include "GnomeForkyChap.h"
#include "GnomeHeart.h"
#include "GnomeHide.h"
#include "GnomeKing.h"
#include "GnomeMultiplier.h"
#include "GnomeNapoleon.h"
#include "GnomePowerUp.h"
#include "GnomeRabbit.h"
#include "GnomeSave.h"
#include "GnomeShop.h"
#include "GnomeSilver.h"
#include "GnomeStrawberry.h"
#include "GnomeStrong.h"
#include "GnomeSwitcher.h"
#include "GnomeWoman.h"
#include "Graphics.h"
#include "Hearts.h"
#include "ImageBlast.h"
#include "MemoryImage.h"
#include "Popper.h"
#include "Res.h"
#include "ShopItem.h"
#include "Stats.h"

using namespace Sexy;

std::vector<Creature*> Sexy::gCreatures; // 0x597f30

// What a kind starts with: the table 0x430340 fills a new creature from. The anchor and the strike point are in pixels of
// the sheet's cell; a carried item is the kind of the creature that comes out when this one is smashed (ITEM_NONE for no
// drop), one of the entries is picked by GetCarriedItem.
namespace
{
struct KindData
{
	int				mKind;
	Image**			mImage;
	Image**			mShadow;
	double			mAnchorX, mAnchorY, mHeadX, mHeadY;
	double			mHeight, mHeightVelocity, mGravity;
	int				mHealth, mScore;
	bool			mCountsTowardLimit;
	int				mEscapePenalty;
	int				mVariant;
	bool			mIsGnome;
	std::vector<int> mCarried;
};
}

static const KindData KIND_DATA[NUM_KINDS] = {
#include "CreatureKinds.inc"
};

// 0x430340
Creature::Creature(int theKind)
{
	mKind = theKind;
	mAnchorX = 0.0;
	mAnchorY = 0.0;
	mHeadX = 0.0;
	mHeadY = 0.0;
	mScore = 0;
	mCountsTowardLimit = false;
	mAge = 0;
	mFrozenTimer = 0;
	mDarkened = false;
	mShielded = false;
	mShatters = false;
	mEscapePenalty = 0;
	mVariant = 0;
	mIsGnome = false;
	mUntouchable = false;
	mFinished = false;
	mSpent = false;
	mX = 0.0;
	mY = 0.0;
	mTopY = 1.7976931348623157e308; // DBL_MAX
	mExposed = 0.0;
	mGlow = 0.0;
	mCubby = nullptr;
	mCarriedItem = -1;
	mCubbyReleased = false;
	gCreatures.push_back(this);

	if (theKind >= 0 && theKind < NUM_KINDS)
	{
		const KindData& aData = KIND_DATA[theKind];
		mImage = aData.mImage != nullptr ? *aData.mImage : nullptr;
		mShadowImage = aData.mShadow != nullptr ? *aData.mShadow : nullptr;
		mAnchorX = aData.mAnchorX;
		mAnchorY = aData.mAnchorY;
		mHeadX = aData.mHeadX;
		mHeadY = aData.mHeadY;
		mHeight = aData.mHeight;
		mHeightVelocity = aData.mHeightVelocity;
		mGravity = aData.mGravity;
		mHealth = aData.mHealth;
		mScore = aData.mScore;
		mCountsTowardLimit = aData.mCountsTowardLimit;
		mEscapePenalty = aData.mEscapePenalty;
		mVariant = aData.mVariant;
		mIsGnome = aData.mIsGnome;
		mCarriedItems = aData.mCarried;
	}
	else
	{
		mImage = nullptr;
		mShadowImage = nullptr;
		mHeight = 0.0;
		mHeightVelocity = 0.0;
		mGravity = 0.0;
		mHealth = 0;
	}
	mState = 0;
	mGlowImage = mImage != nullptr ? GetGlowImage(mImage) : nullptr;
}

// 0x42f8c0
Creature::~Creature()
{
	// takes every entry of this creature out of the list, keeping the order of the others
	std::vector<Creature*>::iterator aKept = gCreatures.begin();
	for (std::vector<Creature*>::iterator anItr = gCreatures.begin(); anItr != gCreatures.end(); ++anItr)
	{
		if (*anItr != this)
			*aKept++ = *anItr;
	}
	gCreatures.erase(aKept, gCreatures.end());

	// it got away alive: the player pays for it
	if (mHealth > 0 && mFrozenTimer < 1 && mEscapePenalty != 0)
	{
		if (gApp->mGameScreen != nullptr && !gApp->IsShopOpen() && !gApp->IsChallengeOpen() && GetGameplay()->mPhase == 0)
		{
			if (GetHearts() != nullptr && GetHearts()->mActive)
				GetHearts()->ChangeHealth(HEALTH_ESCAPE, (double)mEscapePenalty);

			GetStats()->SetInt(STAT_LEVEL_OF_LAST_ESCAPE, gApp->mGameScreen->mLevel);
			GetStats()->SetInt(STAT_QUICK_STREAK, 0);
			double aCombo = GetStats()->Get(STAT_COMBO);
			if (1.0 < aCombo)
			{
				aCombo = aCombo - 0.1;
				if (aCombo <= 1.0)
					aCombo = 1.0;
				GetStats()->Set(STAT_COMBO, aCombo);
			}
		}
	}
}

// 0x42f730
void Creature::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = 0.0;
}

// 0x42f410
void Creature::Update(const double thePos[2])
{
	double aVelocity = mGravity + mHeightVelocity;
	mAge++;
	mHeightVelocity = aVelocity;
	double aHeight = aVelocity + mHeight;
	if (aHeight < 0.0)
		aHeight = 0.0;
	mHeight = aHeight;
	if (aHeight == 0.0)
	{
		mHeightVelocity = aVelocity * -0.4;
		if (aVelocity * -0.4 < 0.8)
			mHeightVelocity = 0.0;
	}

	mX = thePos[0];
	mY = thePos[1];
	mTopY = mTopY <= thePos[1] ? mTopY : thePos[1];
	if (thePos[1] <= mTopY)
		mExposed = 0.0;
	else
		mExposed = thePos[1] - mTopY;

	double aGlow;
	if (mCubby == nullptr || !mCubby->mHover)
	{
		aGlow = mGlow - 0.02;
		if (aGlow <= 0.0)
			aGlow = 0.0;
	}
	else
	{
		aGlow = mGlow + 0.08;
		if (1.0 <= aGlow)
			aGlow = 1.0;
	}
	mGlow = aGlow;

	mFrozenTimer = mFrozenTimer - 1;
	if (mFrozenTimer < 0)
		mFrozenTimer = 0;
}

// 0x42f400: what the level introduction and the game-over box call for their gnome, which is in no cubby; the base only ages it
void Creature::UpdateWithoutCubby(const double thePos[2])
{
	mAge++;
}

// 0x42f550
void Creature::DrawShadow(Graphics* g, double theX, double theY)
{
	if (mShadowImage == nullptr)
		return;

	g->SetColorizeImages(true);
	double anAlpha = 255.0 - mHeight * 6.0;
	if (!(0.0 < anAlpha))
		anAlpha = 0.0;
	g->SetColor(Color(255, 255, 255, (int)anAlpha));
	Image* aShadow = mShadowImage;
	g->DrawImage(aShadow, (int)(theX - (double)aShadow->mWidth * 0.5), (int)(theY - (double)aShadow->mHeight * 0.5));
	g->SetColorizeImages(false);
}

// 0x42f600
void Creature::GetPositionInCell(double thePos[2], double theX, double theY) const
{
	thePos[0] = theX - mAnchorX;
	thePos[1] = (theY - mAnchorY) - mHeight;
}

// 0x42f620: how much of the flash shows at a point of the flash's time (0 to pi); the bomb and a silver gnome in its last (fading) stage never flash
int Creature::FlashAlpha(double theTime) const
{
	if (0.0 <= theTime && theTime <= 3.1415926535 && mKind != KIND_BOMB)
	{
		if (mKind == KIND_SILVER)
		{
			const GnomeSilver* aSilver = dynamic_cast<const GnomeSilver*>(this);
			if (aSilver != nullptr && aSilver->IsFading())
				return 0;
		}
		return (int)(fabs(sin(((4.0 - (theTime * 0.31830988619288864 + theTime * 0.31830988619288864)) * theTime) * 5.0)) * 255.0);
	}
	return 0;
}

// 0x42f6f0
bool Creature::Hit()
{
	int aDamage = GetHammerDamage();
	if (mHealth != aDamage && -1 < mHealth - aDamage)
	{
		mHealth -= GetHammerDamage();
		return mHealth == 0;
	}
	mHealth = 0;
	return true;
}

// 0x42f750
bool Creature::PixelHit(const double thePos[2], int theX, int theY)
{
	double aLocalX = thePos[0] - mAnchorX;
	double aLocalY = (thePos[1] - mAnchorY) - mHeight;
	int aCellW = mImage->mWidth / mImage->mNumCols;
	int aCellH = mImage->mHeight / mImage->mNumRows;
	double aCell[2];
	GetSheetCell(aCell);

	int aX = theX - (int)aLocalX;
	int aY = theY - (int)aLocalY;
	if (aX >= 0 && aY < aCellH && aY >= 0)
	{
		int aPixelX = (int)aCell[1] * aCellW + aX;
		int aPixelY = (int)aCell[0] * aCellH + aY;
		if (aPixelX >= 0 && aPixelY >= 0 && aPixelX < mImage->mWidth && aPixelY < mImage->mHeight)
		{
			ulong* aBits = ((MemoryImage*)mImage)->GetBits();
			return (aBits[mImage->mWidth * aPixelY + aPixelX] & 0xff000000) != 0;
		}
	}
	return false;
}

// 0x4300a0: does any point of the mask, laid at (theX, theY) and kept inside the rect, touch a solid pixel of the creature
bool Creature::MaskHit(const double thePos[2], int theX, int theY, const std::vector<Point>& theMask, int theRectX, int theRectY, int theRectW, int theRectH)
{
	double aCell[2];
	GetSheetCell(aCell);
	int aCellW = mImage->mWidth / mImage->mNumCols;
	int aCellH = mImage->mHeight / mImage->mNumRows;
	double aLocalX = thePos[0] - mAnchorX;
	double aLocalY = (thePos[1] - mAnchorY) - mHeight;

	for (int i = 0; i < (int)theMask.size(); i++)
	{
		int aX = theMask[i].mX + theX;
		int aY = theMask[i].mY + theY;
		if (aX >= theRectX && aX < theRectX + theRectW && aY >= theRectY && aY < theRectY + theRectH)
		{
			aX = aX - (int)aLocalX;
			aY = aY - (int)aLocalY;
			if (aX >= 0 && aX < aCellW && aY >= 0 && aY < aCellH)
			{
				int aPixelX = (int)aCell[1] * aCellW + aX;
				int aPixelY = (int)aCell[0] * aCellH + aY;
				ulong* aBits = ((MemoryImage*)mImage)->GetBits();
				if ((aBits[mImage->mWidth * aPixelY + aPixelX] & 0xff000000) != 0)
					return true;
			}
		}
	}
	return false;
}

// 0x430040
Rect Creature::GetScreenRect(const double thePos[2]) const
{
	int aCellH = mImage->mHeight / mImage->mNumRows;
	int aCellW = mImage->mWidth / mImage->mNumCols;
	int aY = (int)((thePos[1] - mAnchorY) - mHeight);
	int aX = (int)(thePos[0] - mAnchorX);
	return Rect(aX, aY, aCellW, aCellH);
}

// 0x4314d0: the health as hearts over the creature, or "x 7" when there are too many to line up
void Creature::DrawHealth(Graphics* g, double theX, double theY, int theHealth)
{
	if (mDarkened)
		return;

	Image* aHeart = IMAGE_TINYHEART;
	if (theHealth < 6)
	{
		int aX = (int)(theX - (double)((aHeart->mWidth * theHealth) / 2));
		int aY = (int)((theY - mHeight) - 10.0);
		for (int i = 0; i < theHealth; i++)
			g->DrawImage(IMAGE_TINYHEART, aHeart->mWidth * i + aX, aY);
	}
	else
	{
		int aX = (int)theX - 20;
		int aY = (int)((theY - mHeight) - 10.0);
		g->DrawImage(IMAGE_TINYHEART, aX, aY);
		g->SetFont(FONT_ARIAL10B);
		std::string aText = StrFormat("x %d", theHealth);
		g->SetColor(Color(0, 0, 0));
		g->DrawString(aText, IMAGE_TINYHEART->mWidth + 4 + aX, aY + 11);
		g->DrawString(aText, IMAGE_TINYHEART->mWidth + 4 + aX, aY + 13);
		g->DrawString(aText, IMAGE_TINYHEART->mWidth + 2 + aX, aY + 11);
		g->DrawString(aText, IMAGE_TINYHEART->mWidth + 2 + aX, aY + 13);
		g->SetColor(Color(100, 0, 0));
		g->DrawString(aText, IMAGE_TINYHEART->mWidth + 3 + aX, aY + 12);
	}
}

// 0x42fab0: the sprite, with its glow (while the hammer is over it), the flash when it is hit and the ice when it is frozen
void Creature::DrawBody(Graphics* g, double theX, double theY, int theCol, int theRow, int theFlash, double theAlpha)
{
	double aGlow = mDarkened ? 1.0 : mGlow;
	Graphics aGraphics(*g);
	Rect aSrcRect = mImage->GetCelRect(theCol, theRow);
	int aFrozenTimer = mFrozenTimer;

	Rect aGlowDest(0, 0, 0, 0);
	Rect aGlowSrc(0, 0, 0, 0);
	if (aGlow > 0.0 || theFlash != 0 || aFrozenTimer > 0)
	{
		Rect aGlowCell = mGlowImage->GetCelRect(theCol, theRow);
		aGlowSrc = Rect(aGlowCell.mX + 1, aGlowCell.mY + 1, aGlowCell.mWidth - 2, aGlowCell.mHeight - 2);
		aGlowDest = Rect((int)theX - 2, (int)theY - 2, aSrcRect.mWidth + 4, aSrcRect.mHeight + 4);
	}

	if (aGlow <= 0.0)
	{
		if (theFlash != 0)
		{
			aGraphics.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			Image* aBackdrop = GetGameplay()->GetBackdrop();
			if (aBackdrop == IMAGE_BACKDROP_WINTER_1 || aBackdrop == IMAGE_BACKDROP_SUMMER_1 || gApp->mGameMode == GAME_MODE_MARATHON)
				aGraphics.SetColor(Color(0x10, 0x90, 0x10, theFlash));
			else
				aGraphics.SetColor(Color(0x40, 0x40, 0xff, theFlash));
			aGraphics.SetColorizeImages(true);
			aGraphics.SetFastStretch(false);
			aGraphics.DrawImage(mGlowImage, aGlowDest, aGlowSrc);
		}
	}
	else
	{
		int anAlpha;
		bool anAdditive = true;
		if (!gApp->IsShopOpen())
		{
			if (gApp->IsChallengeOpen())
			{
				anAdditive = true;
			}
			else
			{
				Image* aBackdrop = GetGameplay()->GetBackdrop();
				if (aBackdrop != IMAGE_BACKDROP_WINTER_1 && aBackdrop != IMAGE_BACKDROP_SUMMER_1 && gApp->mGameMode != GAME_MODE_MARATHON)
					anAdditive = true;
				else
					anAdditive = false;
			}
		}
		anAlpha = (int)(Ease(aGlow, 0.2, 0.8) * theAlpha * 255.0);
		if (anAdditive)
		{
			aGraphics.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			aGraphics.SetColor(Color(255, 255, 255, anAlpha));
		}
		else
		{
			aGraphics.SetDrawMode(Graphics::DRAWMODE_NORMAL);
			aGraphics.SetColor(Color(0x20, 0x20, 0xff, anAlpha));
		}
		aGraphics.SetColorizeImages(true);
		aGraphics.SetFastStretch(false);
		aGraphics.DrawImage(mGlowImage, aGlowDest, aGlowSrc);
	}

	if (theAlpha == 1.0)
	{
		aGraphics.SetColorizeImages(false);
	}
	else
	{
		aGraphics.SetColorizeImages(true);
		aGraphics.SetColor(Color(255, 255, 255, (int)(theAlpha * 255.0)));
	}
	if (mDarkened)
	{
		aGraphics.SetColorizeImages(true);
		aGraphics.SetColor(Color(0, 0, 0, 0x8c));
	}
	aGraphics.SetDrawMode(Graphics::DRAWMODE_NORMAL);
	aGraphics.DrawImage(mImage, (int)theX, (int)theY, aSrcRect);

	if (0.0 < aGlow)
	{
		int aLevel = (int)(Ease(aGlow, 0.2, 0.8) * theAlpha * 32.0);
		aGraphics.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		aGraphics.SetColorizeImages(true);
		aGraphics.SetColor(Color(aLevel, aLevel, aLevel));
		aGraphics.DrawImage(mGlowImage, aGlowDest, aGlowSrc);
	}

	if (0 < aFrozenTimer)
	{
		aGraphics.SetDrawMode(Graphics::DRAWMODE_NORMAL);
		int aFrozenAlpha;
		if (mFrozenTimer < 0x1f5 && 0x46 < mFrozenTimer)
			aFrozenAlpha = 200;
		else if (mFrozenTimer > 500)
			aFrozenAlpha = (int)((double)(600 - mFrozenTimer) * 0.01 * 200.0);
		else
			aFrozenAlpha = (int)((1.0 - (double)(70 - mFrozenTimer) * 0.014285714285714285) * 200.0);
		aGraphics.SetColor(Color(0x96, 0x96, 0xff, aFrozenAlpha));
		aGraphics.SetColorizeImages(true);
		aGraphics.SetFastStretch(false);
		aGraphics.DrawImage(mGlowImage, aGlowDest, aGlowSrc);
	}
}

// 0x4316d0
void Creature::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
{
	Graphics aGraphics(*g);
	DrawShadow(&aGraphics, theX, theY);

	double aCell[2];
	GetSheetCell(aCell);
	double aTopLeftY = (theY - mAnchorY) - mHeight;
	double aTopLeftX = theX - mAnchorX;
	int aFlash = FlashAlpha(theFlashTime);
	DrawBody(&aGraphics, aTopLeftX, aTopLeftY, (int)aCell[1], (int)aCell[0], aFlash, 1.0);

	// the signs and the shop's items, the strawberry and the other bonuses show no health
	int aKind = mKind;
	bool aShowsHealth = !((aKind >= 0x1c && aKind <= 0x25) || aKind == KIND_SHOP || aKind == KIND_MULTIPLIER || aKind == KIND_RABBIT || aKind == KIND_BONUS_DUCK ||
		aKind == KIND_STRAWBERRY || aKind == KIND_BONUS_ICE_CREAM || aKind == KIND_CHALLENGE || aKind == KIND_HEART || aKind == KIND_CHEST ||
		aKind == KIND_CUPIDO || aKind == KIND_BOUNCING || aKind == KIND_FLYING || aKind == KIND_SAVE);
	if (aShowsHealth)
	{
		DrawHealth(&aGraphics, theX, theY, GetHealth());
		if (gApp->mLevelIntro != nullptr && mAge % 60 < 30)
		{
			aGraphics.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			DrawHealth(&aGraphics, theX, theY, GetHealth());
		}
	}
}

// 0x4301f0: the pieces it breaks into when it is smashed
ImageBlast* Creature::SpawnPieces(bool theFlag)
{
	double aCell[2];
	GetSheetCell(aCell);
	int aCellW = mImage->GetCelWidth();
	int aCellH = mImage->GetCelHeight();
	Rect aSrcRect((int)((double)aCellW * aCell[1]), (int)((double)aCellH * aCell[0]), aCellW, aCellH);
	double aX = mX - mAnchorX;
	double aY = ((mY + mHeight) - mAnchorY) - mHeight;

	Cubby* aCubby = nullptr;
	if (mCubby->IsSinking() || mCubby->IsRising())
		aCubby = mCubby;

	int aStyle;
	if (mKind == KIND_BUBBLEHEAD)
		aStyle = 1;
	else
		aStyle = (mKind != KIND_KING ? 0 : 2);
	bool aShatter = mFrozenTimer > 0 || mShatters;
	return MakeImageBlast(aStyle, aX, aY, mImage, &aSrcRect, aCellH / 20, aCellW / 20, mY, aCubby, theFlag, aShatter);
}

// 0x431ac0
std::string Creature::SaveExtra()
{
	return "";
}

// 0x4314b0
void Creature::LoadExtra(const std::string& theData)
{
}

// 0x408d10
void Creature::Unused20()
{
}

// 0x42fa80
int Creature::GetScore()
{
	return mScore;
}

// 0x42fa90
void Creature::Remove()
{
	mUntouchable = true;
	mFinished = true;
}

// 0x42faa0
int Creature::GetHealth()
{
	return mHealth;
}

// 0x42f350
void Creature::SetFrozen(bool theFrozen)
{
	if (theFrozen)
	{
		mFrozenTimer = 600;
		double aPos[2];
		if (mCubby == nullptr)
		{
			aPos[0] = mX;
			aPos[1] = mY;
		}
		else
		{
			mCubby->GetPlatformCenter(aPos);
		}
		double aDirection[2] = { 0.0, 0.0 };
		MakePopper(aPos[0], aPos[1] - 100.0, 4, aDirection);
		return;
	}
	mFrozenTimer = 0;
}

// 0x42f6b0
void Creature::SetState(int theState)
{
	mState = theState;
	if (theState == 1 && mHeight == 0.0 && mKind != KIND_MULTIPLIER && mKind != KIND_RABBIT)
		mHeightVelocity = 1.5;
}

// 0x42f8a0
void Creature::SetCubby(Cubby* theCubby)
{
	mCubby = theCubby;
}

// 0x42f8b0
void Creature::ClearEscapePenalty()
{
	mEscapePenalty = 0;
}

// 0x42f340
bool Creature::IsFrozen() const
{
	return 0 < mFrozenTimer;
}

// 0x42f840: what a creature drops when the rules give it something extra: a coin, a big coin, or a heart when the player is hurt
static int PickBonusDrop()
{
	static const int aDrops[3] = { KIND_COIN, KIND_BIG_COIN, KIND_HEART };
	double aHealth = GetPlayerHealth();
	double aCapacity = GetHealthCapacity();
	int aChoices = 2;
	if (aHealth != aCapacity)
		aChoices = 3;
	return aDrops[Rand() % aChoices];
}

// 0x4318b0
int Creature::GetCarriedItem()
{
	if (mCarriedItem == -1)
	{
		int aCount = (int)mCarriedItems.size();
		if (aCount < 1)
		{
			mCarriedItem = ITEM_NONE;
		}
		else
		{
			int aRoll = Rand();
			mCarriedItem = mCarriedItems[aRoll % aCount];
		}

		for (int i = 0; i < (int)mCarriedItems.size(); i++)
		{
			if (mCarriedItems[i] != ITEM_NONE)
			{
				if (mCarriedItem == ITEM_NONE && 0.0 < GetGameplay()->mCloverTimer)
					mCarriedItem = PickBonusDrop();

				if (gApp->mGameMode == GAME_MODE_CLASSIC || gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
				{
					if (GetPlayerHealth() < 1.0)
					{
						int aLevel = gApp->mGameScreen->mLevel;
						int aHeartCount = CountHeartCreatures();
						if ((aLevel < 4 && aHeartCount < 2) || (aLevel < 6 && aHeartCount == 0))
							mCarriedItem = KIND_HEART;
					}
				}
				break;
			}
		}

		if (mKind == KIND_GNOME && mCubby != nullptr)
		{
			int aLevel = gApp->mGameScreen->mLevel;
			if (aLevel == 1 && !GetGameplay()->mFlagCC)
			{
				if (GetWaveIndex() == 0 && gApp->mGameMode == GAME_MODE_CLASSIC)
				{
					mCarriedItem = ITEM_NONE;
				}
				else if (mCubby->mName == "class Sexy::Cubby_00001")
				{
					mCarriedItem = KIND_COIN;
				}
			}
		}

		if (gApp->mGameMode == GAME_MODE_MARATHON && mCarriedItem == KIND_HEART)
			mCarriedItem = KIND_COIN;

		if (mCubby != nullptr && mCubby->IsSinking())
			mCarriedItem = ITEM_NONE;
	}
	return mCarriedItem;
}

// 0x41d040: a ramp: quadratic up to theStart, then straight, then settling to 1 at the top; held between 0 and 1
double Sexy::Ease(double theValue, double theStart, double theEnd)
{
	double aSlope = 2.0 / ((theEnd + 1.0) - theStart);
	double aResult;
	if (theStart <= theValue)
	{
		double aRamp = aSlope * theStart * 0.5;
		if (theEnd <= theValue)
		{
			aResult = (theEnd - theStart) * aSlope + aRamp +
				((theEnd * theEnd * 0.5 + ((theValue - theValue * theValue * 0.5) - theEnd)) * aSlope) / (1.0 - theEnd);
		}
		else
		{
			aResult = (theValue - theStart) * aSlope + aRamp;
		}
	}
	else
	{
		aResult = (aSlope * theValue * theValue) / (theStart + theStart);
	}
	if (aResult < 0.0)
		return 0.0;
	if (1.0 < aResult)
		aResult = 1.0;
	return aResult;
}

// 0x41c4c0
Image* Sexy::GetGlowImage(Image* theImage)
{
	static std::map<Image*, Image*> aCache; // 0x597934

	std::map<Image*, Image*>::iterator anItr = aCache.find(theImage);
	if (anItr != aCache.end())
		return anItr->second;

	// how much the sheet is shrunk by: the glow is a small soft copy that is drawn stretched
	int aWidthDivisor = 4;
	int aHeightDivisor = 4;
	if (theImage == IMAGE_GNOME_1)
	{
		aWidthDivisor = 2;
	}
	else if (theImage == IMAGE_GNOME_BOMB || theImage == IMAGE_GNOME_BUBBLEHEAD)
	{
		aWidthDivisor = 3;
		aHeightDivisor = 4;
	}
	else if (theImage == IMAGE_GNOME_HIDE)
	{
		aWidthDivisor = 4;
		aHeightDivisor = 4;
	}
	else if (theImage == IMAGE_GNOME_ARNOLD)
	{
		aWidthDivisor = 4;
		aHeightDivisor = 2;
	}
	else if (theImage == IMAGE_GNOME_FORK)
	{
		aWidthDivisor = 4;
		aHeightDivisor = 5;
	}
	else if (theImage == IMAGE_PLAYER_HAMMER_2 || theImage == IMAGE_PLAYER_HAMMER_3 || theImage == IMAGE_PLAYER_HAMMER_4 || theImage == IMAGE_SHOP_FLAME ||
		theImage == IMAGE_SHOP_HEART || theImage == IMAGE_SHOP_HEART_BIG || theImage == IMAGE_SHOP_HEART_EXTRA)
	{
		aWidthDivisor = 2;
		aHeightDivisor = 2;
	}
	else if (theImage == IMAGE_HEART)
	{
		aWidthDivisor = 3;
		aHeightDivisor = 2;
	}
	else if (theImage == IMAGE_COIN)
	{
		aWidthDivisor = 3;
		aHeightDivisor = 3;
	}

	DDImage* aGlow = new DDImage(gApp->mDDInterface);
	aGlow->Create(theImage->mWidth / aWidthDivisor + theImage->mNumCols * 2, theImage->mHeight / aHeightDivisor + theImage->mNumRows * 2);
	aGlow->mNumRows = theImage->mNumRows;
	aGlow->mNumCols = theImage->mNumCols;

	Graphics aGlowGraphics(aGlow);
	aGlowGraphics.SetFastStretch(false);

	MemoryImage* aCell = new MemoryImage(gApp);
	aCell->Create(theImage->GetCelWidth(), theImage->GetCelHeight());

	for (int aCol = 0; aCol < aGlow->mNumCols; aCol++)
	{
		for (int aRow = 0; aRow < aGlow->mNumRows; aRow++)
		{
			aCell->Clear();
			Rect aSrcRect = theImage->GetCelRect(aCol, aRow);
			Rect aDestCell = aGlow->GetCelRect(aCol, aRow);
			Rect aDestRect(aDestCell.mX + 1, aDestCell.mY + 1, aDestCell.mWidth - 2, aDestCell.mHeight - 2);

			Graphics aCellGraphics(aCell);
			aCellGraphics.DrawImage(theImage, 0, 0, aSrcRect);
			aGlowGraphics.DrawImage(aCell, aDestRect, aCell->GetCelRect(0, 0));
		}
	}
	delete aCell;

	// every pixel white, keeping its alpha
	ulong* aBits = aGlow->GetBits();
	for (int i = 0; i < aGlow->mWidth * aGlow->mHeight; i++)
		aBits[i] |= 0xffffff;
	aGlow->BitsChanged();

	aCache[theImage] = aGlow;
	return aGlow;
}

// 0x432290: the creature of the class that goes with a kind (Cubby::Spawn calls it); the shop's items are also given how
// low their cubby is, which sets how far they stand above it. The jester and the aviator are not creatures of a cubby, and
// their kinds (10 and 11) make a plain one, as kinds with no class of their own do.
Creature* Sexy::MakeCreature(int theKind, double theCubbyY)
{
	switch (theKind)
	{
	case KIND_WOMAN: return new GnomeWoman();
	case KIND_STRONG: return new GnomeStrong();
	case KIND_BOMB: return new GnomeBomb();
	case KIND_BUBBLEHEAD: return new GnomeBubbleHead();
	case KIND_HIDE: return new GnomeHide();
	case KIND_FORKY_CHAP: return new GnomeForkyChap();
	case KIND_SILVER: return new GnomeSilver();
	case KIND_MULTIPLIER: return new GnomeMultiplier();
	case KIND_CUPIDO: return new GnomeCupido();
	case KIND_HEART: return new GnomeHeart();
	case KIND_COIN:
	case KIND_BIG_COIN: return new GnomeCoin(theKind);
	case KIND_SHOP: return new GnomeShop();
	case KIND_CHALLENGE: return new GnomeChallenge();
	case KIND_CHEST: return new GnomeChest();
	case KIND_CLOVER: return new GnomeClover();
	case KIND_BONUS_ICE_CREAM:
	case KIND_BONUS_DUCK: return new GnomeBonus(theKind);
	case KIND_STRAWBERRY: return new GnomeStrawberry();
	case KIND_RABBIT: return new GnomeRabbit();
	case KIND_KING: return new GnomeKing();
	case KIND_SAVE: return new GnomeSave();
	case KIND_NAPOLEON: return new GnomeNapoleon();
	case KIND_POWER_UP: return new GnomePowerUp();
	case KIND_SWITCHER: return new GnomeSwitcher();
	case KIND_SHOP_HEART:
	case KIND_SHOP_BIG_HEART:
	case KIND_SHOP_EXTRA_HEART:
	case KIND_SHOP_HAMMER_2:
	case KIND_SHOP_HAMMER_3:
	case KIND_SHOP_HAMMER_4:
	case KIND_SHOP_ICE_CREAM:
	case KIND_SHOP_CLOVER:
	case KIND_SHOP_STRAWBERRY: return new ShopItem(theKind, theCubbyY);
	case KIND_SHOP_FLAME: return new ShopItemFlame(theKind, theCubbyY);
	case KIND_SHOP_GIFT: return new ShopItemGift(theKind, theCubbyY);
	default: return new Creature(theKind);
	}
}
