// port: Cosmic Bugs's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.
//
// The game's players are the ten slots of gProfiles (cb.dat), each with a name (a free slot has the placeholder text Localize(GT(...))),
// the level reached and the game in progress; the eleventh slot keeps the difficulty. The menu chooses (a slot item), renames and
// deletes the current player, and a free slot asks for a name. This implements the page's view with the same fields and
// SaveProfiles, as those menu items do (CBMenu.cpp, the cases 0x1b to 0x1f and Level_SetUserName).

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "CBLevel.h"
#include "R64Sys.h"
#include "R64TextInput.h"
#include "WebProfiles.h"

#include <string.h>
#include <strings.h>

using namespace cb;

// The text input of a name (TextInput_Init with 0xc characters, case mode 1) takes digits, letters (as capitals), the space and the
// punctuation keys, unshifted and shifted, whose character the font has
static bool Typable(char c)
{
	if (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z')) || (c == ' '))
		return true;
	return strchr(",.-=[]\\;'/`)!@#$%^&*(<>_+{}|:\"?~", c) != NULL && (c != 0);
}

// theName as the text input takes it: capitals, at most theMax characters, then at most theWidth pixels (0: no limit)
static std::string Typed(const std::string& theName, int theMax, int theWidth, Font* theFont)
{
	std::string aResult;
	for (size_t i = 0; (i < theName.size()) && ((int) aResult.size() < theMax); i++)
	{
		char c = theName[i];
		if ((c >= 'a') && (c <= 'z'))
			c = (char) (c - 0x20);
		if (Typable(c) && ((theFont == NULL) || (Font_CharWidth(theFont, c) > 0)))
			aResult += c;
	}
	while ((theWidth > 0) && (theFont != NULL) && !aResult.empty() && (Font_StringWidth(theFont, aResult.c_str()) > theWidth))
		aResult.erase(aResult.size() - 1);
	return aResult;
}

static const char* Empty()
{
	return Localize(GT(0x458a84));
}

static bool Used(int theSlot)
{
	return strcmp(gProfiles[theSlot].mName, Empty()) != 0;
}

class Players : public Byopop::SiteProfiles
{
public:
	virtual std::vector<std::string> Names()
	{
		std::vector<std::string> aNames;
		for (int i = 0; i < 10; i++)
			if (Used(i))
				aNames.push_back(gProfiles[i].mName);
		return aNames;
	}

	virtual std::string Current() { return ((gProfile >= 0) && (gProfile < 10) && Used(gProfile)) ? gProfiles[gProfile].mName : ""; }

	virtual std::string Find(const std::string& theName)
	{
		int aSlot = SlotOf(theName);
		return (aSlot >= 0) ? std::string(gProfiles[aSlot].mName) : std::string();
	}

	virtual std::string Fit(const std::string& theName) { return Typed(theName, 0xc, 0, Font_Find("bonus_font")); }

	// A free slot's request for a name (case 0x1b) and its OK (Level_SetUserName): the slot is chosen and takes the name
	virtual bool Create(const std::string& theName, std::string* theWhy)
	{
		for (int i = 0; i < 10; i++)
		{
			if (!Used(i))
			{
				gProfile = i;
				strcpy(gProfiles[i].mName, theName.c_str());
				gAutoFire = gProfiles[i].mAutoFire;
				SaveProfiles();
				return true;
			}
		}
		*theWhy = "The game keeps at most 10 players.";
		return false;
	}

	// A slot of another player (case 0x1b)
	virtual bool Select(const std::string& theName)
	{
		int aSlot = SlotOf(theName);
		if (aSlot < 0)
			return false;
		gProfile = aSlot;
		gAutoFire = gProfiles[gProfile].mAutoFire;
		SaveProfiles();
		return true;
	}

	virtual bool CanRename() { return true; }

	// The rename's OK (Level_SetUserName): the slot takes the name and keeps its progress. The records the player holds keep the
	// name they were set under, as after the game's own rename.
	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy)
	{
		int aSlot = SlotOf(theFrom);
		if (aSlot < 0)
			return false;
		strcpy(gProfiles[aSlot].mName, theTo.c_str());
		SaveProfiles();
		return true;
	}

	virtual bool CanDelete() { return true; }

	// Case 0x1f: the slot is emptied (name, level reached, shooter mode) and the first player left is chosen; the game does not
	// delete the only player
	virtual bool Delete(const std::string& theName, std::string* theWhy)
	{
		int aSlot = SlotOf(theName);
		if (aSlot < 0)
			return false;
		if (Names().size() < 2)
		{
			*theWhy = "The game does not delete its only player.";
			return false;
		}
		gProfiles[aSlot].mAutoFire = 1;
		gProfiles[aSlot].mLevelReached = 0;
		strcpy(gProfiles[aSlot].mName, Empty());
		if (aSlot == gProfile)
		{
			for (int i = 0; i < 10; i++)
			{
				if (Used(i))
				{
					gProfile = i;
					gAutoFire = gProfiles[i].mAutoFire;
					break;
				}
			}
		}
		SaveProfiles();
		return true;
	}

	virtual const char* Rules()
	{
		return "Up to 12 characters, as capitals: letters, digits, spaces and punctuation the font has; ten players at most";
	}

	virtual const char* DeleteNote() { return "the player's name and the level reached (as the game's delete does; a saved game and the records stay)"; }

	virtual int MaxPlayers() { return 10; }

private:
	// The slot of the first player with that name, whatever the case, or -1
	int SlotOf(const std::string& theName)
	{
		for (int i = 0; i < 10; i++)
			if (Used(i) && (strcasecmp(gProfiles[i].mName, theName.c_str()) == 0))
				return i;
		return -1;
	}
};

void cb::SiteStart()
{
	Players aPlayers;
	Byopop::SiteStart("cosmicbugs", aPlayers);
}

void cb::SiteChanged()
{
	Players aPlayers;
	Byopop::SiteChanged("cosmicbugs", aPlayers);
}

void cb::SiteFillName(r64::TextInput* theInput)
{
	std::string aName = Byopop::SiteNameFor("cosmicbugs");
	if (aName.empty())
		return;
	aName = Typed(aName, 0xc, 0, Font_Find("bonus_font"));
	if (!aName.empty())
		TextInput_SetText(theInput, aName.c_str());
}

void cb::SiteFillHighScore(r64::TextInput* theInput, void* theFont)
{
	std::string aName = Byopop::SiteNameFor("cosmicbugs");
	if (aName.empty())
		return;
	aName = Typed(aName, 0x10, 0x1e0, (Font*) theFont);
	if (!aName.empty())
		TextInput_SetText(theInput, aName.c_str());
}

void cb::SiteFirstPlayer(char* theName, int theSize)
{
	std::string aName = Byopop::SiteNameFor("cosmicbugs");
	if (aName.empty())
		return;
	aName = Typed(aName, 0xc, 0, Font_Find("bonus_font"));
	if (!aName.empty() && ((int) aName.size() < theSize))
		strcpy(theName, aName.c_str());
}
#endif
