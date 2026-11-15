#ifndef __PIZZAFRENZY_SCREENNODES_H__
#define __PIZZAFRENZY_SCREENNODES_H__

// The port's changes to the nodes of a screen's layout, for the screens' parts that led out of the game (the original's
// links and its online scores), which the port removes: a node goes from its screen, and a button goes with its label.

#include "ScreenLayout.h"

#include <string>

namespace pizzafrenzy
{

// The node, with the nodes below it, is taken out of its parent and out of the screen's names; nothing when the
// screen has no such node
void removeScreenNode(engine::ScreenLayout* theScreen, const std::string& theName);

// The button, and every text of its parent that is at the button's position, move to theX (the same y)
void moveButtonWithLabels(engine::ScreenLayout* theScreen, const std::string& theButton, float theX);

// The text with every "%1" replaced by the insert and "%%" by "%": the exe's one insert of FormatMessageA
// (0x4bd850)
std::string formatMessage(const std::string& theText, const std::string& theInsert);

// The format with its one conversion (%s or %d) filled, as the exe's printf into a string (0x46db00) does
std::string formatText(const std::string& theFormat, const std::string& theText);
std::string formatText(const std::string& theFormat, int theNumber);

} // namespace pizzafrenzy

#endif
