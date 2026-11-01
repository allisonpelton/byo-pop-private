// The game's globals (see Globals.h).

#include "Globals.h"
#include "Font.h"

#include <map>

namespace bonnies
{

BonniesApp* gBonnies;
Options* gOptions;
Game* gGame;
Dictionary* gDictionary;
Music* gMusic;

Screen* gLogoScreen;
Screen* gMenuScreen;
GameScreen* gGameScreen;
Screen* gState4Screen;
Screen* gState5Screen;
Screen* gCreditsScreen;
Screen* gState7Screen;
Screen* gOverlayScreen;

FontDesc* exeFont(uint32_t theAddress)
{
	static std::map<uint32_t, FontDesc*> aFonts;
	FontDesc*& aDesc = aFonts[theAddress];
	if (aDesc == nullptr)
	{
		aDesc = new FontDesc();
		aDesc->mName = exeGlobalString(theAddress);
		aDesc->mImage = exeGlobalString(theAddress + 8);
		aDesc->mSpacing = exeFloat(theAddress + 0x10);
		aDesc->mLineGap = exeFloat(theAddress + 0x14);
		aDesc->mColonX = exeFloat(theAddress + 0x18);
		aDesc->mColonY = exeFloat(theAddress + 0x1c);
		aDesc->mSpaceWidth = exeFloat(theAddress + 0x20);
	}
	return aDesc;
}

} // namespace bonnies
