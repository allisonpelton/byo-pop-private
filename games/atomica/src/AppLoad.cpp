// The app's loading thread (Atomica.exe 0x406f90-0x40b1e0): the game's pictures, the atoms' frames drawn from
// them, and the sounds. The original keeps the atom frames in a cache file (images\cached\Atoms.WAD) and draws
// them the first time only; the port draws them every start (docs/GAME_NOTES.md section 3).

#include "AtomicaApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstring>

using namespace Sexy;

// 0x429470
Sound::Sound(AtomicaApp* theApp, const char* theName) : mApp(theApp), mId(-1)
{
	std::string aPath = std::string("sounds\\") + theName;
	if (mApp->mSoundManager != NULL)
	{
		mId = mApp->mSoundManager->LoadSound(aPath);
		if (mId < 0)
		{
			mApp->Popup("Failed to load sound: " + aPath);
			mApp->mLoadingFailed = true;
		}
	}
}

// 0x429250
void Sound::Play()
{
	if (mId >= 0)
		mApp->PlaySample(mId);
}

Sound* AtomicaApp::LoadSound(const char* theName)
{
	Sound* aSound = new Sound(this, theName);
	mCompletedLoadingThreadTasks++;
	return aSound;
}

// 0x437c50: the pixels of two images' rectangles blended, A by (256 - w) and B by w parts of 256, w = the
// weight times 256 cut to an integer; the result is a new image of A's rectangle's size
Sprite* AtomicaApp::BlendSprites(Sprite* theA, Sprite* theB, double theWeight)
{
	MemoryImage* anA = dynamic_cast<MemoryImage*>(theA->mImage);
	MemoryImage* aB = dynamic_cast<MemoryImage*>(theB->mImage);
	const Rect& aRectA = theA->mSrc;
	const Rect& aRectB = theB->mSrc;
	if ((anA == NULL) || (aB == NULL) || (aRectA.mX < 0) || (aRectA.mY < 0) ||
		(aRectA.mX + aRectA.mWidth > anA->GetWidth()) || (aRectA.mY + aRectA.mHeight > anA->GetHeight()) ||
		(aRectB.mX < 0) || (aRectB.mY < 0) || (aRectB.mWidth + aRectB.mX > aB->GetWidth()) ||
		(aRectB.mHeight + aRectB.mY > aB->GetHeight()))
		return NULL;

	int aWidth = aRectA.mWidth;
	int aHeight = aRectA.mHeight;
	Sprite* aResult = mSprites->Blank(aWidth, aHeight);
	unsigned int* aDest = aResult->GetBits();
	unsigned int* aBitsA = (unsigned int*) anA->GetBits();
	unsigned int* aBitsB = (unsigned int*) aB->GetBits();
	int aStrideA = anA->GetWidth();
	int aStrideB = aB->GetWidth();
	int aWeightB = (int) (theWeight * 256.0);
	int aWeightA = 0x100 - aWeightB;
	for (int y = 0; y < aHeight; y++)
	{
		unsigned int* aPixelA = aBitsA + (aRectA.mY + y) * aStrideA + aRectA.mX;
		unsigned int* aPixelB = aBitsB + (aRectB.mY + y) * aStrideB + aRectB.mX;
		for (int x = 0; x < aWidth; x++)
		{
			unsigned int a = *aPixelA++;
			unsigned int b = *aPixelB++;
			*aDest++ = ((((a & 0xff0000) * aWeightA + (b & 0xff0000) * aWeightB) & 0xff0000ff) |
				(((a & 0xff) * aWeightA + (b & 0xff) * aWeightB) & 0xff00) |
				(((a & 0xff00) * aWeightA + (b & 0xff00) * aWeightB) & 0xff0000)) >> 8 |
				((((a >> 8) & 0xff0000) * aWeightA + ((b >> 8) & 0xff0000) * aWeightB) & 0xff000000);
		}
	}
	aResult->BitsChanged();
	return aResult;
}

// 0x406110: a sprite's pixels lose the colour of the mask image, channel by channel, never below 0; the mask
// is read straight through, one pixel for each pixel of the sprite (it is as wide as the sprite)
void AtomicaApp::SubtractMask(Sprite* theSprite, Sprite* theMask)
{
	unsigned char* aBits = (unsigned char*) theSprite->GetBits();
	unsigned char* aMask = (unsigned char*) theMask->GetBits();
	int aImageWidth = theSprite->mImage->GetWidth();
	for (int y = 0; y < theSprite->mHeight; y++)
	{
		unsigned char* aPixel = aBits + ((theSprite->mSrc.mY + y) * aImageWidth + theSprite->mSrc.mX) * 4;
		for (int x = 0; x < theSprite->mWidth; x++)
		{
			for (int c = 0; c < 3; c++)
			{
				int aValue = (int) aPixel[c] - (int) aMask[c];
				aPixel[c] = (unsigned char) (aValue < 0 ? 0 : aValue);
			}
			aPixel += 4;
			aMask += 4;
		}
	}
	theSprite->BitsChanged();
}

// The shrinking loops: a clear copy of the source's size, with the source drawn into it centred and smaller by
// 4% of its size for each step (the stretch copy of 0x4297d0)
void AtomicaApp::ShrinkInto(Sprite* theSource, Sprite* theDest, int theStep)
{
	float aHalfW = (float) (theSource->mWidth / 2);
	float aHalfH = (float) (theSource->mHeight / 2);
	float aScale = (float) theStep * 0.04f;
	float aW = aHalfW - aScale * aHalfW;
	float aH = aHalfH - aScale * aHalfH;
	Graphics g(theDest->mImage);
	g.DrawImage(theSource->mImage,
		Rect((int) ((float) (theDest->mHeight / 2) - aW), (int) ((float) (theDest->mHeight / 2) - aH),
			(int) (aW + aW), (int) (aH + aH)),
		theSource->mSrc);
	theDest->BitsChanged();
}

// 0x406f90
void AtomicaApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = 0x60a;
	mCompletedLoadingThreadTasks = 0;
	mPi = 3.14159265f;

	// the explosion's frames, then three sizes of every other frame
	mBigExplosionSheet = mSprites->Load("bigexplosion");
	for (int i = 0; i < 20; i++)
	{
		mBigExplosion[i] = mSprites->Cell(mBigExplosionSheet, 110, 110, i);
		mCompletedLoadingThreadTasks++;
	}
	mLoadingText = GT(0x4af420);
	for (int aType = 0; aType < 3; aType++)
	{
		int aSize = (aType == 0) ? 0x96 : (aType == 1) ? 0x12c : 0x1c2;
		for (int i = 0; i < 10; i++)
		{
			mExplosion[aType][i] = mSprites->Blank(aSize, aSize);
			Graphics g(mExplosion[aType][i]->mImage);
			g.DrawImage(mBigExplosion[i * 2]->mImage, Rect(0, 0, aSize, aSize), mBigExplosion[i * 2]->mSrc);
			mExplosion[aType][i]->BitsChanged();
			mCompletedLoadingThreadTasks++;
		}
	}
	mLoadingText = GT(0x4af404);
	PumpLoading();

	mAtomsSheet = mSprites->Load("atoms");
	mCompletedLoadingThreadTasks++;
	mElectron = mSprites->Load("electron");
	mCompletedLoadingThreadTasks++;
	mGridBug = mSprites->Load("gridbug");
	mCompletedLoadingThreadTasks++;
	mElectronDot = mSprites->SubSprite(mElectron, 4, 4, 5, 5);
	mCompletedLoadingThreadTasks++;
	mLevelFont = new ImageFont(this, "data\\levelfont.txt");
	mCompletedLoadingThreadTasks++;
	mStencilFont = new ImageFont(this, "data\\stencilfont.txt");
	mCompletedLoadingThreadTasks++;
	mCommentFont = new ImageFont(this, "data\\commentfont.txt");

	mBiohazardSheet = mSprites->Load("biohazard");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 6; i++)
		mBiohazard[i] = mSprites->Cell(mBiohazardSheet, 0x37, 0x37, i);
	mBadRingSheet = mSprites->Load("badring");
	mCompletedLoadingThreadTasks++;
	mBadRing[0] = mSprites->Cell(mBadRingSheet, 0x30, 0x31, 0);
	mBadRing[1] = mSprites->Cell(mBadRingSheet, 0x30, 0x31, 1);
	mBonusFrame = mSprites->Load("bonusframe");
	mCompletedLoadingThreadTasks++;
	mBioCountdownSheet = mSprites->Load("biocountdown");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 6; i++)
		mBioCountdown[i] = mSprites->Cell(mBioCountdownSheet, 0xf, 0x14, i);
	mAnnounceFont = new ImageFont(this, "data\\announcefont.txt");
	mMouseoverFont = new ImageFont(this, "data\\mouseoverfont.txt");

	mLoadingText = GT(0x4af21c);
	PumpLoading();
	mChromium = mSprites->Load("chromiumatom");
	mCompletedLoadingThreadTasks++;
	for (int aStep = 1; aStep < 0x19; aStep++)
	{
		Sprite* aCopy = mSprites->Blank(mChromium->mWidth + 9, mChromium->mHeight + 9);
		mChromiumShrink[aStep - 1] = aCopy;
		float aHalfW = (float) (mChromium->mWidth / 2);
		float aHalfH = (float) (mChromium->mHeight / 2);
		float aScale = (float) aStep * 0.04f;
		float aW = aHalfW - aScale * aHalfW;
		float aH = aHalfH - aScale * aHalfH;
		Graphics g(aCopy->mImage);
		g.DrawImage(mChromium->mImage,
			Rect((int) ((float) (aCopy->mWidth / 2) - aW), (int) ((float) (aCopy->mHeight / 2) - aH), (int) (aW + aW),
				(int) (aH + aH)),
			mChromium->mSrc);
		aCopy->BitsChanged();
		mCompletedLoadingThreadTasks++;
	}
	mAdjacentSheet = mSprites->Load("adjacentreflections");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 4; i++)
		mAdjacent[i] = mSprites->Cell(mAdjacentSheet, 0x37, 0x37, i);
	mFloatyFont = new ImageFont(this, "data\\floatyfont.txt");
	mScoreFont = new ImageFont(this, "data\\scorefont.txt");

	LoadAtoms();

	mLoadingText = GT(0x4af14c);
	PumpLoading();
	mPreAtom = mSprites->Load("preatom");
	mCompletedLoadingThreadTasks++;
	for (int aStep = 1; aStep < 0x19; aStep++)
	{
		Sprite* aCopy = mSprites->Blank(mPreAtom->mWidth, mPreAtom->mHeight);
		mPreAtomShrink[aStep - 1] = aCopy;
		ShrinkInto(mPreAtom, aCopy, aStep);
		mCompletedLoadingThreadTasks++;
	}
	mStarBurst = mSprites->Load("starburst");
	mCompletedLoadingThreadTasks++;

	mLoadingText = GT(0x4af110);
	PumpLoading();
	mMachineScore = mSprites->Load("atomicmachine(score)");
	mCompletedLoadingThreadTasks++;
	mMachineLevelMeter = mSprites->Load("atomicmachine(levelmeter)");
	mCompletedLoadingThreadTasks++;
	mMachineMonitor = mSprites->Load("atomicmachine(atomicusmonitor)");
	mCompletedLoadingThreadTasks++;
	mMachineRim = mSprites->Load("atomicrim");
	mCompletedLoadingThreadTasks++;
	mMachineBomber = mSprites->Load("atomicmachine(bomber)");
	mCompletedLoadingThreadTasks++;
	mMachineBomberMeter = mSprites->Load("atomicmachine(bombermeter)");
	mCompletedLoadingThreadTasks++;
	mMachineButtonSeqSheet = mSprites->Load("atomicmachine(buttonsequence)");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 11; i++)
		mMachineButtonSeq[i] = mSprites->Cell(mMachineButtonSeqSheet, 0x44, 0x2b, i);
	mMachineButtonPressed = mSprites->Load("atomicmachine(buttonpressed)");
	mCompletedLoadingThreadTasks++;
	mBomberChargingSheet = mSprites->Load("atomicmachine(bombercharging)");
	mCompletedLoadingThreadTasks++;
	mBomberCharging[0] = mSprites->Cell(mBomberChargingSheet, 0x82, 0x14, 0);
	mCompletedLoadingThreadTasks++;
	mBomberCharging[1] = mSprites->Cell(mBomberChargingSheet, 0x82, 0x14, 1);
	mCompletedLoadingThreadTasks++;
	mBomberCapSheet = mSprites->Load("atomicmachine(bombercap)");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 0x18; i++)
		mBomberCap[i] = mSprites->Cell(mBomberCapSheet, 0x5e, 0x5d, i);
	mTopLightsSheet = mSprites->Load("toplights");
	mCompletedLoadingThreadTasks++;
	mTopLights[0] = mSprites->Cell(mTopLightsSheet, 0x11, 0x22, 0);
	mCompletedLoadingThreadTasks++;
	mTopLights[1] = mSprites->Cell(mTopLightsSheet, 0x11, 0x22, 1);
	mCompletedLoadingThreadTasks++;
	mSideButtonsSheet = mSprites->Load("sidebarbuttons");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 6; i++)
		mSideButtons[i] = mSprites->Cell(mSideButtonsSheet, 100, 0x20, i);
	mMonitorMask = mSprites->Load("atomicusmonitormask");
	mCompletedLoadingThreadTasks++;
	mMonitorBuffer = mSprites->Blank(mMonitorMask->mWidth, mMonitorMask->mHeight);
	mCompletedLoadingThreadTasks++;
	mMonitorBlank = mSprites->SubSprite(mMachineMonitor, 0, 0x30, 0x96, 0x96);
	mCompletedLoadingThreadTasks++;
	mSmallHeadSheet = mSprites->Load("smallatomicushead");
	mCompletedLoadingThreadTasks++;
	mCoat = mSprites->Load("coat");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 11; i++)
	{
		mSmallHead[i] = mSprites->Cell(mSmallHeadSheet, 0x70, 0x7b, i);
		mCompletedLoadingThreadTasks++;
	}
	mLeftNodSheet = mSprites->Load("atomicusleftnod");
	mCompletedLoadingThreadTasks++;
	mRightNodSheet = mSprites->Load("atomicusrightnod");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 5; i++)
	{
		mLeftNod[i] = mSprites->Cell(mLeftNodSheet, 0x6e, 0x7b, i);
		mCompletedLoadingThreadTasks++;
		mRightNod[i] = mSprites->Cell(mRightNodSheet, 0x70, 0x7b, i);
		mCompletedLoadingThreadTasks++;
	}
	for (int i = 0; i < 3; i++)
	{
		int aCell = 3 - i;
		mLeftNod[5 + i] = mSprites->Cell(mLeftNodSheet, 0x6e, 0x7b, aCell);
		mCompletedLoadingThreadTasks++;
		mRightNod[5 + i] = mSprites->Cell(mRightNodSheet, 0x70, 0x7b, aCell);
		mCompletedLoadingThreadTasks++;
	}
	mNukeFlash = mSprites->Load("nukeflash");
	mCompletedLoadingThreadTasks++;
	mSpark = mSprites->Load("spark");
	mCompletedLoadingThreadTasks++;
	mMiniSpark = mSprites->Load("minispark");
	mCompletedLoadingThreadTasks++;
	mBigWordsSheet = mSprites->Load("bigwords");
	mCompletedLoadingThreadTasks++;
	mBigWords[0] = mSprites->SubSprite(mBigWordsSheet, 0x31, 0x1f, 0x136, 0x7d);
	mBigWords[1] = mSprites->SubSprite(mBigWordsSheet, 0x30, 0xe6, 0x124, 0x80);
	mBigWords[2] = mSprites->SubSprite(mBigWordsSheet, 0x1a, 0x1b0, 0x14f, 0x7d);
	mMouthSheet = mSprites->Load("atomicmouth");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 0x32; i++)
		mMouth[i] = mSprites->Cell(mMouthSheet, 0x32, 0x26, i);
	mMouthLeftSheet = mSprites->Load("atomicmouthleft");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 0x32; i++)
		mMouthLeft[i] = mSprites->Cell(mMouthLeftSheet, 0x32, 0x26, i);
	mSliderBar = mSprites->Load("sliderbar");
	mCompletedLoadingThreadTasks++;
	mSlider = mSprites->Load("slider");
	mCompletedLoadingThreadTasks++;
	mFreeze = mSprites->Load("Freeze");
	mCompletedLoadingThreadTasks++;

	mLoadingText = GT(0x4aee54);
	PumpLoading();
	mElementalSheet = mSprites->Load("elemental");
	mCompletedLoadingThreadTasks++;
	{
		int aCell = 0;
		for (int aRow = 0; aRow < 5; aRow++)
			for (int aCol = 0; aCol < 16; aCol++)
				mElemental[aRow][aCol] = mSprites->Cell(mElementalSheet, 0x37, 0x37, aCell++);
	}
	mBonusLevelsSheet = mSprites->Load("bonuslevels");
	mCompletedLoadingThreadTasks++;
	mBonusNucleusSheet = mSprites->Load("bonuslevelsnucleus");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 160; i++)
	{
		mBonusLevels[i] = mSprites->Cell(mBonusLevelsSheet, 0x3d, 0x3d, i);
		mBonusNucleus[i] = mSprites->Cell(mBonusNucleusSheet, 0x3d, 0x3d, i);
	}
	mBonusWordsSheet = mSprites->Load("bonuswords");
	mCompletedLoadingThreadTasks++;
	mBonusWords[0] = mSprites->Cell(mBonusWordsSheet, 0x116, 0x32, 0);
	mBonusWords[1] = mSprites->Cell(mBonusWordsSheet, 0x116, 0x32, 1);
	mBonusWords[2] = mSprites->Cell(mBonusWordsSheet, 0x116, 0x32, 4);
	mBonusWords[3] = mSprites->Cell(mBonusWordsSheet, 0x116, 0x32, 2);
	mBonusWords[4] = mSprites->Cell(mBonusWordsSheet, 0x116, 0x32, 3);
	mLevelAndScore = mSprites->Load("levelandscore");
	mCompletedLoadingThreadTasks++;
	mGeigerGlow = mSprites->Load("geigerglow");
	mCompletedLoadingThreadTasks++;
	mAtomicExplosionSheet = mSprites->Load("atomicexplosion");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 16; i++)
	{
		mAtomicExplosion[i] = mSprites->Cell(mAtomicExplosionSheet, 100, 100, i);
		mCompletedLoadingThreadTasks++;
	}
	mClouds = mSprites->Load("clouds");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 6; i++)
	{
		mCloud[i] = mSprites->Cell(mClouds, 100, 100, i);
		mCompletedLoadingThreadTasks++;
	}
	mGameTypeSheet = mSprites->Load("gametypeimages");
	mCompletedLoadingThreadTasks++;
	mGameType[0] = mSprites->SubSprite(mGameTypeSheet, 0, 0, 0x52, 0x4e);
	mGameType[1] = mSprites->SubSprite(mGameTypeSheet, 0, 0x53, 0x52, 0x51);
	mGameType[2] = mSprites->SubSprite(mGameTypeSheet, 0, 0xa6, 0x52, 0x56);
	mBombSymbol = mSprites->Load("AtomicBombSymbol");
	mCompletedLoadingThreadTasks++;
	mRankSheet = mSprites->Load("RankStrip");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 0x16; i++)
	{
		mRank[i] = mSprites->Cell(mRankSheet, 0x6e, 0x6e, i);
		mCompletedLoadingThreadTasks++;
	}
	mGreenAtomicus = mSprites->Load("GreenAtomicus");
	mCompletedLoadingThreadTasks++;
	mScoreArrowsSheet = mSprites->Load("HighScoreArrows");
	mCompletedLoadingThreadTasks++;
	mScoreArrows[0] = mSprites->Cell(mScoreArrowsSheet, 0x18, 0x1b, 0);
	mCompletedLoadingThreadTasks++;
	mScoreArrows[1] = mSprites->Cell(mScoreArrowsSheet, 0x18, 0x1b, 1);
	mCompletedLoadingThreadTasks++;
	mTextEdit = mSprites->Load("HSTextEdit");
	mCompletedLoadingThreadTasks++;
	mCancelTip = mSprites->Load("canceltip");
	mCompletedLoadingThreadTasks++;
	mTipGraphicsSheet = mSprites->Load("tipgraphics");
	mCompletedLoadingThreadTasks++;
	// stored at +0x30cc 0, +0x30d0 2, +0x30d4 4, +0x30d8 3, +0x30dc 1
	mTipGraphics[0] = mSprites->Cell(mTipGraphicsSheet, 0x40, 0x40, 0);
	mCompletedLoadingThreadTasks++;
	mTipGraphics[4] = mSprites->Cell(mTipGraphicsSheet, 0x40, 0x40, 1);
	mCompletedLoadingThreadTasks++;
	mTipGraphics[1] = mSprites->Cell(mTipGraphicsSheet, 0x40, 0x40, 2);
	mCompletedLoadingThreadTasks++;
	mTipGraphics[3] = mSprites->Cell(mTipGraphicsSheet, 0x40, 0x40, 3);
	mCompletedLoadingThreadTasks++;
	mTipGraphics[2] = mSprites->Cell(mTipGraphicsSheet, 0x40, 0x40, 4);
	mCompletedLoadingThreadTasks++;
	mMakeGroups = mSprites->Load("makegroups");
	mCompletedLoadingThreadTasks++;
	mMouseOver = mSprites->Load("mouseover");
	mCompletedLoadingThreadTasks++;
	mMagnetSheet = mSprites->Load("magnetsheet");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 8; i++)
	{
		mMagnet[i] = mSprites->Cell(mMagnetSheet, 0x4d, 0x4d, i);
		mCompletedLoadingThreadTasks++;
	}
	mExpertSheet = mSprites->Load("expertmode");
	mCompletedLoadingThreadTasks++;
	mExpert[0] = mSprites->SubSprite(mExpertSheet, 0, 0, 0x3c, 0x2d);
	mCompletedLoadingThreadTasks++;
	mExpert[1] = mSprites->SubSprite(mExpertSheet, 0, 100, 0x3c, 0x31);
	mCompletedLoadingThreadTasks++;
	mLevelNumbersSheet = mSprites->Load("levelnumbers");
	mCompletedLoadingThreadTasks++;
	for (int i = 0; i < 10; i++)
	{
		mLevelNumber[i][0] = mSprites->Cell(mLevelNumbersSheet, 0xc, 0x13, i);
		mCompletedLoadingThreadTasks++;
		mLevelNumber[i][1] = mSprites->Cell(mLevelNumbersSheet, 0xc, 0x13, i + 10);
		mCompletedLoadingThreadTasks++;
	}
	mLevelWord = mSprites->SubSprite(mLevelNumbersSheet, 2, 0x27, 0x1c, 0xd);
	mCompletedLoadingThreadTasks++;

	mLoadingText = GT(0x4aec78);
	PumpLoading();
	mSndAtomAppear = LoadSound("atomappear");
	mSndCreateMolecule = LoadSound("createmolecule");
	mSndSelectAtom = LoadSound("selectatom");
	mSndWhoosh = LoadSound("whoosh");
	mSndLowWhoosh = LoadSound("lowwhoosh");
	mSndLevelUp = LoadSound("voices\\levelup");
	mSndMoveAtom = LoadSound("moveatom");
	mSndMoveBioAtom = LoadSound("movebioatom");
	mSndAtomicBomb = LoadSound("voices\\atomicbomb");
	mSndNukeBlast = LoadSound("nukeblast");
	mSndGameOver = LoadSound("voices\\gameover");
	mSndGo = LoadSound("voices\\go");
	mSndHeartbeat = LoadSound("heartbeat");
	mSndElectric[0] = LoadSound("electric1");
	mSndElectric[1] = LoadSound("electric2");
	mSndElectric[2] = LoadSound("electric3");
	mSndCombo[0] = LoadSound("voices\\Combo");
	mSndCombo[1] = LoadSound("voices\\supercombo");
	mSndCombo[2] = LoadSound("voices\\megacombo");
	mSndCombo[3] = LoadSound("voices\\ultracombo");
	mSndCombo[4] = LoadSound("voices\\hypercombo");
	mSndCombo[5] = LoadSound("voices\\gigacombo");
	mSndCombo[6] = LoadSound("voices\\maxcombo");
	mSndGeiger1 = LoadSound("geiger1");
	mGeiger1Instance = mSoundManager->GetSoundInstance(mSndGeiger1->mId);
	mSndGeiger2 = LoadSound("geiger2");
	mGeiger2Instance = mSoundManager->GetSoundInstance(mSndGeiger2->mId);
	mSndSwoop = LoadSound("swoop");
	mSndDoorSlam = LoadSound("doorslam");
	mSndDoorOpen = LoadSound("dooropen");
	mSndBombReady = LoadSound("voices\\atomicbombready");
	mSndBiohazard = LoadSound("voices\\biohazard");
	mSndBonusMolecule = LoadSound("voices\\bonusmolecule");
	mSndBonusReady = LoadSound("voices\\bonusmoleculeready");
	mSndBonusUpgraded = LoadSound("voices\\bonusmoleculeupgraded");
	mSndGetReady = LoadSound("voices\\getready");
	mSndNewHighScore = LoadSound("voices\\anewhighscore");
	mSndFire = LoadSound("voices\\fire");
	mSndIce = LoadSound("voices\\ice");
	mSndLightning = LoadSound("voices\\lightning");
	mSndWind = LoadSound("voices\\wind");
	mSndEther = LoadSound("voices\\ether");
	mSndCool = LoadSound("voices\\cool");
	mSndExcellent = LoadSound("voices\\excellent");
	mSndGreat = LoadSound("voices\\great");
	mSndNifty = LoadSound("voices\\nifty");
	mSndTerrific = LoadSound("voices\\terrific");
	mSndWicked = LoadSound("voices\\wicked");
	mSndFreeze = LoadSound("freeze");
	mSndMagnet = LoadSound("magnet");

	mLoadingText = "";
	mCompletedLoadingThreadTasks = mNumLoadingThreadTasks;
}

// 0x407..0x408ca0: every atom type's pictures. The sheet holds three cells for each type (3k, 3k+1, 3k+2); the
// plain atoms and the chromium atom (types 0 to 11) also get 20 turned copies of the mask and the atom blended
// with each. Types 5, 7 and 9 lose a symbol's lines (the colour-blind fix: swirl, teeth, crescent).
void AtomicaApp::LoadAtoms()
{
	Sprite* aSwirl = mSprites->Load("colorizeswirl");
	mCompletedLoadingThreadTasks++;
	Sprite* aTeeth = mSprites->Load("colorizeteeth");
	mCompletedLoadingThreadTasks++;
	Sprite* aCrescent = mSprites->Load("colorizecrescent");
	mCompletedLoadingThreadTasks++;

	for (int aType = 0; aType < NUM_ATOM_TYPES; aType++)
	{
		mAtomA[aType] = mSprites->Cell(mAtomsSheet, 0x37, 0x37, aType * 3);
		mCompletedLoadingThreadTasks++;
		mAtomC[aType] = mSprites->Cell(mAtomsSheet, 0x37, 0x37, aType * 3 + 2);
		mCompletedLoadingThreadTasks++;
		mAtomShrink[aType][0] = mSprites->Cell(mAtomsSheet, 0x37, 0x37, aType * 3 + 1);
		mCompletedLoadingThreadTasks++;
		Sprite* aCells[3] = {mAtomA[aType], mAtomShrink[aType][0], mAtomC[aType]};
		for (int c = 0; c < 3; c++)
		{
			aCells[c]->mSrc.mX += 2;
			aCells[c]->mSrc.mY += 1;
			aCells[c]->mSrc.mWidth = 0x32;
			aCells[c]->mSrc.mHeight = 0x32;
			aCells[c]->mWidth = 0x32;
			aCells[c]->mHeight = 0x32;
		}
		for (int aStep = 1; aStep < 0x19; aStep++)
		{
			Sprite* aCopy = mSprites->Blank(aCells[1]->mWidth, aCells[1]->mHeight);
			mAtomShrink[aType][aStep] = aCopy;
			ShrinkInto(aCells[1], aCopy, aStep);
			mCompletedLoadingThreadTasks++;
		}

		if (aType < NUM_SPIN_TYPES)
		{
			Sprite* aMask = mSprites->Load("AtomMask");
			mCompletedLoadingThreadTasks++;
			// the mask turned a step at a time, with the unturned mask's transparency
			for (int j = 0; j < NUM_SPIN_FRAMES; j++)
			{
				Sprite* aTurned = mSprites->Blank(aMask->mWidth, aMask->mHeight);
				mAtomMask[aType][j] = aTurned;
				Graphics g(aTurned->mImage);
				double aRot = (double) ((float) (j * 18) * mPi * 0.0055555557f);
				g.DrawImageRotated(aMask->mImage, 0, 0, aRot);
				mCompletedLoadingThreadTasks++;
				unsigned char* aDest = (unsigned char*) aTurned->GetBits();
				unsigned char* aSrc = (unsigned char*) aMask->GetBits();
				for (int i = 0; i < aMask->mWidth * aMask->mHeight; i++)
					aDest[i * 4 + 3] = aSrc[i * 4 + 3];
				aTurned->BitsChanged();
			}
			mSprites->Free(aMask);
			for (int j = 0; j < NUM_SPIN_FRAMES; j++)
			{
				mAtomSpin[aType][j] = BlendSprites(mAtomShrink[aType][0], mAtomMask[aType][j], 0.29499998688697815);
				mCompletedLoadingThreadTasks++;
			}

			Sprite* aSymbol = NULL;
			if (aType == 5)
				aSymbol = aSwirl;
			else if (aType == 7)
				aSymbol = aTeeth;
			else if (aType == 9)
				aSymbol = aCrescent;
			if (aSymbol != NULL)
			{
				for (int j = 0; j < NUM_SPIN_FRAMES; j++)
				{
					SubtractMask(mAtomSpin[aType][j], aSymbol);
					SubtractMask(mAtomMask[aType][j], aSymbol);
				}
				for (int j = 0; j < NUM_ATOM_FRAMES; j++)
					SubtractMask(mAtomShrink[aType][j], aSymbol);
				SubtractMask(mAtomC[aType], aSymbol);
				SubtractMask(mAtomA[aType], aSymbol);
			}
		}
	}
	mSprites->Free(aSwirl);
	mSprites->Free(aTeeth);
	mSprites->Free(aCrescent);
}
