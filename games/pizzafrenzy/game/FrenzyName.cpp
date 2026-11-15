#include "FrenzyName.h"
#include "App.h"

using namespace pizzafrenzy;

namespace
{
// The string table's ids of the names, from the first (a combo of 5 to 9) on, and the one past the last
const int STRING_FIRST_FRENZY = 0xd3;
const int STRING_LONGEST_FRENZY = 0xd8;

// A frenzy comes with each this many in a combo
const int COMBO_PER_FRENZY = 5;
}

// 0x457fc0
std::string pizzafrenzy::getFrenzyName(int theCombo)
{
	int aFrenzy = theCombo / COMBO_PER_FRENZY;
	if (aFrenzy - 1 < 0)
		return "";
	int anId = STRING_LONGEST_FRENZY;
	if (aFrenzy >= 1 && aFrenzy <= 5)
		anId = STRING_FIRST_FRENZY + aFrenzy - 1;
	return engine::App::instance()->loadString(anId);
}
