// TextFade: a text that swells in, holds and goes (0x46f5a0..0x470a60 in the original), the game's caption for the
// intro and the map. It is a line of text (ObjString) with three ways of playing, picked by SetMode before Start: mode
// 0 grows with a wobble, holds and shrinks away; modes 1 and 2 spin in and pulse, mode 2 for as long as it is not
// told to go (a click does that).
#pragma once

#include "ObjString.h"

namespace Sexy
{

const int TEXTFADE_MODES = 3;
const int TEXTFADE_MODE0_KEYS = 4;
const int TEXTFADE_MODE1_KEYS = 7;
const int TEXTFADE_MODE2_KEYS = 6;

class TextFade : public ObjString
{
public:
	int mExtraHold;          // +0x2a8: frames added to the next mode 0 Start's hold of 100
	AnimKey* mMode0Keys[TEXTFADE_MODE0_KEYS];     // +0x2ac (+0x2bc is never used)
	AnimKey* mMode1Keys[TEXTFADE_MODE1_KEYS];     // +0x2c0
	AnimKey* mMode2Keys[TEXTFADE_MODE2_KEYS];     // +0x2dc
	AnimTrack* mModeTracks[TEXTFADE_MODES];       // +0x2f4: the track each mode plays, kept to set its delay
	int mModeTrackIndex[TEXTFADE_MODES];          // +0x300: the track's index in the object
	int mQuickOutTrackIndex; // +0x30c: mode 2's short track out
	bool mClickSkips;        // +0x310: a click ends the text's keys early (nothing in this class sets it; SetMode clears it)
	bool mSkipping;          // +0x311: set by the click; Notify then ends each key as it starts
	int mMode;               // +0x314
	int mClickQuickOut;      // +0x318: a click starts the quick track out (SetMode sets it for mode 2)

	TextFade();              // 0x46f5a0
	~TextFade();             // 0x46f650
	void Notify(ObjBase* theObj) override;       // 0x46f680
	bool Update() override;                      // 0x46f6b0

	void Hide();             // 0x46f6f0
	void SetMode(int theMode);                   // 0x46f710
	void SetExtraHold(int theFrames);            // 0x46f730
	void Start(const char* theText, float theX, float theY, float theScale, float theRed, float theGreen,
		float theBlue, int theDelay);            // 0x46f740
	void BuildMode0Keys();   // 0x46f960
	void BuildMode1Keys();   // 0x46fd10
	void BuildMode2Keys();   // 0x470360
};

} // namespace Sexy
