// The widgets of the options and name screens (0x4364d0-0x437ae0, 0x43a720).

#include "Widgets.h"

#include "ImageRecord.h"
#include "Level.h"
#include "Profiles.h"
#include "SceneTag.h"
#include "TextObject.h"
#include "VeniceDraw.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstring>

namespace
{
const float PercentOfFull = 100.0f;
const float LowestMusicVolume = 10.0f; // a music volume that is not off is not lower than this
const int LevelsOfStartLevelSlider = 11;
const int HotSoundBaseFrequency = 40100;
const int HotSoundFrequencyRange = 8000;
}

// 0x4364f0
void UserDataDisplay::Load(const char* theText, SceneTag* theTag)
{
	strcpy(mDisplayType, theTag->GetString("display_type"));
	strcpy(mFontName, theTag->GetString("font"));
	SceneObject::Load(theText, theTag);
}

void UserDataDisplay::DrawSelf()
{
	if (mVisible != 0 && (mStages.mCursor == nullptr || mStages.mCursor->mVisible != 0)) {
		if (strcmp(mDisplayType, "current_user_name") == 0)
			DrawTextOnObject(this, CurrentProfile().mName, mFontName, 0.0f, 0.0f, 0.0f, JustifyLeft, 1.0f, 1.0f);
	}
}

// 0x436690
void UserListBox::Load(const char* theText, SceneTag* theTag)
{
	strcpy(mFontName, theTag->GetString("font"));
	mTextScale = (float)theTag->GetFloat("size");
	SceneObject::Load(theText, theTag);
}

float UserListBox::RowPitch() const
{
	FontRecord* aFont = AcquireFont(mFontName, nullptr, 0);
	return aFont->MeasureWidth("M", mTextScale, 0.0f) + 5.0f;
}

double UserListBox::RowY(int theRow) const
{
	return ((double)theRow - 5.0 - 1.0) * (double)RowPitch() + (double)mLocal[AttributeY];
}

// A click on the row of a name makes the player current.
bool UserListBox::Update()
{
	int aRow = 0;
	for (int aProfile = 0; aProfile < ProfileCount; aProfile++) {
		if (strlen(gProfiles[aProfile].mName) != 0) {
			float aRowPitch = RowPitch();
			float aHalfWidth = (mSourceRight - mSourceLeft) / 2.0f;
			if (gActiveLevel->mLeftClick != 0.0f) {
				double aRowY = RowY(aRow);
				if (RectsOverlap(gActiveLevel->mMouseX, gActiveLevel->mMouseY, 1.0f, 1.0f, mWorld[AttributeX],
						(float)(aRowY + (double)mWorld[AttributeY]), aHalfWidth, aRowPitch))
					gCurrentProfile = aProfile;
			}
			aRow++;
		}
	}
	return SceneObject::Update();
}

// The current player's row is lit (a quarter white) while the box is at its full size, and each name is drawn in the middle.
void UserListBox::Draw()
{
	SceneObject::Draw();
	int aRow = 0;
	for (int aProfile = 0; aProfile < ProfileCount; aProfile++) {
		if (strlen(gProfiles[aProfile].mName) != 0) {
			if (aProfile == gCurrentProfile && VeniceAbs(mWorld[AttributeXScale] - 1.0f) < 0.01f) {
				float aRowPitch = RowPitch();
				float aHalfWidth = (mSourceRight - mSourceLeft) / 2.0f;
				double aRowCentreY = RowY(aRow) + (double)mWorld[AttributeY];
				DrawBox((int)(mWorld[AttributeX] - aHalfWidth), (int)(aRowCentreY - (double)aRowPitch / 2.0),
					(int)(mWorld[AttributeX] + aHalfWidth), (int)(aRowCentreY + (double)aRowPitch / 2.0), 1.0f, 1.0f, 1.0f, 0.25f,
					1.0f, 1.0f, 1.0f, 0.0f);
			}
			DrawTextOnObject(this, gProfiles[aProfile].mName, mFontName, mLocal[AttributeX], (float)RowY(aRow), 0.0f,
				JustifyCenter, mTextScale, 1.0f);
			aRow++;
		}
	}
}

// 0x437270: the knob starts where the setting it sets is now
void Slider::Load(const char* theText, SceneTag* theTag)
{
	mLeftValue = (float)theTag->GetFloat("left_value");
	mRightValue = (float)theTag->GetFloat("right_value");
	if (theTag->Has("position"))
		mLocal[AttributeX] = (float)theTag->GetFloat("position");
	if (theTag->Has("slider_type")) {
		const char* aKindName = theTag->GetString("slider_type");
		if (strcmp(aKindName, "music_volume") == 0)
			mKind = SliderKindMusicVolume;
		else if (strcmp(aKindName, "sound_volume") == 0)
			mKind = SliderKindSoundVolume;
		else if (strcmp(aKindName, "start_level") == 0)
			mKind = SliderKindStartLevel;
	}
	SceneObject::Load(theText, theTag);
	float aPercent = 0.0f;
	if (mKind == SliderKindSoundVolume)
		aPercent = (float)gSoundVolume;
	else if (mKind == SliderKindMusicVolume)
		aPercent = (float)gMusicVolume;
	aPercent = aPercent / PercentOfFull;
	mLocal[AttributeX] = mLeftValue + aPercent * (mRightValue - mLeftValue);
}

// A press on the slider's row, in the width of its travel and a little more, takes hold of the knob, which then follows the
// pointer to the ends of the travel until the button is let go.
bool Slider::Update()
{
	UpdateWorldValues();
	float aPreviousX = mLocal[AttributeX];
	// asking for the press moves on the button's remembered state, as the level's own check does (the answer is not used)
	LeftButtonPressed();
	bool aButtonDown = IsLeftButtonDown();
	bool aGrabbed = (gActiveLevel->mLeftClick != 0.0f || (aButtonDown && mIsSliding)) &&
		RectsOverlap(gActiveLevel->mMouseX, gActiveLevel->mMouseY, 1.0f, 1.0f, gScreenWidth / 2.0f, mWorld[AttributeY],
			(mRightValue - mLeftValue) + 40.0f, mImage->mFrameHeight);
	if (aGrabbed)
		mIsSliding = true;
	else if (!aButtonDown)
		mIsSliding = false;
	if (mIsSliding)
		mLocal[AttributeX] = GetLocalX(gActiveLevel->mMouseX, gActiveLevel->mMouseY);
	mLocal[AttributeX] = mLocal[AttributeX] > mLeftValue ? mLocal[AttributeX] : mLeftValue;
	mLocal[AttributeX] = mRightValue > mLocal[AttributeX] ? mLocal[AttributeX] : mRightValue;
	if (aPreviousX != mLocal[AttributeX])
		ApplyPercent();
	return SceneObject::Update();
}

float Slider::GetPercent() const
{
	float aTravelSoFar = mLocal[AttributeX] - mLeftValue;
	float aTravel = mRightValue - mLeftValue;
	return (aTravelSoFar * PercentOfFull) / aTravel;
}

// The music's volume is not turned down below a tenth (it is either off or heard), and the music already playing in each scene
// follows the knob.
void Slider::ApplyPercent()
{
	if (mKind == SliderKindMusicVolume) {
		float aVolume = GetPercent();
		if (aVolume != 0.0f && aVolume < LowestMusicVolume)
			aVolume = LowestMusicVolume;
		SetVolumes((int)aVolume, -1);
		for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext)
			static_cast<Level*>(aScene)->SetMusicVolume(aVolume);
	} else if (mKind == SliderKindSoundVolume) {
		SetVolumes(-1, (int)GetPercent());
	} else if (mKind == SliderKindStartLevel) {
		gLevelNumber = ((int)GetPercent() * LevelsOfStartLevelSlider) / (int)PercentOfFull;
	}
}

// 0x4378c0
void Checkbox::Load(const char* theText, SceneTag* theTag)
{
	if (theTag->Has("checked"))
		mChecked = theTag->GetInt("checked");
	if (theTag->Has("right_area"))
		mRightArea = (float)theTag->GetFloat("right_area");
	if (theTag->Has("check_type")) {
		const char* aKindName = theTag->GetString("check_type");
		if (strcmp(aKindName, "3d") == 0)
			mKind = CheckboxKindAccelerated;
		else if (strcmp(aKindName, "fullscreen") == 0)
			mKind = CheckboxKindFullscreen;
		else if (strcmp(aKindName, "instructions") == 0)
			mKind = CheckboxKindInstructions;
	}
	SceneObject::Load(theText, theTag);
}

// Hot like any object, but the box counts as hit by the stripe of the width "right_area" that ends at the pointer: the box is hot
// while the pointer is on it or up to that far to its right (over its label), and only on the top scene.
bool Checkbox::IsHot()
{
	if (gActiveLevel->mNext != nullptr)
		return false;
	bool aIsHot = HitTest(gActiveLevel->mMouseX - mRightArea / 2.0f, gActiveLevel->mMouseY, mRightArea, 1.0f, 0.0f);
	if (aIsHot && !mIsHot) {
		int aFrequency = (int)RandomBelow(HotSoundFrequencyRange) + HotSoundBaseFrequency;
		PlaySoundByName("audio\\hot.ogg", aFrequency);
	}
	mIsHot = aIsHot;
	return aIsHot;
}

// A click while hot turns the box over; then the box shows what it sets (the display and the player's flag can be changed
// elsewhere), the tick as clear as it is checked.
bool Checkbox::Update()
{
	if (IsHot() && gActiveLevel->mLeftClick != 0.0f)
		Toggle();
	if (mKind == CheckboxKindAccelerated)
		mChecked = IsAccelerated() ? 1 : 0;
	else if (mKind == CheckboxKindFullscreen)
		mChecked = IsWindowedOnScreen() ? 0 : 1;
	else if (mKind == CheckboxKindInstructions)
		mChecked = CurrentProfile().mInstructionsHidden != 0 ? 1 : 0;
	mLocal[AttributeAlpha] = mChecked != 0 ? 1.0f : 0.0f;
	return SceneObject::Update();
}

void Checkbox::Toggle()
{
	mChecked = 1 - mChecked;
	if (mKind == CheckboxKindAccelerated)
		SetAcceleratedDisplay(mChecked != 0);
	else if (mKind == CheckboxKindFullscreen)
		SetWindowedDisplay(mChecked != 1);
	else if (mKind == CheckboxKindInstructions)
		CurrentProfile().mInstructionsHidden = mChecked;
}
