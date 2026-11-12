#include "Avatar.h"

#include <cmath>

#include "Application.h"
#include "Color.h"
#include "Creature.h"
#include "Cubby.h"
#include "DrawUtil.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Gameplay.h"
#include "Graphics.h"
#include "HammerDisplay.h"
#include "Hearts.h"
#include "Image.h"
#include "Res.h"
#include "Stats.h"
#include "Sticker.h"

using namespace Sexy;

static ObjectRef gAvatarRef;			// 0x597f0c

// 0x42d6b0: the hammer's image for its level (0 to 3) and whether it is on fire
static Image* GetHammerImage(int theLevel, bool theFire)
{
	switch (theLevel)
	{
	case 0:
		return theFire ? IMAGE_PLAYER_HAMMER_1_FIRE : IMAGE_PLAYER_HAMMER_1;
	case 1:
		return theFire ? IMAGE_PLAYER_HAMMER_2_FIRE : IMAGE_PLAYER_HAMMER_2;
	case 2:
		return theFire ? IMAGE_PLAYER_HAMMER_3_FIRE : IMAGE_PLAYER_HAMMER_3;
	case 3:
		return theFire ? IMAGE_PLAYER_HAMMER_4_FIRE : IMAGE_PLAYER_HAMMER_4;
	}
	return nullptr;
}

// 0x42def0
Avatar::Avatar() :
	Object(0x200),
	mMouseX(0.0),
	mMouseY(0.0),
	mHammerX(0.0),
	mHammerY(0.0),
	mSwing(SWING_NONE),
	mOverTarget(false),
	mStruck(false),
	mBoing(0.0),
	mBombShake(0.0),
	mFrame(0.0),
	mCrosshairVisible(true),
	mMaskWidth(0x34),
	mMaskHeight(0x34),
	mHammerFade(0.0),
	mClickCount(0)
{
	// The hammer's reach: the distinct points of a circle of radius 26, in the order a walk round it meets them
	for (int anAngle = 0; anAngle < 360; anAngle++)
	{
		double aRadians = (double)anAngle * 0.017453292519943295;
		Point aPoint((int)(std::cos(aRadians) * 26.0), (int)(std::sin(aRadians) * 26.0));

		bool aKnown = false;
		for (size_t i = 0; i < mMask.size(); i++)
		{
			if (mMask[i].mX == aPoint.mX && mMask[i].mY == aPoint.mY)
			{
				aKnown = true;
				break;
			}
		}
		if (!aKnown)
			mMask.push_back(aPoint);
	}
}

// 0x42e4f0: the original's own factory names the object with a blank name
Avatar* Sexy::MakeAvatar()
{
	Avatar* anAvatar = new Avatar();
	anAvatar->SetName("");
	gAvatarRef.Set(anAvatar);
	return anAvatar;
}

// 0x42d7f0
Avatar* Sexy::GetAvatar()
{
	return dynamic_cast<Avatar*>(gAvatarRef.Get());
}

// 0x42d680
SimpleObject* Avatar::GetTarget() const
{
	return dynamic_cast<SimpleObject*>(mTarget.Get());
}

// 0x42d820
void Avatar::SetMouse(int theX, int theY)
{
	mMouseX = (double)theX;
	mMouseY = (double)theY;
}

// 0x42d840
void Avatar::ClearHammerFade()
{
	mHammerFade = 0.0;
}

// 0x42d850
void Avatar::Deflect()
{
	mBoing = 1.0;
	gApp->PlaySample(SOUND_HAMMER_BOING);
}

// 0x42d880
void Avatar::BombHit()
{
	mBombShake = 0.65;
}

// 0x42e0d0
void Avatar::AddClick(int theX, int theY)
{
	if (mClicks.empty() && !IsGiftReady())
		mClicks.push_back(std::pair<double, double>((double)theX, (double)theY));
}

// 0x42e110
void Avatar::ClearClicks()
{
	mClicks.clear();
}

// 0x42d730
void Avatar::Update(float theFrac)
{
	double aBoing = mBoing - (double)theFrac * 0.01;
	if (aBoing < 0.0)
		aBoing = 0.0;
	mBoing = aBoing;

	if (mBombShake > 0.0)
	{
		mBombShake = mBombShake - (double)theFrac * 0.01;
		if (mBombShake <= 0.0)
		{
			mBombShake = 0.0;
			int aFrame = (int)mFrame;
			if (aFrame % 7 < 6)
				mFrame = ((mFrame - (double)aFrame) + (double)(aFrame / 7)) + 6.0;
		}
	}
}

// 0x42e120: theHover is true to find what the pointer is on, false to find what a click hits. The objects that are not
// cubbies are asked first, in the order they were made, then the cubbies from the front; the first that answers is the
// one. Hovering marks it; a click makes it the swing's target and puts the hammer on the point it is struck at.
// (The original also asks every object Hit() with the hammer's mask and keeps the first yes, which it never uses.)
bool Avatar::Probe(int theX, int theY, bool theHover)
{
	ObjectManager* aManager = ObjectManager::Get();
	int aCount = (int)aManager->mObjects.size();

	if (theHover)
	{
		for (int i = 0; i < aCount; i++)
			aManager->mObjects[i]->mHover = false;
	}

	Object* aFound = nullptr;
	for (int i = 0; i < aCount; i++)
	{
		Object* anObject = aManager->mObjects[i];
		if (dynamic_cast<Cubby*>(anObject) == nullptr && anObject->mActive)
		{
			if (anObject->MouseOver(theX, theY) && aFound == nullptr)
				aFound = anObject;
		}
	}

	for (int i = (int)aManager->mCubbiesByDepth.size() - 1; i >= 0; i--)
	{
		Cubby* aCubby = aManager->mCubbiesByDepth[i];
		if (aCubby->mActive)
		{
			if (aCubby->MouseOver(theX, theY) && aFound == nullptr)
				aFound = aCubby;
		}
	}

	if (aFound == nullptr)
		return false;

	if (!theHover)
	{
		double aPoint[2];
		if (Cubby* aCubby = dynamic_cast<Cubby*>(aFound))
		{
			aCubby->GetStrikePoint(aPoint);
			mHammerX = aPoint[0];
			mHammerY = aPoint[1];
		}
		else if (Sticker* aSticker = dynamic_cast<Sticker*>(aFound))
		{
			aSticker->GetStrikePoint(aPoint);
			mHammerX = aPoint[0];
			mHammerY = aPoint[1];
		}
		else
		{
			mHammerX = mMouseX;
			mHammerY = mMouseY;
		}
		mTarget.Set(aFound);
	}
	else
		aFound->mHover = true;

	return true;
}

// 0x42e490: the pointer is on something the hammer can hit (and that thing is marked); never while a gift is to be opened
bool Avatar::UpdateHover()
{
	if (IsGiftReady())
	{
		ObjectManager* aManager = ObjectManager::Get();
		int aCount = (int)aManager->mObjects.size();
		for (int i = 0; i < aCount; i++)
			aManager->mObjects[i]->mHover = false;
		return false;
	}

	return Probe((int)mMouseX, (int)mMouseY, true);
}

// 0x42e5f0: takes the waiting click once the last swing is over
void Avatar::HandleClick()
{
	if (mClicks.empty())
		return;
	if (mSwing != SWING_NONE)
		return;

	// clicks while the game is paused are dropped, except in the shop and in a challenge
	if (gApp->mPauseCount > 0 && !gApp->IsShopOpen() && !gApp->IsChallengeOpen())
	{
		mClicks.erase(mClicks.begin());
		return;
	}

	double aClickX = mClicks[0].first;
	double aClickY = mClicks[0].second;
	mClicks.erase(mClicks.begin());
	mClickCount++;

	int aX = (int)aClickX;
	int aY = (int)aClickY;

	if (!gApp->IsShopOpen() && !gApp->IsChallengeOpen())
		GetStats()->AddInt(STAT_SWINGS_IN_LEVEL, 1);

	if (Probe(aX, aY, false))
	{
		// (the original also tells the target that a swing has begun, a call that a Cubby passes to its creature's empty
		// slot +0x20 and a Sticker ignores)
		mFrame = 0.0;
		mHammerFade = 1.0;
		mSwing = SWING_HIT;
		mBoing = 0.0;
		mStruck = false;
		gApp->PlaySample(GetStats()->GetInt(STAT_HAMMER_ON_FIRE) == 1 ? SOUND_HAMMER_WOOSH_FAST : SOUND_HAMMER_WOOSH);
		return;
	}

	// a click that hits nothing
	if (!gApp->IsShopOpen() && !gApp->IsChallengeOpen())
	{
		int aLevel = gApp->mGameScreen->mLevel;
		if (GetGameplay()->IsTimerB0Active())
		{
			MakeFloater(7, (double)aX, (double)aY, nullptr, 0, "");
			gApp->PlaySample(SOUND_VARIOUS_SQUISH_STRAWBERRY);
		}
		else if (GetStats()->GetInt(STAT_KINGS_DESTROYED) == 0)
		{
			GetHearts()->LoseHealth(0, 0.0);
			GetStats()->AddInt(STAT_MISSES, 1);
			GetStats()->AddInt(STAT_MISSES_IN_LEVEL, 1);
			GetStats()->SetInt(STAT_LEVEL_OF_LAST_MISS, aLevel);
			GetStats()->SetInt(STAT_STREAK, 0);
		}

		double aCombo = GetStats()->Get(STAT_COMBO);
		if (1.0 < aCombo)
		{
			aCombo = aCombo - 0.1;
			if (aCombo <= 1.0)
				aCombo = 1.0;
			GetStats()->Set(STAT_COMBO, aCombo);
		}

		// the first level's hints: after the first click, and after the others
		if (aLevel == 1 && gApp->mGameMode != GAME_MODE_TOUGH_COOKIE)
		{
			if (mClickCount == 1)
				MakeFloater(0xb, 405.0, 133.0, nullptr, 0, "");
			else
				MakeFloater(0xc, 395.0, 528.0, nullptr, 0, "");
		}
	}
	else if (gApp->IsChallengeOpen())
		GetStats()->AddInt(STAT_CHALLENGE_MISSES, 1);

	// the miss shows in the level when it costs something, and in a challenge
	bool aShowMiss;
	if (!gApp->IsShopOpen())
		aShowMiss = !GetGameplay()->IsTimerB0Active() || gApp->IsChallengeOpen();
	else
		aShowMiss = gApp->IsChallengeOpen();

	if (aShowMiss)
	{
		MakeFloater(10, (double)aX, (double)aY, nullptr, 1, "");
		gApp->PlaySample(SOUND_HAMMER_MISS);
	}
}

// 0x42eac0
void Avatar::FixedUpdate()
{
	Object::FixedUpdate();
	HandleClick();

	if (mBombShake <= 0.0)
		mFrame = mFrame + (GetStats()->GetInt(STAT_HAMMER_ON_FIRE) == 1 ? 0.4 : 0.15);

	mOverTarget = UpdateHover();

	switch (mSwing)
	{
	case SWING_NONE:
		if (mHammerFade > 0.0)
		{
			mHammerFade = mHammerFade - 0.03;
			if (mHammerFade <= 0.0)
				mHammerFade = 0.0;
		}
		return;
	case SWING_HIT:
		if (mFrame >= 3.0 && !mStruck)
		{
			mStruck = true;
			if (SimpleObject* aTarget = GetTarget())
				aTarget->Strike(mHammerX, mHammerY);
		}
		if (!(mFrame < 6.0))
		{
			mSwing = SWING_NONE;
			mFrame = 0.0;
		}
		return;
	case SWING_LONG:
		if (!(mFrame < 12.0))
		{
			mSwing = SWING_NONE;
			mFrame = 0.0;
		}
		return;
	}
}

// 0x42da00
void Avatar::Draw(Graphics* g)
{
	Graphics aGraphics(*g);

	Image* aCrosshair = GetGameplay()->GetCrosshair();
	if (mCrosshairVisible)
	{
		if (!mOverTarget)
		{
			aGraphics.DrawImage(aCrosshair, (int)(mMouseX - (double)aCrosshair->GetWidth() * 0.5),
				(int)(mMouseY - (double)aCrosshair->GetHeight() * 0.5));
		}
		aGraphics.SetColorizeImages(false);
		if (mOverTarget)
		{
			Image* aGlow = (aCrosshair == IMAGE_CROSSHAIR) ? IMAGE_CROSSHAIR_GLOW : IMAGE_CROSSHAIR_SNOW_GLOW;
			aGraphics.DrawImage(aGlow, (int)(mMouseX - (double)aGlow->GetWidth() * 0.5),
				(int)(mMouseY - (double)aGlow->GetHeight() * 0.5));
		}
	}

	int aFrame = 0;
	if (mSwing == SWING_HIT || mSwing == SWING_LONG)
		aFrame = (int)mFrame % 7;

	Image* aHammer = GetHammerImage(GetHammerDisplay()->GetLevel(), GetHammerDisplay()->IsOnFire());
	if (aHammer == nullptr)
		return;

	aGraphics.SetColorizeImages(true);
	double anAlpha = mHammerFade + mHammerFade;
	if (!(anAlpha < 1.0))
		anAlpha = 1.0;
	aGraphics.SetColor(Color(255, 255, 255, (int)(anAlpha * 255.0)));

	double aWobble = std::sin(mFrame * 3.0) * mBoing * 8.0;
	if (mBombShake == 0.0)
	{
		DrawImageCelF(&aGraphics, aHammer, (mHammerX - (double)(aHammer->GetCelWidth() / 2)) + aWobble,
			mHammerY - (double)(aHammer->GetCelHeight() / 2), aFrame, 0);
	}
	else
	{
		// after a bomb the hammer spins and hops: a quarter turn and a half in 0.65 of the shake
		double anEase = Ease(1.0 - mBombShake * 1.5384615384615383, 0.0, 0.8);
		double aHop = std::sin(3.1415927410125732 * anEase) * -90.0;
		double aSpin = anEase * 14.13716733455658;
		Rect aSrcRect = aHammer->GetCelRect(aFrame, 0);
		aGraphics.DrawImageRotatedF(aHammer,
			(float)((mHammerX - (double)(aHammer->GetCelWidth() / 2)) + aWobble),
			(float)((mHammerY - (double)(aHammer->GetCelHeight() / 2)) + aHop),
			aSpin, &aSrcRect);
	}

	if (GetGameplay()->mDoubleHitTimer > 0.0)
		aGraphics.DrawImage(IMAGE_PLUS_TWO, (int)(mHammerX + 10.0), (int)mHammerY - 65);
}
