#include "MetaBuilding.h"

#include "Anim.h"
#include "Emitter.h"
#include "GameApp.h"
#include "MenuObj.h"
#include "ObjString.h"
#include "Player.h"

#include <cstdio>
#include <cstring>

using namespace Sexy;

ObjBase* MetaBuilding::sMetaGame = nullptr;

// The key events of the building's tracks
enum BuildingEvent
{
	BUILDING_EVENT_PLACED = 1,   // no key has this one: the building is put at its place and dropped
	BUILDING_EVENT_LANDED = 2,   // the drop has landed
	BUILDING_EVENT_LEFT = 3,     // the building has gone up out of sight: the next level drops in
};

// The picture of a building's level sets a sprite beside it moving through this many cels, back and forth
const int SPRITE_CELS = 3;
const int DUST_PUFFS = 4;
const int DUST_PARTICLES = 250;

// 0x447060: the building of the kind theKind, standing on theMetaGame's screen (not yet shown)
MetaBuilding::MetaBuilding(int theKind, ObjBase* theMetaGame)
{
	mKind = theKind;
	mHomeX = 0.0f;
	mHomeY = 0.0f;
	mAffordable = true;
	mStage = 1;
	sMetaGame = theMetaGame;
	mAnim = nullptr;
	mPrices[0] = 0;
	mPrices[1] = 0;
	mPrices[2] = 0;
	mLevel = 0;
	mDropping = false;
	for (int i = 0; i < DUST_PUFFS; i++)
		mDust[i] = nullptr;
	// the original leaves these as the allocator gives them until the setup below or a later call writes them
	mFrameDelayLeft = 0;
	mLanded = false;
	mDrawShadow = false;
	mPrices[3] = 0;
	mStagePicture = nullptr;
	mDropTrack = 0;
	mLeaveTrack = 0;
	mIdleTrack = 0;
	mDropKey = nullptr;
	mDropRestKey = nullptr;
	mDropLandKey = nullptr;
	mLeaveKey = nullptr;
	mLeaveEndKey = nullptr;
	mIdleKey = nullptr;
	mPriceText = nullptr;
	memset(mSprites, 0, sizeof(mSprites));
	memset(mStages, 0, sizeof(mStages));

	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetMouseable(32.0f, 32.0f);
	// the swells of dust, from the one furthest back (-2) to the nearest (2)
	static const float DUST_DEPTHS[DUST_PUFFS] = {-2.0f, -1.0f, 1.0f, 2.0f};
	for (int i = 0; i < DUST_PUFFS; i++) {
		mDust[i] = new EmitterSet(this);
		mDust[i]->MakeRisingSwell(DUST_DEPTHS[i], DUST_PARTICLES);
	}
	SetupKind();
	BuildTracks();
	BuildPriceText();
	LoadStagePicture();
}

// 0x447260
MetaBuilding::~MetaBuilding()
{
	for (int i = 0; i < DUST_PUFFS; i++) {
		if (mDust[i] != nullptr)
			delete mDust[i];
	}
	if (mAnim != nullptr) {
		delete mAnim;
		mAnim = nullptr;
	}
}

// 0x447340 (slot 1): back to the state of a building not built
void MetaBuilding::Init()
{
	mHomeX = 0.0f;
	mHomeY = 0.0f;
	mLevel = 0;
	mStage = 1;
	mDropping = false;
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	Deactivate(this);
}

// 0x447390: the keys that end tell the building what to do next
void MetaBuilding::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == BUILDING_EVENT_PLACED) {
		SetState(STATE_X, mHomeX);
		SetState(STATE_Y, mHomeY);
		SetParent(sMetaGame, nullptr);
		Activate(this, false);
		StartDrop();
		mStage = 2;
	} else if (anEvent == BUILDING_EVENT_LANDED) {
		mLanded = true;
		mDropping = false;
	} else if (anEvent == BUILDING_EVENT_LEFT) {
		StartDrop();
	}
}

// 0x447430: while the building drops, four dust swells burst at random places about it; the picture's cels turn
bool MetaBuilding::Update()
{
	if (!ObjBase::Update())
		return false;
	mDrawShadow = false;
	if (mAnim != nullptr) {
		if (mDropping) {
			float aHalfWidth = (float)(mAnim->GetSourceCelWidth() * 0.5);
			float aY = (float)(int)(GameRandomNext() % 13 - 0x24);
			float aX = MenuObj::RandomWhole(-aHalfWidth, aHalfWidth);
			mDust[1]->BurstRisingSwell(aX, aY);
			aY = (float)(int)(GameRandomNext() % 11 - 0x14);
			aX = MenuObj::RandomWhole(-aHalfWidth, aHalfWidth);
			mDust[2]->BurstRisingSwell(aX, aY);
			float aQuarterWidth = (float)(aHalfWidth * 0.5);
			aY = (float)(int)(GameRandomNext() % 13 - 0x3e);
			aX = MenuObj::RandomWhole(-aQuarterWidth, aQuarterWidth);
			mDust[0]->BurstRisingSwell(aX, aY);
			aY = (float)(int)(GameRandomNext() % 14 - 9);
			aX = MenuObj::RandomWhole(-aQuarterWidth, aQuarterWidth);
			mDust[3]->BurstRisingSwell(aX, aY);
		}
		if (0 < mFrameDelayLeft) {
			mFrameDelayLeft--;
			return true;
		}
		int aNextCel = (int)(mState[STATE_FRAME] + 1.0) % mStagePicture->mCels;
		SetStateInt(STATE_FRAME, aNextCel);
		mFrameDelayLeft = mStagePicture->mFrameDelay;
	}
	return true;
}

// 0x447610: a building the player has not built yet is drawn again dimmed (mDrawShadow is never set, so it never shows)
void MetaBuilding::Draw(Graphics* g)
{
	ObjBase::Draw(g);
	if (mDrawShadow && mStage == 2) {
		float aSavedAlpha = mDrawn[STATE_ALPHA];
		mDrawMode = 1;
		mDrawn[STATE_ALPHA] = 0.4f;
		ObjBase::Draw(g);
		mDrawMode = 0;
		mDrawn[STATE_ALPHA] = aSavedAlpha;
	}
}

// 0x447680
void MetaBuilding::SetDepthFromY(float theDepth)
{
	SetState(STATE_Z, theDepth);
}

// 0x4476a0: what the level costs; a level below 0 means the building's own (a level 3 building reads the fourth price, 0)
int MetaBuilding::GetPrice(int theLevel)
{
	if (theLevel < 0)
		theLevel = mLevel;
	return mPrices[theLevel];
}

// 0x4476c0: whether the player has the bananas for the next level (a building that is not shown, or at level 3, says
// yes without checking)
bool MetaBuilding::CanAfford()
{
	if (!IsActive())
		return false;
	if (mLevel < 3 && Player::sCurrent->mBananas < mPrices[mLevel]) {
		mAffordable = false;
		return false;
	}
	mAffordable = true;
	return true;
}

// 0x447710: the picture of the level it is at (none for level 0)
void MetaBuilding::LoadStagePicture()
{
	if (mLevel > 0 && mLevel < 4) {
		mStagePicture = &mStages[mLevel - 1];
		char aPath[44];
		sprintf(aPath, "images/meta/%s", mStagePicture->mName);
		if (mAnim != nullptr) {
			delete mAnim;
			mAnim = nullptr;
		}
		mAnim = Anim::Get(aPath, ANIM_SCALE_SMOOTH, false, mStagePicture->mCelWidth, mStagePicture->mCelHeight,
			mStagePicture->mCels, false);
		mFrameDelayLeft = mStagePicture->mFrameDelay;
	} else if (mAnim != nullptr) {
		delete mAnim;
		mAnim = nullptr;
	}
}

// 0x4477e0 (ECX the name, EDX the cel width; the rest pushed): a sprite beside the building that plays its three cels
// back and forth; the ones for levels above the first stay out of sight
void MetaBuilding::AddSprite(const char* theName, int theCelWidth, int theIndex, int theX, int theY, int theCelHeight,
	int theDelay)
{
	MetaPicture* aSprite = &mSprites[theIndex];
	aSprite->mName = theName;
	aSprite->mCelWidth = theCelWidth;
	aSprite->mCelHeight = theCelHeight;
	aSprite->mCels = SPRITE_CELS;
	aSprite->mFrameDelay = theDelay;
	ObjBase* anObj = new ObjBase();
	aSprite->mObj = anObj;
	anObj->SetParent(this, nullptr);
	char aPath[36];
	sprintf(aPath, "images/meta/%s", aSprite->mName);
	anObj->mAnim = Anim::Get(aPath, ANIM_SCALE_SMOOTH, false, aSprite->mCelWidth, aSprite->mCelHeight, aSprite->mCels,
		false);
	anObj->SetState(STATE_X, (float)theX);
	anObj->SetState(STATE_Y, (float)theY);
	anObj->SetState(STATE_Z, 0.01f);
	Activate(anObj, false);

	AnimKey* aCycle = new AnimKey();
	aCycle->SetDuration(150);
	aCycle->AddModifier(STATE_FRAME, false, 0.0f, MOD_PING_PONG, (float)aSprite->mFrameDelay, (float)aSprite->mCels,
		0.0f, nullptr);
	AnimTrack* aCycleTrack = new AnimTrack();
	aCycleTrack->AddKey(aCycle, aCycle);
	anObj->AddTrack(aCycleTrack, nullptr);
	if (theIndex > 0)
		Deactivate(anObj);
}

// A level's picture: the name, and the cels' size, count and delay when the picture is an animated one
static void SetStage(MetaPicture* theStage, const char* theName, int theCelWidth, int theCelHeight, int theCels,
	int theDelay)
{
	theStage->mName = theName;
	theStage->mCelWidth = theCelWidth;
	theStage->mCelHeight = theCelHeight;
	theStage->mCels = theCels;
	theStage->mFrameDelay = theDelay;
}

// 0x447980: the pictures, prices, place and sprites of the building's kind
void MetaBuilding::SetupKind()
{
	mPrices[0] = 0;
	mPrices[1] = 0;
	mPrices[2] = 0;
	memset(mStages, 0, sizeof(mStages));
	mStages[0].mCels = 1;
	mStages[1].mCels = 1;
	mStages[2].mCels = 1;
	memset(mSprites, 0, sizeof(mSprites));

	// the place from the design space's corner, and the sprite's (name, cel width, index, x, y, cel height, delay)
	switch (mKind) {
	case BUILDING_LEISURE:
		SetStage(&mStages[0], "meta_leisure1", 0xf7, 0x10d, 2, 0xf);
		SetStage(&mStages[1], "meta_leisure2", 0x141, 0x1c7, 2, 0xe);
		mHomeX = FromCorner(780.0f, 1920.0f);
		mHomeY = FromCorner(454.0f, 1200.0f);
		mPrices[0] = 0xdc;
		mPrices[1] = 700;
		mPrices[2] = 0;
		AddSprite("metamonkey_leisure", 0x3f, 0, -0x20, 0, 0x3f, 4);
		break;
	case BUILDING_HOUSE:
		mStages[0].mName = "meta_house1";
		mStages[1].mName = "meta_house2";
		mStages[2].mName = "meta_house3";
		mHomeX = FromCorner(1274.0f, 1920.0f);
		mHomeY = FromCorner(448.0f, 1200.0f);
		mPrices[0] = 0x15e;
		mPrices[1] = 400;
		mPrices[2] = 900;
		AddSprite("metamonkey_house", 0x3f, 0, 0, -0x2a, 0x3f, 2);
		break;
	case BUILDING_MILITARY:
		SetStage(&mStages[0], "meta_military2", 0x13d, 0x149, 3, 200);
		mStages[1].mName = "meta_military1";
		SetStage(&mStages[2], "meta_military3", 0x141, 0x14d, 3, 200);
		mHomeX = FromCorner(420.0f, 1920.0f);
		mHomeY = FromCorner(1160.0f, 1200.0f);
		mPrices[0] = 0x96;
		mPrices[1] = 600;
		mPrices[2] = 1000;
		AddSprite("metamonkey_military", 0x3f, 0, -0x3c, -0x38, 0x3f, 2);
		break;
	case BUILDING_SHOT_SPEED:
		mStages[0].mName = "meta_shotspeed1";
		mStages[1].mName = "meta_shotspeed2";
		mStages[2].mName = "meta_shotspeed3";
		mHomeX = FromCorner(970.0f, 1920.0f);
		mHomeY = FromCorner(612.0f, 1200.0f);
		mPrices[0] = 0x177;
		mPrices[1] = 0x1c2;
		mPrices[2] = 0xed8;
		break;
	case BUILDING_SPEAR_CRAFT:
		mStages[0].mName = "meta_spearcraft1";
		mStages[1].mName = "meta_spearcraft2";
		mStages[2].mName = "meta_spearcraft3";
		mHomeX = FromCorner(1232.0f, 1920.0f);
		mHomeY = FromCorner(1170.0f, 1200.0f);
		mPrices[0] = 0x9c4;
		mPrices[1] = 0xdac;
		mPrices[2] = 4000;
		break;
	case BUILDING_HELPER:
		mStages[0].mName = "meta_helper1";
		mStages[1].mName = "meta_helper2";
		mStages[2].mName = "meta_helper3";
		mHomeX = FromCorner(1240.0f, 1920.0f);
		mHomeY = FromCorner(900.0f, 1200.0f);
		mPrices[0] = 0x4b0;
		mPrices[1] = 4000;
		mPrices[2] = 0x1194;
		AddSprite("metamonkey_petshop", 0x3f, 0, 0, -0x14, 0x3f, 2);
		break;
	case BUILDING_FLIGHT_SCHOOL:
		SetStage(&mStages[0], "meta_flightschool1", 0x121, 0x15d, 3, 4);
		SetStage(&mStages[1], "meta_flightschool2", 0x177, 0x15f, 3, 3);
		SetStage(&mStages[2], "meta_flightschool3", 0x1ad, 0x19b, 3, 2);
		mHomeX = FromCorner(270.0f, 1920.0f);
		mHomeY = FromCorner(925.0f, 1200.0f);
		mPrices[0] = 300;
		mPrices[1] = 600;
		mPrices[2] = 0xdac;
		break;
	case BUILDING_GREENHOUSE:
		mStages[0].mName = "meta_greenhouse1";
		mStages[1].mName = "meta_greenhouse2";
		SetStage(&mStages[2], "meta_greenhouse3", 0x1a7, 0x107, 3, 0x14);
		mHomeX = FromCorner(400.0f, 1920.0f);
		mHomeY = FromCorner(430.0f, 1200.0f);
		mPrices[0] = 0x1a4;
		mPrices[1] = 600;
		mPrices[2] = 0x578;
		AddSprite("metamonkey_greenhouse", 0x3f, 0, 0, -0x20, 0x3f, 2);
		break;
	case BUILDING_WEATHER:
		mStages[0].mName = "meta_weather1";
		mStages[1].mName = "meta_weather2";
		mStages[2].mName = "meta_weather3";
		mHomeX = FromCorner(755.0f, 1920.0f);
		mHomeY = FromCorner(850.0f, 1200.0f);
		mPrices[0] = 800;
		mPrices[1] = 0x5dc;
		mPrices[2] = 2000;
		AddSprite("meta_weather_rain", 0x57, 0, 8, -254, 0x4d, 3);
		AddSprite("meta_weather_tornado", 0x6b, 1, -100, -140, 0x6b, 10);
		AddSprite("meta_weather_earthquake", 0x61, 2, 110, -150, 0x53, 1);
		break;
	case BUILDING_IDOL:
		mStages[0].mName = "meta_idol1";
		mStages[1].mName = "meta_idol2";
		mStages[2].mName = "meta_idol3";
		mHomeX = FromCorner(540.0f, 1920.0f);
		mHomeY = FromCorner(670.0f, 1200.0f);
		mPrices[0] = 0x9c4;
		mPrices[1] = 3000;
		mPrices[2] = 4000;
		AddSprite("metamonkey_idol", 0x3f, 0, 0x10, -0x40, 0x3f, 2);
		break;
	default:
		break;
	}
	SetDepthFromY(mHomeY / 1200.0f + 100.0f);
}

// 0x4480e0 (EAX this): the building is at theLevel (above 0) and shows it: dropped in (a level above the first, after
// the old picture goes up) when theAnimate, or put in place at once
void MetaBuilding::SetLevel(int theLevel, bool theAnimate)
{
	if (theLevel == 0)
		return;
	SetState(STATE_X, mHomeX);
	SetState(STATE_Y, mHomeY);
	mLevel = theLevel;
	SetParent(sMetaGame, nullptr);
	Activate(this, false);
	if (!theAnimate) {
		LoadStagePicture();
		mIdleKey->mValue[STATE_OFFSET_Y] = -(float)(mAnim->GetSourceCelHeight() * 0.5);
		mIdleKey->mValue[STATE_EXTRA_HEIGHT] = 0.0f;
		StartTrack(mIdleTrack);
	} else if (mStage == 2) {
		StartLeave();
	} else {
		StartDrop();
	}
	for (int i = 0; i < META_STAGES; i++) {
		ObjBase* aSprite = mSprites[i].mObj;
		if (aSprite != nullptr) {
			if (i > theLevel - 1)
				Deactivate(aSprite);
			else
				Activate(aSprite, false);
		}
	}
	mStage = 2;
	mLanded = true;
}

// 0x4481f0 (EAX this): the building drops in from above, taller than the picture, and dust puffs up as it comes
void MetaBuilding::StartDrop()
{
	LoadStagePicture();
	mDropping = true;
	float aHeight = mAnim->GetSourceCelHeight();
	float aHalf = (float)(0.5 * aHeight);
	mDropKey->mValue[STATE_OFFSET_Y] = (float)(aHeight - aHalf);
	mDropKey->mValue[STATE_EXTRA_HEIGHT] = (float)(-aHeight + -aHeight);
	float aNegHalf = -aHalf;
	mDropRestKey->mValue[STATE_OFFSET_Y] = aNegHalf;
	mDropRestKey->mValue[STATE_EXTRA_HEIGHT] = 0.0f;
	mDropLandKey->mValue[STATE_OFFSET_Y] = aNegHalf;
	mDropLandKey->mValue[STATE_EXTRA_HEIGHT] = 0.0f;
	StartTrack(mDropTrack);
}

// 0x4482a0 (EAX this): the old picture goes up out of sight; the key that ends it tells the building to drop the next
void MetaBuilding::StartLeave()
{
	if (mAnim == nullptr)
		return;
	mDropping = true;
	float aHeight = mAnim->GetSourceCelHeight();
	float aHalf = (float)(0.5 * aHeight);
	mLeaveKey->mValue[STATE_OFFSET_Y] = -aHalf;
	mLeaveKey->mValue[STATE_EXTRA_HEIGHT] = 0.0f;
	mLeaveEndKey->mValue[STATE_OFFSET_Y] = (float)(aHeight - aHalf);
	mLeaveEndKey->mValue[STATE_EXTRA_HEIGHT] = (float)(-aHeight + -aHeight);
	StartTrack(mLeaveTrack);
}

// 0x448330: the three tracks. Keys 0x38c to 0x394 drop the building in (the picture's height sets their values as it
// starts), 0x398 and 0x39c send it away, and 0x3a0 holds it still.
void MetaBuilding::BuildTracks()
{
	AnimKey* aDropFall = new AnimKey();
	aDropFall->SetDuration(100);
	aDropFall->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aDropFall->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aDropFall->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aDropFall->AddModifier(STATE_OFFSET_X, false, 0.0f, MOD_RANDOM, 2.0f, -1.0f, 1.0f, nullptr);
	aDropFall->AddValue(STATE_OFFSET_Y, 600.0f, MOD_TOWARD_NEXT);
	aDropFall->AddValue(STATE_EXTRA_HEIGHT, -600.0f, MOD_TOWARD_NEXT);
	mDropKey = aDropFall;

	AnimKey* aDropRest = new AnimKey();
	aDropRest->SetDuration(1);
	aDropRest->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aDropRest->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	aDropRest->AddValue(STATE_EXTRA_HEIGHT, 0.0f, MOD_TOWARD_NEXT);
	mDropRestKey = aDropRest;

	AnimKey* aDropLand = new AnimKey();
	aDropLand->mEventId = BUILDING_EVENT_LANDED;
	aDropLand->SetDuration(10);
	aDropLand->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aDropLand->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	mDropLandKey = aDropLand;

	AnimTrack* aDropTrack = new AnimTrack();
	aDropTrack->AddKey(aDropFall, nullptr);
	aDropTrack->AddKey(aDropRest, nullptr);
	aDropTrack->AddKey(aDropLand, aDropLand);
	mDropTrack = AddTrack(aDropTrack, nullptr);

	AnimKey* aLeave = new AnimKey();
	aLeave->SetDuration(100);
	aLeave->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aLeave->AddModifier(STATE_OFFSET_X, false, 0.0f, MOD_RANDOM, 2.0f, -1.0f, 1.0f, nullptr);
	aLeave->AddValue(STATE_OFFSET_Y, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_EXTRA_HEIGHT, 0.0f, MOD_TOWARD_NEXT);
	mLeaveKey = aLeave;

	AnimKey* aLeaveEnd = new AnimKey();
	aLeaveEnd->SetDuration(1);
	aLeaveEnd->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aLeaveEnd->AddValue(STATE_OFFSET_Y, 600.0f, MOD_TOWARD_NEXT);
	aLeaveEnd->AddValue(STATE_EXTRA_HEIGHT, -600.0f, MOD_TOWARD_NEXT);
	mLeaveEndKey = aLeaveEnd;

	AnimKey* aLeft = new AnimKey();
	aLeft->SetDuration(10);
	aLeft->mEventId = BUILDING_EVENT_LEFT;
	aLeft->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aLeft->AddValue(STATE_OFFSET_Y, 1200.0f, MOD_HOLD);
	aLeft->AddValue(STATE_EXTRA_HEIGHT, -1200.0f, MOD_HOLD);

	AnimTrack* aLeaveTrack = new AnimTrack();
	aLeaveTrack->AddKey(aLeave, nullptr);
	aLeaveTrack->AddKey(aLeaveEnd, nullptr);
	aLeaveTrack->AddKey(aLeft, aLeft);
	mLeaveTrack = AddTrack(aLeaveTrack, nullptr);

	AnimKey* aIdle = new AnimKey();
	aIdle->SetDuration(10);
	aIdle->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aIdle->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aIdle->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aIdle->AddValue(STATE_OFFSET_X, 0.0f, MOD_HOLD);
	aIdle->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	aIdle->AddValue(STATE_EXTRA_HEIGHT, 0.0f, MOD_HOLD);
	mIdleKey = aIdle;

	AnimTrack* aIdleTrack = new AnimTrack();
	aIdleTrack->AddKey(aIdle, aIdle);
	mIdleTrack = AddTrack(aIdleTrack, nullptr);
}

// 0x448980: the number text that grows from the building and rises, hidden until used
void MetaBuilding::BuildPriceText()
{
	ObjString* aText = new ObjString();
	mPriceText = aText;
	aText->SetParent(this, nullptr);
	aText->SetState(STATE_X, -16.0f);
	aText->SetState(STATE_Y, 16.0f);
	aText->SetState(STATE_Z, 5.0f);
	aText->SetFont(gApp->mFont);
	aText->SetStateInt(STATE_SCALE_X, 0);
	aText->SetStateInt(STATE_SCALE_Y, 0);
	aText->SetState(STATE_RED, 0.1f);
	aText->SetStateInt(STATE_GREEN, 1);
	aText->SetState(STATE_BLUE, 0.3f);
	aText->SetStateInt(STATE_ALPHA, 0);
	aText->Create("000", true, 0);

	AnimKey* aGrow = new AnimKey();
	aGrow->SetDuration(5);
	aGrow->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aGrow->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aGrow->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_X, 1.3f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_Y, 1.3f, MOD_TOWARD_NEXT);

	AnimKey* aShown = new AnimKey();
	aShown->SetDuration(5);
	aShown->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aShown->AddValue(STATE_SCALE_X, 1.3f, MOD_TOWARD_NEXT);
	aShown->AddValue(STATE_SCALE_Y, 1.3f, MOD_TOWARD_NEXT);
	aShown->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);

	AnimKey* aHold = new AnimKey();
	aHold->SetDuration(150);
	aHold->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aHold->AddValue(STATE_SCALE_X, 1.3f, MOD_TOWARD_NEXT);
	aHold->AddValue(STATE_SCALE_Y, 1.3f, MOD_TOWARD_NEXT);
	aHold->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);

	AnimKey* aRise = new AnimKey();
	aRise->SetDuration(50);
	aRise->AddValue(STATE_Y, -128.0f, MOD_HOLD);
	aRise->AddValue(STATE_SCALE_X, 1.35f, MOD_HOLD);
	aRise->AddValue(STATE_SCALE_Y, 1.35f, MOD_HOLD);
	aRise->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aGrow, nullptr);
	aTrack->AddKey(aShown, nullptr);
	aTrack->AddKey(aHold, nullptr);
	aTrack->AddKey(aRise, nullptr);
	aText->AddTrack(aTrack, nullptr);
	Deactivate(aText);
}
