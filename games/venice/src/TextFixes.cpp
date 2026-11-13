#include "TextFixes.h"

#include "platform/GameOptions.h"

#include <cstring>

namespace
{
// Every misspelt text of the data files: the name box's request on a name the game cannot take (levels/invalid_profile_name.xml),
// the win and lose messages and titles of five trick shot levels and a bonus level (tricky_02_06, tricky_03_10, tricky_05_06,
// tricky_05_07, tricky_07_09 and bonus07_02)
struct Correction
{
	const char* mMisspelt;
	const char* mCorrect;
};

const Correction TextCorrections[] = {
	{"PLESE TYPE IN A", "PLEASE TYPE IN A"},
	{"SQUAKED IT THROUGH!", "SQUEAKED IT THROUGH!"},
	{"YOU GOT THE RYTHM!", "YOU GOT THE RHYTHM!"},
	{"GODE JUDGEMENT!", "GOOD JUDGEMENT!"},
	{"GET THAN ANGLE!", "GET THAT ANGLE!"},
	{"SWIRLD IN, SWIRL OUT", "SWIRLED IN, SWIRL OUT"},
	{"DONT LOOK AT ME", "DON'T LOOK AT ME"},
};
}

// port: the fixes mod's "typos" option; the original shows the texts as its data files spell them
const char* CorrectedText(const char* theText)
{
	if (theText == nullptr || !Platform::FixOn("typos"))
		return theText;
	for (const Correction& aCorrection : TextCorrections) {
		if (strcmp(theText, aCorrection.mMisspelt) == 0)
			return aCorrection.mCorrect;
	}
	return theText;
}
