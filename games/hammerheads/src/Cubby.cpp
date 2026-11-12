#include "Cubby.h"

#include <cmath>

#include "Application.h"
#include "Avatar.h"
#include "ChallengeMeter.h"
#include "Creature.h"
#include "DDImage.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Gameplay.h"
#include "GnomeBonus.h"
#include "Graphics.h"
#include "ImageBlast.h"
#include "MemoryImage.h"
#include "Popper.h"
#include "Res.h"
#include "ShopItem.h"
#include "Score.h"
#include "Stats.h"
#include "Sticker.h"

using namespace Sexy;

// The speed of the hammer's last hits, from the stat that times them: the limits of the six speed tiers (0x557350 and the
// five before it)
static const double SPEED_TIER_LIMITS[6] = { 1.75, 1.6, 1.45, 1.3, 1.15, 1.0 };

// The sticker each speed tier shows when it is reached (the tier before it)
static const int SPEED_STICKERS[6] = { -1, 0, 1, 2, 3, 4 };

static MemoryImage* AsMemoryImage(Image* theImage)
{
	return dynamic_cast<MemoryImage*>(theImage);
}

// 0x433080
Cubby::Cubby(double theX, double theY, int theVariant, Image* theBackdrop) :
	SimpleObject(LAYER_CUBBIES),
	mState(STATE_EMPTY),
	mVariant(theVariant),
	mRise(0.0),
	mTimer(0.0),
	mCreature(nullptr),
	mCover(nullptr),
	mBackImage(nullptr),
	mPlatformImage(nullptr),
	mCreatureCount(0),
	mRiseSpeed(0.008),
	mPieces(nullptr)
{
	mX = theX;
	mY = theY;

	if (gApp->mChallengeScreen != nullptr)
	{
		mPlatformImage = IMAGE_CHALLENGE_CUBBY_PLATFORM;
		mBackImage = IMAGE_CUBBY_BACK_LIVINGROOM;
	}
	else if (gApp->IsShopOpen())
	{
		mPlatformImage = IMAGE_CUBBY_PLATFORM;
		mBackImage = IMAGE_CUBBY_BACK_LIVINGROOM;
	}
	else
	{
		mPlatformImage = GetGameplay()->GetPlatformImage();
		mBackImage = GetGameplay()->GetCubbyBackImage();
	}

	mState = STATE_EMPTY;
	if (!gApp->IsShopOpen())
		mTimer = (double)Rand() * 4.656612875245797e-10 * 3.0 + 2.0;
	else
		mTimer = 0.01;
	UpdateHitRect();

	// the cover: the backdrop under the cubby, shown over the lower half of the hole's mask, hides a creature that is
	// still sinking below the ground
	Image* aBack = mBackImage;
	ulong* aBackBits = AsMemoryImage(aBack)->GetBits();
	mCover = new MemoryImage(gSexyAppBase);
	mCover->Create(aBack->mWidth, aBack->mHeight);
	ulong* aBits = mCover->GetBits();
	Graphics aGraphics(mCover);
	Rect aSrcRect((int)theX, (int)theY, mCover->mWidth, mCover->mHeight);
	aGraphics.DrawImage(theBackdrop, 0, 0, aSrcRect);
	for (int i = 0; i < mCover->mWidth * mCover->mHeight; i++)
	{
		if ((double)(aBack->mWidth * aBack->mHeight) * 0.5 <= (double)i)
			aBits[i] = ((255 - (aBackBits[i] >> 24)) << 24) | (aBits[i] & 0xffffff);
		else
			aBits[i] = 0;
	}
	mCover->BitsChanged();
}

// 0x431f90
Cubby::~Cubby()
{
	delete mCover;
	delete mPieces;
	if (mCreature != nullptr)
	{
		mCreature->mCubbyReleased = true;
		delete mCreature;
		mCreature = nullptr;
	}
}

// 0x431be0
void Cubby::GetCreaturePosition(double thePos[2]) const
{
	int aPlatformW = IMAGE_CUBBY_PLATFORM->mWidth;
	int aPlatformH = IMAGE_CUBBY_PLATFORM->mHeight;
	double aDepth = gApp->IsShopOpen() ? 275.0 : 200.0;
	thePos[0] = mX + (double)(aPlatformW / 2);
	thePos[1] = (1.0 - mRise) * aDepth + mY + (double)(aPlatformH / 2);
}

// 0x431b30
void Cubby::GetPlatformPosition(double thePos[2]) const
{
	double aDepth = gApp->IsShopOpen() ? 275.0 : 200.0;
	thePos[0] = mX;
	thePos[1] = (1.0 - mRise) * aDepth + mY;
}

// 0x431b80
void Cubby::GetPlatformCenter(double thePos[2]) const
{
	int aPlatformW = IMAGE_CUBBY_PLATFORM->mWidth;
	int aPlatformH = IMAGE_CUBBY_PLATFORM->mHeight;
	thePos[0] = mX + (double)(aPlatformW / 2);
	thePos[1] = mY + (double)(aPlatformH / 2);
}

// 0x431cf0
void Cubby::GetStrikePoint(double thePos[2]) const
{
	double aHeadX = mCreature->mHeadX;
	double aHeadY = mCreature->mHeadY;
	double aPos[2];
	GetCreaturePosition(aPos);
	thePos[0] = ((aHeadX - 44.0) + aPos[0]) - mCreature->mAnchorX;
	thePos[1] = ((aPos[1] + (aHeadY - 24.0)) - mCreature->mAnchorY) - mCreature->mHeight;
}

// 0x431d90
void Cubby::GetFloaterAnchor(double thePos[2]) const
{
	double aHeadX = mCreature->mHeadX;
	double aHeadY = mCreature->mHeadY;
	double aPos[2];
	GetCreaturePosition(aPos);
	thePos[0] = (aHeadX + aPos[0]) - mCreature->mAnchorX;
	thePos[1] = ((aPos[1] + (aHeadY - 10.0)) - mCreature->mAnchorY) - mCreature->mHeight;
}

// 0x432ae0
Rect Cubby::GetEffectRect() const
{
	int aY = (int)((double)mBackImage->mHeight + (mY - 300.0));
	int aX = (int)(mX - 50.0);
	return Rect(aX, aY, mBackImage->mWidth + 100, 300);
}

// 0x431ea0
int Cubby::GetCreatureCount() const
{
	return mCreatureCount;
}

// 0x431eb0
bool Cubby::IsSinking() const
{
	return mState == STATE_SINKING;
}

// 0x431ec0
bool Cubby::IsRising() const
{
	return mState == STATE_RISING;
}

// 0x431e20
void Cubby::FreezeOrdinary(Creature* theCreature)
{
	if (theCreature == nullptr || theCreature->mFinished || theCreature->mSpent || theCreature->mUntouchable)
		return;

	switch (theCreature->mKind)
	{
	case KIND_KING:
	case KIND_CHEST:
	case KIND_SHOP:
	case KIND_CHALLENGE:
	case KIND_HEART:
	case KIND_COIN:
	case KIND_BIG_COIN:
	case KIND_BONUS_ICE_CREAM:
	case KIND_CLOVER:
	case KIND_CUPIDO:
	case KIND_BONUS_DUCK:
	case KIND_STRAWBERRY:
	case KIND_RABBIT:
	case KIND_POWER_UP:
	case KIND_SAVE:
		return;
	}
	theCreature->SetFrozen(true);
}

// 0x431cb0
void Cubby::ReleaseCreature()
{
	if (mCreature != nullptr)
	{
		mCreature->mCubbyReleased = true;
		delete mCreature;
		mCreature = nullptr;
	}
}

// 0x432040
void Cubby::UpdateHitRect()
{
	if (mCreature == nullptr)
	{
		mBounds = Rect(0, 0, 0, 0);
		return;
	}

	double aPos[2];
	GetCreaturePosition(aPos);
	Rect aCreatureRect = mCreature->GetScreenRect(aPos);
	int aBackW = mBackImage->mWidth;
	int aBackH = mBackImage->mHeight;
	int aX = (int)(((double)(aBackW / 2)) + mX - 200.0);
	int aY = (int)((double)aBackH + mY - 400.0);
	Rect aWindow(aX, aY, 400, 400);
	mBounds = aWindow.Intersection(aCreatureRect);
}

// 0x432120
bool Cubby::MouseOver(int theX, int theY)
{
	if (mCreature != nullptr && !mCreature->mUntouchable)
	{
		if (mBounds.mX <= theX && theX < mBounds.mWidth + mBounds.mX && mBounds.mY <= theY && theY < mBounds.mHeight + mBounds.mY)
		{
			double aPos[2];
			GetCreaturePosition(aPos);
			if (mCreature->PixelHit(aPos, theX, theY))
				return true;
		}
	}
	return false;
}

// 0x4321b0
bool Cubby::Hit(int theX, int theY, const std::vector<Point>& theMask, int theWidth, int theHeight)
{
	if (mCreature != nullptr && !mCreature->mUntouchable && mBounds.mX - theWidth <= theX &&
		theX < mBounds.mWidth + theWidth * 2 + (mBounds.mX - theWidth) && mBounds.mY - theHeight <= theY &&
		theY < mBounds.mHeight + theHeight * 2 + (mBounds.mY - theHeight))
	{
		double aPos[2];
		GetCreaturePosition(aPos);
		if (mCreature->MaskHit(aPos, theX, theY, theMask, mBounds.mX, mBounds.mY, mBounds.mWidth, mBounds.mHeight))
			return true;
	}
	return false;
}

// 0x432290
void Cubby::Spawn(int theKind)
{
	mCreature = MakeCreature(theKind, mY);
	UpdateHitRect();
	mCreatureCount++;
	mCreature->SetCubby(this);
}

// 0x432d60
void Cubby::SetState(int theState, int theKind)
{
	mState = theState;
	if (theState == STATE_UP)
	{
		int aKind = -1;
		if (mCreature != nullptr)
			aKind = mCreature->mKind;
		if (gApp->IsChallengeOpen() || aKind == -1)
			mTimer = 0.0;
		else
			mTimer = gApp->mGameScreen->GetUpTime(aKind);
		if (mCreature != nullptr)
			mCreature->SetState(1);
		return;
	}

	if (theState == STATE_EMPTY)
	{
		if (!gApp->IsShopOpen())
			mTimer = (double)Rand() * 4.656612875245797e-10 * 3.0 + 2.0;
		else
			mTimer = 0.01;
		ReleaseCreature();
		UpdateHitRect();
		return;
	}

	if (theState == STATE_RISING)
	{
		mRiseSpeed = gApp->mGameScreen->GetRiseSpeed();
		if (!gApp->IsShopOpen())
		{
			if (gApp->IsChallengeOpen())
				theKind = gApp->mChallengeScreen->GetCreatureKind();
			else if (theKind == -1)
				theKind = gApp->mGameScreen->PickCreatureKind();
		}
		else
			theKind = gApp->mShopScreen->GetCreatureKind();
		Spawn(theKind);

		// the sound whose pitch is the closest to the level's
		static const double PITCHES[10] = { 1.42, 1.32, 1.22, 1.12, 1.02, 0.92, 0.82, 0.72, 0.62, 0.52 };
		double aValue = gApp->mGameScreen->GetPitchValue();
		int aClosest = -1;
		double aClosestDistance = 999999999.0;
		for (int i = 0; i < 10; i++)
		{
			if (fabs(PITCHES[i] - aValue) < aClosestDistance)
			{
				aClosest = i;
				aClosestDistance = fabs(PITCHES[i] - aValue);
			}
		}
		gApp->PlaySample(SOUND_MISC_PLATFORM_APPEAR_01 + aClosest);
	}
}

// 0x4334d0
void Cubby::FixedUpdate()
{
	Object::FixedUpdate();

	// port: the original also kept a small escape animation here (0x431ed0 started it when a creature got away: a position
	// that rose and fell); nothing ever read it
	if (mPieces != nullptr)
	{
		mPieces->Update();
		if (mPieces->mFinished)
		{
			delete mPieces;
			mPieces = nullptr;
		}
	}

	if (mCreature == nullptr)
		return;

	bool aFrozen = mCreature->IsFrozen();
	if (GetGameplay()->mPhase == Gameplay::PHASE_OVER)
		return;

	Creature* aCreature = mCreature;
	if (!aCreature->mFinished)
	{
		if (mState == STATE_UP && aCreature->mSpent)
			mTimer = 0.0;
		double aPos[2];
		if (aFrozen)
		{
			GetCreaturePosition(aPos);
			aCreature->Creature::Update(aPos);
			return;
		}
		GetCreaturePosition(aPos);
		aCreature->Update(aPos);
		return;
	}

	int aCarried = aCreature->GetCarriedItem();
	ReleaseCreature();
	if (aCarried != ITEM_NONE)
	{
		if (!gApp->IsShopOpen() && !gApp->IsChallengeOpen() && !aFrozen)
		{
			Spawn(aCarried);
			mTimer = 5.0;
			return;
		}
	}
	if (mState == STATE_UP)
	{
		mTimer = 0.0;
		return;
	}
	if (mState == STATE_RISING)
		mState = STATE_SINKING;
}

// 0x4336a0
void Cubby::Update(float theFrac)
{
	Object::Update(theFrac);
	if (GetGameplay()->mPhase == Gameplay::PHASE_OVER)
		return;

	if (mState == STATE_RISING)
	{
		double aRise = ((double)theFrac * mRiseSpeed) + mRise;
		if (1.0 <= aRise)
			aRise = 1.0;
		mRise = aRise;
		UpdateHitRect();
		if (mRise != 1.0)
			return;
		SetState(STATE_UP, -1);
		return;
	}

	if (mState == STATE_UP)
	{
		UpdateHitRect();
		if (mTimer < 0.0)
			return;
		if (mCreature == nullptr || !mCreature->IsFrozen())
		{
			double aTimer = mTimer - (double)theFrac * 0.01;
			if (aTimer <= 0.0)
				aTimer = 0.0;
			mTimer = aTimer;
		}
		if (mTimer != 0.0)
			return;
		mState = STATE_SINKING;
		return;
	}

	if (mState != STATE_SINKING)
		return;

	double aRise = mRise - ((double)theFrac * mRiseSpeed) * 0.85;
	if (aRise <= 0.0)
		aRise = 0.0;
	mRise = aRise;
	UpdateHitRect();
	if (mRise != 0.0)
		return;
	SetState(STATE_EMPTY, -1);
}

// 0x431c70
void Cubby::DrawCover(Graphics* g)
{
	g->DrawImage(mCover, (int)mX, (int)mY);
}

// 0x432b60
void Cubby::Draw(Graphics* g)
{
	if (mDead)
		return;

	Graphics aBackGraphics(*g);
	aBackGraphics.DrawImage(mBackImage, (int)mX, (int)mY);

	Graphics aFrontGraphics(*g);
	aFrontGraphics.ClipRect(GetEffectRect());

	double aPlatformPos[2];
	GetPlatformPosition(aPlatformPos);
	bool aHover;
	if (GetGameplay()->mPhase == Gameplay::PHASE_OVER)
		aHover = false;
	else
		aHover = mHover;
	aFrontGraphics.DrawImage(mPlatformImage, (int)aPlatformPos[0], (int)aPlatformPos[1]);
	if (aHover)
	{
		aFrontGraphics.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		aFrontGraphics.DrawImage(mPlatformImage, (int)aPlatformPos[0], (int)aPlatformPos[1]);
		aFrontGraphics.SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}

	if (mPieces != nullptr)
		mPieces->Draw(&aFrontGraphics, true);

	if (mCreature != nullptr)
	{
		double aPos[2];
		GetCreaturePosition(aPos);
		double aFlashTime = mState == STATE_UP ? mTimer : 0.0;
		mCreature->Draw(&aFrontGraphics, aPos[0], aPos[1], aHover, aFlashTime);
	}

	DrawCover(&aBackGraphics);

	if (mPieces != nullptr)
		mPieces->Draw(&aFrontGraphics, false);
}

// 0x433a00: the hammer comes down at the point (the creature's strike point, from Avatar); updates the hit, speed and
// smash counts and the combo
void Cubby::Strike(double theX, double theY)
{
	if (mCreature == nullptr)
		return;

	int aHealthBefore = mCreature->GetHealth();
	bool aKilled = mCreature->Hit();
	bool aFrozen = mCreature->IsFrozen();

	if (!aKilled && !aFrozen)
	{
		int aHealthAfter = mCreature->GetHealth();
		if (aHealthBefore == aHealthAfter && !mCreature->mShielded)
		{
			// the hit did nothing to it: the hammer bounces
			double anAnchor[2];
			GetFloaterAnchor(anAnchor);
			double anAnchor2[2];
			GetFloaterAnchor(anAnchor2);
			MakeFloater(9, anAnchor[0], anAnchor2[1], nullptr, 0, "");
			double aDirection[2] = { 0.0, -1.0 };
			MakePopper(theX + 45.0, theY + 20.0, 1, aDirection);
			GetAvatar()->Deflect();
			GetStats()->SetInt(STAT_STREAK, 0);
			return;
		}
		MakeFloater(7, theX + 45.0, theY + 20.0, nullptr, 0, "");
		return;
	}

	MakeFloater(7, theX + 45.0, theY + 20.0, nullptr, 0, "");
	int aKind = mCreature->mKind;
	if ((aKind != KIND_SHOP || aFrozen) && (aKind != KIND_CHALLENGE || aFrozen) && (aKind != KIND_CLOVER || aFrozen) &&
		(aKind != KIND_SILVER || aFrozen))
	{
		if (mCreature->mIsGnome || aFrozen)
		{
			bool aDrops;
			if (gApp->IsChallengeOpen() || mCreature->GetCarriedItem() == ITEM_NONE)
				aDrops = false;
			else
			{
				aDrops = true;
				if (mState == STATE_UP)
					aDrops = false;
			}
			mPieces = mCreature->SpawnPieces(aDrops);
			if (mCreature->mKind == KIND_BOMB)
			{
				if (aFrozen)
					gApp->PlaySample(Rand() % ((SOUND_MISC_ICE_SHATTER_3 - SOUND_MISC_ICE_SHATTER_1) + 1) + SOUND_MISC_ICE_SHATTER_1);
				else
				{
					GetAvatar()->BombHit();
					gApp->PlaySample(SOUND_MISC_GNOME_BREAKING_1);
				}
			}
			else if (aFrozen)
				gApp->PlaySample(Rand() % ((SOUND_MISC_ICE_SHATTER_3 - SOUND_MISC_ICE_SHATTER_1) + 1) + SOUND_MISC_ICE_SHATTER_1);
			else
				gApp->PlaySample(Rand() % ((SOUND_MISC_GNOME_BREAKING_5 - SOUND_MISC_GNOME_BREAKING_1) + 1) + SOUND_MISC_GNOME_BREAKING_1);
		}
		else
		{
			double aDirection[2] = { 0.0, -1.0 };
			MakePopper(theX + 45.0, theY + 20.0, 2, aDirection);
		}
	}

	// the combo: a creature hit in the first third of its time up keeps the quick streak going and raises it
	if (0 < mCreature->mEscapePenalty)
	{
		if (!gApp->IsChallengeOpen() && !gApp->IsShopOpen())
		{
			bool aQuick = false;
			if (mState == STATE_RISING)
				aQuick = true;
			else if (gApp->mGameScreen->GetUpTime(mCreature->mKind) * 0.6666666666666666 < mTimer)
				aQuick = true;

			if (aQuick)
				GetStats()->AddIntQuiet(STAT_QUICK_STREAK, 1);
			else
			{
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

			if (0 < GetStats()->GetInt(STAT_QUICK_STREAK))
			{
				double aCombo = GetStats()->Get(STAT_COMBO);
				if (aCombo < 2.0)
				{
					aCombo = aCombo + 0.025;
					if (2.0 <= aCombo)
						aCombo = 2.0;
					GetStats()->Set(STAT_COMBO, aCombo);
				}
			}
		}
	}

	// the speed chain: smashing in quick succession raises the speed tier, and a sticker marks each new tier
	if (!gApp->IsChallengeOpen() && !gApp->IsShopOpen())
	{
		GetStats()->GetInt(STAT_SPEED_CHAIN);
		int aTier = GetStats()->GetInt(STAT_SPEED_TIER);
		int aChainLength = gApp->mGameMode != GAME_MODE_MARATHON ? 15 : 20;
		for (; aTier >= 0; aTier--)
		{
			if (GetStats()->Get(STAT_SPEED_TIMER) < SPEED_TIER_LIMITS[aTier])
			{
				GetStats()->SetInt(STAT_SPEED_TIER, aTier);
				GetStats()->AddInt(STAT_SPEED_CHAIN, 1);
				if (GetStats()->GetInt(STAT_SPEED_CHAIN) == aChainLength)
				{
					GetStats()->SetInt(STAT_SPEED_CHAIN, 0);
					int aNewTier = GetStats()->GetInt(STAT_SPEED_TIER) + 1 < 6 ? GetStats()->GetInt(STAT_SPEED_TIER) + 1 : 5;
					GetStats()->SetInt(STAT_SPEED_TIER, aNewTier);
					MakeSpeedSticker(SPEED_STICKERS[GetStats()->GetInt(STAT_SPEED_TIER)], 720.0, 120.0);
				}
				break;
			}
		}
		if (aTier == -1)
			GetStats()->Set(STAT_SPEED_CHAIN, 0.0);
		GetStats()->Set(STAT_SPEED_TIMER, 0.0);
	}

	if (mCreature->mIsGnome)
	{
		GetStats()->AddInt(STAT_GNOMES_SMASHED, 1);
		GetStats()->AddInt(STAT_ROUND_SMASHED, 1);
		GetStats()->AddInt(STAT_STREAK, 1);
		if (GetStats()->GetInt(STAT_BEST_STREAK) < GetStats()->GetInt(STAT_STREAK))
			GetStats()->SetInt(STAT_BEST_STREAK, GetStats()->GetInt(STAT_STREAK));
	}

	if (mCreature->mKind == KIND_KING)
		GetStats()->AddInt(STAT_KINGS_DESTROYED, 1);

	if (mCreature->mKind == KIND_BIG_COIN)
	{
		if (gApp->IsChallengeOpen())
			GetChallengeMeter()->AddCount(0, 1);
	}

	int aScore = mCreature->GetScore();
	if (0 < aScore && !gApp->IsShopOpen() && !gApp->IsChallengeOpen())
	{
		double anAnchor[2];
		GetFloaterAnchor(anAnchor);
		double anAnchor2[2];
		GetFloaterAnchor(anAnchor2);
		int aY = (int)anAnchor2[1];
		int aX = (int)anAnchor[0];
		GetScoreDisplay()->AddPoints(aScore, aX, aY, 2);

		GnomeBonus* aBonus = dynamic_cast<GnomeBonus*>(mCreature);
		if (aBonus != nullptr)
		{
			std::string aText;
			if (aBonus->mKind == KIND_BONUS_ICE_CREAM)
				aText = "Cool!";
			else if (aBonus->mKind == KIND_BONUS_DUCK)
				aText = "Quack!";
			else if (aBonus->mKind == KIND_STRAWBERRY)
				aText = "Juicy!";
			double aFloaterAnchor[2];
			GetFloaterAnchor(aFloaterAnchor);
			double aFloaterAnchor2[2];
			GetFloaterAnchor(aFloaterAnchor2);
			MakeFloater(2, aFloaterAnchor[0], aFloaterAnchor2[1] - 35.0, aText.c_str(), 0, "");
		}
	}
	mCreature->Remove();
}

// 0x434360
void Cubby::SaveState(CubbyState& theState) const
{
	theState.mName = mName;
	theState.mActive = mActive;
	theState.mState = mState;
	theState.mVariant = mVariant;
	theState.mRise = mRise;
	theState.mTimer = mTimer;
	theState.mCreatureCount = mCreatureCount;
	if (mCreature == nullptr)
	{
		theState.mHasCreature = false;
		return;
	}
	theState.mHasCreature = true;
	theState.mCreatureState = mCreature->mState;
	theState.mFinished = mCreature->mFinished;
	theState.mSpent = mCreature->mSpent;
	theState.mUntouchable = mCreature->mUntouchable;
	theState.mKind = mCreature->mKind;
	theState.mHeight = mCreature->mHeight;
	theState.mHeightVelocity = mCreature->mHeightVelocity;
	theState.mGravity = mCreature->mGravity;
	theState.mHealth = mCreature->mHealth;
	theState.mFrozenTimer = mCreature->mFrozenTimer;
	theState.mShatters = mCreature->mShatters;
	theState.mExtra = mCreature->SaveExtra();
}

// 0x433810
void Cubby::LoadState(const CubbyState& theState)
{
	mActive = theState.mActive;
	mState = theState.mState;
	mVariant = theState.mVariant;
	mRise = theState.mRise;
	mTimer = theState.mTimer;
	mCreatureCount = theState.mCreatureCount;
	ReleaseCreature();
	if (theState.mHasCreature)
	{
		Spawn(theState.mKind);
		mCreature->mState = theState.mCreatureState;
		mCreature->mFinished = theState.mFinished;
		mCreature->mSpent = theState.mSpent;
		mCreature->mUntouchable = theState.mUntouchable;
		mCreature->mHeight = theState.mHeight;
		mCreature->mHeightVelocity = theState.mHeightVelocity;
		mCreature->mGravity = theState.mGravity;
		mCreature->mHealth = theState.mHealth;
		mCreature->mFrozenTimer = theState.mFrozenTimer;
		mCreature->mShatters = theState.mShatters;
		mCreature->LoadExtra(theState.mExtra);
	}
}

// 0x432870
int Sexy::CountHeartCreatures()
{
	int aCount = 0;
	std::vector<Cubby*>& aCubbies = ObjectManager::Get()->mCubbiesByDepth;
	for (int i = (int)aCubbies.size() - 1; i >= 0; i--)
	{
		Creature* aCreature = aCubbies[i]->mCreature;
		if (aCreature != nullptr && !aCreature->mFinished && !aCreature->mSpent && !aCreature->mUntouchable && aCreature->mKind == KIND_HEART)
			aCount++;
	}
	return aCount;
}

// 0x4328d0
bool Sexy::IsGiftReady()
{
	if (gApp->IsShopOpen())
	{
		std::vector<Cubby*>& aCubbies = ObjectManager::Get()->mCubbiesByDepth;
		for (int i = (int)aCubbies.size() - 1; i >= 0; i--)
		{
			Cubby* aCubby = aCubbies[i];
			Creature* aCreature = aCubby->mCreature;
			if (aCreature != nullptr && !aCreature->mFinished && aCreature->mKind == KIND_SHOP_GIFT && aCubby->mActive)
			{
				ShopItemGift* aGift = dynamic_cast<ShopItemGift*>(aCreature);
				if (aGift->IsReady())
					return true;
			}
		}
	}
	return false;
}

// 0x432950
int Sexy::CountCreaturesOfKind(int theKind)
{
	int aCount = 0;
	std::vector<Cubby*>& aCubbies = ObjectManager::Get()->mCubbiesByDepth;
	for (int i = (int)aCubbies.size() - 1; i >= 0; i--)
	{
		Cubby* aCubby = aCubbies[i];
		Creature* aCreature = aCubby->mCreature;
		if (aCreature != nullptr && !aCreature->mFinished && !aCreature->mSpent && !aCreature->mUntouchable && aCreature->mKind == theKind &&
			aCubby->mActive)
			aCount++;
	}
	return aCount;
}

// 0x4329c0
void Sexy::FreezeAll()
{
	std::vector<Cubby*>& aCubbies = ObjectManager::Get()->mCubbiesByDepth;
	for (int i = (int)aCubbies.size() - 1; i >= 0; i--)
		Cubby::FreezeOrdinary(aCubbies[i]->mCreature);
}

// Takes theCreature off its cubby the way a hammer strike does: without a penalty for its escape, in pieces when it is a
// gnome (a part of 0x432a00 and 0x4344d0)
static void BreakUp(Cubby* theCubby)
{
	theCubby->mCreature->ClearEscapePenalty();
	if (theCubby->mCreature->mIsGnome)
	{
		bool aDrops;
		if (gApp->IsChallengeOpen() || theCubby->mCreature->GetCarriedItem() == ITEM_NONE)
			aDrops = false;
		else
		{
			aDrops = true;
			if (theCubby->mState == Cubby::STATE_UP)
				aDrops = false;
		}
		theCubby->mPieces = theCubby->mCreature->SpawnPieces(aDrops);
	}
	theCubby->ReleaseCreature();
}

// 0x432a00
void Sexy::ExplodeAll(Creature* theExcept)
{
	std::vector<Cubby*>& aCubbies = ObjectManager::Get()->mCubbiesByDepth;
	for (int i = (int)aCubbies.size() - 1; i >= 0; i--)
	{
		Cubby* aCubby = aCubbies[i];
		Creature* aCreature = aCubby->mCreature;
		if (aCreature != nullptr && (aCubby->mState == Cubby::STATE_UP || aCubby->mState == Cubby::STATE_RISING) && !aCreature->mFinished &&
			!aCreature->mSpent && !aCreature->mUntouchable && aCreature != theExcept)
			BreakUp(aCubby);
	}
}

// 0x4344d0
void Sexy::ConvertToHearts(Creature* theExcept)
{
	int aCount = 0;
	std::vector<Cubby*>& aCubbies = ObjectManager::Get()->mCubbiesByDepth;
	for (int i = (int)aCubbies.size() - 1; i >= 0; i--)
	{
		Cubby* aCubby = aCubbies[i];
		Creature* aCreature = aCubby->mCreature;
		if (aCreature == nullptr || (aCubby->mState != Cubby::STATE_UP && aCubby->mState != Cubby::STATE_RISING))
			continue;
		int aKind = aCreature->mKind;
		if (aKind == KIND_HEART || aKind == KIND_CHALLENGE || aKind == KIND_SHOP || aKind == KIND_CUPIDO || aKind == KIND_KING)
			continue;
		if (aCreature->mFinished || aCreature->mSpent || aCreature->mUntouchable || aCreature == theExcept)
			continue;

		BreakUp(aCubby);
		if (1 < aCount)
			gApp->mQuietSpawn = true;
		aCubby->Spawn(KIND_HEART);
		if (1 < aCount)
			gApp->mQuietSpawn = false;
		MakeFloater(0xf, (double)(IMAGE_CUBBY_BACK->mWidth / 2) + aCubby->mX, aCubby->mY - 70.0, nullptr, 0, "");
		aCount++;
	}
}
