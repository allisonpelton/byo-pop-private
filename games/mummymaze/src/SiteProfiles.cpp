// port: Mummy Maze's part of the site's players on the web page (SiteProfiles.h; framework/WebProfiles.h). Web only.
//
// The adventure mode keeps five named saved adventures (adv0.sav to adv4.sav in the user dir, each starting with its name): the
// new-adventure dialog names one when a free slot is chosen, and the delete dialog removes one. They are the game's players. The
// game has no rename: renaming rewrites the name at the start of the file. The page's view of them is built from the game's own
// functions (AdventureName, CreateAdventure, the file helpers), and registers with the framework to run when the game has loaded.
// The score box of the classic modes (RecordScoreDialog, which remembers LastScoreName) is filled through its own rules and is not
// listed: the game reports its adventures.

#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#include "MummyApp.h"
#include "EditWidget.h"
#include "ImageFont.h"
#include "WebProfiles.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <stdio.h>
#include <string.h>

using namespace Sexy;

// NameDialog's box: mFont1, 20 characters (0x14); the OK accepts letters and numbers only (MMApp::ButtonDepress)
static const int kMaxChars = 0x14;

static std::string FileName(int theSlot)
{
	char aBuf[32];
	sprintf(aBuf, "adv%d.sav", theSlot);
	return aBuf;
}

// theName with only what the OK accepts, and the box's count
static std::string Accepted(const std::string& theName, Font* theFont)
{
	std::string aLetters;
	for (size_t i = 0; i < theName.size(); i++)
	{
		unsigned char c = (unsigned char) theName[i];
		if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))
			aLetters += (char) c;
	}
	return Byopop::SiteFitTyped(aLetters, theFont, kMaxChars, -1);
}

class Adventures : public Byopop::SiteProfiles
{
public:
	Adventures(MMApp* theApp) : mApp(theApp) {}

	virtual std::vector<std::string> Names()
	{
		std::vector<std::string> aNames;
		for (int i = 0; i < 5; i++)
		{
			SexyString aName;
			if (mApp->AdventureName(i, aName))
				aNames.push_back(aName);
		}
		return aNames;
	}

	virtual std::string Current()
	{
		SexyString aName;
		if ((mApp->mCurrentSlot >= 0) && (mApp->mCurrentSlot < 5) && mApp->AdventureName(mApp->mCurrentSlot, aName))
			return aName;
		return "";
	}

	virtual std::string Find(const std::string& theName)
	{
		int aSlot = SlotOf(theName);
		SexyString aName;
		return ((aSlot >= 0) && mApp->AdventureName(aSlot, aName)) ? aName : std::string();
	}

	virtual std::string Fit(const std::string& theName) { return Accepted(theName, mApp->mFont1); }

	// The new-adventure dialog's OK: CreateAdventure in the slot, which becomes the current one (the dialog then loads the map and
	// tells the story's first message, which is not done here: the adventure is there when the player chooses it from the slots)
	virtual bool Create(const std::string& theName, std::string* theWhy)
	{
		for (int i = 0; i < 5; i++)
		{
			if (!mApp->AdventureExists(i))
			{
				mApp->mCurrentSlot = i;
				mApp->CreateAdventure(i, theName);
				SiteProfilesChanged(mApp);
				return true;
			}
		}
		*theWhy = "Mummy Maze keeps at most 5 adventures.";
		return false;
	}

	virtual bool Select(const std::string& theName)
	{
		int aSlot = SlotOf(theName);
		if (aSlot < 0)
			return false;
		mApp->mCurrentSlot = aSlot;
		return true;
	}

	virtual bool CanRename() { return true; }

	// The name is the first string of the file ([length][bytes], then the progress): only it is replaced
	virtual bool Rename(const std::string& theFrom, const std::string& theTo, std::string* theWhy)
	{
		int aSlot = SlotOf(theFrom);
		if (aSlot < 0)
			return false;
		std::string aData;
		if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + FileName(aSlot), &aData) || (aData.size() < 4))
		{
			*theWhy = "The adventure could not be read.";
			return false;
		}
		int anOldLength;
		memcpy(&anOldLength, aData.data(), 4);
		if ((anOldLength < 0) || ((size_t) anOldLength + 4 > aData.size()))
		{
			*theWhy = "The adventure could not be read.";
			return false;
		}
		int aNewLength = (int) theTo.size();
		std::string aNewData((const char*) &aNewLength, 4);
		aNewData += theTo;
		aNewData += aData.substr(4 + anOldLength);
		if (!Platform::WriteUserFileAtomic(FileName(aSlot), aNewData))
		{
			*theWhy = "The adventure could not be written.";
			return false;
		}
		if (mApp->mCurrentSlot == aSlot)
			mApp->mAdvName = theTo;
		SiteProfilesChanged(mApp);
		return true;
	}

	virtual bool CanDelete() { return true; }

	// DeleteGame without its DoAdventure (which would open the slots dialog): the file goes
	virtual bool Delete(const std::string& theName, std::string* theWhy)
	{
		int aSlot = SlotOf(theName);
		if (aSlot < 0)
			return false;
		Platform::RemoveUserFile(FileName(aSlot));
		SiteProfilesChanged(mApp);
		return true;
	}

	virtual const char* Rules() { return "Letters and digits only (no spaces), up to 20 characters; not one another adventure has"; }

	virtual const char* DeleteNote() { return "the adventure: its map, the pyramids won and the time played"; }

	virtual int MaxPlayers() { return 5; }

private:
	// The slot of the adventure called that, whatever the case (the site's own rule: the game's OK compares the names
	// exactly, 0x41fff0, so Bob and bob can both be saved in it), or -1
	int SlotOf(const std::string& theName)
	{
		for (int i = 0; i < 5; i++)
		{
			SexyString aName;
			if (mApp->AdventureName(i, aName) && (strcasecmp(aName.c_str(), theName.c_str()) == 0))
				return i;
		}
		return -1;
	}

	MMApp*	mApp;
};

static void Start(SexyAppBase* theApp)
{
	Adventures anAdventures((MMApp*) theApp);
	Byopop::SiteStart("mummymaze", anAdventures);
}

// the framework's start point, when the game has loaded (framework/WebProfiles.h)
static struct Register
{
	Register() { Byopop::gSiteLoaded = Start; }
} sRegister;

void Sexy::SiteProfilesChanged(MMApp* theApp)
{
	Adventures anAdventures(theApp);
	Byopop::SiteChanged("mummymaze", anAdventures);
}

void Sexy::SiteFillAdventureName(EditWidget* theEdit)
{
	std::string aName = Byopop::SiteNameFor("mummymaze");
	if (aName.empty())
		return;
	aName = Accepted(aName, theEdit->mFont);
	if (!aName.empty())
		theEdit->SetText(aName, false);
}
#endif
