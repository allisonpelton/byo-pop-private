// Sexy::TwistDialogListener (4 slots): the game's dialog listener, the framework's DialogListener with two more
// slots. Slot 3 hears a TwistDialog's third button (id 1011); slot 2, which no listener fills and nothing calls, is left
// out.
#pragma once

#include "DialogListener.h"

namespace Sexy {

class TwistDialogListener : public DialogListener {
public:
	virtual void DialogThirdButtonDepress(int theDialogId) {}
};

}  // namespace Sexy
