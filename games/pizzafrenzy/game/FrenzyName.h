#ifndef __PIZZAFRENZY_FRENZYNAME_H__
#define __PIZZAFRENZY_FRENZYNAME_H__

// The name of the frenzy a combo reaches, from the string table (0x457fc0, called by the topping book, 0x43f980 and
// 0x45a300).

#include <string>

namespace pizzafrenzy
{

// A combo of fewer than 5 has no name; each five up to 25 has its own, and a longer one has the last
std::string getFrenzyName(int theCombo);

} // namespace pizzafrenzy

#endif
