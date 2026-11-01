// The game's globals (see Globals.h).

#include "Globals.h"
#include "Font.h"

#include <map>
#include <memory>

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
	// the original's descriptions are static data of its own; the port makes each once and keeps it to the end
	static std::map<uint32_t, std::unique_ptr<FontDesc>> aFonts;
	std::unique_ptr<FontDesc>& aDesc = aFonts[theAddress];
	if (aDesc == nullptr)
	{
		aDesc.reset(new FontDesc());
		aDesc->mName = exeGlobalString(theAddress);
		aDesc->mImage = exeGlobalString(theAddress + 8);
		aDesc->mSpacing = exeFloat(theAddress + 0x10);
		aDesc->mLineGap = exeFloat(theAddress + 0x14);
		aDesc->mColonX = exeFloat(theAddress + 0x18);
		aDesc->mColonY = exeFloat(theAddress + 0x1c);
		aDesc->mSpaceWidth = exeFloat(theAddress + 0x20);
	}
	return aDesc.get();
}

} // namespace bonnies
