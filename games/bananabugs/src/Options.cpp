#include "Options.h"

#include "Anim.h"
#include "GameApp.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The controls' ids (ObjBase::mButtonId); 100 and 101 are BBWindow's OK and Cancel
enum OptionsControl
{
	OPTIONS_OK = 100,
	OPTIONS_CANCEL = 101,
	OPTIONS_MUSIC_SLIDER = 102,
	OPTIONS_SFX_SLIDER = 103,
	OPTIONS_3D_BOX = 104,
	OPTIONS_FULL_SCREEN_BOX = 105,
};

// The panel's size, in 100-pixel tiles
const int OPTIONS_COLUMNS = 11;
const int OPTIONS_ROWS = 7;

// The controls sit 158 pixels right of the panel's middle; a slider's knob runs 200 pixels each way
const int OPTIONS_CONTROL_X = 158;
const double OPTIONS_KNOB_REACH = 200.0;
static const float OPTIONS_SLIDER_Y[OPTIONS_SLIDERS] = { -228.0f, -150.0f };
static const float OPTIONS_CHECKBOX_Y[OPTIONS_CHECKBOXES] = { -65.0f, 16.0f };

// How often the sound slider plays its sample while it is dragged
const int OPTIONS_SFX_SAMPLE_FRAMES = 21;

int Options::sLastSfxKnobX = 0;

// 0x453e30: the controls start at the game's settings, and the lines in yellow name them
Options::Options()
{
	mState = BBWINDOW_NOT_OPEN;
	SetStateInt(STATE_Z, 525);
	ComputeDrawn();
	mAccelerated = gApp->Is3DAccelerated();
	mWasAccelerated = mAccelerated;
	mWindowed = gApp->mIsWindowed;
	mWasWindowed = mWindowed;
	mMusicVolume = (float)gApp->GetMusicVolume();
	mOldMusicVolume = mMusicVolume;
	mSfxVolume = (float)gApp->GetSfxVolume();
	mOldSfxVolume = mSfxVolume;
	mSfxDragFrames = 0;
	mSfxDragging = false;
	BuildSliders();
	BuildCheckboxes();
	Open(OPTIONS_COLUMNS, OPTIONS_ROWS, 2, false);
	BuildLines(gApp->mFont, true);
	static const char* const LINES[] = { "MUSIC", "SOUND FX", "GFX ACCELERATION", "FULL SCREEN" };
	for (int i = 0; i < 4; i++) {
		MoveLines(i, 48.0f, true);
		SetLineColor(i, 1.0f, 1.0f, 0.0f, 1.0f);
		SetLineScale(i, 1.0f, 1.0f);
		SetLineText(i, LINES[i]);
	}
	mState = BBWINDOW_OPEN;
	mKnobs[0]->SetState(STATE_X, (float)(mMusicVolume * 400.0 - OPTIONS_KNOB_REACH));
	mKnobs[0]->ComputeDrawn();
	mKnobs[1]->SetState(STATE_X, (float)(mSfxVolume * 400.0 - OPTIONS_KNOB_REACH));
	mKnobs[1]->ComputeDrawn();
	mCheckboxes[0]->SetStateInt(STATE_FRAME, mAccelerated ? 1 : 0);
	mCheckboxes[1]->SetStateInt(STATE_FRAME, mWindowed ? 0 : 1);
}

// 0x4541a0: the window skips BBWindow's own update, so its buttons do not swell. While the sound slider is dragged a
// sample plays now and then.
bool Options::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mSfxDragging) {
		mSfxDragFrames++;
		if (mSfxDragFrames % OPTIONS_SFX_SAMPLE_FRAMES == 0)
			gApp->mSoundQueue->Request(SOUND_MONKEY_OHH1, 0, false);
	}
	return true;
}

// The knob's place under the mouse, within its reach; both sliders measure from the music one, which is right above
float Options::KnobXAtMouse()
{
	float aKnobX = sMouseDesignX - mSliders[0]->mScreenX;
	if (aKnobX < -OPTIONS_KNOB_REACH)
		aKnobX = (float)-OPTIONS_KNOB_REACH;
	if (aKnobX > OPTIONS_KNOB_REACH)
		aKnobX = (float)OPTIONS_KNOB_REACH;
	return aKnobX;
}

// 0x4541f0: a slider held down follows the mouse, its volume with it
bool Options::MouseDown(ObjBase* theObj, bool isDown)
{
	(void)isDown;
	if (mState != BBWINDOW_OPEN)
		return false;
	if (theObj->mButtonId == OPTIONS_MUSIC_SLIDER) {
		float aKnobX = KnobXAtMouse();
		float aVolume = (float)((OPTIONS_KNOB_REACH + aKnobX) / 400.0);
		gApp->SetMusicVolume(aVolume);
		mMusicVolume = aVolume;
		mKnobs[0]->SetState(STATE_X, aKnobX);
		mKnobs[0]->ComputeDrawn();
		return true;
	}
	if (theObj->mButtonId == OPTIONS_SFX_SLIDER) {
		float aKnobX = KnobXAtMouse();
		mSfxDragging = (float)sLastSfxKnobX != aKnobX;
		sLastSfxKnobX = (int)aKnobX;
		float aVolume = (float)((OPTIONS_KNOB_REACH + aKnobX) / 400.0);
		gApp->SetSfxVolume(aVolume);
		mSfxVolume = aVolume;
		mKnobs[1]->SetState(STATE_X, aKnobX);
		mKnobs[1]->ComputeDrawn();
		return true;
	}
	return false;
}

// 0x4543c0: a slider takes its last place, with a sample for the sounds; a box turns over; OK sets the volumes and the
// screen mode, Cancel the volumes as they were
bool Options::MouseUp(ObjBase* theObj)
{
	mSfxDragging = false;
	if (mState != BBWINDOW_OPEN)
		return false;
	BBWindow::MouseUp(theObj);
	switch (theObj->mButtonId) {
	case OPTIONS_OK:
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		gApp->SetMusicVolume(mMusicVolume);
		gApp->SetSfxVolume(mSfxVolume);
		if (gApp->Is3DAccelerationSupported() && mAccelerated) {
			gApp->Set3DAcclerated(true, true);
			gApp->SetForcedSize(-1, -1);
		} else {
			gApp->Set3DAcclerated(false, true);
			gApp->SetForcedSize(800, 600);
		}
		gApp->SwitchScreenMode(mWindowed, gApp->Is3DAccelerated(), false);
		return true;
	case OPTIONS_CANCEL:
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		gApp->SetMusicVolume(mOldMusicVolume);
		gApp->SetSfxVolume(mOldSfxVolume);
		return true;
	case OPTIONS_MUSIC_SLIDER: {
		float aKnobX = KnobXAtMouse();
		float aVolume = (float)((OPTIONS_KNOB_REACH + aKnobX) / 400.0);
		gApp->SetMusicVolume(aVolume);
		mMusicVolume = aVolume;
		mKnobs[0]->SetState(STATE_X, aKnobX);
		mKnobs[0]->ComputeDrawn();
		return true;
	}
	case OPTIONS_SFX_SLIDER: {
		float aKnobX = KnobXAtMouse();
		float aVolume = (float)((OPTIONS_KNOB_REACH + aKnobX) / 400.0);
		gApp->SetSfxVolume(aVolume);
		mSfxVolume = aVolume;
		mKnobs[1]->SetState(STATE_X, aKnobX);
		mKnobs[1]->ComputeDrawn();
		gApp->mSoundQueue->Request(SOUND_MONKEY_MIKE, 0, false);
		return true;
	}
	case OPTIONS_3D_BOX:
		if (mCheckboxes[0]->mBase[STATE_FRAME] == 0.0f) {
			mCheckboxes[0]->SetStateInt(STATE_FRAME, 1);
			mAccelerated = true;
		} else {
			mCheckboxes[0]->SetStateInt(STATE_FRAME, 0);
			mAccelerated = false;
		}
		break;
	case OPTIONS_FULL_SCREEN_BOX:
		if (mCheckboxes[1]->mBase[STATE_FRAME] == 0.0f) {
			mCheckboxes[1]->SetStateInt(STATE_FRAME, 1);
			mWindowed = false;
		} else {
			mWindowed = true;
			mCheckboxes[1]->SetStateInt(STATE_FRAME, 0);
		}
		break;
	default:
		return true;
	}
	gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
	return true;
}

// 0x454740: each slider is a bar the mouse takes, with its knob on it
void Options::BuildSliders()
{
	for (int i = 0; i < OPTIONS_SLIDERS; i++) {
		ObjBase* aSlider = new ObjBase();
		mSliders[i] = aSlider;
		aSlider->mAnim = Anim::Get("images/menu/slider", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
		aSlider->SetParent(this, nullptr);
		aSlider->SetMouseable(32.0f, 32.0f);
		aSlider->SetMouseListener(this);
		aSlider->SetStateInt(STATE_X, OPTIONS_CONTROL_X);
		aSlider->SetState(STATE_Y, OPTIONS_SLIDER_Y[i]);
		aSlider->SetStateInt(STATE_Z, 1);
		aSlider->SetStateInt(STATE_ALPHA, 1);
		aSlider->SetStateInt(STATE_SCALE_X, 1);
		aSlider->SetStateInt(STATE_SCALE_Y, 1);
		aSlider->mButtonId = OPTIONS_MUSIC_SLIDER + i;
		Activate(aSlider, false);

		ObjBase* aKnob = new ObjBase();
		mKnobs[i] = aKnob;
		aKnob->mAnim = Anim::Get("images/menu/sliderbar", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
		aKnob->SetParent(mSliders[i], nullptr);
		aKnob->SetStateInt(STATE_X, 0);
		aKnob->SetStateInt(STATE_Y, 0);
		aKnob->SetStateInt(STATE_Z, 1);
		aKnob->SetStateInt(STATE_ALPHA, 1);
		aKnob->SetStateInt(STATE_SCALE_X, 1);
		aKnob->SetStateInt(STATE_SCALE_Y, 1);
		Activate(aKnob, false);
	}
}

// 0x454900: two cels, empty and ticked
void Options::BuildCheckboxes()
{
	for (int i = 0; i < OPTIONS_CHECKBOXES; i++) {
		ObjBase* aBox = new ObjBase();
		mCheckboxes[i] = aBox;
		aBox->mAnim = Anim::Get("images/menu/checkbox", ANIM_SCALE_SMOOTH, false, 63, 63, 2, false);
		aBox->SetParent(this, nullptr);
		aBox->SetMouseable(32.0f, 32.0f);
		aBox->SetMouseListener(this);
		aBox->SetStateInt(STATE_X, OPTIONS_CONTROL_X);
		aBox->SetState(STATE_Y, OPTIONS_CHECKBOX_Y[i]);
		aBox->SetStateInt(STATE_Z, 1);
		aBox->SetStateInt(STATE_ALPHA, 1);
		aBox->SetStateInt(STATE_SCALE_X, 1);
		aBox->SetStateInt(STATE_SCALE_Y, 1);
		aBox->SetStateInt(STATE_FRAME, 0);
		aBox->mButtonId = OPTIONS_3D_BOX + i;
		Activate(aBox, false);
	}
}
