// The player's preferences (see Options.h).

#include "Options.h"
#include "File.h"
#include "Globals.h"
#include "Platform.h"

#include <cstring>

namespace bonnies
{

// main (0x403b80) does new, the Str's ctor, then load
Options::Options()
{
	load();
}

// 0x404140
Str Options::path()
{
	Str aPath = "";
	aPath += savedGamesDir(true);
	aPath += lit(0x457a98);
	return aPath;
}

// 0x404200
void Options::reset()
{
	mName = Str();
	mSoundVolume = 1.0f;
	mMusicVolume = 1.0f;
	mFullScreen = 0;
	if (gPortalConfig != nullptr)
		mFullScreen = gPortalConfig->mStartFullScreen;
	mWindowSpare = 0;
	mInterstitial = 0;
	mWindowX = 0x32;
	mWindowY = 0x46;
	mClientX = 0x32;
	mClientY = 0x46;
	memset(mBytes, 0, sizeof(mBytes));
}

// 0x404260
void Options::load()
{
	reset();
	int aFile = fileOpen(path(), false, false);
	if (aFile != -1)
	{
		int aVersion;
		fileReadInt(aFile, &aVersion);
		if (aVersion < 0x65)
		{
			fileReadString(aFile, mName);
			fileReadInt(aFile, &mSoundVolume);
			fileReadInt(aFile, &mMusicVolume);
			fileReadInt(aFile, &mFullScreen);
			fileReadInt(aFile, &mWindowX);
			fileReadInt(aFile, &mWindowY);
			fileReadInt(aFile, &mWindowSpare);
			fileReadInt(aFile, &mClientX);
			fileReadInt(aFile, &mClientY);
			fileReadInt(aFile, &mInterstitial);
		}
		fileClose(aFile);
	}
}

// 0x404320
void Options::save()
{
	int aFile = fileOpen(path(), true, false);
	if (aFile != -1)
	{
		int aVersion = 100;
		fileWriteInt(aFile, &aVersion);
		fileWriteString(aFile, &mName);
		fileWriteInt(aFile, &mSoundVolume);
		fileWriteInt(aFile, &mMusicVolume);
		fileWriteInt(aFile, &mFullScreen);
		fileWriteInt(aFile, &mWindowX);
		fileWriteInt(aFile, &mWindowY);
		fileWriteInt(aFile, &mWindowSpare);
		fileWriteInt(aFile, &mClientX);
		fileWriteInt(aFile, &mClientY);
		fileWriteInt(aFile, &mInterstitial);
		unsigned int i = 0;
		do
		{
			fileWriteByte(aFile, &mBytes[i]);
			i++;
		} while (i < 0x4c);
		fileClose(aFile);
	}
}

// 0x4043f0
int isVowel(char theChar)
{
	if ('@' < theChar && theChar < '[' &&
		(theChar == 'A' || theChar == 'E' || theChar == 'I' || theChar == 'O' || theChar == 'U'))
		return 1;
	return 0;
}

// 0x404420
int isConsonant(char theChar)
{
	if ('@' < theChar && theChar < '[' && theChar != 'A' && theChar != 'E' && theChar != 'I' && theChar != 'O' &&
		theChar != 'U')
		return 1;
	return 0;
}

} // namespace bonnies
