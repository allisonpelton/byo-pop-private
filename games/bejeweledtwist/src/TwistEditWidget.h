// The game's EditWidget (Sexy::EditWidget, 0x134 bytes; 0x4cb390-0x4cd660): SexyAppFramework's EditWidget with four
// fields more, which say where Draw puts the text, its clip and the cursor (the framework's Draw has fixed numbers
// there). Everything else is the framework's.
#pragma once

#include "EditWidget.h"

namespace Sexy {

class TwistEditWidget : public EditWidget {
public:
	int mClipInset = 4;       // +0xc8: the text's clip, in from the left and right
	int mTextX = 4;           // +0xcc: where the text starts
	int mCursorOffsetX = 0;   // +0xd0: added to the cursor's (and a selection's) x
	int mSelectionExtra = 0;  // +0xd4: added to a selection's width

	TwistEditWidget(int theId, EditListener* theEditListener);  // 0x4cb390
	void Draw(Graphics* g) override;                           // 0x4cb8f0
};

}  // namespace Sexy
