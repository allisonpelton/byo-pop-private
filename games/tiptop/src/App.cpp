// The application object (TipTop.exe 0x4197c6-0x426fc7). See App.h.

#include "App.h"
#include "Board.h"
#include "Config.h"
#include "Game.h"
#include "GameText.h"
#include "Gui.h"
#include "Music.h"
#include "Res.h"
#include "SexyAppBase.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <SDL.h>

namespace tiptop
{

App gApp;
uint8_t**& gParticleState = gApp.mParticleImages;
Font* gTextFont = nullptr;  // 0x489b4c image 0x50, the font of the dialogs' texts

namespace
{

// "Not enough memory..." where an image is missing
void CheckLoaded(char theFailed)
{
	if (theFailed != 0)
		NoMemory();
}

void* Allocate(size_t theSize)
{
	void* aBlock = malloc(theSize);
	if (aBlock == nullptr)
		NoMemory();
	return aBlock;
}

} // namespace

// The app's handler procedures (0x4213f3-0x425d28)
uint32_t ScoreMessageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);      // 0x4219c6
uint32_t ZoomImageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);         // 0x421d20
uint32_t SlideImageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);        // 0x421f9f
uint32_t LevelStartProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);        // 0x42217f
uint32_t MenuHighScoresProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);    // 0x42233b
uint32_t HighScoresProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);        // 0x4225a4
uint32_t SummaryHighScoresProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler); // 0x422866
uint32_t MenuAnimationProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);     // 0x422afd
uint32_t MusicStarterProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);      // 0x422cf8
uint32_t PauseHolderProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);       // 0x422d6c
uint32_t ComboAnimProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);         // 0x422daf
uint32_t ComboStarProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);         // 0x422f33
uint32_t LoadBarProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);           // 0x4233c2
uint32_t SummaryStatsProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);      // 0x4236d9
uint32_t BonusTimerProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);        // 0x423bdd
uint32_t PopImageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);          // 0x423fc7
uint32_t BonusEndProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);          // 0x424421
uint32_t LevelCompleteProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);     // 0x4246b1
uint32_t ParticleProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);          // 0x424963
uint32_t SummaryItemProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);       // 0x424e66
uint32_t PauseKeyProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);          // 0x425987
uint32_t ColumnCursorProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);      // 0x4259b7
uint32_t GameOverDelayProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);     // 0x425c93
uint32_t MusicPauseProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);        // 0x425d28
uint32_t PagerProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler);             // 0x4213f3
uint32_t TipsDialog();  // 0x420ea9

// 0x4197c6: the images and fonts the app keeps
int App::LoadImages()
{
	CheckLoaded(LoadSurface(0x26, 1, &mBackground));
	CheckLoaded(LoadSurface(0x32, 9, &mBarFlash));
	CheckLoaded(LoadSurface(0x35, 1, &mBarLow));
	CheckLoaded(LoadSurface(0x34, 1, &mBarHigh));
	CheckLoaded(LoadSurface(0x33, 1, &mBarFrame));
	CheckLoaded(LoadSurface(0x44, 1, &mSkin));
	CheckLoaded(LoadSurface(0x68, 9, &mImage68));
	mTextFont = LoadFont(0x67, GTC(0x0046ed08), 8, 0, 0);
	gTextFont = LoadFont(0x50, GTC(0x0046eca8), 4, 0, 0x40);
	BuildImages();
	return 1;
}

// 0x419991: the images made in the display's format: the particles, the progress bar, the score texts and the
// board's column buffers (again after a switch of the screen mode)
void App::BuildImages()
{
	uint32_t aBpp = lowos::GetDisplayFormat(nullptr, nullptr, nullptr);
	if (mBarBits != nullptr)
		free(mBarBits);
	for (uint32_t i = 0; i < gConfig.mParticleCount; i = i + 1)
	{
		if (mParticleImages[i] != nullptr)
			free(mParticleImages[i]);
		const Particle& aParticle = gConfig.mParticles[i];
		mParticleImages[i] = (uint8_t*)malloc(aParticle.mValue0 * aParticle.mValue0 * (aBpp + (uint8_t)aParticle.mValue7));
		if (mParticleImages[i] == nullptr)
			NoMemory();
	}
	for (int i = 0; i < 4; i++)
	{
		if (mTexts[i].mBits != nullptr)
			free(mTexts[i].mBits);
	}
	if (mBoard->mGlowBuffer != nullptr)
		free(mBoard->mGlowBuffer);
	if (mBoard->mGlowBufferA != nullptr)
		free(mBoard->mGlowBufferA);
	if (mBoard->mGlowBufferB != nullptr)
		free(mBoard->mGlowBufferB);
	if (mBoard->mGlowAlpha != nullptr)
		free(mBoard->mGlowAlpha);
	mBarBits = (uint8_t*)malloc((lowos::SurfaceWidth(mBarFrame) & 0xffff) * lowos::SurfaceHeight(mBarFrame) * aBpp);
	mBarDrawn = 0xffffffff;
	mBarFlashDrawn = 1;
	if (mBarBits == nullptr)
		NoMemory();
	lowos::PixelFormat aDisplay, aLogical;
	lowos::GetDisplayFormat(&aDisplay, nullptr, nullptr);
	lowos::GetLogicalFormat(&aLogical, nullptr, nullptr);
	for (uint32_t p = 0; p < gConfig.mParticleCount; p = p + 1)
	{
		const Particle& aParticle = gConfig.mParticles[p];
		uint8_t* aDest = mParticleImages[p];
		uint32_t aSize = (uint32_t)aParticle.mValue0;
		for (uint32_t y = 0; y < aSize; y = y + 1)
		{
			for (uint32_t x = 0; x < aSize; x = x + 1)
			{
				// the brightness: 1 at the center to 0 at the radius, scaled and curved
				double dx = (double)(int)((aSize >> 1) - x);
				double dy = (double)(int)((aSize >> 1) - y);
				double anIntensity = (dx * dx + dy * dy) / (double)((aSize >> 1) * (aSize >> 1));
				if (anIntensity <= 1.0)
					anIntensity = 1.0 - anIntensity;
				else
					anIntensity = 0.0;
				anIntensity = ((double)(uint8_t)aParticle.mValue8 * anIntensity) / 255.0;
				anIntensity = pow(anIntensity, (double)aParticle.mValue1);
				if (aParticle.mValue7 == 1)
				{
					double anAlpha = (anIntensity * 256.0 < 0.0) ? 0.0 : anIntensity * 256.0;
					if (!(anAlpha < 255.0))
						anAlpha = 255.0;
					*aDest = (uint8_t)(int64_t)anAlpha;
					aDest = aDest + 1;
				}
				// the color: mColor2 to mColor1 towards the highlight at (mValue5, mValue6)
				dx = (double)(int)(aParticle.mValue5 - x);
				dy = (double)(int)(aParticle.mValue6 - y);
				double aDistance = (dx * dx + dy * dy) / (double)((aSize >> 1) * (aSize >> 1));
				double aMix = (0.0 <= 1.0 - aDistance) ? 1.0 - aDistance : 0.0;
				if (1.0 <= aMix)
					aMix = 1.0;
				else
					aMix = (0.0 <= 1.0 - aDistance) ? 1.0 - aDistance : 0.0;
				aMix = pow(aMix, (double)aParticle.mValue2);
				uint32_t aColor = 0;
				for (int aShift = 0; aShift < 0x18; aShift += 8)
				{
					uint32_t aFrom = (aParticle.mColor2 >> aShift) & 0xff;
					uint32_t aTo = (aParticle.mColor1 >> aShift) & 0xff;
					// the difference is unsigned: a darker mColor1 wraps around and saturates
					uint32_t aChannel = (uint32_t)(int64_t)((double)aFrom + (double)(uint32_t)(aTo - aFrom) * aMix);
					if (!(aChannel < 0xff))
						aChannel = 0xff;
					aColor = aColor | aChannel << aShift;
				}
				if (aParticle.mValue7 == 0)
				{
					uint32_t aLit = 0;
					for (int aShift = 0; aShift < 0x18; aShift += 8)
					{
						uint32_t aChannel = (uint32_t)(int64_t)((double)((aColor >> aShift) & 0xff) * anIntensity);
						if (!(aChannel < 0xff))
							aChannel = 0xff;
						aLit = aLit | aChannel << aShift;
					}
					aColor = lowos::ConvertColor(&aLogical, &aDisplay, aLit);
				}
				else
				{
					aColor = lowos::ConvertColor(&aLogical, &aDisplay, aColor);
				}
				if (aBpp == 1)
				{
					*aDest = (uint8_t)aColor;
					aDest = aDest + 1;
				}
				else if (aBpp == 2)
				{
					memcpy(aDest, &aColor, 2);
					aDest = aDest + 2;
				}
				else if (aBpp == 3)
				{
					memcpy(aDest, &aColor, 2);
					aDest[2] = (uint8_t)(aColor >> 0x10);
					aDest = aDest + 3;
				}
				else if (aBpp == 4)
				{
					memcpy(aDest, &aColor, 4);
					aDest = aDest + 4;
				}
			}
		}
	}

	// the texts' images, as wide as their widest values: the font's glyphs A..J are labels and frames
	char aText[12];
	GuiRect anExtent;
	int16_t aWidest = 0;
	char aWidestDigit = '0';
	for (char c = '0'; c < ':'; c = c + 1)
	{
		aText[0] = c;
		aText[1] = 0;
		DrawText(mTextFont, nullptr, 0, aText, nullptr, &anExtent, 0, 0, 0xff, 2);
		if ((int)aWidest < (int)(uint16_t)anExtent.w)
		{
			aWidest = anExtent.w;
			aWidestDigit = c;
		}
	}
	aText[0] = 'A';
	for (int i = 1; i < 0xb; i = i + 1)
		aText[i] = aWidestDigit;
	aText[11] = 0;
	DrawText(mTextFont, nullptr, 0, aText, nullptr, &anExtent, 0, 0, 0xff, 2);
	MakeTextImage(&mTexts[0], (uint16_t)anExtent.w, (uint16_t)anExtent.h, aBpp);

	aText[0] = 'B';
	for (int i = 1; i < 8; i = i + 1)
		aText[i] = aWidestDigit;
	aText[2] = ':';
	aText[5] = ':';
	aText[8] = 0;
	DrawText(mTextFont, nullptr, 0, aText, nullptr, &anExtent, 0, 0, 0xff, 2);
	MakeTextImage(&mTexts[1], (uint16_t)anExtent.w, (uint16_t)anExtent.h, aBpp);

	aText[0] = 'C';
	aText[1] = 'D';
	aText[2] = 0;
	DrawText(mTextFont, nullptr, 0, aText, nullptr, &anExtent, 0, 0, 0xff, 2);
	aWidest = anExtent.w;
	for (char c = 'E'; c <= 'G'; c = c + 1)
	{
		aText[1] = c;
		DrawText(mTextFont, nullptr, 0, aText, nullptr, &anExtent, 0, 0, 0xff, 2);
		if ((int)aWidest < (int)(uint16_t)anExtent.w)
			aWidest = anExtent.w;
	}
	aText[2] = aWidestDigit;
	aText[3] = aWidestDigit;
	aText[4] = 'H';
	aText[5] = 0;
	DrawText(mTextFont, nullptr, 0, aText, nullptr, &anExtent, 0, 0, 0xff, 2);
	if ((int)aWidest < (int)(uint16_t)anExtent.w)
		aWidest = anExtent.w;
	MakeTextImage(&mTexts[2], (uint32_t)(int)aWidest, (uint16_t)anExtent.h, aBpp);

	aText[0] = 'I';
	for (int i = 1; i < 4; i = i + 1)
		aText[i] = aWidestDigit;
	aText[4] = 'J';
	aText[5] = 0;
	DrawText(mTextFont, nullptr, 0, aText, nullptr, &anExtent, 0, 0, 0xff, 2);
	MakeTextImage(&mTexts[3], (uint16_t)anExtent.w, (uint16_t)anExtent.h, aBpp);

	// the board's buffers for the glowing column backgrounds
	uint16_t aWidth = 0;
	for (uint32_t i = 0; i < gConfig.mNumColumns; i = i + 1)
	{
		if ((uint32_t)aWidth < (uint32_t)(gConfig.mBkgcoldx[i] + gConfig.mBkgcolShrink * -2))
			aWidth = (uint16_t)(gConfig.mBkgcoldx[i] + gConfig.mBkgcolShrink * -2);
	}
	aWidth = aWidth + (uint16_t)gConfig.mBkgcolGlowradius * 2;
	uint16_t aHeight = (uint16_t)((uint16_t)gConfig.mNumRows * (uint16_t)gConfig.mBlockHeight +
		(uint16_t)gConfig.mBkgcolyoffset * -2 + (uint16_t)gConfig.mBkgcolGlowradius * 2);
	mBoard->mGlowBuffer = (uint8_t*)Allocate((uint32_t)aWidth * aHeight * aBpp);
	mBoard->mGlowBufferA = (uint8_t*)Allocate((uint32_t)aWidth * aHeight * aBpp);
	mBoard->mGlowBufferB = (uint8_t*)Allocate((uint32_t)aWidth * aHeight * aBpp);
	mBoard->mGlowPitch = aWidth * aBpp;
	mBoard->mGlowValidA = 0;
	mBoard->mGlowValidB = 0;
	mBoard->mGlowAlpha = (uint8_t*)Allocate((uint32_t)aWidth * aHeight * (aBpp + 1));
	mBoard->mGlowAlphaPitch = (uint32_t)aWidth * (aBpp + 1);
	memset(mBoard->mGlowAlpha, 0, (uint32_t)aWidth * aHeight * (aBpp + 1));
}

// A part of 0x419991: a text image of theWidth x theHeight in the display's format
void App::MakeTextImage(TextImage* theImage, uint32_t theWidth, uint32_t theHeight, uint32_t theBpp)
{
	theImage->mBits = (uint8_t*)Allocate(theWidth * theHeight * theBpp);
	theImage->mPitch = theWidth * theBpp;
	theImage->mDrawn = 0;
	theImage->mSurface = lowos::RegisterSurface(theImage->mBits, theImage->mPitch, theHeight, 0x29);
	if (theImage->mSurface == nullptr)
		NoMemory();
}

namespace
{

char gRestoreFullScreen = 0;  // 0x488706 full screen to restore when the window is active again

// Plays theSound with mode theMode (0 stops it, 1 plays it, 2 loops it)
void Play(lowos::Sound* theSound, char theMode)
{
	if (theSound != nullptr)
		lowos::PlaySound(theSound, (uint32_t)gSoundVolume << 0x10, theMode);
}

// The five sounds of the falling blocks (0x4882cc..) and the alarm stop at the end of a level
void StopLevelSounds()
{
	for (int i = 0; i < 5; i = i + 1)
	{
		if (gSounds[0x27 + i] != nullptr)
			lowos::PlaySound(gSounds[0x27 + i], (uint32_t)gSoundVolume << 0x10, 0);
	}
	if (gSounds[0x2c] != nullptr)
		lowos::PlaySound(gSounds[0x2c], (uint32_t)gSoundVolume << 0x10, 0);
}

GuiRect At(int x, int y)
{
	GuiRect aRect = {(int16_t)x, (int16_t)y, 0, 0};
	return aRect;
}

} // namespace

// 0x41b2b8: the window's close box
static void OnClose()
{
	if (gApp.mScreen == SCREEN_NONE)
		gQuit = 1;
	else
		Command(nullptr, 0x18);
}

// 0x41b2da: the window deactivated (isInactive 1: pauses the game and the music, flag 2) or activated (0)
static void OnActivate(char isInactive)
{
	SetPause(isInactive, 0);
	PauseMusic(isInactive);
	if (isInactive == 0)
		gPauseFlags = gPauseFlags & 0xfd;
	else
		gPauseFlags = gPauseFlags | 2;
	App::UpdatePause();
	gSkipMouseDown = 1;
	if ((gRestoreFullScreen != 0) && (isInactive == 0))
	{
		gRestoreFullScreen = 0;
		App::ToggleFullScreen(1);
	}
}

// 0x41b34b: the surfaces were lost (a mode switch): load the images again and rebuild the app's own
static uint32_t OnRestore()
{
	if (ReloadSurfaces() == 0)
	{
		gApp.BuildImages();
		return 0;
	}
	return 1;
}

// 0x41b36c: 0 toggles full screen, 1 sets it, 2 leaves it
void App::ToggleFullScreen(char theMode)
{
	if (theMode == 0)
		gFullScreen = gFullScreen ^ 1;
	else
		gFullScreen = 1 - (theMode != 1);
	SetPause(1, 2);
	PauseMusic(1);
	char aResult = lowos::ScreenScope(2 - (gFullScreen != 0));
	if (aResult == 0)
	{
		lowos::ScreenScope(2);
		lowos::MessageBox_(GTC(0x0046ed30), GTC(0x0046c318));
		exit(0);
	}
	gFullScreen = 1 - (aResult != 1);
	if (gApp.mFullScreenBox != nullptr)
		Send(gApp.mFullScreenBox, MSG_SETVALUE, (uint8_t)gFullScreen, 0);
	SetPause(0, 2);
	PauseMusic(0);
}

static void OnSwitch(char theMode)
{
	App::ToggleFullScreen(theMode);
}

// 0x41b42e: the screen, the combo images' checks, the board, the images, input, the GUI, the sounds and the splash
int App::Init()
{
	// the arrow keys repeat (0x4b, 0x4d and their extended codes)
	uint32_t aRepeatMask[8] = {0, 0, 0x2800, 0, 0, 0, 0x2800, 0};
	char aResult = lowos::InitScreen(1, OnActivate, OnClose, OnSwitch, OnRestore, GTC(0x0046ee40),
		2 - (gFullScreen != 0), 3);
	if (aResult == 0)
	{
		lowos::ScreenScope(2);
		lowos::MessageBox_(GTC(0x0046ee1c), GTC(0x0046c318));
		exit(0);
	}
	for (uint32_t i = 0; i < gConfig.mComboanimParamsCount; i = i + 1)
	{
		if (res::Load(res::GROUP_IMAGES, 1, i + 0xbc) != 0)
			NoMemory();
		lowos::Image* anImage = (lowos::Image*)res::Get(res::GROUP_IMAGES, i + 0xbc);
		if (anImage->mWidth % (uint32_t)gConfig.mComboanimParams[i * 2] != 0)
		{
			lowos::ScreenScope(2);
			lowos::MessageBox_(GTC(0x0046eddc), GTC(0x0046c318));
			exit(0);
		}
		res::Free(res::GROUP_IMAGES, 1, i + 0xbc);
	}
	for (uint32_t i = 0; i < gConfig.mCombostarParamsCount; i = i + 1)
	{
		if (res::Load(res::GROUP_IMAGES, 1, i + 0xc4) != 0)
			NoMemory();
		lowos::Image* anImage = (lowos::Image*)res::Get(res::GROUP_IMAGES, i + 0xc4);
		if (anImage->mWidth % (uint32_t)gConfig.mCombostarParams[i * 4] != 0)
		{
			lowos::ScreenScope(2);
			lowos::MessageBox_(GTC(0x0046eda4), GTC(0x0046c318));
			exit(0);
		}
		res::Free(res::GROUP_IMAGES, 1, i + 0xc4);
	}
	gFullScreen = 1 - (aResult != 1);
	lowos::SetColorKey(0xfefe01);
	mBoard = new Board(gConfig.mBlockgridX, gConfig.mBlockgridY, gConfig.mNumColumns, gConfig.mNumRows);
	if (LoadImages() == 0)
		return 0;
	if (lowos::InitKeyboard() != 0)
	{
		lowos::ScreenScope(2);
		lowos::MessageBox_(GTC(0x0046ed88), GTC(0x0046c318));
		exit(0);
	}
	lowos::SetKeyRepeat(400, 100);
	lowos::SetKeyRepeatMask(aRepeatMask);
	if (lowos::InitMouse(0) != 0)
	{
		lowos::ScreenScope(2);
		lowos::MessageBox_(GTC(0x0046ed70), GTC(0x0046c318));
		exit(0);
	}
	GuiInit(0x20, 0x100, mSkin);
	lowos::InitSound();
	LoadMenuSounds();
	SetScreen(SCREEN_SPLASH);
	return 1;
}

// 0x41b755: the board's copy of the configuration
void App::SetConfig(Config* theConfig)
{
	mBoard->SetConfig(theConfig);
}

// 0x41b771
void App::FreeImages()
{
	if (mBackground != nullptr)
		FreeSurface(mBackground);
	if (mBarFlash != nullptr)
		FreeSurface(mBarFlash);
	if (mBarLow != nullptr)
		FreeSurface(mBarLow);
	if (mBarHigh != nullptr)
		FreeSurface(mBarHigh);
	if (mBarFrame != nullptr)
		FreeSurface(mBarFrame);
	if (mSkin != nullptr)
		FreeSurface(mSkin);
	if (mImage68 != nullptr)
		FreeSurface(mImage68);
	if (mBarBits != nullptr)
		free(mBarBits);
	for (uint32_t i = 0; i < gConfig.mParticleCount; i = i + 1)
	{
		if ((gParticleState != nullptr) && (mParticleImages[i] != nullptr))
			free(mParticleImages[i]);
	}
	if (gParticleState != nullptr)
		free(gParticleState);
	for (int i = 0; i < 4; i++)
	{
		if (mTexts[i].mSurface != nullptr)
			lowos::UnregisterSurface(mTexts[i].mSurface);
	}
	for (int i = 0; i < 4; i++)
	{
		if (mTexts[i].mBits != nullptr)
			free(mTexts[i].mBits);
	}
	if (mTextFont != nullptr)
		FreeFont(mTextFont);
	if (gTextFont != nullptr)
		FreeFont(gTextFont);
}

// 0x41b98a
void App::Shutdown()
{
	SetScreen(SCREEN_NONE);
	StopMusic();
	if (mBoard != nullptr)
		delete mBoard;
	FreeImages();
	GuiShutdown();
	lowos::UninitKeyboard();
	lowos::UninitMouse();
	lowos::UninitSound();
	lowos::UninitScreen();
}

// 0x41b9fe: a frame of the current screen
int App::Frame()
{
	HandleKeys();
	switch (mScreen)
	{
	case SCREEN_SPLASH:
	case SCREEN_MAINMENU:
	case SCREEN_NEWGAME:
	case SCREEN_HIGHSCORES:
		Dispatch(MSG_UPDATE, 0, 0);
		Dispatch(MSG_DRAW, 1, 0);
		break;
	case SCREEN_OPTIONS:
		Dispatch(MSG_UPDATE, 0, 0);
		Dispatch(MSG_DRAW, 1, 0);
		break;
	case SCREEN_INSTRUCTIONS:
		Dispatch(MSG_UPDATE, 0, 0);
		Dispatch(MSG_DRAW, 1, 0);
		break;
	case SCREEN_CREDITS:
		Dispatch(MSG_UPDATE, 0, 0);
		Dispatch(MSG_DRAW, 1, 0);
		break;
	case SCREEN_GAME:
		if ((gBonusRound == 0) || (mSummaryEvents == 0))
		{
			if (mBoard->IsEnding() == 0)
			{
				DrawGame();
			}
			else
			{
				SetScreen(SCREEN_LEVELSUMMARY);
				NextSummaryEvent();
			}
		}
		else
		{
			NextLevel();
			SetScreen(SCREEN_NONE);
			SetScreen(SCREEN_GAME);
		}
		break;
	case SCREEN_LEVELSUMMARY:
		DrawGame();
		if ((((mSummaryEvents < 2) && (gBusy == 0)) && (mScreen == SCREEN_LEVELSUMMARY)) &&
			(mBoard->ClearStep() != 0))
			NextSummaryEvent();
		if (mSummaryEvents == 5)
		{
			NextLevel();
			SetScreen(SCREEN_GAME);
		}
		break;
	case SCREEN_GAMEOVER:
		Dispatch(MSG_UPDATE, 0, 0);
		Dispatch(MSG_DRAW, 1, 0);
	}
	return 0;
}

// 0x41bbce: the level summary's next step (skipping the last one when the game object says so)
void App::NextSummaryEvent()
{
	do
	{
		mSummaryEvents = mSummaryEvents + 1;
		if (mSummaryEvents == gConfig.mLevelsummaryStartevent + 1)
			AddSummaryItems();
	} while ((mSummaryEvents == (uint32_t)((gConfig.mLevelsummaryEndevent != 1) + 2)) &&
		(mBoard->mField->BlowAway() != 0));
}

// 0x41bc36
void App::NewGame()
{
	SetConfig(&gConfig);
	mBoard->Start();
	mBarLevel = 0;
	mBarTime = GameTime();
	m13d = 0;
	mSummaryEvents = 0;
	m144 = 0;
	mCrossed = 0;
	m13f = 0;
}

// 0x41bca1
void App::NextLevel()
{
	mBoard->NextLevel();
	mSummaryEvents = 0;
	m144 = 0;
	mCrossed = 0;
	m13f = 0;
}

// 0x41bce0: the score and the level number in the game's digits
void App::DrawScoreLevel()
{
	char aScore[12];
	char aLevel[8];
	int aValue = mBoard->GetScore();
	sprintf(aScore, "%d", aValue);
	sprintf(aLevel, "%d", mBoard->mState.mLevel);
	GuiRect aRect = {(int16_t)(gConfig.mIngameScorex + -300), (int16_t)(gConfig.mIngameScorey + -0x1e), 600, 0x3c};
	DrawText(mDigitFont, nullptr, 0, aScore, &aRect, nullptr, 0, 0, 0, 0xc);
	aRect.y = (int16_t)(gConfig.mIngameLevely + -0x1e);
	aRect.h = 0x3c;
	aRect.x = (int16_t)(gConfig.mIngameLevelx + -300);
	aRect.w = 600;
	DrawText(mDigitFont, nullptr, 0, aLevel, &aRect, nullptr, 0, 0, 0, 0xc);
}

// 0x41c599: the time trial's gauge (0..1, below 0 when it has run out)
static float GaugeOf(Board* theBoard)
{
	return theBoard->mField->GetMeter();
}

// 0x41c5b0: the board's rectangle (left, top, right, bottom)
static int32_t* BoardRect(Board* theBoard)
{
	return &theBoard->mX;
}

// 0x41bdcc: the game screen: the background around the board, the end of a level or of the game, the board, the
// score, the level and the progress bar
void App::DrawGame()
{
	lowos::BlitRegistered(0x280, BoardRect(mBoard)[1], 0, 0, 0, 0, mBackground);
	lowos::BlitRegistered(BoardRect(mBoard)[0], BoardRect(mBoard)[3] - BoardRect(mBoard)[1], 0, BoardRect(mBoard)[1], 0,
		BoardRect(mBoard)[1], mBackground);
	lowos::BlitRegistered(0x280 - BoardRect(mBoard)[2], BoardRect(mBoard)[3] - BoardRect(mBoard)[1],
		BoardRect(mBoard)[2], BoardRect(mBoard)[1], BoardRect(mBoard)[2], BoardRect(mBoard)[1], mBackground);
	lowos::BlitRegistered(0x280, 0x1e0 - BoardRect(mBoard)[3], 0, BoardRect(mBoard)[3], 0, BoardRect(mBoard)[3],
		mBackground);
	if ((mScreen == SCREEN_GAME) && (gBusy == 0))
	{
		if (gBonusRound == 0)
		{
			if (mBoard->mGameOver != 0)
			{
				// the game is over
				Play(gSounds[0x8], 1);
				StopLevelSounds();
				if (mCrossed == 0)
				{
					GuiRect aRect = At((int16_t)mPosX, (int16_t)mPosY);
					Handler* aHandler = NewHandler(ZoomImageProc, mLayer, &aRect, 1, 0x1b);
					aHandler->mPass = 2;
				}
				else
				{
					GuiRect aRect = At(gConfig.mBlockgridCenterX, gConfig.mBlockgridCenterY);
					Handler* aHandler = NewHandler(PopImageProc, mLayer, &aRect, 0x61, 0x8000001b);
					aHandler->mPass = 2;
				}
				NewHandler(CounterProc, mLayer, nullptr, (uintptr_t)&gBusy, (uintptr_t)&mCounted[2]);
			}
			if ((mBoard->mLevelComplete != 0) && (mBoard->mGameOver == 0))
			{
				// the level is complete
				StopLevelSounds();
				mBoard->mField->ClearShake();
				NewHandler(LevelCompleteProc, mLayer, nullptr, (uintptr_t)mLevelComplete[0], (uintptr_t)mLevelComplete[1]);
			}
		}
		else
		{
			if (mBoard->mGameOver != 0)
			{
				Play(gSounds[0x8], 1);
				StopLevelSounds();
				if (mCrossed == 0)
				{
					if (m13f == 0)
					{
						GuiRect aRect = At((int16_t)mPosX, (int16_t)mPosY);
						Handler* aHandler = NewHandler(ZoomImageProc, mLayer, &aRect, 1, 0x3a);
						aHandler->mPass = 2;
					}
					else
					{
						GuiRect aRect = At(gConfig.mBlockgridCenterX, gConfig.mBlockgridCenterY);
						Handler* aHandler = NewHandler(PopImageProc, mLayer, &aRect, 0x62, 0x8000003a);
						aHandler->mPass = 2;
					}
				}
				else
				{
					GuiRect aRect = At(gConfig.mBlockgridCenterX, gConfig.mBlockgridCenterY);
					Handler* aHandler = NewHandler(PopImageProc, mLayer, &aRect, 0x61, 0x8000003a);
					aHandler->mPass = 2;
				}
				NewHandler(CounterProc, mLayer, nullptr, (uintptr_t)&gBusy, (uintptr_t)&mCounted[2]);
			}
			if ((mBoard->mLevelComplete != 0) && (mBoard->mGameOver == 0))
			{
				Play(gSounds[0x7], 1);
				StopLevelSounds();
				mBoard->mField->ClearShake();
				GuiRect aRect = At(gConfig.mBlockgridCenterX, gConfig.mBlockgridCenterY);
				NewHandler(BonusEndProc, mLayer, &aRect, 0, 0);
			}
		}
	}
	if (((mScreen == SCREEN_GAME) && (gBusy == 0)) && (Send(mEC, MSG_GETVALUE, 0, 0) != 0))
		mBoard->Tap(0xffffffff);
	mBoard->Draw(mBackground);
	if ((mScreen == SCREEN_GAME) || (mScreen == SCREEN_LEVELSUMMARY))
	{
		DrawScoreLevel();
		if (gBonusRound == 0)
		{
			if (gGameMode == 1)
			{
				float aLevel;
				if (GaugeOf(mBoard) >= 0.0f)
					aLevel = (float)((double)GaugeOf(mBoard) * 480.0);
				else
					aLevel = 0.0f;
				DrawBar((uint32_t)(int64_t)aLevel, CfgGoal(mBoard->mState.mConfig) * 0x1e0);
			}
			else
			{
				DrawBar(CfgGoal(mBoard->mState.mConfig) - mBoard->GetBlocksLeft(), CfgGoal(mBoard->mState.mConfig));
			}
		}
		else
		{
			uint32_t aLeft;
			if ((uint32_t)(gConfig.mBonusTime * 1000) < m138 || gConfig.mBonusTime * 1000 - m138 == 0)
				aLeft = 0;
			else
				aLeft = gConfig.mBonusTime * 1000 - m138;
			DrawBar(aLeft, gConfig.mBonusTime * 1000);
		}
	}
	Dispatch(MSG_UPDATE, 0, 0);
	Dispatch(MSG_DRAW, 1, 0);
}

// 0x41c5bf: the keys: F11 full screen, shift+Insert and ctrl+V paste, tab and shift+tab move the focus
void App::HandleKeys()
{
	while (true)
	{
		uint8_t aKey = lowos::NextKey();
		if (aKey == 0)
			return;
		if (aKey == 0x57)
			ToggleFullScreen(0);
		if ((aKey == 0xd2) || (aKey == 0x52))
		{
			if ((lowos::KeyIsDown(0x2a) & 0xff) != 0 || (lowos::KeyIsDown(0x36) & 0xff) != 0)
				Dispatch(MSG_PASTE, 0, 0);
		}
		if (aKey == 0x2f)
		{
			if ((lowos::KeyIsDown(0x1d) & 0xff) != 0 || (lowos::KeyIsDown(0x9d) & 0xff) != 0)
				Dispatch(MSG_PASTE, 0, 0);
		}
		if (aKey == 0xf)
		{
			if ((lowos::KeyIsDown(0x2a) & 0xff) == 0 && (lowos::KeyIsDown(0x36) & 0xff) == 0)
				Dispatch(MSG_TAB, 0, 0);
		}
		if (aKey == 0xf)
		{
			if ((lowos::KeyIsDown(0x2a) & 0xff) != 0 || (lowos::KeyIsDown(0x36) & 0xff) != 0)
				Dispatch(MSG_BACKTAB, 0, 0);
		}
		Dispatch(MSG_KEY, aKey, 0);
	}
}

// 0x41c6dd: the level summary's lines and its button, one after another
void App::AddSummaryItems()
{
	GuiRect aRect = At(gConfig.mLevelsummaryItemx, gConfig.mLevelsummaryScorey);
	Handler* aHandler = NewHandler(SummaryItemProc, mLayer, &aRect, 0, 0);
	aHandler->mPass = 2;
	aRect.y = (int16_t)gConfig.mLevelsummaryTimey;
	aHandler = NewHandler(SummaryItemProc, mLayer, &aRect, 1, gConfig.mLevelsummaryDelay);
	aHandler->mPass = 2;
	aRect.y = (int16_t)gConfig.mLevelsummaryComboy;
	aHandler = NewHandler(SummaryItemProc, mLayer, &aRect, 2, gConfig.mLevelsummaryDelay << 1);
	aHandler->mPass = 2;
	aRect.y = (int16_t)gConfig.mLevelsummaryMatchy;
	aHandler = NewHandler(SummaryItemProc, mLayer, &aRect, 3, gConfig.mLevelsummaryDelay * 3);
	aHandler->mPass = 2;
	aRect.x = (int16_t)gConfig.mLevelsummaryButtonx;
	aRect.y = (int16_t)gConfig.mLevelsummaryButtony;
	aHandler = NewHandler(SummaryItemProc, mLayer, &aRect, 4, gConfig.mLevelsummaryDelay << 2);
	aHandler->mPass = 2;
}

// 0x41c80e: the game screen's buttons: instructions, options, end game and the pause check box
void App::AddGameButtons(Layer* theLayer)
{
	GuiRect aRect = At(gConfig.mIngameInstructionsx, gConfig.mIngameInstructionsy);
	mGameButtons[0] = NewHandler(ShapedButtonProc, theLayer, &aRect, 0x45, 0x16);
	aRect = At(gConfig.mIngameOptionsx, gConfig.mIngameOptionsy);
	mGameButtons[1] = NewHandler(ShapedButtonProc, theLayer, &aRect, 10, 1);
	aRect = At(gConfig.mIngameEndgamex, gConfig.mIngameEndgamey);
	mGameButtons[2] = NewHandler(ShapedButtonProc, theLayer, &aRect, 0xb, 0x40000002);
	aRect = At(gConfig.mIngamePausex, gConfig.mIngamePausey);
	mGameButtons[3] = NewHandler(CheckBoxProc, theLayer, &aRect, 0x80, 0x3e);
}

Font* gScreenFont = nullptr;  // 0x489b50 the text font of the instructions, credits, game and game over screens

namespace
{

char gMusicStarted = 0;         // 0x489c18 the main menu has started the music once
char gTrialText[0x50];          // 0x489b58
char gPlayedText[0x24];         // 0x489ba8
char gSavedCombo[0x14];         // 0x489bcc the saved game's summary on the new game screen
char gSavedScore[0xc];          // 0x489be0
char gSavedLevel[0xc];          // 0x489bec
char gSavedMode[0xc];           // 0x489bf8
char gSavedDate[0x14];          // 0x489c04

// The month names (0x47e21c, 1-based) and the modes' names (0x47e250)
const uint32_t kMonths[13] = {0x0046c399, 0x0046ec9c, 0x0046ec90, 0x0046ec88, 0x0046ec80, 0x0046ec7c, 0x0046ec74,
	0x0046ec6c, 0x0046ec64, 0x0046ec58, 0x0046ec50, 0x0046ec44, 0x0046ec38};
const uint32_t kModeNames[4] = {0x0046ee70, 0x0046ee68, 0x0046ee60, 0x0046ee54};

// A screen's layer: the whole display. theRect becomes the display's rectangle; the screen's handlers are placed
// by moving it, so those that do not size themselves keep the display's size (as in the original, which used one
// rectangle on its stack).
Layer* NewScreenLayer(GuiRect* theRect)
{
	uint16_t aWidth, aHeight;
	lowos::GetDisplayFormat(nullptr, &aWidth, &aHeight);
	theRect->w = (int16_t)aWidth;
	theRect->h = (int16_t)aHeight;
	theRect->x = 0;
	theRect->y = 0;
	return NewLayer(aWidth, aHeight, theRect);
}

void MoveRect(GuiRect* theRect, int x, int y)
{
	theRect->x = (int16_t)x;
	theRect->y = (int16_t)y;
}

// The animations of a menu: records of 6 numbers (x, y, width, image - 0xac, two halves of the parameter)
void AddMenuAnimations(Layer* theLayer, GuiRect* theRect, const int32_t* theAnimations, uint32_t theCount)
{
	for (uint32_t i = 0; i < theCount; i = i + 1)
	{
		const int32_t* anAnimation = theAnimations + i * 6;
		theRect->x = (int16_t)anAnimation[0];
		theRect->y = (int16_t)anAnimation[1];
		theRect->w = (int16_t)anAnimation[2];
		Handler* aHandler = NewHandler(MenuAnimationProc, theLayer, theRect, anAnimation[3] + 0xac,
			(uint32_t)anAnimation[4] | (uint32_t)anAnimation[5] << 0x10);
		aHandler->mPass = 1;
	}
}

// The score fonts (images 0xcc..) of the score messages
void LoadScoreFonts(App* theApp)
{
	for (int i = 0; i < 8; i = i + 1)
	{
		if (res::Load(res::GROUP_IMAGES, 1, i + 0xcc) == 0)
		{
			theApp->mScoreFonts[i] = LoadFont(i + 0xcc, GTC(0x0046eec4), 5, 0, 0x40);
			res::Free(res::GROUP_IMAGES, 1, i + 0xcc);
		}
		else
		{
			theApp->mScoreFonts[i] = nullptr;
		}
	}
}

} // namespace

// 0x41c8f5: leaves the current screen (freeing what it made) and builds screen theScreen
void App::SetScreen(int theScreen)
{
	if (mScreen == theScreen)
		return;
	switch (mScreen)
	{
	case SCREEN_SPLASH:
		mSplashButtons[0] = nullptr;
		mSplashButtons[1] = nullptr;
		mSplashButtons[2] = nullptr;
		mSplashButtons[3] = nullptr;
		FreeAllLayers();
		break;
	case SCREEN_MAINMENU:
		FreeAllLayers();
		break;
	case SCREEN_OPTIONS:
		mSplashButtons[0] = nullptr;
		FreeLayer(mLayer);
		mFullScreenBox = nullptr;
		FreeAllLayers();
		break;
	case SCREEN_INSTRUCTIONS:
	case SCREEN_CREDITS:
		FreeAllLayers();
		FreeFont(gScreenFont);
		break;
	case SCREEN_GAME:
		FreeAllLayers();
		FreeFont(mDigitFont);
		FreeFont(mLevelFont);
		for (int i = 0; i < 8; i = i + 1)
		{
			FreeFont(mScoreFonts[i]);
			mScoreFonts[i] = nullptr;
			FreeSurface(mComboAnims[i]);
			if (i < 4)
				FreeSurface(mComboStars[i]);
		}
		FreeFont(gScreenFont);
		if (gConfig.mLevelcompleteDisable == 0)
		{
			FreeSurface(mLevelComplete[1]);
			FreeSurface(mLevelComplete[0]);
		}
		mEC = nullptr;
		break;
	case SCREEN_LEVELSUMMARY:
		FreeAllLayers();
		FreeFont(mDigitFont);
		for (int i = 0; i < 8; i = i + 1)
		{
			FreeFont(mScoreFonts[i]);
			mScoreFonts[i] = nullptr;
		}
		break;
	case SCREEN_NEWGAME:
		FreeAllLayers();
		break;
	case SCREEN_HIGHSCORES:
		FreeAllLayers();
		break;
	case SCREEN_GAMEOVER:
		FreeAllLayers();
		FreeFont(gScreenFont);
		FreeFont(mDigitFont);
		for (int i = 0; i < 8; i = i + 1)
		{
			FreeFont(mScoreFonts[i]);
			mScoreFonts[i] = nullptr;
		}
	}
	mPrevScreen = mScreen;
	mScreen = theScreen;
	GuiRect aRect = {0, 0, 0, 0};
	Handler* aHandler;
	switch (theScreen)
	{
	case SCREEN_SPLASH:
	{
		mLayer = NewScreenLayer(&aRect);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 0x30, 0);
		MoveRect(&aRect, gConfig.mSplashLoadbarx, gConfig.mSplashLoadbary);
		aHandler = NewHandler(LoadBarProc, mLayer, &aRect, 0x4c,
			(~-(uint32_t)(gRegistered != 0) & (gConfig.mInitialWait + gRunCount * gConfig.mAdditionalWait)) | 0x2e0000);
		aHandler->mPass = 1;
		if (gRegistered == 0)
		{
			MoveRect(&aRect, gConfig.mSplashRegisterx, gConfig.mSplashRegistery);
			mSplashButtons[3] = NewHandler(ButtonProc, mLayer, &aRect, 0x4d, 0x30);
			mSplashButtons[3]->mPass = 1;
			MoveRect(&aRect, gConfig.mSplashUnregisteredx, gConfig.mSplashUnregisteredy);
			mSplashButtons[0] = NewHandler(ImageProc, mLayer, &aRect, 0x55, 0);
			mSplashButtons[0]->mPass = 1;
			MoveRect(&aRect, gConfig.mSplashUnregistered2x, gConfig.mSplashUnregistered2y);
			mSplashButtons[1] = NewHandler(ImageProc, mLayer, &aRect, 0x65, 0);
			mSplashButtons[1]->mPass = 1;
		}
		else
		{
			MoveRect(&aRect, gConfig.mSplashRegisteredx, gConfig.mSplashRegisteredy);
			aHandler = NewHandler(ImageProc, mLayer, &aRect, 0x66, 0);
			aHandler->mPass = 1;
		}
		MoveRect(&aRect, gConfig.mSplashTimesplayedx, gConfig.mSplashTimesplayedy);
		snprintf(gPlayedText, sizeof(gPlayedText), GTC(0x0046ee98), (long)gRunCount,
			gRunCount == 1 ? GTC(0x0046c399) : GTC(0x0046eeb4));
		aHandler = NewHandler(LabelProc, mLayer, &aRect, (uintptr_t)gPlayedText, 0xc0000056);
		Play(gSounds[0x0], 1);
		if ((gConfig.mExecutionlimit != 0) && (gRegistered == 0))
		{
			MoveRect(&aRect, gConfig.mSplashExecutionsleftx, gConfig.mSplashExecutionslefty);
			uint32_t aLeft = (gConfig.mExecutionlimit > gRunCount) ? gConfig.mExecutionlimit - gRunCount : 0;
			snprintf(gTrialText, sizeof(gTrialText), GTC(0x0046ee78), (long)aLeft,
				aLeft == 1 ? GTC(0x0046c399) : GTC(0x0046eeb4), aLeft != 1 ? GTC(0x0046c399) : GTC(0x0046eeb4));
			mSplashButtons[2] = NewHandler(LabelProc, mLayer, &aRect, (uintptr_t)gTrialText, 0xc0000056);
			mSplashButtons[2]->mPass = 1;
			// 0x426fc7 shows the dialog when no play is left; the count was saved by 0x411d0a and 0x411f7a
			if ((TrialCheck() == 0) && (gConfig.mExecutionlimitmode != 0))
				gRunCount = gRunCount + 1;
		}
		break;
	}
	case SCREEN_MAINMENU:
		mLayer = NewScreenLayer(&aRect);
		MoveRect(&aRect, gConfig.mGamewindowX, gConfig.mGamewindowY);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 2, 0);
		MoveRect(&aRect, gConfig.mMainmenuHighscorex, gConfig.mMainmenuHighscorey);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 3, 0x2b);
		MoveRect(&aRect, gConfig.mMainmenuNewgamex, gConfig.mMainmenuNewgamey);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 4, 0x14);
		MoveRect(&aRect, gConfig.mMainmenuOptionsx, gConfig.mMainmenuOptionsy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 6, 0x15);
		MoveRect(&aRect, gConfig.mMainmenuInstructionsx, gConfig.mMainmenuInstructionsy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 7, 0x16);
		MoveRect(&aRect, gConfig.mMainmenuCreditsx, gConfig.mMainmenuCreditsy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 8, 0x17);
		MoveRect(&aRect, gConfig.mMainmenuQuitx, gConfig.mMainmenuQuity);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 9, 0x40000018);
		aHandler = NewHandler(MenuHighScoresProc, mLayer, &aRect, 0, 0);
		aHandler->mPass = 1;
		AddMenuAnimations(mLayer, &aRect, gConfig.mMainmenuanimations, gConfig.mMainmenuanimationsCount);
		if (gMusicStarted == 0)
		{
			gMusicStarted = 1;
			Play(gSounds[0x1], 1);
			NewHandler(MusicStarterProc, mLayer, nullptr, 0, 0);
		}
		break;
	case SCREEN_OPTIONS:
		mLayer = NewScreenLayer(&aRect);
		MoveRect(&aRect, gConfig.mGamewindowX, gConfig.mGamewindowY);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 0xc, 0);
		MoveRect(&aRect, gConfig.mOptionsContinuex, gConfig.mOptionsContinuey);
		aHandler = NewHandler(ButtonProc, mLayer, &aRect, 0xd, 0x40000004);
		aHandler->mPass = 1;
		MoveRect(&aRect, gConfig.mOptionsTipsx, gConfig.mOptionsTipsy);
		aHandler = NewHandler(CheckBoxProc, mLayer, &aRect, 0x4b, 5);
		aHandler->mPass = 1;
		Send(aHandler, MSG_SETVALUE, (uint8_t)gPrefByte, 0);
		MoveRect(&aRect, gConfig.mOptionsFullscreenx, gConfig.mOptionsFullscreeny);
		mFullScreenBox = NewHandler(CheckBoxProc, mLayer, &aRect, 0x11, 6);
		aHandler->mPass = 1;
		Send(mFullScreenBox, MSG_SETVALUE, (uint8_t)gFullScreen, 0);
		MoveRect(&aRect, gConfig.mOptionsVolumex, gConfig.mOptionsSoundy);
		aRect.w = (int16_t)gConfig.mOptionsVolumewidth;
		aHandler = NewHandler(SliderProc, mLayer, &aRect, 0, 7);
		aHandler->mPass = 1;
		Send(aHandler, MSG_SETVALUE, gSoundVolume, 0);
		MoveRect(&aRect, gConfig.mOptionsVolumex, gConfig.mOptionsMusicy);
		aRect.w = (int16_t)gConfig.mOptionsVolumewidth;
		aHandler = NewHandler(SliderProc, mLayer, &aRect, 0, 8);
		aHandler->mPass = 1;
		Send(aHandler, MSG_SETVALUE, MusicVolume() << 8, 0);
		AddMenuAnimations(mLayer, &aRect, gConfig.mOptionsmenuanimations, gConfig.mOptionsmenuanimationsCount);
		if (gRegistered == 0)
		{
			MoveRect(&aRect, gConfig.mOptionsRegisterx, gConfig.mOptionsRegistery);
			mSplashButtons[0] = NewHandler(ButtonProc, mLayer, &aRect, 0x5a, 0x30);
			mSplashButtons[0]->mPass = 1;
		}
		MoveRect(&aRect, gConfig.mOptionsVisitpopcapx, gConfig.mOptionsVisitpopcapy);
		aHandler = NewHandler(ButtonProc, mLayer, &aRect, 0x5b, 0x37);
		aHandler->mPass = 1;
		break;
	case SCREEN_INSTRUCTIONS:
		gScreenFont = LoadFont(0x2d, GTC(0x0046eca8), 4, 0, 0x40);
		mLayer = NewScreenLayer(&aRect);
		MoveRect(&aRect, gConfig.mGamewindowX, gConfig.mGamewindowY);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 0x14, 0);
		aHandler->mPass = 1;
		MoveRect(&aRect, gConfig.mGamewindowX, gConfig.mGamewindowY);
		aHandler = NewHandler(PagerProc, mLayer, &aRect, (uintptr_t)mLayer, 0);
		aHandler->mPass = 2;
		MoveRect(&aRect, gConfig.mInstructionsNextx, gConfig.mInstructionsNexty);
		aHandler = NewHandler(ButtonProc, mLayer, &aRect, 0x15, 0xe);
		aHandler->mPass = 2;
		MoveRect(&aRect, gConfig.mInstructionsPrevx, gConfig.mInstructionsPrevy);
		mInstrPrev = NewHandler(ButtonProc, mLayer, &aRect, 0x16, 0xf);
		mInstrPrev->mPass = 2;
		MoveRect(&aRect, gConfig.mInstructionsOkx, gConfig.mInstructionsOky);
		aHandler = NewHandler(ButtonProc, mLayer, &aRect, 0x17, 0xc0000010);
		aHandler->mPass = 2;
		AddMenuAnimations(mLayer, &aRect, gConfig.mInstructionsmenuanimations, gConfig.mInstructionsmenuanimationsCount);
		break;
	case SCREEN_CREDITS:
		gScreenFont = LoadFont(0x46, GTC(0x0046eca8), 4, 0, 0x40);
		mLayer = NewScreenLayer(&aRect);
		MoveRect(&aRect, gConfig.mGamewindowX, gConfig.mGamewindowY);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 0x19, 0);
		aHandler->mPass = 1;
		MoveRect(&aRect, gConfig.mGamewindowX, gConfig.mGamewindowY);
		aHandler = NewHandler(PagerProc, mLayer, &aRect, (uintptr_t)mLayer, 1);
		aHandler->mPass = 2;
		MoveRect(&aRect, gConfig.mCreditsOkx, gConfig.mCreditsOky);
		aHandler = NewHandler(ButtonProc, mLayer, &aRect, 0x47, 0xc0000011);
		aHandler->mPass = 2;
		AddMenuAnimations(mLayer, &aRect, gConfig.mCreditsmenuanimations, gConfig.mCreditsmenuanimationsCount);
		break;
	case SCREEN_GAME:
		mCounted[1] = nullptr;
		mCounted[0] = nullptr;
		mCounted[3] = nullptr;
		mCounted[2] = nullptr;
		if (mBoard->mField->HasFlags(0x100) != 0)
			NewHandler(CounterProc, mLayer, nullptr, (uintptr_t)&gBusy, (uintptr_t)&mCounted[0]);
		if (mBoard->mField->HasFlags(0x80) == 0)
		{
			if ((m144 == 1) && (gBonusRound != 0))
			{
				MoveRect(&aRect, (int16_t)gConfig.mBlockgridX + (int16_t)((uint32_t)(gConfig.mBlockWidth * gConfig.mNumColumns) >> 1),
					(int16_t)gConfig.mBlockgridY + (int16_t)((uint32_t)(gConfig.mBlockHeight * gConfig.mNumRows) >> 1));
				aHandler = NewHandler(PopImageProc, mLayer, &aRect, 0x60, 0x39);
			}
		}
		else
		{
			NewHandler(CounterProc, mLayer, nullptr, (uintptr_t)&gBusy, (uintptr_t)&mCounted[1]);
			if ((m144 == 0) && (gBonusRound != 0))
			{
				MoveRect(&aRect, (int16_t)gConfig.mBlockgridX + (int16_t)((uint32_t)(gConfig.mBlockWidth * gConfig.mNumColumns) >> 1),
					(int16_t)gConfig.mBlockgridY + (int16_t)((uint32_t)(gConfig.mBlockHeight * gConfig.mNumRows) >> 1));
				aHandler = NewHandler(PopImageProc, mLayer, &aRect, 0x60, 0x39);
			}
		}
		if ((gConfig.mLevelcompleteDisable == 0) &&
			((LoadSurface(0x27, 9, &mLevelComplete[1]) != 0 || (LoadSurface(0x28, 9, &mLevelComplete[0]) != 0))))
			NoMemory();
		gScreenFont = LoadFont(0x29, GTC(0x0046eca8), 4, 0, 0x40);
		mLayer = NewScreenLayer(&aRect);
		AddGameButtons(mLayer);
		Send(mGameButtons[3], MSG_SETVALUE, 0, 0);
		aHandler = NewHandler(PauseKeyProc, mLayer, nullptr, mPrevScreen, 0);
		{
			uint8_t aFlags = gPauseFlags & 0xfd;
			if ((mPrevScreen != SCREEN_OPTIONS) && (mPrevScreen != SCREEN_INSTRUCTIONS))
				aFlags = gPauseFlags & 0xfc;
			gPauseFlags = aFlags;
		}
		UpdatePause();
		mEC = NewHandler(ColumnCursorProc, mLayer, nullptr, 0, 0);
		mDigitFont = LoadFont(0x2a, GTC(0x0046eed8), 5, 0, 0x40);
		for (int i = 0; i < 8; i = i + 1)
		{
			if (res::Load(res::GROUP_IMAGES, 1, i + 0xcc) == 0)
			{
				mScoreFonts[i] = LoadFont(i + 0xcc, GTC(0x0046eec4), 5, 0, 0x40);
				res::Free(res::GROUP_IMAGES, 1, i + 0xcc);
			}
			else
			{
				mScoreFonts[i] = nullptr;
			}
			if (res::Load(res::GROUP_IMAGES, 1, i + 0xbc) == 0)
			{
				LoadSurface(i + 0xbc, 9, &mComboAnims[i]);
				res::Free(res::GROUP_IMAGES, 1, i + 0xbc);
			}
			else
			{
				mComboAnims[i] = nullptr;
			}
			if (i < 4)
			{
				if (res::Load(res::GROUP_IMAGES, 1, i + 0xc4) == 0)
				{
					LoadSurface(i + 0xc4, 9, &mComboStars[i]);
					res::Free(res::GROUP_IMAGES, 1, i + 0xc4);
				}
				else
				{
					mComboStars[i] = nullptr;
				}
			}
		}
		mLevelFont = LoadFont(0x49, GTC(0x0046eeb8), 4, 0, 0x50);
		if (((mPrevScreen != SCREEN_OPTIONS) && (mPrevScreen != SCREEN_INSTRUCTIONS)) && (gBonusRound == 0))
		{
			aHandler = NewHandler(LevelStartProc, mLayer, nullptr, 0, 0);
			aHandler->mPass = 1;
		}
		if (gBonusRound != 0)
			NewHandler(BonusTimerProc, mLayer, nullptr, 0, 0);
		mBoard->ResetClocks();
		mBarTime = GameTime();
		break;
	case SCREEN_LEVELSUMMARY:
		mBoard->EndGame();
		mCounted[0] = nullptr;
		if (mBoard->mField->HasFlags(0x100) != 0)
			NewHandler(CounterProc, mLayer, nullptr, (uintptr_t)&gBusy, (uintptr_t)&mCounted[0]);
		mDigitFont = LoadFont(0x2a, GTC(0x0046eed8), 5, 0, 0x40);
		LoadScoreFonts(this);
		mLayer = NewScreenLayer(&aRect);
		AddGameButtons(mLayer);
		Send(mGameButtons[3], MSG_SETVALUE, 0, 0);
		aHandler = NewHandler(PauseKeyProc, mLayer, nullptr, mPrevScreen, 0);
		{
			uint8_t aFlags = gPauseFlags & 0xfd;
			if ((mPrevScreen != SCREEN_OPTIONS) && (mPrevScreen != SCREEN_INSTRUCTIONS))
				aFlags = gPauseFlags & 0xfc;
			gPauseFlags = aFlags;
		}
		UpdatePause();
		mBoard->ResetClocks();
		mBarTime = GameTime();
		if ((gConfig.mLevelsummaryStartevent + 1U <= mSummaryEvents) &&
			(mSummaryEvents < (uint32_t)(4 - (gConfig.mLevelsummaryEndevent != 1))))
			AddSummaryItems();
		break;
	case SCREEN_NEWGAME:
	{
		// the saved game, if there is one: its date, mode, level, best chain and score
		char aSaved = 0;
		FILE* aFile = UserFopen(GTC(0x0046ec28), "rb");
		if (aFile != nullptr)
		{
			aSaved = 1;
			uint32_t aNow = GameTime();
			uint16_t aDay = 0, aMonth = 0, aYear = 0;
			uint32_t aMode = 0, aLevel = 0, aChain = 0, aScore = 0;
			if (ReadField('D', 2, &aDay, aFile, aNow) != 0 || ReadField('D', 2, &aMonth, aFile, aNow) != 0 ||
				ReadField('D', 2, &aYear, aFile, aNow) != 0 || ReadField('D', 4, &aMode, aFile, aNow) != 0 ||
				ReadField('D', 4, &aLevel, aFile, aNow) != 0 || ReadField('D', 4, &aChain, aFile, aNow) != 0 ||
				ReadField('D', 4, &aScore, aFile, aNow) != 0)
			{
				// ReadField has closed the file
				aSaved = 0;
			}
			else
			{
				snprintf(gSavedDate, sizeof(gSavedDate), GTC(0x0046ef18), GTC(kMonths[aMonth]), (long)aDay, (long)aYear);
				snprintf(gSavedMode, sizeof(gSavedMode), "%s", GTC(kModeNames[aMode]));
				snprintf(gSavedLevel, sizeof(gSavedLevel), GTC(0x0046ef14), (long)aLevel);
				snprintf(gSavedScore, sizeof(gSavedScore), GTC(0x0046ef14), (long)aScore);
				switch (aChain)
				{
				case 0:
				case 1:
					strcpy(gSavedCombo, GTC(0x0046ef08));
					break;
				case 2:
					strcpy(gSavedCombo, GTC(0x0046ef00));
					break;
				case 3:
					strcpy(gSavedCombo, GTC(0x0046eef8));
					break;
				case 4:
					strcpy(gSavedCombo, GTC(0x0046eef0));
					break;
				case 5:
					snprintf(gSavedCombo, sizeof(gSavedCombo), GTC(0x0046eee4), (long)aChain);
					break;
				}
				fclose(aFile);
			}
		}
		mLayer = NewScreenLayer(&aRect);
		MoveRect(&aRect, 0, 0);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 0x3a, 0);
		bool aWarn = !((aSaved == 0) || (gConfig.mDeletegamewarningDisable != 0));
		MoveRect(&aRect, gConfig.mNewgameNormalx, gConfig.mNewgameNormaly);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x36, aWarn ? 0x25 : 0x20);
		MoveRect(&aRect, gConfig.mNewgameSpeedx, gConfig.mNewgameSpeedy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x37, aWarn ? 0x26 : 0x21);
		MoveRect(&aRect, gConfig.mNewgameTimedx, gConfig.mNewgameTimedy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x38, aWarn ? 0x27 : 0x22);
		MoveRect(&aRect, gConfig.mNewgameStrategyx, gConfig.mNewgameStrategyy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x7f, aWarn ? 0x28 : 0x23);
		MoveRect(&aRect, gConfig.mNewgameMainx, gConfig.mNewgameMainy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x39, 0x40000029);
		aHandler->mPass = 1;
		AddMenuAnimations(mLayer, &aRect, gConfig.mNewgamemenuanimations, gConfig.mNewgamemenuanimationsCount);
		if (aSaved != 0)
		{
			MoveRect(&aRect, gConfig.mNewgameContinuex, gConfig.mNewgameContinuey);
			aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x71, 0x24);
			aHandler->mPass = 1;
			MoveRect(&aRect, gConfig.mNewgameSummaryboxx, gConfig.mNewgameSummaryboxy);
			aHandler = NewHandler(ImageProc, mLayer, &aRect, 0x78, 0);
			const struct
			{
				int32_t mX, mY;
				char* mText;
			} kLines[5] = {{gConfig.mNewgameSummarydatex, gConfig.mNewgameSummarydatey, gSavedDate},
				{gConfig.mNewgameSummarytypex, gConfig.mNewgameSummarytypey, gSavedMode},
				{gConfig.mNewgameSummarylevelx, gConfig.mNewgameSummarylevely, gSavedLevel},
				{gConfig.mNewgameSummarycombox, gConfig.mNewgameSummarycomboy, gSavedCombo},
				{gConfig.mNewgameSummaryscorex, gConfig.mNewgameSummaryscorey, gSavedScore}};
			for (int i = 0; i < 5; i++)
			{
				MoveRect(&aRect, (int16_t)kLines[i].mX + (int16_t)gConfig.mNewgameSummaryboxx,
					(int16_t)kLines[i].mY + (int16_t)gConfig.mNewgameSummaryboxy);
				aHandler = NewHandler(LabelProc, mLayer, &aRect, (uintptr_t)kLines[i].mText, 0x79);
				aHandler->mPass = 1;
			}
		}
		break;
	}
	case SCREEN_HIGHSCORES:
		mLayer = NewScreenLayer(&aRect);
		MoveRect(&aRect, 0, 0);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 0x3d, 0);
		aHandler = NewHandler(HighScoresProc, mLayer, &aRect, 0, 0);
		MoveRect(&aRect, gConfig.mHighscoresmenuClearhighscoresx, gConfig.mHighscoresmenuClearhighscoresy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x3b, 0x12);
		MoveRect(&aRect, gConfig.mHighscoresmenuMainx, gConfig.mHighscoresmenuMainy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x3c, 0xc000002a);
		AddMenuAnimations(mLayer, &aRect, gConfig.mHighscoresmenuanimations, gConfig.mHighscoresmenuanimationsCount);
		break;
	case SCREEN_GAMEOVER:
		gScreenFont = LoadFont(0x2c, GTC(0x0046eca8), 4, 0, 0x40);
		mDigitFont = LoadFont(0x2a, GTC(0x0046eed8), 5, 0, 0x40);
		LoadScoreFonts(this);
		mLayer = NewScreenLayer(&aRect);
		MoveRect(&aRect, 0, 0);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 0, (uintptr_t)mBackground);
		MoveRect(&aRect, gConfig.mSummaryX, gConfig.mSummaryY);
		aHandler = NewHandler(ImageProc, mLayer, &aRect, 0x5c, 0);
		aHandler->mPass = 1;
		MoveRect(&aRect, gConfig.mIngameInstructionsx, gConfig.mIngameInstructionsy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0x45, 0x16);
		aHandler->mPass = 2;
		MoveRect(&aRect, gConfig.mIngameOptionsx, gConfig.mIngameOptionsy);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 10, 1);
		aHandler->mPass = 2;
		MoveRect(&aRect, gConfig.mIngameEndgamex, gConfig.mIngameEndgamey);
		aHandler = NewHandler(ShapedButtonProc, mLayer, &aRect, 0xb, 0x40000002);
		aHandler->mPass = 2;
		aHandler = NewHandler(SummaryStatsProc, mLayer, &aRect, 0x38, 0);
		aHandler->mPass = 2;
		aHandler = NewHandler(SummaryHighScoresProc, mLayer, &aRect, 0, 0);
		aHandler->mPass = 2;
	}
}

// 0x41eb9c: a score message at (x, y): theScore with word theWord, in score font theFont (1-based, 0 none)
void App::ScoreMessage(int16_t x, int16_t y, uint32_t theScore, uint8_t theWord, uint8_t theFont)
{
	if (theFont != 0)
	{
		GuiRect aRect = {(int16_t)(x - 200), (int16_t)(y - 0x32), 400, 100};
		Handler* aHandler = NewHandler(ScoreMessageProc, mLayer, &aRect, theScore | (uint32_t)theWord << 0x18,
			(uintptr_t)mScoreFonts[theFont - 1]);
		aHandler->mPass = 2;
	}
}

// 0x41ec11: the combo stars of burst theKind (1-based): of each kind of star, the configured number
void App::Starburst(int16_t x, int16_t y, char theKind)
{
	if ((theKind != 0) && ((uint8_t)(theKind - 1U) < gConfig.mStarburstParamsCount))
	{
		for (uint32_t i = 0; i < gConfig.mCombostarParamsCount; i = i + 1)
		{
			uint32_t j = 0;
			while ((j < (uint32_t)gConfig.mStarburstParams[i + (uint8_t)(theKind - 1U) * 4] && (gParticleCount < 0xc9)))
			{
				// the original left the height unset
				GuiRect aRect = {x, y, (int16_t)gConfig.mCombostarParams[i * 4], 0};
				Handler* aHandler = NewHandler(ComboStarProc, mLayer, &aRect, (uintptr_t)mComboStars[i], i);
				aHandler->mPass = 1;
				j = j + 1;
			}
		}
	}
}

// 0x41ecf4: a combo of theCount blocks (3..) in chain theChain: its score message, animation and stars from the
// tables of 10 counts by 10 chains
void App::Combo(int16_t x, int16_t y, uint8_t theCount, uint8_t theChain)
{
	if (2 < theCount)
	{
		uint32_t aCount;
		if ((uint8_t)(theCount - 3) < 10)
			aCount = (uint8_t)(theCount - 3);
		else
			aCount = 9;
		if (theChain != 0)
			theChain = theChain - 1;
		uint32_t aChain;
		if (theChain < 10)
			aChain = theChain;
		else
			aChain = 9;
		uint32_t anIndex = aCount + aChain * 10;
		ScoreMessage(x, y, (uint32_t)gConfig.mScoretable[anIndex], (uint8_t)gConfig.mScorewordTable[anIndex],
			(uint8_t)gConfig.mScorefontTable[anIndex]);
		int32_t anAnim = gConfig.mComboanimTable[anIndex];
		if (anAnim != 0)
		{
			anAnim = anAnim - 1;
			// the original left the height unset
			GuiRect aRect = {x, y, (int16_t)gConfig.mComboanimParams[anAnim * 2], 0};
			Handler* aHandler = NewHandler(ComboAnimProc, mLayer, &aRect, (uintptr_t)mComboAnims[anAnim],
				(uint32_t)gConfig.mComboanimParams[1 + anAnim * 2]);
			aHandler->mPass = 0;
		}
		Starburst(x, y, (char)gConfig.mStarburstTable[anIndex]);
	}
}

// 0x41ee5a
void App::SetPosition(int x, int y)
{
	mPosX = x + gConfig.mBlockgridX;
	mPosY = y + gConfig.mBlockgridY;
}

// 0x41ee83
void App::SetCrossed()
{
	mCrossed = 1;
}

// 0x41ee96
void App::Nothing()
{
}

// 0x41eea1: the particles of theList's effects
void App::Effects(const GuiRect* theWhere, int theDelay, const EffectList* theList, const Effect* theEffects)
{
	for (uint32_t i = 0; i < theList->mCount; i = i + 1)
	{
		uint32_t j = 0;
		while ((j < (uint32_t)theEffects[i].mValue2 && (gParticleCount < 0xc9)))
		{
			Handler* aHandler = NewHandler(ParticleProc, mLayer, theWhere, theDelay, (uintptr_t)&theEffects[i]);
			aHandler->mPass = 1;
			j = j + 1;
		}
	}
}

// 0x41ef2b: a game can be saved during a level or its summary (or the options or instructions over them), unless
// it is over
char App::CanSave()
{
	if ((mBoard->mGameOver == 0) || (gBonusRound != 0))
	{
		if ((((mScreen == SCREEN_GAME) || (mScreen == SCREEN_LEVELSUMMARY)) ||
			((mScreen == SCREEN_OPTIONS && ((mPrevScreen == SCREEN_GAME) || (mPrevScreen == SCREEN_LEVELSUMMARY))))) ||
			((mScreen == SCREEN_INSTRUCTIONS && ((mPrevScreen == SCREEN_GAME) || (mPrevScreen == SCREEN_LEVELSUMMARY)))))
			return 1;
		return 0;
	}
	return 0;
}

// 0x41efbb: saves the game to savegame.dat: the summary that the new game screen shows, the bonus round, the app's
// state, the screen to come back to and the board. 1 on failure (the file is removed).
char App::SaveGame()
{
	if (CanSave() == 0)
		return 1;
	FILE* aFile = UserFopen(GTC(0x0046ec28), "wb");
	if (aFile == nullptr)
		return 1;
	uint32_t aNow = GameTime();
	int32_t aPrevScreen = mPrevScreen;
	// the screen is left while the game is saved (and built again after)
	SetScreen(SCREEN_NONE);
	// the original's GetLocalTime
	time_t aTime = time(nullptr);
	struct tm* aLocal = localtime(&aTime);
	uint16_t aDay = (uint16_t)aLocal->tm_mday;
	uint16_t aMonth = (uint16_t)(aLocal->tm_mon + 1);
	uint16_t aYear = (uint16_t)(aLocal->tm_year + 1900);
	if ((((((WriteField('D', 2, &aDay, aFile, aNow) == 0) && (WriteField('D', 2, &aMonth, aFile, aNow) == 0)) &&
		(WriteField('D', 2, &aYear, aFile, aNow) == 0)) &&
		((WriteField('D', 4, &gGameMode, aFile, aNow) == 0 &&
		(WriteField('D', 4, &mBoard->mState.mLevel, aFile, aNow) == 0)))) &&
		(WriteField('D', 4, &mBoard->mField->mBestChain, aFile, aNow) == 0)) &&
		(((WriteField('D', 4, &mBoard->mField->mScore, aFile, aNow) == 0 &&
		(WriteField('D', 4, &gConfig.mBonusTime, aFile, aNow) == 0)) &&
		(WriteField('D', 4, &gConfig.mBonusScore, aFile, aNow) == 0))))
	{
		for (uint32_t i = 0; i < 0x44; i = i + 1)
		{
			if (WriteField('D', 1, &gConfig.mBonusQueue[i], aFile, aNow) != 0)
				goto fail;
		}
		for (uint32_t i = 0; i < 0x80; i = i + 1)
		{
			if (WriteField('D', 1, &gConfig.mBonusLayout[i], aFile, aNow) != 0)
				goto fail;
		}
		if (WriteField('D', 4, &gGameMode, aFile, aNow) == 0 &&
			WriteField('D', 1, &gBonusRound, aFile, aNow) == 0 &&
			WriteField('D', 4, &mPosX, aFile, aNow) == 0 &&
			WriteField('D', 4, &mPosY, aFile, aNow) == 0 &&
			WriteField('D', 4, &mBarLevel, aFile, aNow) == 0 &&
			WriteField('D', 4, &m138, aFile, aNow) == 0 &&
			WriteField('D', 1, &m13c, aFile, aNow) == 0 &&
			WriteField('D', 1, &m13d, aFile, aNow) == 0 &&
			WriteField('D', 1, &mCrossed, aFile, aNow) == 0 &&
			WriteField('D', 1, &m13f, aFile, aNow) == 0 &&
			WriteField('D', 4, &mSummaryEvents, aFile, aNow) == 0 &&
			WriteField('D', 4, &m144, aFile, aNow) == 0)
		{
			// the screen the game comes back to: the level or its summary, not the options or instructions over them
			int32_t* aScreen;
			if ((mPrevScreen == SCREEN_OPTIONS) || (mPrevScreen == SCREEN_INSTRUCTIONS))
				aScreen = &aPrevScreen;
			else
				aScreen = &mPrevScreen;
			if ((WriteField('D', 4, aScreen, aFile, aNow) == 0) && (mBoard->Save(aFile, aNow) == 0))
			{
				fclose(aFile);
				UserFileWritten();
				SetScreen(mPrevScreen);
				mPrevScreen = aPrevScreen;
				return 0;
			}
		}
	}
fail:
	// the field writers closed the file when they failed (the original closed it a second time here)
	UserRemove(GTC(0x0046ec28));
	SetScreen(mPrevScreen);
	mPrevScreen = aPrevScreen;
	return 1;
}

// 0x41f499: loads savegame.dat; the screen to go to (6 the level, 7 its summary), 0 on failure
uint32_t App::LoadGame()
{
	FILE* aFile = UserFopen(GTC(0x0046ec28), "rb");
	if (aFile != nullptr)
	{
		uint32_t aNow = GameTime();
		// the summary of the new game screen
		uint16_t aDay, aMonth, aYear;
		uint32_t aMode, aLevel, aChain, aScore;
		uint32_t aScreen;
		if (ReadField('D', 2, &aDay, aFile, aNow) == 0 &&
			ReadField('D', 2, &aMonth, aFile, aNow) == 0 &&
			ReadField('D', 2, &aYear, aFile, aNow) == 0 &&
			ReadField('D', 4, &aMode, aFile, aNow) == 0 &&
			ReadField('D', 4, &aLevel, aFile, aNow) == 0 &&
			ReadField('D', 4, &aChain, aFile, aNow) == 0 &&
			ReadField('D', 4, &aScore, aFile, aNow) == 0 &&
			ReadField('D', 4, &gConfig.mBonusTime, aFile, aNow) == 0 &&
			ReadField('D', 4, &gConfig.mBonusScore, aFile, aNow) == 0)
		{
			for (uint32_t i = 0; i < 0x44; i = i + 1)
			{
				if (ReadField('D', 1, &gConfig.mBonusQueue[i], aFile, aNow) != 0)
					return 0;
			}
			for (uint32_t i = 0; i < 0x80; i = i + 1)
			{
				if (ReadField('D', 1, &gConfig.mBonusLayout[i], aFile, aNow) != 0)
					return 0;
			}
			if (ReadField('D', 4, &gGameMode, aFile, aNow) == 0 &&
				ReadField('D', 1, &gBonusRound, aFile, aNow) == 0 &&
				ReadField('D', 4, &mPosX, aFile, aNow) == 0 &&
				ReadField('D', 4, &mPosY, aFile, aNow) == 0 &&
				ReadField('D', 4, &mBarLevel, aFile, aNow) == 0 &&
				ReadField('D', 4, &m138, aFile, aNow) == 0 &&
				ReadField('D', 1, &m13c, aFile, aNow) == 0 &&
				ReadField('D', 1, &m13d, aFile, aNow) == 0 &&
				ReadField('D', 1, &mCrossed, aFile, aNow) == 0 &&
				ReadField('D', 1, &m13f, aFile, aNow) == 0 &&
				ReadField('D', 4, &mSummaryEvents, aFile, aNow) == 0 &&
				ReadField('D', 4, &m144, aFile, aNow) == 0 &&
				ReadField('D', 4, &aScreen, aFile, aNow) == 0)
			{
				// the levels of the saved game's mode
				if (gGameMode == 0)
				{
					gLevelList = gLevels[0];
					gLevelListCount = gLevelCount[0];
				}
				else if (gGameMode == 1)
				{
					gLevelList = gLevels[2];
					gLevelListCount = gLevelCount[2];
				}
				else if (gGameMode == 2)
				{
					gLevelList = gLevels[1];
					gLevelListCount = gLevelCount[1];
				}
				else if (gGameMode == 3)
				{
					gLevelList = gLevels[3];
					gLevelListCount = gLevelCount[3];
				}
				if (mBoard->Load(aFile, aNow) == 0)
				{
					fclose(aFile);
					return aScreen;
				}
			}
		}
		// the field readers closed the file when they failed (the original closed it a second time here)
	}
	return 0;
}

// 0x420e97
Board* App::GetBoard()
{
	return mBoard;
}

namespace
{

char gPage = 1;                 // 0x477059 the page of the instructions (1-based)
Layer* gPauseLayer = nullptr;   // 0x489ef8 the layer of the pause image
char gPauseShown = 0;           // 0x489efc set while it is up (a CounterProc counter)

// A dialog of the whole screen: image theImage at (x, y) with an OK button (theOk at its offset theOkX/Y, command
// theCommand) and a Cancel button (theCancel, which closes it: Escape presses it). theRect is the original's one
// rectangle on the stack, left at the Cancel button.
Layer* NewConfirmDialog(GuiRect* theRect, int x, int y, int theImage, int theOk, int theOkX, int theOkY,
	uint32_t theCommand, int theCancel, int theCancelX, int theCancelY)
{
	*theRect = {0, 0, 0x280, 0x1e0};
	Layer* aLayer = NewLayer(0x280, 0x1e0, theRect);
	theRect->x = (int16_t)x;
	theRect->y = (int16_t)y;
	Handler* aHandler = NewHandler(ImageProc, aLayer, theRect, theImage, 0);
	aHandler->mPass = 4;
	theRect->x = (int16_t)theOkX + (int16_t)x;
	theRect->y = (int16_t)theOkY + (int16_t)y;
	aHandler = NewHandler(ButtonProc, aLayer, theRect, theOk, theCommand);
	aHandler->mPass = 5;
	theRect->x = (int16_t)theCancelX + (int16_t)x;
	theRect->y = (int16_t)theCancelY + (int16_t)y;
	aHandler = NewHandler(ButtonProc, aLayer, theRect, theCancel, 0xc000001d);
	aHandler->mPass = 5;
	return aLayer;
}

// The end of each dialog: it counts as modal and as an animation that holds the game, and pauses the game clock
void PushDialog(Layer* theLayer, GuiRect* theRect)
{
	NewHandler(CounterProc, theLayer, theRect, (uintptr_t)&gModal, 0);
	NewHandler(CounterProc, theLayer, nullptr, (uintptr_t)&gBusy, 0);
	NewHandler(PauseHolderProc, theLayer, nullptr, 1, 0);
	PushModal(theLayer);
}

// 0x425d56: "are you sure?" (OK sends theCommand)
void AreYouSureDialog(uint32_t theCommand)
{
	if (gModal == 0)
	{
		GuiRect aRect;
		Layer* aLayer = NewConfirmDialog(&aRect, gConfig.mAreyousureX, gConfig.mAreyousureY, 0x20, 0x21,
			gConfig.mAreyousureOkx, gConfig.mAreyousureOky, theCommand, 0x22,
			gConfig.mAreyousureCancelx, gConfig.mAreyousureCancely);
		PushDialog(aLayer, &aRect);
	}
}

// 0x425ea8: "save the game?" (OK sends theCommand)
void SaveGameDialog(uint32_t theCommand)
{
	if (gModal == 0)
	{
		GuiRect aRect;
		Layer* aLayer = NewConfirmDialog(&aRect, gConfig.mSavegameX, gConfig.mSavegameY, 0x72, 0x74,
			gConfig.mSavegameOkx, gConfig.mSavegameOky, theCommand, 0x73,
			gConfig.mSavegameCancelx, gConfig.mSavegameCancely);
		PushDialog(aLayer, &aRect);
	}
}

// 0x425ffa: "a new game deletes the saved one" (OK sends theCommand)
void DeleteGameDialog(uint32_t theCommand)
{
	if (gModal == 0)
	{
		GuiRect aRect;
		Layer* aLayer = NewConfirmDialog(&aRect, gConfig.mDeletegameX, gConfig.mDeletegameY, 0x75, 0x77,
			gConfig.mDeletegameOkx, gConfig.mDeletegameOky, theCommand, 0x76,
			gConfig.mDeletegameCancelx, gConfig.mDeletegameCancely);
		PushDialog(aLayer, &aRect);
	}
}

// 0x42614c: "are you sure?" with a register button in the trial, else 0x425d56
void AreYouSureRegDialog(uint32_t theCommand)
{
	if (gModal == 0)
	{
		if (gRegistered == 0)
		{
			GuiRect aRect;
			Layer* aLayer = NewConfirmDialog(&aRect, gConfig.mAreyousureregX, gConfig.mAreyousureregY, 0x70, 0x6d,
				gConfig.mAreyousureregOkx, gConfig.mAreyousureregOky, theCommand, 0x6e, gConfig.mAreyousureregCancelx,
				gConfig.mAreyousureregCancely);
			aRect.x = (int16_t)gConfig.mAreyousureregRegisterx + (int16_t)gConfig.mAreyousureregX;
			aRect.y = (int16_t)gConfig.mAreyousureregRegistery + (int16_t)gConfig.mAreyousureregY;
			Handler* aHandler = NewHandler(ButtonProc, aLayer, &aRect, 0x6f, 0xc000002f);
			aHandler->mPass = 5;
			PushDialog(aLayer, &aRect);
		}
		else
		{
			AreYouSureDialog(theCommand);
		}
	}
}

// 0x4262fa: "save the game?" with a register button in the trial, else 0x425ea8
void SaveGameRegDialog(uint32_t theCommand)
{
	if (gModal == 0)
	{
		if (gRegistered == 0)
		{
			GuiRect aRect;
			Layer* aLayer = NewConfirmDialog(&aRect, gConfig.mAreyousureregsaveX, gConfig.mAreyousureregsaveY, 0x7b,
				0x6d, gConfig.mAreyousureregsaveOkx, gConfig.mAreyousureregsaveOky, theCommand, 0x6e,
				gConfig.mAreyousureregsaveCancelx, gConfig.mAreyousureregsaveCancely);
			aRect.x = (int16_t)gConfig.mAreyousureregsaveRegisterx + (int16_t)gConfig.mAreyousureregsaveX;
			aRect.y = (int16_t)gConfig.mAreyousureregsaveRegistery + (int16_t)gConfig.mAreyousureregsaveY;
			Handler* aHandler = NewHandler(ButtonProc, aLayer, &aRect, 0x7e, 0xc000002f);
			aHandler->mPass = 5;
			PushDialog(aLayer, &aRect);
		}
		else
		{
			SaveGameDialog(theCommand);
		}
	}
}

// 0x4264a8: "play again?" (yes sends theYes, no theNo; Enter presses no)
void PlayAgainDialog(uint32_t theNo, uint32_t theYes)
{
	GuiRect aRect = {0, 0, 0x280, 0x1e0};
	Layer* aLayer = NewLayer(0x280, 0x1e0, &aRect);
	aRect.x = (int16_t)gConfig.mPlayagainX;
	aRect.y = (int16_t)gConfig.mPlayagainY;
	Handler* aHandler = NewHandler(ImageProc, aLayer, &aRect, 0x3e, 0);
	aHandler->mPass = 2;
	aRect.x = (int16_t)gConfig.mPlayagainX + (int16_t)gConfig.mPlayagainYesx;
	aRect.y = (int16_t)gConfig.mPlayagainY + (int16_t)gConfig.mPlayagainYesy;
	aHandler = NewHandler(ButtonProc, aLayer, &aRect, 0x40, theYes);
	aHandler->mPass = 3;
	aRect.x = (int16_t)gConfig.mPlayagainX + (int16_t)gConfig.mPlayagainNox;
	aRect.y = (int16_t)gConfig.mPlayagainY + (int16_t)gConfig.mPlayagainNoy;
	aHandler = NewHandler(ButtonProc, aLayer, &aRect, 0x22, theNo | 0x80000000);
	aHandler->mPass = 3;
	PushModal(aLayer);
}

// 0x41f922 cases 0x36 and 0x37: the original minimized a full screen window, waited 600 ms and opened theUrl with
// ShellExecute. The framework's OpenURL hands it to the system (and opens nothing in a test run).
void OpenBrowser(const char* theUrl)
{
	if (!Sexy::gSexyAppBase->OpenURL(theUrl))
		lowos::MessageBox_(GTC(0x0046ef24), GTC(0x0046c318));
}

// 0x41f922 cases 0x20-0x23 and 0x2d: a play of the trial is counted when a game starts (the original also kept the
// count, which the port does not)
void CountPlay()
{
	if (gConfig.mExecutionlimitmode == 0)
		gRunCount = gRunCount + 1;
}

} // namespace

// 0x426dd5: shows the pause image over the game while it is paused (and its own game buttons over it), or takes
// it away
void App::UpdatePause()
{
	if ((gApp.mScreen == SCREEN_GAME) || (gApp.mScreen == SCREEN_LEVELSUMMARY))
	{
		if ((gPauseFlags != 0) && (gPauseShown == 0))
		{
			if (InTopModal(gApp.mGameButtons[0]->mLayer) == 0)
				return;
			GuiRect aRect = {0, 0, 0x280, 0x1e0};
			gPauseLayer = NewLayer(0x280, 0x1e0, &aRect);
			aRect.x = (int16_t)gConfig.mBlockgridCenterX;
			aRect.y = (int16_t)gConfig.mBlockgridCenterY;
			Handler* aHandler = NewHandler(PopImageProc, gPauseLayer, &aRect, 0x69, 0x4000003c);
			aHandler->mPass = 2;
			NewHandler(CounterProc, gPauseLayer, nullptr, (uintptr_t)&gBusy, 0);
			NewHandler(PauseHolderProc, gPauseLayer, nullptr, 1, 0);
			NewHandler(MusicPauseProc, gPauseLayer, nullptr, 0, 0);
			NewHandler(CounterProc, gPauseLayer, nullptr, (uintptr_t)&gPauseShown, 0);
			PushModal(gPauseLayer);
			FreeHandler(gApp.mGameButtons[0]);
			FreeHandler(gApp.mGameButtons[1]);
			FreeHandler(gApp.mGameButtons[2]);
			FreeHandler(gApp.mGameButtons[3]);
			gApp.AddGameButtons(gPauseLayer);
			Send(gApp.mGameButtons[3], MSG_SETVALUE, 1, 0);
		}
		if (((gPauseFlags == 0) && (gPauseShown != 0)) && (InTopModal(gApp.mGameButtons[0]->mLayer) != 0))
		{
			FreeLayer(gPauseLayer);
			gPauseLayer = nullptr;
			gApp.AddGameButtons(gApp.mLayer);
			Send(gApp.mGameButtons[3], MSG_SETVALUE, 0, 0);
		}
	}
}

// 0x426fc7: in the trial, when no play is left: the "no more plays" dialog (quit or register). 1 if it is shown.
char App::TrialCheck()
{
	if ((gConfig.mExecutionlimit == 0) || (gRegistered != 0))
		return 0;
	uint32_t aLeft;
	if (gRunCount < (uint32_t)gConfig.mExecutionlimit)
		aLeft = gConfig.mExecutionlimit - gRunCount;
	else
		aLeft = 0;
	if (aLeft == 0)
	{
		GuiRect aRect = {0, 0, 0x280, 0x1e0};
		Layer* aLayer = NewLayer(0x280, 0x1e0, &aRect);
		aRect.x = (int16_t)gConfig.mSplashNomoreexecutionsx;
		aRect.y = (int16_t)gConfig.mSplashNomoreexecutionsy;
		Handler* aHandler = NewHandler(ImageProc, aLayer, &aRect, 0x6a, 0);
		aHandler->mPass = 1;
		aRect.x = (int16_t)gConfig.mSplashNmequitx + (int16_t)gConfig.mSplashNomoreexecutionsx;
		aRect.y = (int16_t)gConfig.mSplashNmequity + (int16_t)gConfig.mSplashNomoreexecutionsy;
		aHandler = NewHandler(ButtonProc, aLayer, &aRect, 0x6b, 0x19);
		aHandler->mPass = 1;
		aRect.x = (int16_t)gConfig.mSplashNmeregisterx + (int16_t)gConfig.mSplashNomoreexecutionsx;
		aRect.y = (int16_t)gConfig.mSplashNmeregistery + (int16_t)gConfig.mSplashNomoreexecutionsy;
		aHandler = NewHandler(ButtonProc, aLayer, &aRect, 0x6c, 0x30);
		aHandler->mPass = 1;
		PushModal(aLayer);
		gApp.mDialog = aLayer;
		return 1;
	}
	return 0;
}

// 0x41f922: what the buttons, check boxes, sliders and animations do (theCommand without its key flags)
uint32_t Command(Handler* theSource, uint32_t theCommand)
{
	switch (theCommand)
	{
	case 1:
		gApp.SetScreen(SCREEN_OPTIONS);
		break;
	case 2:
		// end the game: save it first?
		if (gApp.CanSave() == 0)
			AreYouSureDialog(3);
		else
			SaveGameDialog(3);
		break;
	case 3:
		FreeLayer(theSource->mLayer);
		gApp.SaveGame();
		gApp.SetScreen(SCREEN_MAINMENU);
		break;
	case 4:
		gApp.SetScreen(gApp.mPrevScreen);
		break;
	case 5:
		gPrefByte = Send(theSource, MSG_GETVALUE, 0, 0) != 0;
		break;
	case 6:
		if (Send(theSource, MSG_GETVALUE, 0, 0) == 0)
			App::ToggleFullScreen(2);
		else
			App::ToggleFullScreen(1);
		break;
	case 7:
		gSoundVolume = (uint16_t)Send(theSource, MSG_GETVALUE, 0, 0);
		Play(gSounds[0x2d], 2);
		Play(gSounds[0x2d], 3);
		break;
	case 8:
		BuildMusicVolumes(Send(theSource, MSG_GETVALUE, 0, 0) >> 8);
		break;
	case 9:
		FreeLayer(theSource->mLayer);
		gApp.mBoard->ResumeQueue();
		break;
	case 0xc:
		gPrefByte = Send(theSource, MSG_GETVALUE, 0, 0) != 0;
		break;
	case 0xd:
		// the high score's name was entered: the score goes into the mode's table
		gHighScoreEntered = 1;
		for (int i = 0; i < 5; i = i + 1)
		{
			if ((uint32_t)gHighScores[gGameMode][i].mScore < gApp.mBoard->GetScore())
			{
				if (i != 4)
					memmove(&gHighScores[gGameMode][i + 1], &gHighScores[gGameMode][i], (4 - i) * sizeof(HighScore));
				gHighScores[gGameMode][i].mScore = gApp.mBoard->GetScore();
				gHighScores[gGameMode][i].mLevel = gApp.mBoard->mState.mLevel;
				Send(gApp.mNameInput, MSG_SETVALUE, (uintptr_t)gHighScores[gGameMode][i].mName, 0);
				if (gHighScores[gGameMode][i].mName[0] == 0)
					strcpy(gHighScores[gGameMode][i].mName, GTC(0x0046c380));
				break;
			}
		}
		FreeLayer(theSource->mLayer);
		break;
	case 0xe:
		gPage = gPage + 1;
		break;
	case 0xf:
		if (gPage != 0)
			gPage = gPage - 1;
		break;
	case 0x10:
		gApp.SetScreen(gApp.mPrevScreen);
		gPage = 1;
		break;
	case 0x11:
		gApp.SetScreen(gApp.mPrevScreen);
		break;
	case 0x12:
		AreYouSureDialog(0x13);
		break;
	case 0x13:
		FreeLayer(theSource->mLayer);
		ResetHighScores();
		break;
	case 0x14:
		gApp.SetScreen(SCREEN_NEWGAME);
		break;
	case 0x15:
		gApp.SetScreen(SCREEN_OPTIONS);
		break;
	case 0x16:
		gApp.SetScreen(SCREEN_INSTRUCTIONS);
		break;
	case 0x17:
		gApp.SetScreen(SCREEN_CREDITS);
		break;
	case 0x18:
		// quit: save the game first?
		if (gApp.CanSave() == 0)
		{
			if (gApp.mDialog == nullptr)
				AreYouSureRegDialog(0x19);
			else
				AreYouSureDialog(0x19);
		}
		else
		{
			SaveGameRegDialog(0x19);
		}
		break;
	case 0x19:
		gApp.SaveGame();
		gQuit = 1;
		break;
	case 0x1a:
		gApp.GetBoard()->EndGame();
		FreeHandler(gApp.mCounted[3]);
		break;
	case 0x1b:
		NewHandler(GameOverDelayProc, theSource->mLayer, nullptr, 0, 0);
		FreeHandler(gApp.mCounted[2]);
		break;
	case 0x1c:
	{
		// the original left the size unset
		GuiRect aRect = {0x140, 0xf0, 0, 0};
		Handler* aHandler = NewHandler(SlideImageProc, theSource->mLayer, &aRect, 0x1a, 0x1c);
		aHandler->mPass = 2;
		break;
	}
	case 0x1d:
		FreeLayer(theSource->mLayer);
		break;
	case 0x1e:
		gQuit = 1;
		break;
	case 0x1f:
		gApp.SetScreen(gApp.mPrevScreen);
		break;
	case 0x20:
		if (gApp.TrialCheck() == 0)
		{
			CountPlay();
			gLevelList = gLevels[0];
			gLevelListCount = gLevelCount[0];
			gGameMode = 0;
			gApp.NewGame();
			gApp.SetScreen(SCREEN_GAME);
		}
		break;
	case 0x21:
		if (gApp.TrialCheck() == 0)
		{
			CountPlay();
			gLevelList = gLevels[1];
			gLevelListCount = gLevelCount[1];
			gGameMode = 2;
			gApp.NewGame();
			gApp.SetScreen(SCREEN_GAME);
		}
		break;
	case 0x22:
		if (gApp.TrialCheck() == 0)
		{
			CountPlay();
			gLevelList = gLevels[2];
			gLevelListCount = gLevelCount[2];
			gGameMode = 1;
			gApp.NewGame();
			gApp.SetScreen(SCREEN_GAME);
		}
		break;
	case 0x23:
		if (gApp.TrialCheck() == 0)
		{
			CountPlay();
			gLevelList = gLevels[3];
			gLevelListCount = gLevelCount[3];
			gGameMode = 3;
			gApp.NewGame();
			gApp.SetScreen(SCREEN_GAME);
		}
		break;
	case 0x24:
	{
		// continue the saved game (then it is deleted)
		if (gConfig.mExecutionlimitAllowcontinue == 0)
		{
			if (gApp.TrialCheck() != 0)
				return 0;
			CountPlay();
		}
		uint32_t aScreen = gApp.LoadGame();
		if (aScreen == 0)
		{
			gApp.SetScreen(SCREEN_NONE);
			gApp.SetScreen(SCREEN_NEWGAME);
		}
		else
		{
			gApp.SetScreen(SCREEN_NONE);
			gApp.SetScreen(aScreen);
		}
		UserRemove(GTC(0x0046ec28));
		break;
	}
	case 0x25:
		DeleteGameDialog(0x20);
		break;
	case 0x26:
		DeleteGameDialog(0x21);
		break;
	case 0x27:
		DeleteGameDialog(0x22);
		break;
	case 0x28:
		DeleteGameDialog(0x23);
		break;
	case 0x29:
		gApp.SetScreen(SCREEN_MAINMENU);
		break;
	case 0x2a:
		if (gApp.mPrevScreen == SCREEN_MAINMENU)
			gApp.SetScreen(SCREEN_MAINMENU);
		else if (gModal == 0)
			PlayAgainDialog(0x2c, 0x2d);
		break;
	case 0x2b:
		gApp.SetScreen(SCREEN_HIGHSCORES);
		break;
	case 0x2c:
		gApp.SetScreen(SCREEN_MAINMENU);
		break;
	case 0x2d:
		// play again, in the same mode
		if (gApp.TrialCheck() == 0)
		{
			CountPlay();
			gApp.NewGame();
			gApp.SetScreen(SCREEN_GAME);
		}
		break;
	case 0x2e:
	{
		// the load bar is full: the splash screen's play button
		GuiRect aRect = At(gConfig.mSplashPlayx, gConfig.mSplashPlayy);
		Handler* aHandler = NewHandler(ButtonProc, theSource->mLayer, &aRect, 0x4e, 0x80000029);
		aHandler->mPass = 1;
		break;
	}
	case 0x2f:
		FreeLayer(theSource->mLayer);
		// fall through
	case 0x30:
		// 0x30-0x33: the registration dialog and its code check, which the port leaves out: it plays as registered,
		// so nothing opens the dialog
		break;
	case 0x34:
		FreeLayer(theSource->mLayer);
		break;
	case 0x35:
		FreeLayer(theSource->mLayer);
		break;
	case 0x36:
		OpenBrowser(gConfig.mRegisterUrl);
		break;
	case 0x37:
		OpenBrowser(gConfig.mPublisherUrl);
		break;
	case 0x38:
	{
		// the game over screen's play again button, and the name of a new high score
		GuiRect aRect = At(gConfig.mSummaryPlayagainx, gConfig.mSummaryPlayagainy);
		Handler* aHandler = NewHandler(ButtonProc, gApp.mLayer, &aRect, 0x5d, 0x8000002d);
		aHandler->mPass = 2;
		uint32_t i = 0;
		do
		{
			if (((uint32_t)gHighScores[gGameMode][i].mScore < gApp.mBoard->GetScore()) && (gHighScoreEntered == 0))
			{
				uint16_t aWidth, aHeight;
				lowos::GetDisplayFormat(nullptr, &aWidth, &aHeight);
				aRect = At(0, 0);
				Layer* aLayer = NewLayer(aWidth, aHeight, &aRect);
				aRect = At(gConfig.mHighscoresBgx, gConfig.mHighscoresBgy);
				aHandler = NewHandler(ImageProc, aLayer, &aRect, 0x12, 0);
				aHandler->mPass = 2;
				aRect = At(gConfig.mHighscoresTextx, gConfig.mHighscoresTexty - 0x3c);
				aRect.w = 600;
				aRect.h = 0x78;
				gApp.mNameInput = NewHandler(TextInputProc, aLayer, &aRect, (uintptr_t)gScreenFont, 0xa000d);
				gApp.mNameInput->mPass = 3;
				aRect = At(gConfig.mHighscoresOkx, gConfig.mHighscoresOky);
				aHandler = NewHandler(ButtonProc, aLayer, &aRect, 0x13, 0xc000000d);
				aHandler->mPass = 3;
				PushModal(aLayer);
				return 0;
			}
			i = i + 1;
		} while (i != 5);
		break;
	}
	case 0x39:
		// the level's start animation is over (a bonus round has two)
		if ((gBonusRound == 0) || (gApp.m144 = gApp.m144 + 1, 1 < gApp.m144))
		{
			FreeHandler(gApp.mCounted[1]);
			if (gPrefByte == 0)
			{
				gApp.mBoard->ResumeQueue();
			}
			else if (TipsDialog() != 0)
			{
				gApp.mBoard->ResumeQueue();
			}
		}
		break;
	case 0x3a:
		FreeHandler(gApp.mCounted[2]);
		if (gApp.mBoard->mField->BlowAway() != 0)
			gApp.mSummaryEvents = gApp.mSummaryEvents + 1;
		break;
	case 0x3b:
		gApp.NextSummaryEvent();
		break;
	case 0x3c:
		// the pause image was clicked
		if (gPauseFlags == 0)
			return 0;
		gPauseFlags = gPauseFlags & 0xfe;
		if (gPauseFlags != 0)
			return 0;
		App::UpdatePause();
		return 1;
	case 0x3d:
		Dispatch(MSG_PASTE, 0, 0);
		break;
	case 0x3e:
		gPauseFlags = gPauseFlags ^ 1;
		App::UpdatePause();
	}
	return 0;
}

namespace
{

// The items of a page of text (the tips' and the pager's): "@TEXT x y width height words..." up to the next item
// mark. The words stay in the resource: the end of the last one is overwritten with a terminator, as the original
// did. Returns the text and sets *thePos after it; 0 if the resource ended first.
const char* ReadTextItem(const char** thePos, GuiRect* theRect, int theX, int theY, bool isTip)
{
	if (SkipToToken(thePos, 0) != 0)
		return nullptr;
	ReadToken(thePos);
	theRect->x = (int16_t)TokenInt() + (int16_t)theX;
	if (SkipToToken(thePos, 0) != 0)
		return nullptr;
	ReadToken(thePos);
	theRect->y = (int16_t)TokenInt() + (int16_t)theY;
	if (SkipToToken(thePos, 0) != 0)
		return nullptr;
	ReadToken(thePos);
	theRect->w = (int16_t)TokenInt();
	if (SkipToToken(thePos, 0) != 0)
		return nullptr;
	ReadToken(thePos);
	theRect->h = (int16_t)TokenInt();
	int aDone = SkipToToken(thePos, 0);
	const char* aText = *thePos;
	if (aDone != 0)
		return nullptr;
	// the original left the end unset when the first token is already an item mark (it never is)
	const char* anEnd = *thePos;
	do
	{
		ReadToken(thePos);
		if ((SDL_strcasecmp(Token(), GTC(0x0046ef58)) == 0) || (SDL_strcasecmp(Token(), GTC(0x0046ef50)) == 0) ||
			(isTip && (SDL_strcasecmp(Token(), GTC(0x0046ef60)) == 0)))
		{
			*(char*)anEnd = 0;
			break;
		}
		anEnd = *thePos;
		aDone = SkipToToken(thePos, 0);
	} while (aDone == 0);
	*thePos = anEnd + 1;
	return aText;
}

} // namespace

// 0x420ea9: the tip of the level about to start (or of the bonus round, the first time), from text 0x12: "@TIP mode
// level" or "@TIP B" followed by its texts and images. 1 if there is none (the game goes on), 0 if it is shown (its
// OK button sends 9).
uint32_t TipsDialog()
{
	const uint8_t kModes[4] = {'N', 'T', 'S', 'R'};
	GuiRect aRect = {0, 0, 0x280, 0x1e0};
	Layer* aLayer = NewLayer(0x280, 0x1e0, &aRect);
	NewHandler(ResourceProc, aLayer, nullptr, 3, 0x12);
	const char* aPos = (const char*)res::Get(res::GROUP_TEXTS, 0x12);
	if (aPos == nullptr)
	{
		FreeLayer(aLayer);
		return 1;
	}
	uint8_t aKind;
	uint32_t aMode = 0;
	int32_t aLevel = 0;
	do
	{
		while (true)
		{
			do
			{
				if (SkipToToken(&aPos, 0) != 0)
				{
					FreeLayer(aLayer);
					return 1;
				}
				ReadToken(&aPos);
			} while (SDL_strcasecmp(Token(), GTC(0x0046ef60)) != 0);
			SkipToToken(&aPos, 0);
			ReadToken(&aPos);
			aKind = (uint8_t)TokenChar(0);
			if (aKind != 'B')
			{
				SkipToToken(&aPos, 0);
				ReadToken(&aPos);
				aLevel = TokenInt();
				for (aMode = 0; (aMode < 4 && (aKind != kModes[aMode])); aMode = aMode + 1)
				{
				}
			}
			if ((aKind == 'B') || (gBonusRound != 0))
				break;
			if ((aMode == gGameMode) && (aLevel == (int32_t)gApp.mBoard->mState.mLevel))
				goto found;
		}
	} while ((gBonusRound == 0) || ((aKind != 'B' || (gApp.m13d != 0))));
	// the bonus round's tip is shown once
	gApp.m13d = 1;
found:
	while (SkipToToken(&aPos, 0) == 0)
	{
		ReadToken(&aPos);
		if (SDL_strcasecmp(Token(), GTC(0x0046ef58)) == 0)
		{
			const char* aText = ReadTextItem(&aPos, &aRect, gConfig.mTipsBackgroundx, gConfig.mTipsBackgroundy, true);
			if (aText == nullptr)
				break;
			Layer* aTextLayer = NewLayer(aRect.w, aRect.h, &aRect);
			Handler* aHandler = NewHandler(TextAreaProc, aTextLayer, nullptr, (uintptr_t)aText, (uintptr_t)gScreenFont);
			aHandler->mPass = 3;
			if (aTextLayer->mVScroll != nullptr)
				aTextLayer->mVScroll->mPass = 3;
			AddChildLayer(aLayer, aTextLayer);
			continue;
		}
		if (SDL_strcasecmp(Token(), GTC(0x0046ef50)) == 0)
		{
			if (SkipToToken(&aPos, 0) != 0)
				break;
			ReadToken(&aPos);
			aRect.x = (int16_t)TokenInt() + (int16_t)gConfig.mTipsBackgroundx;
			if (SkipToToken(&aPos, 0) != 0)
				break;
			ReadToken(&aPos);
			aRect.y = (int16_t)TokenInt() + (int16_t)gConfig.mTipsBackgroundy;
			if (SkipToToken(&aPos, 0) != 0)
				break;
			ReadToken(&aPos);
			int32_t anImage = TokenInt();
			Handler* aHandler = NewHandler(ImageProc, aLayer, &aRect, (anImage + 0x8c) | 0x10000, 0);
			aHandler->mPass = 3;
			continue;
		}
		if (SDL_strcasecmp(Token(), GTC(0x0046ef60)) == 0)
			break;
	}
	aRect.x = (int16_t)gConfig.mTipsBackgroundx;
	aRect.y = (int16_t)gConfig.mTipsBackgroundy;
	Handler* aHandler = NewHandler(ImageProc, aLayer, &aRect, 0x1b, 0);
	aHandler->mPass = 1;
	aRect.x = (int16_t)gConfig.mTipsOkx + (int16_t)gConfig.mTipsBackgroundx;
	aRect.y = (int16_t)gConfig.mTipsOky + (int16_t)gConfig.mTipsBackgroundy;
	aHandler = NewHandler(ButtonProc, aLayer, &aRect, 0x1c, 0xc0000009);
	aHandler->mPass = 2;
	aRect.x = (int16_t)gConfig.mTipsTogglex + (int16_t)gConfig.mTipsBackgroundx;
	aRect.y = (int16_t)gConfig.mTipsToggley + (int16_t)gConfig.mTipsBackgroundy;
	aHandler = NewHandler(CheckBoxProc, aLayer, &aRect, 0x1f, 0xc);
	Send(aHandler, MSG_SETVALUE, 1, 0);
	aHandler->mPass = 2;
	NewHandler(CounterProc, aLayer, nullptr, (uintptr_t)&gBusy, 0);
	PushModal(aLayer);
	return 0;
}

namespace
{

uint32_t gPagerKind = 0;               // 0x489c24 0 the instructions, 1 the credits
Layer* gPagerLayer = nullptr;          // 0x489c28 the screen's layer
const char* gPagerText = nullptr;      // 0x489c2c
Handler* gPagerHandlers[0x80];         // 0x489c30 the page's texts and images
Layer* gPagerLayers[0x20];             // 0x489e30 the texts' layers
uint32_t gPagerPage = 0;               // 0x489eb0 the page shown (0 none)

// The text resource of a page: the instructions' pages are texts 1.., the credits text 0
int PageText(uint32_t thePage)
{
	return ((gPagerKind == 0) - 1) + (int)thePage;
}

} // namespace

// 0x4213f3: the pages of the instructions (gPage, which wraps around) or the credits. Create: a = the screen's layer,
// b = 0 the instructions, 1 the credits.
uint32_t PagerProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		gPagerPage = 0;
		memset(gPagerLayers, 0, sizeof(gPagerLayers));
		memset(gPagerHandlers, 0, sizeof(gPagerHandlers));
		gPagerText = nullptr;
		gPagerLayer = (Layer*)theA;
		gPagerKind = (uint32_t)theB;
	}
	else if (aMsg == MSG_UPDATE)
	{
		uint8_t aPage = (gPagerKind == 0) ? gPage : 1;
		if (aPage != gPagerPage)
		{
			for (uint32_t i = 0; i < 0x80; i = i + 1)
			{
				FreeHandler(gPagerHandlers[i]);
				gPagerHandlers[i] = nullptr;
			}
			for (uint32_t i = 0; i < 0x20; i = i + 1)
			{
				FreeLayer(gPagerLayers[i]);
				gPagerLayers[i] = nullptr;
			}
			if (gPagerPage != 0)
				res::Free(res::GROUP_TEXTS, 1, PageText(gPagerPage));
			gPagerPage = aPage;
			uint32_t aLayerCount = 0;
			uint32_t aHandlerCount = 0;
			if ((aPage == 0) || (res::Load(res::GROUP_TEXTS, 1, PageText(aPage)) != 0))
			{
				// before the first page: the last; after the last: the first
				if ((gPagerKind == 0) && (aPage != 0))
					aPage = 1;
				if ((gPagerKind == 0) && (aPage == 0))
					aPage = 0x10;
				if (gPagerKind == 1)
					aPage = 1;
				while (res::Load(res::GROUP_TEXTS, 1, PageText(aPage)) != 0)
				{
					// the original passed the text as the caption and the caption as the text
					if (gPagerKind == 1)
					{
						lowos::MessageBox_(GTC(0x0046c318), GTC(0x0046ef84));
						return 0;
					}
					if (aPage == 1)
					{
						lowos::MessageBox_(GTC(0x0046c318), GTC(0x0046ef68));
						return 0;
					}
					aPage = aPage - 1;
				}
				if (gPagerKind == 0)
					gPage = aPage;
				gPagerPage = aPage;
			}
			if (gPagerKind == 0)
			{
				// the previous page button, from the second page on
				if (gPage == 1)
				{
					if (gApp.mInstrPrev != nullptr)
					{
						FreeHandler(gApp.mInstrPrev);
						gApp.mInstrPrev = nullptr;
					}
				}
				else if (gApp.mInstrPrev == nullptr)
				{
					// the original left the size unset (the button takes its image's)
					GuiRect aRect = At(gConfig.mInstructionsPrevx, gConfig.mInstructionsPrevy);
					gApp.mInstrPrev = NewHandler(ButtonProc, gApp.mLayer, &aRect, 0x16, 0xf);
					gApp.mInstrPrev->mPass = 2;
				}
			}
			gPagerText = (const char*)res::Get(res::GROUP_TEXTS, PageText(aPage));
			const char* aPos = gPagerText;
			GuiRect aRect = {0, 0, 0, 0};
			while (SkipToToken(&aPos, 0) == 0)
			{
				ReadToken(&aPos);
				if (SDL_strcasecmp(Token(), GTC(0x0046ef58)) == 0)
				{
					const char* aText = ReadTextItem(&aPos, &aRect, 0, 0, false);
					if (aText == nullptr)
						return 0;
					gPagerLayers[aLayerCount] = NewLayer(aRect.w, aRect.h, &aRect);
					gPagerHandlers[aHandlerCount] = NewHandler(TextAreaProc, gPagerLayers[aLayerCount], nullptr,
						(uintptr_t)aText, (uintptr_t)gScreenFont);
					gPagerHandlers[aHandlerCount]->mPass = 3;
					if (gPagerLayers[aLayerCount]->mVScroll != nullptr)
						gPagerLayers[aLayerCount]->mVScroll->mPass = 3;
					aLayerCount = aLayerCount + 1;
					aHandlerCount = aHandlerCount + 1;
				}
				else if (SDL_strcasecmp(Token(), GTC(0x0046ef50)) == 0)
				{
					if (SkipToToken(&aPos, 0) != 0)
						return 0;
					ReadToken(&aPos);
					aRect.x = (int16_t)TokenInt();
					if (SkipToToken(&aPos, 0) != 0)
						return 0;
					ReadToken(&aPos);
					aRect.y = (int16_t)TokenInt();
					if (SkipToToken(&aPos, 0) != 0)
						return 0;
					ReadToken(&aPos);
					int32_t anImage = TokenInt();
					gPagerHandlers[aHandlerCount] = NewHandler(ImageProc, gPagerLayer, &aRect,
						(anImage + 0x8c) | 0x10000, 0);
					gPagerHandlers[aHandlerCount]->mPass = 3;
					aHandlerCount = aHandlerCount + 1;
				}
			}
		}
	}
	else if ((aMsg == MSG_CLASSFREE) && (gPagerPage != 0))
	{
		res::Free(res::GROUP_TEXTS, 1, PageText(gPagerPage));
	}
	return 0;
}

namespace
{

// 0x47e260: the ms a score message's word waits before its score rises, by word
const uint32_t kWordDelays[10] = {0, 100, 200, 300, 400, 500, 600, 700, 800, 500};

// The rise of a score message after theTime ms (0..1000)
uint32_t RiseOf(uint32_t theTime)
{
	uint32_t aRise = (uint32_t)(int64_t)(pow((double)(int64_t)theTime / 1000.0,
		(double)gConfig.mScoremessageMoveexponent) * 1000.0);
	if (1000 < aRise)
		aRise = 1000;
	return aRise;
}

} // namespace

// 0x4219c6: a score message rising and fading: its word (a letter of the score font) first, then the score. Create:
// a = the score | the word << 24 (word 9 shows only the score), b = the font.
uint32_t ScoreMessageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = theA;
		aSlots[1] = theB;
		aSlots[2] = GameTime();
		aSlots[3] = kWordDelays[theA >> 0x18];
		if (theA >> 0x18 == 9)
			aSlots[0] = aSlots[0] & 0xffffff;
		gEffectCount = gEffectCount + 1;
	}
	else if (aMsg == MSG_DESTROY)
	{
		gEffectCount = gEffectCount - 1;
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		if (aNow < aSlots[2])
			aSlots[2] = aNow;
		uint32_t aValue = (uint32_t)aSlots[0];
		uint32_t aTime;
		if (aSlots[3] < aNow - aSlots[2])
			aTime = (aNow - aSlots[2]) - aSlots[3];
		else
			aTime = 0;
		uint32_t anEnd;
		if (aValue >> 0x18 == 0)
			anEnd = 1000;
		else
			anEnd = gConfig.mScoremessageDelay + 1000 + aSlots[3];
		if (aTime < anEnd)
		{
			char aText[12];
			uint32_t aRise = RiseOf(aTime);
			GuiRect aRect;
			aRect.x = theHandler->mRect.x;
			aRect.y = theHandler->mRect.y - (int16_t)((uint32_t)(aRise * gConfig.mScoremessageDistance) / 1000);
			aRect.w = theHandler->mRect.w;
			aRect.h = theHandler->mRect.h;
			if ((aValue & 0xff000000) != 0)
			{
				// the word, then after the delay the score
				if (aTime < 1000)
				{
					GuiRect anExtent;
					aText[0] = (char)(aValue >> 0x18) + '@';
					aText[1] = 0;
					DrawText((Font*)aSlots[1], nullptr, 0, aText, &aRect, &anExtent, 0, 0,
						~(uint8_t)((aRise * 0xff) / 1000), 0x1c);
				}
				if (aTime < (uint32_t)gConfig.mScoremessageDelay)
					return 0;
				if (aSlots[3] < aTime - gConfig.mScoremessageDelay)
					aTime = (aTime - gConfig.mScoremessageDelay) - aSlots[3];
				else
					aTime = 0;
				aValue = aValue & 0xffffff;
			}
			aRise = RiseOf(aTime);
			aRect.y = theHandler->mRect.y - (int16_t)((uint32_t)(aRise * gConfig.mScoremessageDistance) / 1000);
			// MSVC's _itoa
			snprintf(aText + ((aValue & 0xff000000) != 0), sizeof(aText) - 1, "%u", aValue & 0xffffff);
			if ((aValue & 0xff000000) != 0)
				aText[0] = (char)(aValue >> 0x18) + '@';
			DrawText((Font*)aSlots[1], nullptr, 0, aText, &aRect, nullptr, 0, 0, ~(uint8_t)((aRise * 0xff) / 1000),
				0x1c);
		}
		else
		{
			FreeHandler(theHandler);
		}
	}
	return 0;
}

// 0x421d20: an image that pulses at its center for a second, then blinks; after 3 s it sends its command. Create:
// a = the image, b = the command.
uint32_t ZoomImageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		if (LoadSurface((int)theA, 9, (lowos::Surface**)&aSlots[0]) != 0)
			NoMemory();
		aSlots[1] = GameTime();
		aSlots[2] = theB;
		aSlots[3] = theA;
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeSurface((lowos::Surface*)aSlots[0]);
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		if (aNow < aSlots[1])
		{
			FreeHandler(theHandler);
			Command(theHandler, (uint32_t)aSlots[2]);
		}
		else
		{
			uint32_t aTime = aNow - (uint32_t)aSlots[1];
			lowos::Surface* anImage = (lowos::Surface*)aSlots[0];
			double anAmplitude;
			if (aTime < 1000)
			{
				anAmplitude = 1.0 - (double)(int64_t)aTime / 1000.0;
			}
			else
			{
				anAmplitude = 0.0;
				if (2999 < aTime)
				{
					FreeHandler(theHandler);
					Command(theHandler, (uint32_t)aSlots[2]);
					return 0;
				}
				aTime = aTime % 500;
				if (0x14c < aTime)
					return 0;
			}
			int32_t aDivisor = (int32_t)(int64_t)((1.0 - cos((double)(int64_t)aTime / 50.0) / 2.0 * anAmplitude) *
				(double)0x10000);
			uint16_t aWidth = (uint16_t)((int32_t)((uint32_t)(uint16_t)lowos::SurfaceWidth(anImage) << 0x10) / aDivisor);
			uint16_t aHeight = (uint16_t)((int32_t)((uint32_t)lowos::SurfaceHeight(anImage) << 0x10) / aDivisor);
			int16_t x = (int16_t)(theHandler->mRect.x - aWidth / 2);
			int16_t y = (int16_t)(theHandler->mRect.y - aHeight / 2);
			if (lowos::BlitScaled(aWidth, aHeight, (uint16_t)lowos::SurfaceWidth(anImage), lowos::SurfaceHeight(anImage),
				x, y, 0, 0, anImage) != 0)
			{
				// where scaling failed: the image at its size
				uint32_t aSrcWidth = lowos::SurfaceWidth(anImage) & 0xffff;
				uint16_t aSrcHeight = lowos::SurfaceHeight(anImage);
				lowos::BlitRegistered(aSrcWidth, aSrcHeight, theHandler->mRect.x - aSrcWidth / 2,
					theHandler->mRect.y - aSrcHeight / 2, 0, 0, anImage);
			}
		}
	}
	return 0;
}

// 0x421f9f: an image that slides in from the right, stays and slides out to the left (2.4 s), then sends its
// command. Create: a = the image, b = the command.
uint32_t SlideImageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		if (LoadSurface((int)theA, 9, (lowos::Surface**)&aSlots[0]) != 0)
			NoMemory();
		aSlots[1] = GameTime();
		aSlots[2] = theB;
		aSlots[3] = theA;
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeSurface((lowos::Surface*)aSlots[0]);
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		if (aNow < aSlots[1])
		{
			FreeHandler(theHandler);
			Command(theHandler, (uint32_t)aSlots[2]);
		}
		else
		{
			uint32_t aTime = aNow - (uint32_t)aSlots[1];
			lowos::Surface* anImage = (lowos::Surface*)aSlots[0];
			uint32_t aWidth = lowos::SurfaceWidth(anImage);
			uint16_t aHeight = lowos::SurfaceHeight(anImage);
			int16_t x = (int16_t)(theHandler->mRect.x - (uint16_t)aWidth / 2);
			int32_t aDistance = 0x280 - x;
			if (aDistance < (int32_t)((int32_t)x + (aWidth & 0xffff)))
				aDistance = (int32_t)x + (aWidth & 0xffff);
			if (aTime < 600)
				x = x + (int16_t)((int32_t)((600 - aTime) * aDistance) / 600);
			if (0x708 < aTime)
				x = x - (int16_t)((int32_t)((aTime - 0x708) * aDistance) / 600);
			if (aTime < 0x960)
			{
				lowos::BlitRegistered(aWidth & 0xffff, aHeight, x, (int16_t)(theHandler->mRect.y - aHeight / 2), 0, 0,
					anImage);
			}
			else
			{
				FreeHandler(theHandler);
				Command(theHandler, (uint32_t)aSlots[2]);
			}
		}
	}
	return 0;
}

// 0x42217f: the level's number dropping into the middle of the board and fading (1.8 s)
uint32_t LevelStartProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = GameTime();
	}
	else if (aMsg == MSG_DRAW)
	{
		char aText[12];
		snprintf(aText, sizeof(aText), GTC(0x0046efb8), (long)gApp.mBoard->mState.mLevel);
		GuiRect aSize;
		DrawText(gApp.mLevelFont, nullptr, 0, aText, nullptr, &aSize, 0, 0, 0xff, 2);
		int16_t x = (int16_t)((int16_t)((int16_t)gConfig.mBlockWidth * (int16_t)gConfig.mNumColumns - aSize.w) / 2 +
			(int16_t)gConfig.mBlockgridX);
		int16_t y = (int16_t)((int16_t)((int16_t)gConfig.mBlockHeight * (int16_t)gConfig.mNumRows - aSize.h) / 2 +
			(int16_t)gConfig.mBlockgridY);
		uint32_t aNow = GameTime();
		if (aNow < aSlots[0])
			aSlots[0] = aNow;
		uint32_t aTime = aNow - (uint32_t)aSlots[0];
		if (aTime < 0x709)
		{
			if (aTime < 0x2ee)
				y = (int16_t)((uint32_t)(aTime * aTime * ((int32_t)y + (uint32_t)(uint16_t)aSize.h)) / 0x89544) -
					aSize.h;
			uint8_t aLevel;
			if (aTime < 0x641)
				aLevel = 0xff;
			else
				aLevel = (uint8_t)(((0x708 - aTime) * 0xff) / 200);
			DrawText(gApp.mLevelFont, nullptr, 0, aText, nullptr, nullptr, x, y, aLevel, 0x10);
		}
		else
		{
			FreeHandler(theHandler);
		}
	}
	return 0;
}

namespace
{

// The brightness of a high score line, a sine wave that runs down the table
uint8_t Shimmer(double thePhase)
{
	return (uint8_t)(int64_t)((sin(thePhase) + 1.0) * 127.0);
}

// A line of a high score table: name, score and level in font 0x2e, and in font 0x2f over them at theLevel
void DrawHighScore(Font* const* theFonts, const HighScore* theScore, int theNameX, int theScoreX, int theLevelX, int y,
	uint8_t theLevel)
{
	char aText[12];
	snprintf(aText, sizeof(aText), "%d", theScore->mScore);
	DrawText(theFonts[0], nullptr, 0, theScore->mName, nullptr, nullptr, (int16_t)theNameX, (int16_t)y, 0, 0);
	DrawText(theFonts[0], nullptr, 0, aText, nullptr, nullptr, (int16_t)theScoreX, (int16_t)y, 0, 0x20);
	DrawText(theFonts[1], nullptr, 0, theScore->mName, nullptr, nullptr, (int16_t)theNameX, (int16_t)y, theLevel, 0x10);
	DrawText(theFonts[1], nullptr, 0, aText, nullptr, nullptr, (int16_t)theScoreX, (int16_t)y, theLevel, 0x30);
	snprintf(aText, sizeof(aText), "%d", theScore->mLevel);
	DrawText(theFonts[0], nullptr, 0, aText, nullptr, nullptr, (int16_t)theLevelX, (int16_t)y, 0, 0x20);
	DrawText(theFonts[1], nullptr, 0, aText, nullptr, nullptr, (int16_t)theLevelX, (int16_t)y, theLevel, 0x30);
}

// The two fonts of the high score tables (slots 0 and 1)
void LoadHighScoreFonts(uintptr_t* theSlots)
{
	theSlots[0] = (uintptr_t)LoadFont(0x2e, GTC(0x0046eca8), 4, 0, 0x40);
	theSlots[1] = (uintptr_t)LoadFont(0x2f, GTC(0x0046eca8), 4, 0, 0x40);
}

void FreeHighScoreFonts(uintptr_t* theSlots)
{
	FreeFont((Font*)theSlots[0]);
	FreeFont((Font*)theSlots[1]);
}

// A configuration error the game cannot go on with (the original's three calls)
void Fatal(uint32_t theText)
{
	lowos::ScreenScope(2);
	lowos::MessageBox_(GTC(theText), GTC(0x0046c318));
	exit(0);
}

} // namespace

// 0x42233b: the main menu's high scores: the best of each mode
uint32_t MenuHighScoresProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		LoadHighScoreFonts(aSlots);
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeHighScoreFonts(aSlots);
	}
	else if (aMsg == MSG_DRAW)
	{
		double aPhase = (double)(int64_t)GameTime() / 200.0;
		const int32_t* aLines = &gConfig.mMainmenuHsRegulary;
		for (int i = 0; i < 4; i = i + 1)
		{
			uint8_t aLevel = Shimmer(aPhase - (double)(int64_t)(uint32_t)aLines[i] / 50.0);
			DrawHighScore((Font* const*)aSlots, &gHighScores[i][0], gConfig.mMainmenuHsNamex, gConfig.mMainmenuHsScorex,
				gConfig.mMainmenuHsLevelx, (int16_t)aLines[i], aLevel);
		}
	}
	return 0;
}

// 0x4225a4: the high scores screen: the five of each mode
uint32_t HighScoresProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		LoadHighScoreFonts(aSlots);
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeHighScoreFonts(aSlots);
	}
	else if (aMsg == MSG_DRAW)
	{
		double aPhase = (double)(int64_t)GameTime() / 200.0;
		const int32_t* aTables = &gConfig.mHighscoresmenuNormaly;
		for (int i = 0; i < 4; i = i + 1)
		{
			int32_t anOffset = 0;
			for (int j = 0; j < 5; j = j + 1)
			{
				uint8_t aLevel = Shimmer(aPhase - ((double)(int64_t)(uint32_t)aTables[i] + (double)anOffset) / 50.0);
				DrawHighScore((Font* const*)aSlots, &gHighScores[i][j], gConfig.mHighscoresmenuNamex,
					gConfig.mHighscoresmenuScorex, gConfig.mHighscoresmenuLevelx, (int16_t)(aTables[i] + anOffset), aLevel);
				anOffset = anOffset + gConfig.mHighscoresmenuSkipy;
			}
		}
	}
	return 0;
}

// 0x422866: the game over screen's high scores of the mode played
uint32_t SummaryHighScoresProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		LoadHighScoreFonts(aSlots);
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeHighScoreFonts(aSlots);
	}
	else if (aMsg == MSG_DRAW)
	{
		double aPhase = (double)(int64_t)GameTime() / 200.0;
		uint32_t aMode = gGameMode;
		int32_t anOffset = 0;
		for (int j = 0; j < 5; j = j + 1)
		{
			aPhase = aPhase - 1.0;
			uint8_t aLevel = Shimmer(aPhase - (double)(int64_t)(uint32_t)(gConfig.mSummaryHsy + anOffset) / (double)50.0f);
			DrawHighScore((Font* const*)aSlots, &gHighScores[aMode][j], gConfig.mSummaryHsnamex, gConfig.mSummaryHsscorex,
				gConfig.mSummaryHslevelx, (int16_t)(gConfig.mSummaryHsy + anOffset), aLevel);
			anOffset = anOffset + gConfig.mSummaryHsskipy;
		}
	}
	return 0;
}

// 0x422afd: a menu animation: an image of frames of the rect's width, the first held for the low half of b, the
// others for its high half each, from a random start. Create: a = the image.
uint32_t MenuAnimationProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		if (LoadSurface((int)theA, 9, (lowos::Surface**)&aSlots[0]) != 0)
			NoMemory();
		theHandler->mRect.h = (int16_t)lowos::SurfaceHeight((lowos::Surface*)aSlots[0]);
		aSlots[1] = GameTime();
		aSlots[2] = theA;
		aSlots[3] = theB;
		if ((int)((lowos::SurfaceWidth((lowos::Surface*)aSlots[0]) & 0xffff) % (uint16_t)theHandler->mRect.w) != 0)
			Fatal(0x0046efd8);
		uint32_t aRandom = (uint32_t)Rand();
		if (aSlots[1] < aRandom)
			aSlots[1] = aSlots[1] + aRandom;
		else
			aSlots[1] = aSlots[1] - aRandom;
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeSurface((lowos::Surface*)aSlots[0]);
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		if (aNow < aSlots[1])
			aSlots[1] = aNow;
		uint32_t aFrames = (int)((lowos::SurfaceWidth((lowos::Surface*)aSlots[0]) & 0xffff) /
			(uint16_t)theHandler->mRect.w);
		uint32_t aTime = (aNow - (uint32_t)aSlots[1]) %
			((aSlots[3] & 0xffff) + (aFrames - 1) * ((uint32_t)aSlots[3] >> 0x10));
		uint32_t aFrame;
		if (aTime < (aSlots[3] & 0xffff))
			aFrame = 0;
		else
			aFrame = (aTime - (aSlots[3] & 0xffff)) / ((uint32_t)aSlots[3] >> 0x10) + 1;
		lowos::BlitRegistered((uint16_t)theHandler->mRect.w, (uint16_t)theHandler->mRect.h, theHandler->mRect.x,
			theHandler->mRect.y, aFrame * (uint16_t)theHandler->mRect.w, 0, (lowos::Surface*)aSlots[0]);
	}
	return 0;
}

// 0x422cf8: starts the music when it is destroyed or after the configured delay
uint32_t MusicStarterProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = GameTime();
	}
	else if (aMsg == MSG_DESTROY)
	{
		StartMusic();
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		if (aNow < aSlots[0])
			aSlots[0] = aNow;
		if ((uint32_t)gConfig.mMusicDelay < aNow - aSlots[0])
			FreeHandler(theHandler);
	}
	return 0;
}

// 0x422d6c: stops the game clock (gPaused[a]) while it lives
uint32_t PauseHolderProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		SetPause(1, (uint8_t)theA);
		theHandler->u.mSlots[0] = theA;
	}
	else if (aMsg == MSG_DESTROY)
	{
		SetPause(0, (uint8_t)theHandler->u.mSlots[0]);
	}
	return 0;
}

// 0x422daf: a combo animation centered on the rect's corner: frames of the rect's width, b ms each. Create: a = the
// image.
uint32_t ComboAnimProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = theA;
		aSlots[1] = theB;
		aSlots[2] = GameTime();
		theHandler->mRect.h = (int16_t)lowos::SurfaceHeight((lowos::Surface*)theA);
		theHandler->mRect.x = (int16_t)(theHandler->mRect.x - (uint16_t)theHandler->mRect.w / 2);
		theHandler->mRect.y = (int16_t)(theHandler->mRect.y - theHandler->mRect.h / 2);
		if ((int)((lowos::SurfaceWidth((lowos::Surface*)theA) & 0xffff) % (uint16_t)theHandler->mRect.w) != 0)
			Fatal(0x0046f018);
		gEffectCount = gEffectCount + 1;
	}
	else if (aMsg == MSG_DESTROY)
	{
		gEffectCount = gEffectCount - 1;
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		if (aNow < aSlots[2])
			aSlots[2] = aNow;
		lowos::Surface* anImage = (lowos::Surface*)aSlots[0];
		uint32_t aSrcX = ((aNow - (uint32_t)aSlots[2]) / (uint32_t)aSlots[1]) * (uint16_t)theHandler->mRect.w;
		if (aSrcX < (lowos::SurfaceWidth(anImage) & 0xffff))
		{
			lowos::BlitRegistered((uint16_t)theHandler->mRect.w, (uint16_t)theHandler->mRect.h, theHandler->mRect.x,
				theHandler->mRect.y, aSrcX, 0, anImage);
		}
		else
		{
			FreeHandler(theHandler);
		}
	}
	return 0;
}

// 0x422f33: a combo star thrown up and falling, with drag, while it animates and fades. Create: a = the image (frames
// of the rect's width), b = its kind (a group of 4 in COMBOSTAR_PARAMS: width, the most it waits, opacity, ms a frame).
uint32_t ComboStarProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	const double kDrag = 0.01;
	const double kGravity = 500.0;
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = theA;
		aSlots[1] = theB;
		aSlots[2] = GameTime();
		aSlots[3] = (uint32_t)Rand();
		theHandler->mRect.h = (int16_t)lowos::SurfaceHeight((lowos::Surface*)theA);
		theHandler->mRect.x = (int16_t)(theHandler->mRect.x - (uint16_t)theHandler->mRect.w / 2);
		theHandler->mRect.y = (int16_t)(theHandler->mRect.y - theHandler->mRect.h / 2);
		theHandler->mRect.x = (int16_t)(theHandler->mRect.x - ((int16_t)(Rand() % 0x1e) - 0xf));
		theHandler->mRect.y = (int16_t)(theHandler->mRect.y - (int16_t)(Rand() % 0x1e));
		if ((int)((lowos::SurfaceWidth((lowos::Surface*)theA) & 0xffff) % (uint16_t)theHandler->mRect.w) != 0)
			Fatal(0x0046f078);
		gParticleCount = gParticleCount + 1;
		gEffectCount = gEffectCount + 1;
	}
	else if (aMsg == MSG_DESTROY)
	{
		gParticleCount = gParticleCount - 1;
		gEffectCount = gEffectCount - 1;
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		if (aNow < aSlots[2])
			aSlots[2] = aNow;
		const int32_t* aParams = gConfig.mCombostarParams + aSlots[1] * 4;
		uint32_t aRandom = (uint32_t)aSlots[3];
		uint32_t aDelay = aRandom % (uint32_t)aParams[1];
		if (aNow - aSlots[2] < aDelay)
			return 0;
		uint32_t aTime = (aNow - (uint32_t)aSlots[2]) - aDelay;
		lowos::Surface* anImage = (lowos::Surface*)aSlots[0];
		int32_t aFrames = (int32_t)(uint16_t)lowos::SurfaceWidth(anImage) / (int32_t)(uint16_t)theHandler->mRect.w;
		uint32_t aSrcX = ((aTime / (uint32_t)aParams[3]) % (uint32_t)aFrames) * (uint16_t)theHandler->mRect.w;
		// the throw: its direction and speed from the random number
		double aSpeedX = cos((double)(int64_t)aRandom / 100.0);
		double aRise = sin((double)(int64_t)aRandom / 100.0) / 2.0;
		if (aRise < 0.0)
			aRise = -aRise;
		aSpeedX = (double)(int64_t)(aRandom % 200) * aSpeedX;
		aRise = ((double)(int64_t)aRandom + 32767.0) * aRise / 32767.0;
		double aSeconds = (double)(int64_t)aTime / 1000.0;
		// up and down with quadratic drag; sideways slowing down
		double y = log(cosh((aSeconds - aRise) * sqrt(kDrag * kGravity))) / kDrag;
		y = y - log(cosh(-aRise * sqrt(kDrag * kGravity))) / kDrag;
		double x = pow(0.3, aSeconds) * aSpeedX / log(0.3) - aSpeedX / log(0.3);
		uint8_t aLevel = 0xff;
		if (aTime < 300)
			aLevel = (uint8_t)((aTime * 0xff) / 300);
		uint32_t aFade = aRandom % 1000 + 500;
		if (aFade < aTime)
			aLevel = (uint8_t)(((aFade + 400 - aTime) * 0xff) / 400);
		if (aTime < aFade + 400)
		{
			lowos::BlitLevel((uint16_t)theHandler->mRect.w, (uint16_t)theHandler->mRect.h,
				theHandler->mRect.x + (int32_t)(int64_t)x, theHandler->mRect.y + (int32_t)(int64_t)y, aSrcX, 0, anImage,
				(uint8_t)((aLevel * (uint32_t)aParams[2]) / 0xff));
		}
		else
		{
			FreeHandler(theHandler);
		}
	}
	return 0;
}

namespace
{

lowos::Surface* gLoadBarImage = nullptr;   // 0x489ebc
uint32_t gLoadBarLoaded = 0;               // 0x489eb4 the entries preloaded by the bar
uint32_t gLoadBarCount = 0;                // 0x489eb8 the table's sounds, music and texts (types 4, 2 and 8)

bool IsPreloaded(const res::Entry& theEntry)
{
	return (theEntry.mType == 4) || (theEntry.mType == 2) || (theEntry.mType == 8);
}

} // namespace

// 0x4233c2: the splash screen's load bar: it preloads the sounds, music and texts one per step, and in the trial
// fills more slowly (the low half of b, ms). When full it sends the high half of b and loads the game's sounds.
// Create: a = the image.
uint32_t LoadBarProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = theA;
		aSlots[1] = theB;
		aSlots[2] = 0;
		aSlots[3] = 0;
		LoadSurface((int)theA, 1, &gLoadBarImage);
		theHandler->mRect.w = (int16_t)lowos::SurfaceWidth(gLoadBarImage);
		theHandler->mRect.h = (int16_t)lowos::SurfaceHeight(gLoadBarImage);
		gLoadBarCount = 0;
		for (gLoadBarLoaded = 0; res::GameTable()[gLoadBarLoaded].mFlags != 0; gLoadBarLoaded = gLoadBarLoaded + 1)
		{
			if (IsPreloaded(res::GameTable()[gLoadBarLoaded]))
				gLoadBarCount = gLoadBarCount + 1;
		}
		gLoadBarLoaded = 0;
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeSurface(gLoadBarImage);
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aTotal;
		if (gRegistered == 0)
			aTotal = aSlots[1] & 0xffff;
		else
			aTotal = 0;
		aTotal = aTotal + gLoadBarCount;
		uint32_t aStep = aTotal / gLoadBarCount;
		if (aSlots[2] == 0)
			aSlots[2] = GameTime();
		uint32_t aTime = GameTime();
		if (aTime < aSlots[2])
			aSlots[2] = aTime;
		// it waits while a dialog is over it
		if (InTopModal(theHandler->mLayer) == 0)
			aSlots[2] = aTime;
		aTime = aTime - (uint32_t)aSlots[2];
		if ((aStep < aTime) && (gLoadBarLoaded < gLoadBarCount))
		{
			for (; res::GameTable()[gPreloaded].mFlags != 0; gPreloaded = gPreloaded + 1)
			{
				const res::Entry& anEntry = res::GameTable()[gPreloaded];
				if (IsPreloaded(anEntry))
				{
					res::Load(anEntry.mGroup, 1, anEntry.mId);
					gLoadBarLoaded = gLoadBarLoaded + 1;
					gPreloaded = gPreloaded + 1;
					aTime = 0;
					aSlots[2] = GameTime();
					break;
				}
			}
		}
		if (gLoadBarLoaded == gLoadBarCount)
		{
			aTime = aTotal;
			if (aSlots[3] == 0)
			{
				Command(theHandler, (uint32_t)aSlots[1] >> 0x10);
				aSlots[3] = 1;
				LoadGameSounds();
			}
		}
		else
		{
			aTime = aTime + aStep * gLoadBarLoaded;
		}
		uint32_t aShown = aTotal;
		if (aTime <= aTotal)
			aShown = aTime;
		lowos::BlitRegistered(((uint16_t)theHandler->mRect.w * aShown) / aTotal, (uint16_t)theHandler->mRect.h,
			theHandler->mRect.x, theHandler->mRect.y, 0, 0, gLoadBarImage);
	}
	return 0;
}

namespace
{

char gSummaryText[0x14];                   // 0x489ec0
Font* gSummaryFont = nullptr;              // 0x489ed4
uint32_t gSummaryStats[5];                 // 0x489ed8 the game's time, level, best chain, best match and score

uint32_t gBonusMagnification = 0;          // 0x47e288
uint32_t gBonusCriticalTime1 = 0;          // 0x47e28c
uint32_t gBonusCriticalTime2 = 0;          // 0x47e290
Font* gBonusFont = nullptr;                // 0x489eec

} // namespace

// 0x4236d9: the game summary's statistics sliding in one after another (the time, the level reached, the best
// combo, the best match and the score); once the fourth has stopped it sends its command. Create: a = the command.
uint32_t SummaryStatsProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	(void)theB;
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	uint32_t aStarts[6];
	aStarts[0] = 0;
	aStarts[1] = 0x1c2;
	aStarts[2] = 900;
	aStarts[3] = 0x546;
	aStarts[4] = 0x708;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = GameTime();
		gSummaryStats[0] = gApp.mBoard->mField->mTotalTime;
		gSummaryStats[1] = gApp.mBoard->mState.mLevel;
		gSummaryStats[2] = gApp.mBoard->mField->mBestChain;
		gSummaryStats[3] = gApp.mBoard->mField->mBestMatch;
		gSummaryStats[4] = gApp.mBoard->GetScore();
		aSlots[1] = 0;
		aSlots[2] = theA;
		aSlots[3] = 0;
		gSummaryFont = LoadFont(0x5e, GTC(0x0046eca8), 4, 0, 0x40);
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeFont(gSummaryFont);
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		if (aNow < aSlots[0])
			aSlots[0] = aNow;
		aStarts[5] = aNow - (uint32_t)aSlots[0];
		gApp.DrawScoreLevel();
		uint32_t aFadeTime = (uint32_t)gConfig.mSummaryFademovetime;
		int32_t aDistance = gConfig.mSummaryMovedistance;
		aStarts[0] = 0;
		aStarts[1] = (uint32_t)gConfig.mSummaryDuration;
		aStarts[2] = (uint32_t)gConfig.mSummaryDuration << 1;
		aStarts[3] = (uint32_t)gConfig.mSummaryDuration * 3;
		aStarts[4] = (uint32_t)gConfig.mSummaryDuration << 2;
		for (int anIndex = 0; anIndex < 5; anIndex = anIndex + 1)
		{
			if (aStarts[anIndex] <= aStarts[5])
			{
				if (anIndex == 0)
				{
					// h:mm:ss, without a leading 0:0 or 0
					gSummaryText[7] = 0;
					gSummaryText[6] = (char)((gSummaryStats[0] / 1000) % 10) + '0';
					uint32_t aTime = (gSummaryStats[0] / 1000) / 10;
					gSummaryText[5] = (char)(aTime % 6) + '0';
					aTime = aTime / 6;
					gSummaryText[4] = ':';
					gSummaryText[3] = (char)(aTime % 10) + '0';
					aTime = aTime / 10;
					gSummaryText[2] = (char)(aTime % 6) + '0';
					aTime = aTime / 6;
					gSummaryText[1] = ':';
					gSummaryText[0] = (char)(aTime % 10) + '0';
					if ((gSummaryText[0] == '0') && (gSummaryText[2] == '0'))
						memmove(gSummaryText, gSummaryText + 3, 5);
					else if (gSummaryText[0] == '0')
						memmove(gSummaryText, gSummaryText + 2, 6);
				}
				else if (anIndex == 2)
				{
					if (gSummaryStats[2] < 2)
						strcpy(gSummaryText, GTC(0x0046ef08));
					else if (gSummaryStats[2] == 2)
						strcpy(gSummaryText, GTC(0x0046ef00));
					else if (gSummaryStats[2] == 3)
						strcpy(gSummaryText, GTC(0x0046eef8));
					else if (gSummaryStats[2] == 4)
						strcpy(gSummaryText, GTC(0x0046eef0));
					else if (gSummaryStats[2] == 5)
						snprintf(gSummaryText, sizeof(gSummaryText), GTC(0x0046eee4), (long)gSummaryStats[2]);
				}
				else
				{
					// MSVC's _itoa
					snprintf(gSummaryText, sizeof(gSummaryText), "%d", (int32_t)gSummaryStats[anIndex]);
				}
				GuiRect aRect;
				aRect.y = (int16_t)((int16_t)(&gConfig.mSummaryTimey)[anIndex] - 0x1e);
				aRect.h = 0x3c;
				aRect.x = (int16_t)(&gConfig.mSummaryTimex)[anIndex];
				aRect.w = 600;
				uint32_t aTime = aStarts[5] - aStarts[anIndex];
				if (aFadeTime < aTime)
					aTime = aFadeTime;
				uint32_t aLeft = (uint32_t)(int64_t)(pow((double)(int64_t)(aFadeTime - aTime) / (double)(int64_t)aFadeTime,
					(double)gConfig.mSummaryMotionexponent) * (double)(int64_t)aFadeTime);
				if (aFadeTime < aLeft)
					aLeft = aFadeTime;
				aRect.x = (int16_t)(aRect.x + (int16_t)((int32_t)(aLeft * aDistance) / (int32_t)aFadeTime));
				DrawText(gSummaryFont, nullptr, 0, gSummaryText, &aRect, nullptr, aRect.x, 0,
					(uint8_t)(((aFadeTime - aLeft) * 0xff) / aFadeTime), 0x18);
			}
		}
		if ((aSlots[3] != 0) && (gModal == 0) && (aSlots[1] == 0))
		{
			aSlots[1] = 1;
			Command(theHandler, (uint32_t)aSlots[2]);
		}
		if (aFadeTime + aStarts[3] < aStarts[5])
			aSlots[3] = 1;
	}
	return 0;
}

// 0x423bdd: the bonus round's seconds left: two digits that start large in the middle and shrink into their place.
// The time runs while the game is not paused or held.
uint32_t BonusTimerProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	(void)theA;
	(void)theB;
	(void)theHandler;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		gBonusCriticalTime1 = (uint32_t)gConfig.mBonustimerCriticaltime1;
		gBonusCriticalTime2 = (uint32_t)gConfig.mBonustimerCriticaltime2;
		gBonusMagnification = (uint32_t)gConfig.mBonustimerInitialmagnification;
		gApp.m134 = GameTime();
		gBonusFont = LoadFont(0x5f, GTC(0x0046eed8), 4, 0, 0x40);
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeFont(gBonusFont);
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		uint32_t aLast = aNow;
		if (gApp.m134 <= aNow)
			aLast = gApp.m134;
		if ((gApp.m13c == 0) && (gBusy == 0))
			gApp.m138 = gApp.m138 + (aNow - aLast);
		gApp.m134 = aNow;
		if (gApp.m138 != 0)
		{
			uint32_t aSeconds = 0;
			if (gApp.m138 / 1000 < (uint32_t)gConfig.mBonusTime)
				aSeconds = (uint32_t)gConfig.mBonusTime - gApp.m138 / 1000;
			uint8_t aTens = (uint8_t)((char)(aSeconds / 10) + '0');
			uint8_t anOnes = (uint8_t)((char)(aSeconds % 10) + '0');
			uint32_t aMagnification;
			int16_t x;
			int16_t y;
			if (gApp.m138 < gBonusCriticalTime1)
			{
				aMagnification = gBonusMagnification;
				x = (int16_t)gConfig.mBonusLargetimecountx;
				y = (int16_t)gConfig.mBonusLargetimecounty;
			}
			else if (gApp.m138 < gBonusCriticalTime2)
			{
				double aFraction = pow((double)(int64_t)(gApp.m138 - gBonusCriticalTime1) /
					(double)(int64_t)(gBonusCriticalTime2 - gBonusCriticalTime1),
					(double)gConfig.mBonustimerMotionexponent);
				y = (int16_t)gConfig.mBonusLargetimecounty;
				x = (int16_t)gConfig.mBonusLargetimecountx;
				aMagnification = gBonusMagnification;
				aMagnification = aMagnification -
					(uint32_t)(int64_t)((double)(int64_t)(uint32_t)(gBonusMagnification - 0x100) * aFraction);
				x = (int16_t)(x + (int16_t)(int64_t)((double)(int32_t)((int16_t)gConfig.mBonusTimecountx -
					(int16_t)gConfig.mBonusLargetimecountx) * aFraction));
				y = (int16_t)(y + (int16_t)(int64_t)((double)(int32_t)((int16_t)gConfig.mBonusTimecounty -
					(int16_t)gConfig.mBonusLargetimecounty) * aFraction));
			}
			else
			{
				x = (int16_t)gConfig.mBonusTimecountx;
				y = (int16_t)gConfig.mBonusTimecounty;
				aMagnification = 0x100;
			}
			lowos::Surface* aSurface = gBonusFont->mSurface;
			uint32_t aTensWidth = gBonusFont->mGlyphs[aTens].mWidth;
			uint32_t anOnesWidth = gBonusFont->mGlyphs[anOnes].mWidth;
			uint16_t aHeight = (uint16_t)(((uint32_t)(uint16_t)(lowos::SurfaceHeight(aSurface) - 1) * aMagnification) >> 8);
			x = (int16_t)(x - (uint16_t)((uint32_t)(uint16_t)(aTensWidth + anOnesWidth) * aMagnification >> 8) / 2);
			y = (int16_t)(y - aHeight / 2);
			lowos::BlitScaled((int)((aTensWidth * aMagnification) >> 8), aHeight, (int)aTensWidth,
				lowos::SurfaceHeight(aSurface) - 1, x, y, gBonusFont->mGlyphs[aTens].mX, 0, aSurface);
			lowos::BlitScaled((int)((anOnesWidth * aMagnification) >> 8), aHeight, (int)anOnesWidth,
				lowos::SurfaceHeight(aSurface) - 1, (int16_t)(x + (int16_t)((aTensWidth * aMagnification) >> 8)), y,
				gBonusFont->mGlyphs[anOnes].mX, 0, aSurface);
		}
	}
	return 0;
}

// 0x423fc7: an image that pops up at its center, wobbling, and then shrinks flat; it holds the game while it shows,
// and then sends its command. Create: a = the image, b = the command | 0x80000000 to stay 0.75 s longer | 0x40000000
// to stay until a click on the board or a key, then only shrink. Its rect's x and y are its center.
uint32_t PopImageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = RealTime();
		aSlots[1] = theB;
		aSlots[3] = 0;
		if (LoadSurface((int)theA, 9, (lowos::Surface**)&aSlots[2]) != 0)
			NoMemory();
		lowos::Surface* anImage = (lowos::Surface*)aSlots[2];
		theHandler->mRect.w = (int16_t)lowos::SurfaceWidth(anImage);
		theHandler->mRect.h = (int16_t)lowos::SurfaceHeight(anImage);
		gBusy = gBusy + 1;
		return 0;
	}
	if (aMsg == MSG_DESTROY)
	{
		FreeSurface((lowos::Surface*)aSlots[2]);
		gBusy = gBusy - 1;
		return 0;
	}
	if (aMsg == MSG_MOUSEDOWN)
	{
		// only a click on the board
		if ((uint32_t)theA < (uint32_t)gConfig.mBlockgridX)
			return 0;
		if ((uint32_t)theB < (uint32_t)gConfig.mBlockgridY)
			return 0;
		if ((uint32_t)(gConfig.mBlockgridX + gConfig.mBlockWidth * gConfig.mNumColumns) < (uint32_t)theA)
			return 0;
		if ((uint32_t)(gConfig.mBlockgridY + gConfig.mBlockHeight * gConfig.mNumRows) < (uint32_t)theB)
			return 0;
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aLonger = ((aSlots[1] & 0x80000000) != 0) ? 0x2ee : 0;
		uint32_t aPeriod = 1000;
		uint32_t aShrinkStart;
		uint32_t anEnd;
		if ((aSlots[1] & 0x40000000) == 0)
		{
			aShrinkStart = 0x5dc + aLonger;
			anEnd = 0x708 + aLonger;
		}
		else
		{
			aShrinkStart = 0;
			anEnd = 0x708 - 0x5dc;
		}
		uint32_t aNow = RealTime();
		uint32_t aStart = aNow;
		if ((uint32_t)aSlots[0] <= aNow)
			aStart = (uint32_t)aSlots[0];
		aSlots[0] = aStart;
		uint32_t aTime = aNow - (uint32_t)aSlots[0];
		uint32_t aWidthScale;
		uint32_t aHeightScale;
		if ((aTime < aShrinkStart) || ((aSlots[3] == 0) && ((aSlots[1] & 0x40000000) != 0)))
		{
			// a wobble around 128/256 that settles at full size after its first peak
			double aSeconds = (double)(int64_t)aTime / (double)(int64_t)aPeriod;
			double aScale = sin(aSeconds * 4.0) * 200.0 + 128.0;
			double aPeak = (3.141592 - asin((256.0 - 128.0) / 200.0)) / 4.0;
			if (aPeak < aSeconds)
			{
				double aDamping = 1.0 - (aSeconds - aPeak) * 6.0;
				if (aDamping < 0.0)
					aDamping = 0.0;
				aScale = (aScale - 256.0) * aDamping + 256.0;
			}
			aWidthScale = (uint32_t)(int64_t)aScale;
			aHeightScale = aWidthScale;
		}
		else
		{
			if (anEnd <= aTime)
			{
				if (gModal != 0)
					return 0;
				if (Command(theHandler, (uint32_t)aSlots[1] & 0x3fffffff) != 0)
					return 0;
				FreeHandler(theHandler);
				return 0;
			}
			double aFraction = pow((double)(int64_t)(aTime - aShrinkStart) / (double)(int64_t)(anEnd - aShrinkStart),
				1.5);
			aWidthScale = 0x100;
			aHeightScale = (uint32_t)(int64_t)(256.0 * aFraction);
			if (0x100 < aHeightScale)
				aHeightScale = 0x100;
			aHeightScale = 0x100 - aHeightScale;
		}
		if (aWidthScale == 0)
			return 0;
		if (aHeightScale == 0)
			return 0;
		uint32_t aHeight = (uint32_t)(uint16_t)theHandler->mRect.h * aHeightScale;
		lowos::BlitScaled((int)((uint32_t)(uint16_t)theHandler->mRect.w * aWidthScale >> 8), (int)(aHeight >> 8),
			(uint16_t)theHandler->mRect.w, (uint16_t)theHandler->mRect.h,
			(int)theHandler->mRect.x - (int)((uint32_t)(uint16_t)theHandler->mRect.w * aWidthScale >> 9),
			(int)theHandler->mRect.y - (int)(aHeight >> 9), 0, 0, (lowos::Surface*)aSlots[2]);
		return 0;
	}
	else if (aMsg != MSG_CHAR)
	{
		return 0;
	}
	// a click on the board or a key lets a waiting image shrink
	if (((aSlots[1] & 0x40000000) != 0) && (aSlots[3] == 0))
	{
		aSlots[3] = 1;
		aSlots[0] = RealTime();
	}
	return 0;
}

// 0x424421: the end of a bonus round: its image pops up while combo stars burst over the board (2.5 s), then a big
// star and the bonus's points at the image; at 5 s it sends command 0x3a. Create: a = the image.
uint32_t BonusEndProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	(void)theB;
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = GameTime();
		aSlots[1] = 0;
		aSlots[2] = 0;
		gBusy = gBusy + 1;
		Handler* anImage = NewHandler(PopImageProc, theHandler->mLayer, &theHandler->mRect, theA, 0x80000000);
		anImage->mPass = 2;
	}
	else if (aMsg == MSG_DESTROY)
	{
		gBusy = gBusy - 1;
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		uint32_t aStart = aNow;
		if ((uint32_t)aSlots[0] <= aNow)
			aStart = (uint32_t)aSlots[0];
		aSlots[0] = aStart;
		uint32_t aTime = aNow - (uint32_t)aSlots[0];
		if (aTime < 0x9c4)
		{
			// five stars a second somewhere on the board, away from its middle fifth
			while ((uint32_t)aSlots[1] < (aTime * 5) / 0x9c4)
			{
				uint32_t aFifthWidth = (uint32_t)(gConfig.mBlockWidth * gConfig.mNumColumns) / 5;
				uint32_t aFifthHeight = (uint32_t)(gConfig.mBlockHeight * gConfig.mNumRows) / 5;
				uint32_t x = (uint32_t)Rand();
				x = x % (aFifthWidth << 1);
				uint32_t y = (uint32_t)Rand();
				y = y % (aFifthHeight << 1);
				if (aFifthWidth < x)
					x = x + aFifthWidth;
				if (aFifthHeight < y)
					y = y + aFifthHeight;
				int16_t aGridX = (int16_t)gConfig.mBlockgridX;
				int16_t aGridY = (int16_t)gConfig.mBlockgridY;
				char aKind;
				if (gConfig.mCombostarParamsCount == 1)
					aKind = 0;
				else
					aKind = (char)((uint32_t)Rand() % (uint32_t)(gConfig.mCombostarParamsCount - 1));
				gApp.Starburst((int16_t)((int16_t)x + (int16_t)aFifthWidth + aGridX),
					(int16_t)((int16_t)y + (int16_t)aFifthHeight + aGridY), (char)(aKind + 1));
				aSlots[1] = aSlots[1] + 1;
			}
		}
		else if (2999 < aTime)
		{
			if (aTime < 5000)
			{
				if (aSlots[2] == 0)
				{
					aSlots[2] = 1;
					gApp.Starburst(theHandler->mRect.x, theHandler->mRect.y, (char)gConfig.mCombostarParamsCount);
					gApp.ScoreMessage(theHandler->mRect.x, theHandler->mRect.y, (uint32_t)gConfig.mBonusScore, 9, 1);
					gApp.mBoard->mField->mScore = gApp.mBoard->mField->mScore + gConfig.mBonusScore;
				}
			}
			else
			{
				Command(theHandler, 0x3a);
				FreeHandler(theHandler);
			}
		}
	}
	return 0;
}

// 0x4246b1: a completed level: after a delay, its image over the board with 24 sparkles around it (or, where the
// configuration turns it off, command 0x1a at once). It holds the game until the animations end. Create: a = the
// image, b = the sparkle's image.
uint32_t LevelCompleteProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = GameTime();
		aSlots[1] = theA;
		aSlots[2] = theB;
		Play(gSounds[0x7], 1);
		gBusy = gBusy + 1;
	}
	else if (aMsg == MSG_DESTROY)
	{
		gBusy = gBusy - 1;
	}
	else if (aMsg == MSG_UPDATE)
	{
		uint32_t aNow = GameTime();
		uint32_t aStart = aNow;
		if ((uint32_t)aSlots[0] <= aNow)
			aStart = (uint32_t)aSlots[0];
		aSlots[0] = aStart;
		uint32_t aTime = aNow - (uint32_t)aSlots[0];
		// it waits for the stars and particles to go
		if (((uint32_t)gConfig.mLevelcompleteDelay <= aTime) && (gEffectCount == 0))
		{
			// the original left the rect's size as the stack held it (the animation sizes itself)
			GuiRect aRect = {0, 0, 0, 0};
			AnimationParams aParams;
			if (gConfig.mLevelcompleteDisable == 0)
			{
				aRect.x = (int16_t)gConfig.mBlockgridCenterX;
				aRect.y = (int16_t)((int16_t)gConfig.mBlockgridCenterY + 0x28);
				aParams.mImage = (lowos::Surface*)aSlots[1];
				aParams.mDelay = 0;
				aParams.mRise = 0x50;
				aParams.mFade = 0x640;
				aParams.mHold = 800;
				Handler* anAnimation = NewHandler(AnimationProc, gApp.mLayer, &aRect, (uintptr_t)&aParams, 0x1001a);
				anAnimation->mPass = 2;
				lowos::Surface* anImage = (lowos::Surface*)aSlots[1];
				for (int anIndex = 0; anIndex < 0x18; anIndex = anIndex + 1)
				{
					aRect.x = (int16_t)gConfig.mBlockgridCenterX;
					aRect.x = (int16_t)(aRect.x - (int16_t)((lowos::SurfaceWidth(anImage) & 0xffff) / 2));
					int32_t aRandom = Rand();
					aRect.x = (int16_t)(aRect.x + (int16_t)(aRandom % (int32_t)(lowos::SurfaceWidth(anImage) & 0xffff)));
					aRect.y = (int16_t)((int16_t)gConfig.mBlockgridCenterY + 0x28);
					aRect.y = (int16_t)(aRect.y - lowos::SurfaceHeight(anImage) / 2);
					aRandom = Rand();
					aRect.y = (int16_t)(aRect.y + (int16_t)(aRandom % (int32_t)(uint32_t)lowos::SurfaceHeight(anImage)));
					aParams.mImage = (lowos::Surface*)aSlots[2];
					aParams.mDelay = (uint16_t)(Rand() % 0x6d6);
					aParams.mRise = 0x3c;
					aParams.mFade = 400;
					aParams.mHold = 0;
					anAnimation = NewHandler(AnimationProc, gApp.mLayer, &aRect, (uintptr_t)&aParams, 0x80010000);
					anAnimation->mPass = 3;
				}
				NewHandler(CounterProc, gApp.mLayer, nullptr, (uintptr_t)&gBusy, (uintptr_t)&gApp.mCounted[3]);
				FreeHandler(theHandler);
			}
			else
			{
				NewHandler(CounterProc, gApp.mLayer, nullptr, (uintptr_t)&gBusy, (uintptr_t)&gApp.mCounted[3]);
				FreeHandler(theHandler);
				Command(theHandler, 0x1a);
			}
		}
	}
	return 0;
}

namespace
{

uint32_t gParticleAngle = 0;               // 0x489ef0 the last particle's direction (0..0x7fff)

} // namespace

// 0x424963: a particle of an effect (a column nuke's or a color bomb's) flying straight out from its rect's center
// and sinking a little, fading in, holding (pulsing between two levels) and fading out. The particles of one effect
// spread evenly around the circle, each a little off at random. Create: a = the delay (ms), b = the Effect.
uint32_t ParticleProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		const Effect* anEffect = (const Effect*)theB;
		aSlots[0] = GameTime();
		aSlots[1] = theA;
		aSlots[2] = (uint32_t)(Rand() % 0x8000);
		aSlots[3] = theB;
		int16_t aSize = (int16_t)gConfig.mParticles[anEffect->mValue0].mValue0;
		theHandler->mRect.w = aSize;
		theHandler->mRect.h = aSize;
		theHandler->mRect.x = (int16_t)(theHandler->mRect.x - aSize / 2);
		theHandler->mRect.y = (int16_t)(theHandler->mRect.y - aSize / 2);
		aSlots[2] = (uint32_t)aSlots[2] / (uint32_t)anEffect->mValue2;
		aSlots[2] = (uint32_t)aSlots[2] + gParticleAngle;
		aSlots[2] = (uint32_t)aSlots[2] % 0x8000;
		aSlots[2] = (uint32_t)aSlots[2] + 0x8000;
		aSlots[2] = (uint32_t)aSlots[2] - 0x8000 / (uint32_t)(anEffect->mValue2 << 1);
		aSlots[2] = 0x8000 / (uint32_t)anEffect->mValue2 + (uint32_t)aSlots[2];
		aSlots[2] = (uint32_t)aSlots[2] % 0x8000;
		gParticleAngle = (uint32_t)aSlots[2];
		gParticleCount = gParticleCount + 1;
		gEffectCount = gEffectCount + 1;
	}
	else if (aMsg == MSG_DESTROY)
	{
		gParticleCount = gParticleCount - 1;
		gEffectCount = gEffectCount - 1;
	}
	else if (aMsg == MSG_DRAW)
	{
		const Effect* anEffect = (const Effect*)aSlots[3];
		const Particle* aParticle = &gConfig.mParticles[anEffect->mValue0];
		uint32_t aNow = GameTime();
		uint32_t aStart = aNow;
		if ((uint32_t)aSlots[0] <= aNow)
			aStart = (uint32_t)aSlots[0];
		aSlots[0] = aStart;
		if ((uint32_t)(aSlots[1] + anEffect->mValue1) < aNow - (uint32_t)aSlots[0])
		{
			uint32_t aTime = (aNow - (uint32_t)aSlots[0]) - (uint32_t)(aSlots[1] + anEffect->mValue1);
			double anAngle = (double)(int64_t)(uint32_t)aSlots[2] * 6.2831853 / 32767.0;
			double aDistance = (double)(int64_t)(uint32_t)(aTime * (uint32_t)anEffect->mValue3) / 1000.0;
			double x = cos(anAngle) * aDistance;
			double y = sin(anAngle) * aDistance;
			y = y - 0.05 * aDistance;
			uint32_t aLevel;
			if (aTime < (uint32_t)anEffect->mValue7)
			{
				aLevel = (aTime * 0x100) / (uint32_t)anEffect->mValue7;
			}
			else if (aTime < (uint32_t)(anEffect->mValue7 + anEffect->mValue8))
			{
				aLevel = 0xff;
			}
			else
			{
				if ((uint32_t)(anEffect->mValue7 + anEffect->mValue8 + anEffect->mValue9) <= aTime)
				{
					FreeHandler(theHandler);
					return 0;
				}
				aLevel = (((uint32_t)(anEffect->mValue7 + anEffect->mValue8 + anEffect->mValue9) - aTime) * 0x100) /
					(uint32_t)anEffect->mValue9;
			}
			uint32_t aPulse;
			if (anEffect->mValue6 == 0)
			{
				aPulse = (uint32_t)(uint8_t)anEffect->mValue4;
			}
			else
			{
				// a triangle between the two levels
				uint32_t aPhase = aTime % (uint32_t)anEffect->mValue6;
				if ((uint32_t)anEffect->mValue6 >> 1 < aPhase)
					aPhase = (uint32_t)anEffect->mValue6 - aPhase;
				aPulse = (aPhase * ((uint32_t)(uint8_t)anEffect->mValue4 - (uint32_t)(uint8_t)anEffect->mValue5)) /
					((uint32_t)anEffect->mValue6 >> 1) + (uint32_t)(uint8_t)anEffect->mValue5;
			}
			aLevel = (aLevel * aPulse) / 0xff;
			int32_t aSize = aParticle->mValue0;
			if (aParticle->mValue7 == 1)
			{
				// with its own alpha channel
				uint8_t aBlitLevel;
				if (aLevel < 0xff)
					aBlitLevel = (uint8_t)aLevel;
				else
					aBlitLevel = 0xff;
				const uint8_t* aBits = gParticleState[anEffect->mValue0];
				uint8_t aFormat = lowos::GetDisplayFormat(nullptr, nullptr, nullptr);
				lowos::Blit(aSize, aSize, (int)theHandler->mRect.x - (int)(int16_t)(int64_t)x,
					(int)theHandler->mRect.y - (int)(int16_t)(int64_t)y, 0, 0, (uint32_t)(aSize * (aFormat + 1)), aBits, 7,
					aBlitLevel);
			}
			else
			{
				if (0xfe < aLevel)
					aLevel = 0xff;
				const uint8_t* aBits = gParticleState[anEffect->mValue0];
				uint8_t aFormat = lowos::GetDisplayFormat(nullptr, nullptr, nullptr);
				lowos::Blit(aSize, aSize, (int)theHandler->mRect.x - (int)(int16_t)(int64_t)x,
					(int)theHandler->mRect.y - (int)(int16_t)(int64_t)y, 0, 0, (uint32_t)(aSize * aFormat), aBits, 0x15,
					(uint8_t)aLevel);
			}
		}
	}
	return 0;
}

namespace
{

uint8_t gSummaryItems = 0;                 // 0x489ef4 the level summary's items on the screen
char gSummaryContinued = 0;                // 0x489ef5 its button was pressed: the items fade out
uint8_t gButtonDrawn = 1;                  // 0x47e294 (the button's image needs no text)

} // namespace

// 0x424e66: an item of the level summary (0 the level's score, 1 its time, 2 its best combo, 3 its best match, 4
// the button to go on) sliding in from the right and bouncing against its place, squashed. The button waits for the
// summary's other events, lights under the mouse, and when pressed (or Enter) sends command 0x3b; then all items
// fade out and the last one gone starts the next summary event. The texts are letters of the text font: a label
// glyph, then the value. Create: a = the item, b = its delay (ms).
uint32_t SummaryItemProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if ((aMsg == MSG_CLASSINIT) || (aMsg == MSG_CLASSFREE))
		return 0;
	uint32_t anItem;
	if (aMsg == MSG_CREATE)
		anItem = (uint32_t)theA;
	else
		anItem = (uint32_t)aSlots[0];
	char aText[16];
	uint8_t* aBits = nullptr;
	uint8_t* aDrawn = nullptr;
	uint32_t aPitch = 0;
	lowos::Surface* aSurface = nullptr;
	if (anItem == 0)
	{
		aText[0] = 'A';
		// MSVC's _itoa
		snprintf(aText + 1, sizeof(aText) - 1, "%d", (int)gApp.GetBoard()->mField->GetLevelScore());
		aBits = gApp.mTexts[0].mBits;
		aDrawn = &gApp.mTexts[0].mDrawn;
		aPitch = gApp.mTexts[0].mPitch;
		aSurface = gApp.mTexts[0].mSurface;
	}
	else if (anItem == 1)
	{
		// h:mm:ss, without a leading 0:0 or 0
		aText[0] = 'B';
		uint32_t aTime = gApp.GetBoard()->mField->mLevelTime / 1000;
		aText[8] = 0;
		aText[7] = (char)(aTime % 10) + '0';
		aTime = aTime / 10;
		aText[6] = (char)(aTime % 6) + '0';
		aTime = aTime / 6;
		aText[5] = ':';
		aText[4] = (char)(aTime % 10) + '0';
		aTime = aTime / 10;
		aText[3] = (char)(aTime % 6) + '0';
		aText[2] = ':';
		aText[1] = (char)((aTime / 6) % 10) + '0';
		if ((aText[1] == '0') && (aText[3] == '0'))
			memmove(aText + 1, aText + 4, 5);
		else if (aText[1] == '0')
			memmove(aText + 1, aText + 3, 6);
		aBits = gApp.mTexts[1].mBits;
		aDrawn = &gApp.mTexts[1].mDrawn;
		aPitch = gApp.mTexts[1].mPitch;
		aSurface = gApp.mTexts[1].mSurface;
	}
	else if (anItem == 2)
	{
		// up to 4 a word glyph (C + the chain), else the number and the glyph H
		aText[0] = 'C';
		if (gApp.GetBoard()->mField->mLevelBestChain < 5)
		{
			aText[1] = (char)gApp.GetBoard()->mField->mLevelBestChain + 'C';
			aText[2] = 0;
		}
		else
		{
			snprintf(aText + 1, sizeof(aText) - 1, "%d", (int)gApp.GetBoard()->mField->mLevelBestChain);
			if (gApp.GetBoard()->mField->mLevelBestChain < 10)
			{
				aText[2] = 'H';
				aText[3] = 0;
			}
			else
			{
				aText[3] = 'H';
				aText[4] = 0;
			}
		}
		aBits = gApp.mTexts[2].mBits;
		aDrawn = &gApp.mTexts[2].mDrawn;
		aPitch = gApp.mTexts[2].mPitch;
		aSurface = gApp.mTexts[2].mSurface;
	}
	else if (anItem == 3)
	{
		aText[0] = 'I';
		int32_t aMatch = (int32_t)gApp.GetBoard()->mField->mLevelBestMatch;
		if (999 < aMatch)
			aMatch = 999;
		snprintf(aText + 1, sizeof(aText) - 1, "%d", aMatch);
		size_t aLength = strlen(aText);
		aText[aLength] = 'J';
		aText[aLength + 1] = 0;
		aBits = gApp.mTexts[3].mBits;
		aDrawn = &gApp.mTexts[3].mDrawn;
		aPitch = gApp.mTexts[3].mPitch;
		aSurface = gApp.mTexts[3].mSurface;
	}
	else if (anItem == 4)
	{
		aSurface = gApp.mImage68;
		aBits = lowos::SurfaceBits(gApp.mImage68);
		aPitch = lowos::SurfaceWidth(aSurface) & 0xffff;
		aPitch = aPitch * lowos::GetDisplayFormat(nullptr, nullptr, nullptr);
		aDrawn = &gButtonDrawn;
		aText[0] = 0;
	}
	if (aMsg == MSG_CREATE)
	{
		if (theA == 4)
		{
			// the button's two states, one above the other
			theHandler->mRect.w = (int16_t)lowos::SurfaceWidth(aSurface);
			theHandler->mRect.h = (int16_t)(lowos::SurfaceHeight(aSurface) / 2);
		}
		else
		{
			GuiRect anExtent;
			DrawText(gApp.mTextFont, aBits, aPitch, aText, nullptr, &anExtent, 0, 0, 0xff, 2);
			theHandler->mRect.w = anExtent.w;
			theHandler->mRect.h = anExtent.h;
			theHandler->mRect.y = (int16_t)(theHandler->mRect.y - (int16_t)(((uint16_t)anExtent.h + 1) / 2));
		}
		aSlots[0] = theA;
		aSlots[1] = theB;
		aSlots[2] = GameTime();
		aSlots[3] = 0;
		*aDrawn = 0;
		gButtonDrawn = 1;
		gSummaryContinued = 0;
		gSummaryItems = gSummaryItems + 1;
	}
	else if (aMsg == MSG_DESTROY)
	{
		gSummaryItems = gSummaryItems - 1;
		if ((gSummaryItems == 0) && (gSummaryContinued != 0))
			gApp.NextSummaryEvent();
	}
	else if (aMsg == MSG_MOUSEDOWN)
	{
		int aHit = PointInRect((uint16_t)(int16_t)theA, (uint16_t)(int16_t)theB, &theHandler->mRect);
		if ((aHit != 0) && (aSlots[0] == 4) && (gSummaryContinued == 0) && ((aSlots[3] & 4) != 0))
		{
			Play(gSounds[0x3], 1);
			Command(theHandler, 0x3b);
			gSummaryContinued = 1;
		}
	}
	else if (aMsg == MSG_MOUSEMOVE)
	{
		int aHit = PointInRect((uint16_t)(int16_t)theA, (uint16_t)(int16_t)theB, &theHandler->mRect);
		if ((aHit == 0) || (aSlots[0] != 4) || ((aSlots[3] & 4) == 0))
			aSlots[3] = aSlots[3] & 0xfffffffe;
		else if ((~aSlots[3] & 1) != 0)
			aSlots[3] = aSlots[3] | 1;
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aNow = GameTime();
		uint32_t aStart = aNow;
		if ((uint32_t)aSlots[2] <= aNow)
			aStart = (uint32_t)aSlots[2];
		aSlots[2] = aStart;
		if (((uint32_t)aSlots[1] <= aNow - (uint32_t)aSlots[2]) || (gSummaryContinued != 0))
		{
			uint32_t aTime = (aNow - (uint32_t)aSlots[2]) - (uint32_t)aSlots[1];
			if ((gApp.mSummaryEvents < (uint32_t)(3 - (gConfig.mLevelsummaryEndevent != 1))) && (aSlots[0] == 4))
			{
				// the button waits for the summary's other events
				aSlots[1] = 0;
				aSlots[2] = aNow;
			}
			else
			{
				if ((gSummaryContinued != 0) && ((~aSlots[3] & 8) != 0))
				{
					aSlots[3] = aSlots[3] | 8;
					aSlots[2] = aNow;
				}
				if (gSummaryContinued != 0)
					aTime = 100000;
				GuiRect aRect = theHandler->mRect;
				if (aTime < (uint32_t)gConfig.mLevelsummaryEntertime)
				{
					aRect.x = (int16_t)(0x280 - (int16_t)((uint32_t)((0x280 - (int32_t)theHandler->mRect.x) * aTime) /
						(uint32_t)gConfig.mLevelsummaryEntertime));
				}
				else
				{
					// it hits its place at the entering speed and bounces against it like a spring, squashed (but
					// at most by the configured part of its width), and settles
					aTime = aTime - (uint32_t)gConfig.mLevelsummaryEntertime;
					double aSpeed = (double)(int32_t)((0x280 - (int32_t)theHandler->mRect.x) * 1000) /
						(double)(int64_t)(uint32_t)gConfig.mLevelsummaryEntertime;
					double anOmega = 6283.1853 / (double)(int64_t)(uint32_t)gConfig.mLevelsummaryBouncetime;
					double anAmplitude = aSpeed / anOmega;
					double aHalf = (double)(int64_t)(uint32_t)gConfig.mLevelsummaryBouncetime / 2000.0;
					if ((double)(int32_t)(uint16_t)theHandler->mRect.w * (double)gConfig.mLevelsummaryMaxcompression <
						anAmplitude)
					{
						anAmplitude = (double)(int32_t)(uint16_t)theHandler->mRect.w *
							(double)gConfig.mLevelsummaryMaxcompression;
						anOmega = aSpeed / anAmplitude;
						aHalf = 6.2831853 / anOmega * 0.5;
					}
					double aSeconds = (double)(int64_t)aTime / 1000.0;
					double aSquash = sin(anOmega * aSeconds) * anAmplitude;
					double aSettle = aHalf;
					double aDone;
					if (aSeconds >= aHalf)
					{
						if (aSeconds - aHalf >= aSettle)
						{
							aDone = 1.0;
							aSlots[3] = aSlots[3] | 4;
						}
						else
						{
							aDone = pow((aSeconds - aHalf) / aSettle, 0.7);
						}
					}
					else
					{
						aDone = 0.0;
					}
					aSquash = (1.0 - aDone) * aSquash;
					if (aDone == 1.0)
					{
						aRect = theHandler->mRect;
					}
					else
					{
						aRect.w = (int16_t)(uint16_t)(int64_t)((double)(int32_t)(uint16_t)aRect.w - aSquash);
						aRect.h = (int16_t)(uint16_t)(((uint32_t)(uint16_t)theHandler->mRect.w *
							(uint16_t)theHandler->mRect.h + (uint16_t)aRect.w / 2) / (uint16_t)aRect.w);
						aRect.y = (int16_t)(aRect.y + ((uint16_t)theHandler->mRect.h + 1) / 2);
						aRect.y = (int16_t)(aRect.y - ((uint16_t)aRect.h + 1) / 2);
					}
				}
				if (*aDrawn == 0)
				{
					*aDrawn = 1;
					DrawText(gApp.mTextFont, aBits, aPitch, aText, nullptr, nullptr, 0, 0, 0xff, 0);
				}
				if (gSummaryContinued == 0)
				{
					uint32_t aSrcY;
					if ((aSlots[3] & 1) == 0)
						aSrcY = 0;
					else
						aSrcY = (uint16_t)theHandler->mRect.h;
					lowos::BlitScaled((uint16_t)aRect.w, (uint16_t)aRect.h, (uint16_t)theHandler->mRect.w,
						(uint16_t)theHandler->mRect.h, aRect.x, aRect.y, 0, (int)aSrcY, aSurface);
				}
				else
				{
					aTime = aNow - (uint32_t)aSlots[2];
					if ((uint32_t)gConfig.mLevelsummaryFadetime <= aTime)
					{
						FreeHandler(theHandler);
						return 0;
					}
					lowos::BlitLevel((uint16_t)aRect.w, (uint16_t)aRect.h, aRect.x, aRect.y, 0, 0, aSurface,
						(uint8_t)((((uint32_t)gConfig.mLevelsummaryFadetime - 1 - aTime) * 0x100) /
							(uint32_t)gConfig.mLevelsummaryFadetime));
				}
				// a sound when the mouse comes onto the button
				if ((aSlots[3] & 3) == 1)
					Play(gSounds[0x2], 1);
				aSlots[3] = aSlots[3] & 0xfffffffd;
				aSlots[3] = aSlots[3] + (aSlots[3] & 1) * 2;
			}
		}
	}
	else if ((aMsg == MSG_CHAR) && (theA == 0xd) && (gSummaryContinued == 0) && (aSlots[0] == 4) &&
		((aSlots[3] & 4) != 0))
	{
		Play(gSounds[0x3], 1);
		Command(theHandler, 0x3b);
		gSummaryContinued = 1;
	}
	return 0;
}

// 0x425987: the space bar pauses the game
uint32_t PauseKeyProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	(void)theB;
	(void)theHandler;
	if (((uint8_t)theMsg == MSG_CHAR) && (theA == 0x20))
	{
		gPauseFlags = gPauseFlags | 1;
		App::UpdatePause();
	}
	return 0;
}

namespace
{

// The column under the mouse
uint32_t ColumnAt(uint16_t x)
{
	uint16_t aX;
	if (x < (uint32_t)gConfig.mBlockgridX)
		aX = (uint16_t)gConfig.mBlockgridX;
	else
		aX = x;
	uint32_t aColumn = (uint32_t)(((int32_t)aX - gConfig.mBlockgridX) / gConfig.mBlockWidth);
	if ((uint32_t)gConfig.mNumColumns - 1 < aColumn)
		aColumn = (uint32_t)(gConfig.mNumColumns - 1);
	return aColumn;
}

} // namespace

// 0x4259b7: the column the player drops into: the one under the mouse, or moved with the arrow keys until the mouse
// moves more than 5 pixels; a click or the space bar taps. The board asks it with message 0x10 (the column) and
// 0x11 (1 once after a tap).
uint32_t ColumnCursorProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	(void)theB;
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	uint16_t x;
	uint16_t y;
	if (aMsg == MSG_CREATE)
	{
		lowos::GetMousePos(&x, &y);
		aSlots[0] = ColumnAt(x);
		aSlots[1] = 0;
		aSlots[3] = 0;
	}
	else if (aMsg == MSG_MOUSEDOWN)
	{
		aSlots[1] = 1;
	}
	else if (aMsg == MSG_KEY)
	{
		if (theA == 0x20)
			aSlots[1] = 1;
		// the left arrow or keypad 4 (DirectInput scan codes), and below the right arrow or keypad 6
		if ((theA == 0xcb) || (theA == 0x4b))
		{
			aSlots[3] = 1;
			lowos::GetMousePos(&x, &y);
			aSlots[2] = (uint32_t)x;
			aSlots[2] = (uint32_t)aSlots[2] | (uint32_t)y << 0x10;
			if (aSlots[0] != 0)
				aSlots[0] = aSlots[0] - 1;
		}
		if ((theA == 0xcd) || (theA == 0x4d))
		{
			aSlots[3] = 1;
			lowos::GetMousePos(&x, &y);
			aSlots[2] = (uint32_t)x;
			aSlots[2] = (uint32_t)aSlots[2] | (uint32_t)y << 0x10;
			aSlots[0] = aSlots[0] + 1;
			uint32_t aColumn;
			if ((uint32_t)gConfig.mNumColumns - 1 < (uint32_t)aSlots[0])
				aColumn = (uint32_t)(gConfig.mNumColumns - 1);
			else
				aColumn = (uint32_t)aSlots[0];
			aSlots[0] = aColumn;
		}
	}
	else if (aMsg == MSG_COVERED)
	{
		aSlots[1] = 0;
	}
	else if (aMsg == MSG_SETVALUE)
	{
		lowos::GetMousePos(&x, &y);
		uint32_t aKeyMouse = (uint32_t)aSlots[2];
		if ((aSlots[3] != 0) && ((uint32_t)x <= (aKeyMouse & 0xffff) + 5) && ((aKeyMouse & 0xffff) <= (uint32_t)x + 5) &&
			((uint32_t)y <= (aKeyMouse >> 0x10) + 5) && (aKeyMouse >> 0x10 <= (uint32_t)y + 5))
			return (uint32_t)aSlots[0];
		aSlots[3] = 0;
		uint32_t aColumn = ColumnAt(x);
		aSlots[0] = aColumn;
		return aColumn;
	}
	else if (aMsg == MSG_GETVALUE)
	{
		if (aSlots[1] != 0)
		{
			aSlots[1] = 0;
			return 1;
		}
		return 0;
	}
	return 0;
}

// 0x425c93: holds the game after it is over and then shows the game summary
uint32_t GameOverDelayProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	(void)theA;
	(void)theB;
	uintptr_t* aSlots = theHandler->u.mSlots;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aSlots[0] = GameTime();
		gBusy = gBusy + 1;
	}
	else if (aMsg == MSG_DESTROY)
	{
		gBusy = gBusy - 1;
	}
	else if (aMsg == MSG_UPDATE)
	{
		uint32_t aNow = GameTime();
		uint32_t aStart = aNow;
		if ((uint32_t)aSlots[0] <= aNow)
			aStart = (uint32_t)aSlots[0];
		aSlots[0] = aStart;
		if ((uint32_t)gConfig.mSummaryDelay <= aNow - (uint32_t)aSlots[0])
			gApp.SetScreen(SCREEN_GAMEOVER);
	}
	return 0;
}

// 0x425d28: pauses the music while it exists
uint32_t MusicPauseProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	(void)theA;
	(void)theB;
	(void)theHandler;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
		PauseMusic(1);
	else if (aMsg == MSG_DESTROY)
		PauseMusic(0);
	return 0;
}

// 0x4265a6: the progress bar at theValue of theMax: its height follows the value (up a pixel each 15 ms, down two),
// its color blends from the low image to the high one and then to the top one; in a timed game (or a bonus round)
// below the flash part of the time it flashes and sounds the alarm. The bar is drawn into mBarBits when it changes.
void App::DrawBar(uint32_t theValue, uint32_t theMax)
{
	if (theMax < theValue)
		theValue = theMax;
	uint32_t aTime = GameTime();
	if (aTime < mBarTime)
		mBarTime = aTime;
	aTime = aTime - mBarTime;
	uint32_t aHeight = lowos::SurfaceHeight(mBarFrame);
	uint32_t aTarget = (uint32_t)(((uint64_t)theValue * (uint64_t)aHeight) / (uint64_t)theMax);
	aTime = aTime / 0xf;
	mBarTime = aTime * 0xf + mBarTime;
	if (mBarLevel < aTarget)
	{
		mBarLevel = mBarLevel + aTime;
		if (aTarget < mBarLevel)
			mBarLevel = aTarget;
	}
	else
	{
		uint32_t aDown = aTime * 2;
		if (mBarLevel < aDown)
			mBarLevel = 0;
		else
			mBarLevel = mBarLevel + aTime * -2;
		aTime = aDown;
		if (mBarLevel < aTarget)
			mBarLevel = aTarget;
	}
	uint32_t aLevel = mBarLevel;
	// the alarm
	if ((theMax / (uint32_t)gConfig.mFlashDivisor < theValue) ||
		(((gGameMode != 1) && (gBonusRound == 0)) || (mScreen != SCREEN_GAME) || ((gBusy != 0) || (mSummaryEvents != 0))))
		Play(gSounds[0x2c], 0);
	else
		Play(gSounds[0x2c], 2);
	lowos::Surface* anImage;
	lowos::Surface* anOver;
	uint32_t anAlpha;
	char aFlash;
	if ((theMax / (uint32_t)gConfig.mFlashDivisor < theValue) ||
		((uint32_t)gConfig.mFlashDuration >> 1 <= GameTime() % (uint32_t)gConfig.mFlashDuration) ||
		((gGameMode != 1) && (gBonusRound == 0)) || (gBusy != 0) || (mSummaryEvents != 0))
	{
		aFlash = 0;
		if (aLevel < aHeight >> 1)
		{
			anImage = mBarLow;
			anOver = mBarHigh;
			anAlpha = (aLevel * 0xff) / (aHeight >> 1);
			if (0xff < anAlpha)
				anAlpha = 0xff;
		}
		else
		{
			anImage = mBarHigh;
			anOver = mBarFrame;
			anAlpha = ((aLevel - (aHeight >> 1)) * 0xff) / (aHeight >> 1);
			if (0xff < anAlpha)
				anAlpha = 0xff;
		}
	}
	else
	{
		anImage = mBarFlash;
		anOver = mBarFlash;
		anAlpha = 0;
		aFlash = 1;
	}
	lowos::PixelFormat aFormat;
	uint8_t aBytes = lowos::GetDisplayFormat(&aFormat, nullptr, nullptr);
	lowos::PixelFormat aLogical;
	lowos::GetLogicalFormat(&aLogical, nullptr, nullptr);
	if ((mBarDrawn != aLevel) || (mBarFlashDrawn != aFlash))
	{
		// the image's rows of the bar's height, and over them the next image at the blend's level (the original
		// does the top clip's rows apart from the rest, the same way)
		uint32_t aWidth = lowos::SurfaceWidth(mBarFrame) & 0xffff;
		uint32_t aPitch = aWidth * aBytes;
		uint32_t aTop = aHeight - aLevel;
		if ((uint32_t)gConfig.mProgressbarTopclip < aLevel)
		{
			uint32_t aClip = (uint32_t)gConfig.mProgressbarTopclip;
			lowos::BlitPixels(&aFormat, &aFormat, mBarBits + aTop * aPitch, lowos::SurfaceBits(anImage) + aTop * aPitch,
				(int)aClip, (int)aWidth, aPitch, aPitch, 0, aBytes, aBytes, 0, 0);
			lowos::BlitPixels(&aFormat, &aFormat, mBarBits + (aTop + aClip) * aPitch,
				lowos::SurfaceBits(anImage) + (aTop + aClip) * aPitch, (int)(aLevel - aClip), (int)aWidth, aPitch, aPitch,
				0, aBytes, aBytes, 0, 0);
			lowos::BlitPixels(&aFormat, &aFormat, mBarBits + aTop * aPitch, lowos::SurfaceBits(anOver) + aTop * aPitch,
				(int)aClip, (int)aWidth, aPitch, aPitch, 4, aBytes, aBytes, 0, (uint8_t)anAlpha);
			lowos::BlitPixels(&aFormat, &aFormat, mBarBits + (aTop + aClip) * aPitch,
				lowos::SurfaceBits(anOver) + (aTop + aClip) * aPitch, (int)(aLevel - aClip), (int)aWidth, aPitch, aPitch,
				4, aBytes, aBytes, 0, (uint8_t)anAlpha);
		}
		else
		{
			lowos::BlitPixels(&aFormat, &aFormat, mBarBits + aTop * aPitch, lowos::SurfaceBits(anImage) + aTop * aPitch,
				(int)aLevel, (int)aWidth, aPitch, aPitch, 0, aBytes, aBytes, 0, 0xff);
			// the original passes the width as the pitches here, not the width in bytes: the few rows of a nearly
			// empty bar blend from the wrong pixels
			lowos::BlitPixels(&aFormat, &aFormat, mBarBits + aTop * aPitch, lowos::SurfaceBits(anOver) + aTop * aPitch,
				(int)aLevel, (int)aWidth, aWidth, aWidth, 4, aBytes, aBytes, 0, (uint8_t)anAlpha);
		}
		mBarDrawn = aLevel;
		mBarFlashDrawn = aFlash;
	}
	uint32_t aWidth = lowos::SurfaceWidth(mBarFrame) & 0xffff;
	lowos::Blit((int)aWidth, (int)aLevel, gConfig.mProgressbarX, gConfig.mProgressbarY + (int32_t)(aHeight - aLevel), 0,
		(int)(aHeight - aLevel), aWidth * aBytes, mBarBits, 1, 0xff);
}

// 0x426d39: the motion of a kind of block: the special blocks' own, else the player's block's (theFlag) or the
// floaters'
Motion* MotionOf(int theKind, char theFlag)
{
	Motion* aMotion;
	if (theKind == KIND_ROCKET)
	{
		aMotion = &gConfig.mRocketMotion;
	}
	else if (theKind == KIND_EXPLODER)
	{
		if (theFlag == 0)
			aMotion = &gConfig.mExflMotion;
		else
			aMotion = &gConfig.mExfaMotion;
	}
	else if (theKind == KIND_HAMMER)
	{
		aMotion = &gConfig.mHammerMotion;
	}
	else if (theKind == KIND_MAGNET)
	{
		aMotion = &gConfig.mMagnetMotion;
	}
	else if (theKind == KIND_RAINBOW)
	{
		aMotion = &gConfig.mRainbowMotion;
	}
	else if (theKind == KIND_COLORBOMB)
	{
		aMotion = &gConfig.mColorbombMotion;
	}
	else if (theKind == KIND_COLUMNNUKE)
	{
		aMotion = &gConfig.mColumnnukeMotion;
	}
	else if (theFlag == 0)
	{
		aMotion = &gConfig.mFloaterMotion;
	}
	else
	{
		aMotion = &gConfig.mPrimaryfallerMotion;
	}
	return aMotion;
}

// 0x41bbce for the board (Board.cpp)
void AppNextSummaryEvent(App* theApp)
{
	theApp->NextSummaryEvent();
}

} // namespace tiptop
