#include "Settings.h"
#include "SavedGame.h"
#include "Button.h"
#include "Host.h"
#include "ScreenLayout.h"

using namespace engine;

namespace
{

// 0x421d80: a DWORD value, theDefault when it cannot be read
int readInt(const char* theName, int theDefault)
{
	int aValue;
	if (!getHost()->registryReadInt(theName, aValue))
		return theDefault;
	return aValue;
}

// 0x422030
std::string readString(const char* theName, const std::string& theDefault)
{
	std::string aValue;
	if (!getHost()->registryReadString(theName, aValue))
		return theDefault;
	return aValue;
}

}

// 0x40a000
Settings::Settings()
	: mScreen(nullptr), mSoundBox(nullptr), mMusicBox(nullptr), mFullScreenBox(nullptr), mSaverEnabled(false),
	  mSoundEnabled(false), mMusicEnabled(false), mFullScreen(false)
{
	mDefaultScoreName = "";
	unsigned int i = 0;
	do
	{
		mSavedGames[i] = nullptr;
		i++;
	} while (i < 3);
	// (set in the same loop in the original)
	mShowPuzzleHelp = true;
	mShowSolitaireHelp = true;
	mShowActionHelp = true;
	mHighestLevels[0] = 11;
	mHighestLevels[2] = 10;
	mHighestLevels[1] = 15;
	mShowLink = false;
	mVisitURL = "NO URL";
	mILink = "";
}

// 0x409db0
bool Settings::isSoundEnabled()
{
	return mSoundBox->isChecked();
}

// 0x409dc0
bool Settings::isMusicEnabled()
{
	return mMusicBox->isChecked();
}

// 0x409dd0
bool Settings::isFullScreen()
{
	mFullScreenBox->setChecked(getHost()->isFullScreen());
	return mFullScreenBox->isChecked();
}

// 0x409d50
void Settings::setHighestLevel(int theMode, int theLevel)
{
	if (mHighestLevels[theMode] < theLevel)
		mHighestLevels[theMode] = theLevel;
}

// 0x409d70
void Settings::setSavedGame(int theMode, SavedGame* theGame)
{
	SavedGame* anOld = mSavedGames[theMode];
	if (anOld != nullptr)
	{
		delete anOld;
		mSavedGames[theMode] = theGame;
		return;
	}
	mSavedGames[theMode] = theGame;
}

// 0x409e00
SavedGame* Settings::newSavedGame(int theMode)
{
	SavedGame* anOld = mSavedGames[theMode];
	if (anOld != nullptr)
		delete anOld;
	SavedGame* aGame = new SavedGame(theMode);
	mSavedGames[theMode] = aGame;
	return aGame;
}

// 0x40a110
void Settings::setScreen(ScreenLayout* theScreen)
{
	mScreen = theScreen;
	mFullScreenBox = static_cast<CheckBox*>(mScreen->getNode("fullScreenBox"));
	mSoundBox = static_cast<CheckBox*>(mScreen->getNode("soundBox"));
	mMusicBox = static_cast<CheckBox*>(mScreen->getNode("musicBox"));
	mFullScreenBox->setChecked(mFullScreen);
	getHost()->setFullScreen(mFullScreen);
	mSoundBox->setChecked(mSoundEnabled);
	mMusicBox->setChecked(mMusicEnabled);
}

// 0x40a250: the saved games' strings are read and parsed here in the original (with 0x40c8f0's reader); they come
// with the save code
void Settings::load()
{
	mFullScreen = readInt("FullScreen", 1) != 0;
	mMusicEnabled = readInt("MusicEnabled", 1) != 0;
	mSoundEnabled = readInt("SoundEnabled", 1) != 0;
	mHighestLevels[0] = readInt("HighestPuzzleLevel", 11);
	mHighestLevels[2] = readInt("HighestActionLevel", 10);
	mHighestLevels[1] = readInt("HighestSolitaireLevel", 15);
	mDefaultScoreName = readString("DefaultScoreName", "");
	mShowLink = readInt("ShowLink", 1) != 0;
	mVisitURL = readString("VisitURL", "");
	mShowPuzzleHelp = readInt("ShowPuzzleHelp", 1) != 0;
	mShowActionHelp = readInt("ShowActionHelp", 1) != 0;
	mShowSolitaireHelp = readInt("ShowSolitaireHelp", 1) != 0;
	mILink = readString("ILink", "");
	mSaverEnabled = readInt("SaverEnabled", 0) != 0;
}

// 0x40aa10: the saved games are written here in the original (0x41e820, "" for none); they come with the save code
void Settings::save()
{
	Host* aHost = getHost();
	aHost->registryWriteInt("SoundEnabled", isSoundEnabled());
	aHost->registryWriteInt("MusicEnabled", isMusicEnabled());
	aHost->registryWriteInt("FullScreen", isFullScreen());
	aHost->registryWriteString("DefaultScoreName", mDefaultScoreName);
	aHost->registryWriteInt("ShowLink", mShowLink);
	aHost->registryWriteString("VisitURL", mVisitURL);
	aHost->registryWriteString("ILink", mILink);
	aHost->registryWriteInt("HighestPuzzleLevel", mHighestLevels[0]);
	aHost->registryWriteInt("HighestActionLevel", mHighestLevels[2]);
	aHost->registryWriteInt("HighestSolitaireLevel", mHighestLevels[1]);
	aHost->registryWriteInt("ShowPuzzleHelp", mShowPuzzleHelp);
	aHost->registryWriteInt("ShowActionHelp", mShowActionHelp);
	aHost->registryWriteInt("ShowSolitaireHelp", mShowSolitaireHelp);
	aHost->registryWriteInt("SaverEnabled", mSaverEnabled);
}
