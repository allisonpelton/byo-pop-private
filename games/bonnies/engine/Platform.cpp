// The platform layer and the platform object (see Platform.h).

#include "Platform.h"
#include "Events.h"
#include "File.h"
#include "FloatText.h"
#include "Graphics.h"
#include "Host.h"
#include "Input.h"
#include "Language.h"
#include "Resources.h"
#include "Sound.h"

namespace bonnies
{

Platform* gPlatform = nullptr;
int gQuit = 0;
uint32_t gLastActiveTime = 0;
PortalConfig* gPortalConfig;

namespace
{
// 0x481ac0: made at start (0x42f5b0) and freed at the end (0x42f600), never used
struct Unused
{
	int mField00 = 0;
	int mField04 = 0;
	Str mStr08;
	Str mStr10;
};
Unused* gUnused = nullptr;
bool gPrintScreenDown = false;  // 0x4809cc
}

// 0x41f9d0
int systemMessageBox(const Str& theText, const Str& theCaption, unsigned int theFlags)
{
	unsigned int aType;
	if ((theFlags & 1) == 0)
	{
		aType = theFlags;
		if ((theFlags & 4) != 0)
			aType = 4 - ((theFlags & 2) != 0);
	}
	else
		aType = theFlags >> 1 & 1;
	int aResult = host()->messageBox(theText, theCaption, aType);
	if (aResult == 1)
		return 1;
	if (aResult == 2)
		return 2;
	if (aResult == 6)
		return 4;
	if (aResult == 7)
		return 8;
	return 0;
}

// 0x41fb20. The menu handle 0x4809c4 is never set, so the window never has a menu; windowed, the original then
// refreshes the window's frame (SetWindowPos with SWP_NOMOVE | SWP_NOSIZE), which changes nothing here.
void updateWindowMenu()
{
}

// 0x41fbb0
void checkMenuItem(unsigned int theId, int theChecked)
{
	(void)theId;
	(void)theChecked;
}

// 0x41fbd0
void moveWindow(int theX, int theY)
{
	host()->moveWindow(theX, theY);
}

// 0x41fc10
void moveWindowDefault()
{
	host()->moveWindow(0x32, 100);
}

// 0x41fc50
void getDesktop(int* theWidth, int* theHeight, int* theBits)
{
	*theWidth = 0x400;
	*theHeight = 0x300;
	*theBits = 0x20;
	host()->getDesktop(theWidth, theHeight, theBits);
}

// 0x41fcd0
void setClipboard(const Str& theText)
{
	if (!theText.empty())
		host()->setClipboard(theText);
}

// 0x41fdf0
Str getClipboard()
{
	return host()->getClipboard();
}

// 0x41fee0
void checkPrintScreen()
{
	bool aDown = host()->getKeyState()[SDLK_PRINT] != 0;
	if (gPrintScreenDown && !aDown)
	{
		gEvents->add(8, 0x13c, 0x13c);
		gEvents->add(9, 0x13c, 0x13c);
	}
	gPrintScreenDown = aDown;
}

// 0x41ff40
int getCursorPos(int* theX, int* theY)
{
	if (isFullScreen())
	{
		int aX;
		int aY;
		if (host()->getCursorPos(&aX, &aY))
		{
			int aLeft;
			int aTop;
			if (!isFullScreen())
				gPlatform->getWindowPos2(&aLeft, &aTop);
			else
			{
				aTop = 0;
				aLeft = 0;
			}
			*theX = aX - aLeft;
			*theY = aY - aTop;
			return 1;
		}
	}
	return 0;
}

// 0x41ffc0. One copy runs in a browser tab or a native window, so no other copy is ever found.
int checkRunning(const Str& theName, const Str& theText, const Str& theCaption)
{
	(void)theName;
	(void)theText;
	(void)theCaption;
	return 0;
}

// 0x4200e0
void closeRunning()
{
}

// 0x420100
int isIconic()
{
	return host()->isIconic() ? 1 : 0;
}

// 0x420160
Str userName()
{
	return host()->userName();
}

// 0x420230
int doubleClickTime()
{
	return ftol((float)host()->doubleClickTime() * 1.2f);
}

// 0x420260
int doubleClickDistance()
{
	return 0x14;
}

// 0x420270
Str savedGamesDir(bool theSlash)
{
	Str aDir = "";
	aDir += "SavedGames";
	if (theSlash)
		aDir += "/";
	return aDir;
}

// 0x420320
Str soundDir(bool theSlash)
{
	Str aDir = "";
	aDir += "Sound";
	if (theSlash)
		aDir += "/";
	return aDir;
}

// 0x4203d0
Str dataDir(bool theSlash)
{
	Str aDir = "";
	aDir += "Data";
	if (theSlash)
		aDir += "/";
	return aDir;
}

// 0x420480
Str graphicsDir(bool theSlash)
{
	Str aDir = "";
	aDir += "Graphics";
	if (theSlash)
		aDir += "/";
	return aDir;
}

// 0x420530
int isInactive()
{
	if ((host()->getAppState() & SDL_APPACTIVE) == 0 || isIconic() != 0)
		return 1;
	if (gGraphics != nullptr && !isFullScreen() && !hasInputFocus())
		return 1;
	return 0;
}

// 0x420580 (GetVersion's major version 6 or more)
bool isVista()
{
	return true;
}

// 0x420590
int isLowColour()
{
	if (gPlatform != nullptr && gPlatform->mDesktopBits < 9)
		return 1;
	return 0;
}

// 0x4205c0
void openURL(const Str& theURL)
{
	if (!theURL.empty())
		host()->openURL(theURL);
}

// 0x420760
void pumpEvents()
{
	host()->pumpEvents();
}

// 0x420770
void waitUntil(int theTime, int theInactive)
{
	int aNow = (int)host()->getTicks();
	do
	{
		if (theTime <= aNow)
			return;
		host()->pumpEvents();
		aNow = (int)host()->getTicks();
		int aWait = theTime - aNow;
		if (1999 < aWait)
			aWait = 2000;
		if (theInactive != 0 && 199 < aWait)
			aWait = 200;
		if (0 < aWait - 3)
		{
			host()->sleep(aWait - 3);
			if (theInactive == 0)
				return;
			host()->pumpEvents();
			if ((host()->getAppState() & SDL_APPACTIVE) != 0 && isIconic() == 0)
			{
				if (gGraphics == nullptr)
					return;
				if (isFullScreen())
					return;
				if (hasInputFocus())
					return;
			}
		}
		aNow = (int)host()->getTicks();
	} while (true);
}

// 0x41f4c0
void noteActiveTime()
{
	if (isInactive() == 0)
		gLastActiveTime = host()->getTicks();
}

// 0x420830
PortalConfig::PortalConfig()
{
	mStartFullScreen = 0;
	mQuote = 1;
	mDistributorLogoFirst = 0;
}

// 0x420860
PortalConfig::~PortalConfig()
{
}

// 0x41f560
void PortalConfig::unload()
{
}

// 0x420870
int readConfigValue(const Str& theText, int* thePos, const Str& theKey, Str* theValue, int* theBool)
{
	int aKeyLen = (int)theKey.size();
	int anEnd = aKeyLen + *thePos;
	bool aMatch = false;
	if (anEnd <= (int)theText.size())
		aMatch = equalsNoCase(theKey, substr(theText, *thePos, anEnd));
	if (!aMatch)
		return 0;
	int aLen = (int)theText.size();
	*thePos = *thePos + aKeyLen;
	if (*thePos < aLen)
	{
		while (*thePos < aLen)
		{
			unsigned char c = (unsigned char)theText[*thePos];
			if (0x20 < c && c != '=')
				break;
			if (c == '\n' || c == '\r')
				break;
			*thePos = *thePos + 1;
		}
	}
	int aStart = *thePos;
	if (aLen <= aStart || 0x20 < (unsigned char)theText[aStart])
		*thePos = aStart + 1;
	if (*thePos < aLen)
	{
		do
		{
			unsigned char c = (unsigned char)theText[*thePos];
			if (c < 0x20 || c == '\n' || c == '\r')
				break;
			*thePos = *thePos + 1;
		} while (*thePos < aLen);
	}
	int aStop = *thePos;
	if (aStart < aStop)
		*theValue = substr(theText, aStart, aStop);
	if (aStart <= aStop)
	{
		switch (at(*theValue, 0))
		{
		case '0':
		case 'F':
		case 'N':
		case 'f':
		case 'n':
			*theBool = 0;
			break;
		default:
			*theBool = 1;
		}
		return 1;
	}
	return 0;
}

// 0x420ad0
void PortalConfig::load()
{
	mStartFullScreen = 0;
	mQuote = 1;
	Str aPath = "";
	aPath = dataDir(true);
	aPath += "Portal.cfg";
	int aHandle = fileOpen(aPath, false, false);
	if (aHandle != -1)
	{
		Str aText = fileReadText(aHandle, 100000);
		int aLen = (int)aText.size();
		int aCount = 0;
		int aPos = 0;
		if (0 < aLen)
		{
			do
			{
				int aTry = aCount;
				aCount = aCount + 1;
				if (5000 <= aTry || aLen <= aPos)
					break;
				bool anEnd = false;
				while (aPos < (int)aText.size() && (unsigned char)aText[aPos] <= 0x20)
				{
					aTry = aCount;
					aCount = aCount + 1;
					if (5000 <= aTry)
						break;
					aPos = aPos + 1;
					if (aLen <= aPos)
					{
						anEnd = true;
						break;
					}
				}
				if (anEnd || aLen <= aPos)
					break;
				Str aValue = "";
				int aBool = 0;
				if (readConfigValue(aText, &aPos, "StartFullScreen", &aValue, &aBool))
					mStartFullScreen = aBool;
				else if (readConfigValue(aText, &aPos, "Quote", &aValue, &aBool))
					mQuote = aBool;
				else if (readConfigValue(aText, &aPos, "DistributorLogoFirst", &aValue, &aBool))
					mDistributorLogoFirst = aBool;
				else if (readConfigValue(aText, &aPos, "BuyNowURL", &aValue, &aBool))
					mBuyNowURL = aValue;
				else
					aPos = aPos + 1;
			} while (aPos < aLen);
		}
		fileClose(aHandle);
	}
}

// 0x42a4e0
Platform::Platform()
{
}

// 0x42a4f0
Platform::~Platform()
{
}

// 0x42a690
void Platform::init(int theWidth, int theHeight, int theFullScreen)
{
	host()->trace("PL_BaseInit 1");
	gPlatform = this;
	mField38 = 0;
	gQuit = 0;
	mShowFPS = 0;
	mFullRedraw = 0;
	mFlashRects = 0;
	mDebugTexts = 0;
	mDebugScreens = 0;
	mFrameLimit = 0;
	mSkipDraws = 0;
	mField1C = 1;
	mActiveTime = host()->getTicks();
	mInactiveTime = 0;
	host()->trace("PL_BaseInit 2");
	loadLanguage();
	host()->trace("PL_BaseInit 3");
	getDesktop(&mDesktopWidth, &mDesktopHeight, &mDesktopBits);
	host()->trace("PL_BaseInit 4");
	buildAngleTable();
	seedRandom(0);
	host()->trace("PL_BaseInit 5");
	gUnused = new Unused();
	host()->trace("PL_BaseInit 6");
	gPak = new Pak();
	gPak->open();
	host()->trace("PL_BaseInit 7");
	gResources = new Resources();
	host()->trace("PL_BaseInit 8");
	gFloatTexts = new FloatTexts();
	host()->trace("PL_BaseInit 9");
	gEvents = new EventQueue();
	host()->trace("PL_BaseInit a");
	gInput = new Input();
	host()->trace("PL_BaseInit b");
	gGraphics = new Graphics(theWidth, theHeight, theFullScreen);
	gGraphics->setCursor(gArrowCursor);
	gSound = new SoundSystem();
	// SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE), SDL_EnableUNICODE(1), SDL_EnableKeyRepeat(500, 45)
	host()->enableKeyRepeat(500, 0x2d);
}

// 0x42ade0
void Platform::shutdown()
{
	// The original releases the resource cache after it has deleted the sound and the graphics, so the cache's free
	// (0x421b30) skips every image and sample still in it and they leak; the port releases it first (DEVIATIONS.md)
	gResources->releaseAll();
	gSound->shutdown();
	delete gSound;
	gSound = nullptr;
	gGraphics->shutdown();
	delete gGraphics;
	gGraphics = nullptr;
	delete gInput;
	gInput = nullptr;
	delete gEvents;
	gEvents = nullptr;
	gFloatTexts->clear();
	delete gFloatTexts;
	gFloatTexts = nullptr;
	delete gResources;
	gResources = nullptr;
	gPak->close();
	delete gPak;
	gPak = nullptr;
	delete gUnused;
	gUnused = nullptr;
	freeLanguage();
}

// 0x42a500
void Platform::draw()
{
	noteActiveTime();
}

// 0x42a510
void Platform::update()
{
	checkPrintScreen();
	gEvents->pump();
	gResources->update();
	gFloatTexts->update();
	gSound->update();
	if (isInactive() != 0)
	{
		mInactiveTime = host()->getTicks();
		return;
	}
	mActiveTime = host()->getTicks();
}

// 0x42a570
int Platform::handleEvent(Event* theEvent)
{
	(void)theEvent;
	return 1;
}

// 0x42a580
int Platform::messageBox(int theFlags, const Str& theText1, const Str& theText2, const Str& theText3,
	const Str& theText4, const Str& theText5)
{
	(void)theFlags;
	(void)theText2;
	(void)theText3;
	(void)theText4;
	(void)theText5;
	systemMessageBox(theText1, "", 1);
	return 1;
}

// 0x42a4d0
int Platform::isPaused()
{
	return 0;
}

// 0x4217d0
bool Platform::isRunning()
{
	return isPaused() == 0;
}

// 0x4217e0
float Platform::getSoundVolume()
{
	return 1.0f;
}

// 0x4217e0
float Platform::getMusicVolume()
{
	return 1.0f;
}

// 0x4217f0
void Platform::windowMoved(int theX, int theY, int theClientX, int theClientY)
{
	(void)theX;
	(void)theY;
	(void)theClientX;
	(void)theClientY;
}

// 0x401050
void Platform::getWindowPos(int* theX, int* theY)
{
	*theX = 0x1e;
	*theY = 0x32;
}

// 0x401050
void Platform::getWindowPos2(int* theX, int* theY)
{
	*theX = 0x1e;
	*theY = 0x32;
}

// 0x42a4d0
int Platform::isIdle()
{
	return 0;
}

// 0x421800
void Platform::menuCommand(int theCommand)
{
	(void)theCommand;
}

// 0x42a4d0
int Platform::isModal()
{
	return 0;
}

// 0x41f560
void Platform::beginScreen()
{
}

// 0x41f560
void Platform::endScreen()
{
}

} // namespace bonnies
