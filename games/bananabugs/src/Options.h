// Options: the options window of the main menu (0x453e30..0x454a30 in the original). Two sliders set the music's and
// the sounds' volume as they are dragged, and two boxes choose 3D acceleration and full screen; OK keeps the volumes
// and applies the screen choices, and Cancel puts the volumes back as they were.
#pragma once

#include "BBWindow.h"

namespace Sexy
{

const int OPTIONS_SLIDERS = 2;
const int OPTIONS_CHECKBOXES = 2;

class Options : public BBWindow
{
public:
	bool mAccelerated;       // +0x350: the 3D box's choice
	bool mWindowed;          // +0x351: the full screen box's choice, the other way round
	float mMusicVolume;      // +0x354
	float mSfxVolume;        // +0x358
	bool mWasAccelerated;    // +0x35c: as it was when the window opened; nothing reads it
	bool mWasWindowed;       // +0x35d: likewise
	float mOldMusicVolume;   // +0x360: what Cancel puts back
	float mOldSfxVolume;     // +0x364
	int mSfxDragFrames;      // +0x36c: frames the sound slider has moved, a sample every 21
	bool mSfxDragging;       // +0x370: the sound slider moved on the last mouse-down update
	ObjBase* mSliders[OPTIONS_SLIDERS];      // +0x374: music, sounds
	ObjBase* mKnobs[OPTIONS_SLIDERS];        // +0x37c
	ObjBase* mCheckboxes[OPTIONS_CHECKBOXES]; // +0x384: 3D, full screen

	static int sLastSfxKnobX; // 0x601344: where the sound slider's knob was, in whole pixels

	Options();               // 0x453e30
	bool Update() override;  // 0x4541a0
	bool MouseDown(ObjBase* theObj, bool isDown) override; // 0x4541f0
	bool MouseUp(ObjBase* theObj) override;     // 0x4543c0

private:
	float KnobXAtMouse();
	void BuildSliders();     // 0x454740
	void BuildCheckboxes();  // 0x454900
};

} // namespace Sexy
