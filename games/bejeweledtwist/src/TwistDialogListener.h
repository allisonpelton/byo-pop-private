// Sexy::TwistDialogListener (4 slots): the game's dialog listener, the framework's DialogListener with two more
// slots (2 and 3, whose use is not yet known; every listener seen so far leaves them empty).
#pragma once

#include "DialogListener.h"

namespace Sexy {

class TwistDialogListener : public DialogListener {
public:
	virtual void DialogUnknown2() {}
	virtual void DialogUnknown3() {}
};

}  // namespace Sexy
