// The level (0x40e820-0x40f960).

#include "Level.h"

#include "Profiles.h"
#include "SceneLoader.h"
#include "VeniceDraw.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"
#include "VeniceMath.h"

#include <cstring>

// sets the volume of the voice this level started on a piece of music: a sound that several levels play keeps only the channel
// of its latest start, so the level's own channel is put in place for the call
static void SetMusicChannelVolume(const LevelMusic& theMusic, float thePercent)
{
	int aSavedChannel = theMusic.mSound->mChannel;
	theMusic.mSound->mChannel = theMusic.mChannel;
	theMusic.mSound->SetVolume(thePercent);
	theMusic.mSound->mChannel = aSavedChannel;
}

// 0x40ea20: a level that has another scene above it silences its music (the sounds go to volume 0), and its main music is freed
// unless the scene now on top plays it too
Level::~Level()
{
	if (mNext == nullptr)
		return;
	if (mIntroMusic.mSound != nullptr)
		SetMusicChannelVolume(mIntroMusic, 0.0f);
	if (mMainMusic.mSound != nullptr)
		SetMusicChannelVolume(mMainMusic, 0.0f);
	if (mWinMusic.mSound != nullptr)
		SetMusicChannelVolume(mWinMusic, 0.0f);
	if (mMainMusic.mSound != nullptr && this != gScenes.mTail &&
		mMainMusic.mSound != static_cast<Level*>(gScenes.mTail)->mMainMusic.mSound)
		FreeSound(mMainMusic.mSound);
}

// 0x40ec10
void Level::Load(const char* theText, SceneTag* theTag)
{
	SceneObject::Load(theText, theTag);
	if (theTag->Has("intro_music")) {
		strcpy(mIntroMusic.mName, theTag->GetString("intro_music"));
		mIntroMusic.mSound = LoadSound(mIntroMusic.mName, false, true);
	}
	mIntroTicksLeft = (float)(theTag->GetFloat("intro_time") / 4.0);
	mIntroTime = (float)theTag->GetFloat("intro_time");
	if (theTag->Has("main_music")) {
		strcpy(mMainMusic.mName, theTag->GetString("main_music"));
		mMainMusic.mSound = LoadSound(mMainMusic.mName, true, true);
	}
	if (theTag->Has("win_music")) {
		strcpy(mWinMusic.mName, theTag->GetString("win_music"));
		mWinMusic.mSound = LoadSound(mWinMusic.mName, false, true);
	}
	if (theTag->Has("thumbnail"))
		strcpy(mThumbnail, theTag->GetString("thumbnail"));
	if (theTag->Has("win_message"))
		strcpy(mWinMessage, theTag->GetString("win_message"));
	if (theTag->Has("lose_message"))
		strcpy(mLoseMessage, theTag->GetString("lose_message"));
	if (theTag->Has("title")) {
		strcpy(mTitle, theTag->GetString("title"));
		// a level with a title gets its title text object from a file of objects, which one depends on the level
		const char* aTextFile = nullptr;
		if (strcmp(mName, "trickshot") == 0)
			aTextFile = CurrentProfile().mMode == GameModeTrickShot ? "objects\\trickymode_text.xml" : "objects\\trickshot_text.xml";
		else if (strcmp(mName, "bonus") == 0)
			aTextFile = "objects\\bonus_text.xml";
		else if (strstr(mName, "win_sequence") == nullptr)
			aTextFile = "objects\\text.xml";
		std::string aText;
		if (aTextFile != nullptr && ReadGameFile(aTextFile, &aText))
			LoadSceneObjects(aText.c_str(), this);
	}
}

// 0x40ef50: the pictures of the sparkles and particles, the layer they are drawn in, and the intro music, which starts at once
void Level::OnFirstUpdate()
{
	SceneObject::OnFirstUpdate();
	mSparkleImage = AcquireImageRecord("gfx\\sparkle.png", 0, 0, 0, 0);
	mLevelParticleImage = AcquireImageRecord("gfx\\particle2.png", 0, 0, 0, 0);
	mWhiteParticleImage = AcquireImageRecord("gfx\\white_particle.png", 0, 0, 0, 0);
	gSparkleLayer = new SceneObject();
	gSparkleLayer->mType = ObjectTypeSparkleLayer;
	gSparkleLayer->mZOrder = ParticleZOrder;
	gSparkleLayer->AddStage(1000.0f, 1000, 1, 1, nullptr);
	NewChild(gSparkleLayer);
	if (mIntroMusic.mSound != nullptr) {
		mIntroMusic.mSound->Play(-1);
		mIntroMusic.mChannel = mIntroMusic.mSound->mChannel;
	}
}

// 0x40f090
bool Level::Update()
{
	gActiveLevel = this;
	gShooter = FindChildByType(ObjectTypeShooter, nullptr);
	gSparkleLayer = FindChildByType(ObjectTypeSparkleLayer, nullptr);
	gOuterLayer = FindChildByType(ObjectTypeLayer, nullptr);
	gMiddleLayer = gOuterLayer == nullptr ? nullptr : gOuterLayer->FindChildByType(ObjectTypeLayer, nullptr);
	gPlateLayer = gMiddleLayer == nullptr ? nullptr : gMiddleLayer->FindChildByType(ObjectTypeLayer, nullptr);
	gTutorial = FindChildByType(ObjectTypeTutorial, nullptr);
	if (mIsDead)
		return false;
	if (mPaused != 0)
		return true;

	if (mNext == nullptr) {
		mMouseX = (float)gMouseX;
		mMouseY = (float)gMouseY;
		mLeftPressed = (float)(LeftButtonPressed() ? 1 : 0);
		mRightPressed = (float)(RightButtonPressed() ? 1 : 0);
		mLeftClick = (mLeftPressed == 0.0f || mPreviousLeftPressed != 0.0f) ? 0.0f : 1.0f;
		mRightClick = (mRightPressed == 0.0f || mPreviousRightPressed != 0.0f) ? 0.0f : 1.0f;
		mPreviousLeftPressed = mLeftPressed;
		mPreviousRightPressed = mRightPressed;
	} else {
		mLeftPressed = 0;
		mRightPressed = 0;
		mLeftClick = 0;
		mRightClick = 0;
		mPreviousLeftPressed = 0;
		mPreviousRightPressed = 0;
	}
	mComboTicksLeft = mComboTicksLeft - 1;
	if (mComboTicksLeft < 1) {
		mComboTicksLeft = 0;
		mComboCount = 0;
	}
	if (gMiddleLayer != nullptr && strstr(gMiddleLayer->mStages.mCursor->mName, "defea") != nullptr &&
		RandomBetween(1, 3) == 1) {
		// bubbles rise from the water of a sunk level
		ImageRecord* aBubbleImage = AcquireImageRecord("gfx\\bubble.png", 0, 0, 0, 0);
		SceneObject* aBubble = new SceneObject();
		aBubble->mZOrder = (float)((double)FindChildByName("water", nullptr)->mZOrder - 0.001);
		NewChild(aBubble);
		aBubble->SetImageRecord(aBubbleImage);
		aBubble->AddStage((float)RandomBetween(1000, 3000), 0, 0, 1, nullptr);
		aBubble->SetAttribute(AttributeAlpha, (float)RandomFloat(0.7f, 1.0f), 0.0f);
		aBubble->mLocal[AttributeY] = (float)((double)gScreenHeight / 2.0 - 10.0);
		aBubble->mLocal[AttributeX] = (float)RandomFloat((0.0f - gScreenWidth) / 2.0f, gScreenWidth / 2.0f);
		aBubble->AddFilter(FilterVelocity, AttributeY, (float)RandomFloat(-1.0f, -0.3f), 0.008f, 0.0f);
		aBubble->AddFilter(FilterVelocity, AttributeX, (float)RandomFloat(-0.4f, 0.4f), 0.0f, 0.0f);
		aBubble->mZOrder = ParticleZOrder;
	}
	FadeMusic();
	if (gMiddleLayer != nullptr && strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") != nullptr)
		mScrollTicks = mScrollTicks + 1;
	if (mEndlessSinkTicks != 0) {
		mEndlessSinkTicks = mEndlessSinkTicks + 1;
		if (mEndlessSinkTicks > 750) {
			PlaySoundByName("audio\\endless_sink.ogg", -1);
			mEndlessSinkTicks = 1;
		}
	}
	return SceneObject::Update();
}

// 0x40f5e0
void Level::FadeMusic()
{
	if (CrossFade(mIntroMusic, mMainMusic, &mIntroTicksLeft, 1000.0f))
		mIntroMusic.mSound = nullptr;
	if (CrossFade(mMainMusic, mWinMusic, &mWinFadeTicks, 80.0f))
		mMainMusic.mSound = nullptr;
}

// 0x422fd0 (the fade actions): the volume of each of the level's pieces of music, on the channel this level started it on
void Level::SetMusicVolume(float thePercent)
{
	if (mIntroMusic.mSound != nullptr)
		SetMusicChannelVolume(mIntroMusic, thePercent);
	if (mMainMusic.mSound != nullptr)
		SetMusicChannelVolume(mMainMusic, thePercent);
	if (mWinMusic.mSound != nullptr)
		SetMusicChannelVolume(mWinMusic, thePercent);
}

// 0x422fd0 (the stop music action): the sounds' own latest voices stop
void Level::StopMusic()
{
	if (mIntroMusic.mSound != nullptr)
		mIntroMusic.mSound->SetVolume(-1.0f);
	if (mMainMusic.mSound != nullptr)
		mMainMusic.mSound->SetVolume(-1.0f);
	if (mWinMusic.mSound != nullptr)
		mWinMusic.mSound->SetVolume(-1.0f);
}

// 0x40f690: while the counter runs down, the next music starts silent when it reaches the period and then rises as the
// current one falls (the main music only down to a fifth); at the end the current music stops, unless it is the main music
bool Level::CrossFade(LevelMusic& theFrom, LevelMusic& theTo, float* theTicksLeft, float thePeriod)
{
	if (*theTicksLeft == 0.0f)
		return false;
	*theTicksLeft = *theTicksLeft - 1.0f;
	if (*theTicksLeft == thePeriod && theTo.mSound != nullptr) {
		theTo.mSound->Play(-1);
		theTo.mChannel = theTo.mSound->mChannel;
		theTo.mSound->SetVolume(0.01f);
	} else if (*theTicksLeft <= thePeriod && thePeriod != *theTicksLeft) {
		float aTicks = *theTicksLeft;
		if (theFrom.mSound == mMainMusic.mSound && aTicks <= thePeriod / 5.0f)
			aTicks = thePeriod / 5.0f;
		if (theFrom.mSound != nullptr)
			SetMusicChannelVolume(theFrom, (float)(((double)aTicks * 100.0) / (double)thePeriod));
		if (theTo.mSound != nullptr)
			SetMusicChannelVolume(theTo, (float)(100.0 - ((double)*theTicksLeft * 100.0) / (double)thePeriod));
	}
	if (*theTicksLeft == 0.0f && theFrom.mSound != mMainMusic.mSound) {
		if (theFrom.mSound != nullptr)
			SetMusicChannelVolume(theFrom, -1.0f);
		return true;
	}
	return false;
}

// 0x40f8e0: the bottom scene is drawn over black when it is scaled, turned or dimmed
void Level::Draw()
{
	if (mPrev == nullptr && (mWorld[AttributeXScale] != 1.0f || mWorld[AttributeYScale] != 1.0f ||
			mWorld[AttributeRotation] != 0.0f || mDimmed != 0))
		FillScreenBlack();
	SceneObject::Draw();
}
