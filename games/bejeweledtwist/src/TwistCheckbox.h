// Sexy::TwistCheckbox (0x108 bytes; 0x7546e6-0x7547bb): the options' check boxes and radio buttons. Checked is cel 0
// of the image, unchecked cel 1; it is drawn lit (additively, at half strength) once, and once more while the mouse
// is over it, unless it is pressed. A label goes to the right of it.
#pragma once

#include "Checkbox.h"

namespace Sexy {

class TwistCheckbox : public Checkbox {
public:
	SexyString mLabel;  // +0xec: never set by the game, so empty

	// 0x7546e6: either image null is IMAGE_MAINMENU_RADIOBUTTONCUSTOMCURSORS
	TwistCheckbox(Image* theUncheckedImage, Image* theCheckedImage, int theId, CheckboxListener* theListener);
	~TwistCheckbox() override;  // 0x7547bb

	void Draw(Graphics* g) override;  // 0x75457b
};

}  // namespace Sexy
