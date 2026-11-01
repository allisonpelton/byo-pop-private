#ifndef __BONNIES_DIALOG_H__
#define __BONNIES_DIALOG_H__

// The game's dialog (0x41bf10, the screen in Screens.h).

#include "Str.h"

namespace bonnies
{

// 0x41bf10: theText and up to four buttons on Bonnie's notepad (each with its key; a lone button takes Esc and
// Enter); with bit 0 of theFlags2 the registration's reminder. When theBlocking, the button pressed (1-4).
int gameDialog(int theBlocking, Str theText, Str theButton1, Str theButton2, Str theButton3, Str theButton4,
	int theKey1, int theKey2, int theKey3, int theKey4, int theFlags2);

// 0x41c410: gameDialog with the first button "OK" (text 3) when it has none
int messageDialog(int theBlocking, Str theText, Str theButton1, Str theButton2, Str theButton3, Str theButton4,
	int theKey1, int theKey2, int theKey3, int theKey4, int theFlags2 = 0);

} // namespace bonnies

#endif
