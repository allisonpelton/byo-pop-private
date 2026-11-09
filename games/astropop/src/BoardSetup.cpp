// Board: what the constructor and a new level set up: the hint panels (SetupHints, 0x475f50), the hints a player at a
// level has seen (MarkHintsSeen, 0x454410), the rules of the classic mode (SetupClassicRules, 0x44f670) and of the
// level mode's first level (SetupLevel1Rules, 0x47f3d0), the step to the next level (AdvanceLevel, 0x441980) and the
// demo's script (LoadDemo, 0x470130).

#include "Board.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "XcfFile.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include <stdlib.h>
#include <string.h>

using namespace Sexy;

// A float from the bits the original loads (the decompiler prints them as integers)
static float FloatBits(unsigned int theBits)
{
	float aFloat;
	memcpy(&aFloat, &theBits, sizeof(aFloat));
	return aFloat;
}

// A percentage of the data files, as a share (the original multiplies in extended precision before it stores a float)
static float Share(XcfFile* theFile, const std::string& thePath)
{
	return (float) ((double) theFile->GetFloat(thePath) * 0.01);
}

// A panel of the hint screens with the parts all of them set
static StageBg* NewPanel(Image* thePic, Image* theIcon, int theX, int theY, bool theHasSecond, unsigned int theTitle,
						 unsigned int theText, const Color& theColor1, const Color& theColor2)
{
	StageBg* aPanel = new StageBg();
	aPanel->mPic = thePic;
	aPanel->m5c = theIcon;
	aPanel->mX = theX;
	aPanel->mY = theY;
	if (theHasSecond)
	{
		aPanel->mX2 = theX;
		aPanel->mY2 = theY;
	}
	aPanel->mText = GT(theText);
	aPanel->mTitle = GT(theTitle);
	aPanel->mColor1 = theColor1;
	aPanel->mColor2 = theColor2;
	return aPanel;
}

// 0x475f50: the panels of the tutorial hints (m210: the first of each; m220 the two of the grab and release hints'
// chain) and the pop-ups that describe the bricks (m1d0). Every panel's title and text are written once into a picture
// that is not kept, to learn their heights.
void Board::SetupHints()
{
	// the grab hint, chained to the release hint and the match hint
	StageBg* aGrab = new StageBg();
	aGrab->mPic = IMAGE_TUT_MOUSE_LEFT;
	aGrab->m70.SetNumFrames(IMAGE_TUT_MOUSE_LEFT->mNumCols);
	aGrab->m70.SetDelay(0x32);
	aGrab->m70.SetStopped(false);
	aGrab->m70.mHoldLast = false;
	aGrab->mX = 0x1fe;
	aGrab->mY = 0x129;
	aGrab->mText = GT(0x566964);
	aGrab->mTitle = GT(0x56695c);
	aGrab->mfc = 300;
	aGrab->mColor2 = Color(0, 0xff, 0xf);
	aGrab->mColor1 = Color(0x1c, 0xf6, 0x4f);
	m210.push_back(aGrab);
	m220.push_back(aGrab);
	m220.push_back(aGrab);

	StageBg* aRelease = new StageBg();
	aRelease->mPic = IMAGE_TUT_MOUSE_RIGHT;
	aRelease->m114 = 0xff;
	aRelease->m100 = 0xff;
	aRelease->m70.SetNumFrames(IMAGE_TUT_MOUSE_RIGHT->mNumCols);
	aRelease->m70.SetDelay(0x32);
	aRelease->m70.SetStopped(false);
	aRelease->m70.mHoldLast = false;
	aRelease->mX = 0x1fe;
	aRelease->mY = 0x129;
	aRelease->mText = GT(0x566940);
	aRelease->mTitle = GT(0x566938);
	aRelease->mfc = 300;
	aRelease->mColor2 = Color(5, 0x5e, 0xb0);
	aRelease->mColor1 = Color(0x1c, 0x78, 0xf6);
	m210[0]->mNext = aRelease;

	StageBg* aMatch = new StageBg();
	aMatch->mPic = IMAGE_TH_MATCH;
	aMatch->mX = 0x1fe;
	aMatch->mY = 0x11c;
	aMatch->m114 = 0xff;
	aMatch->m100 = 0xff;
	aMatch->mText = GT(0x566904);
	aMatch->mTitle = GT(0x5668fc);
	aMatch->mfc = 300;
	aMatch->mColor2 = Color(0xf0, 0, 0xff);
	aMatch->mColor1 = Color(0xff, 0, 0xfa);
	m210[0]->mNext->mNext = aMatch;

	// the panels of the bricks (picture, small picture, the place; colour 1 then colour 2 as the original sets them)
	StageBg* aPanel;

	aPanel = NewPanel(IMAGE_TH_BONUS, IMAGE_FUELCELL, 0x118, 0x11c, true, 0x5668b0, 0x5668c0, Color(0, 0x87, 0xcd), Color(0, 0x34, 0xaa));
	aPanel->mf4 = 0x8c;
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_RADIAL, IMAGE_BRICK_RADIAL, 0x118, 0x11c, true, 0x566868, 0x566874, Color(0xff, 0xd2, 0), Color(0xe2, 0xa0, 0));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_STONE, IMAGE_BRICK_STONE, 0x118, 0x11c, true, 0x56682c, 0x566838, Color(0xb9, 0xaa, 0x91), Color(0xc0, 0xbb, 0xa3));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_ROW, IMAGE_BRICK_ROW_NUKE, 0x118, 0x11c, true, 0x5667e4, 0x5667f0, Color(0xff, 0xff, 0), Color(0xff, 0xfa, 0));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_STEEL, IMAGE_BRICK_STEEL_1, 0x118, 0x11c, true, 0x5667a4, 0x5667b0, Color(0xb9, 0xdc, 0xdc), Color(0xc0, 0xd7, 0xd7));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_BRICK_FREEZE, NULL, 0x118, 0x11c, false, 0x566758, 0x566764, Color(0, 0xff, 0xf0), Color(5, 0xaf, 0xaf));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_TOXIC, IMAGE_BRICK_MUTATOR, 0x118, 0x11c, true, 0x5666f8, 0x566708, Color(0x1c, 0xf6, 0x9c), Color(5, 0xb0, 0x96));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_COLOR, IMAGE_BRICK_KILLALL_PINK, 0x118, 0x11c, true, 0x5666c0, 0x5666cc, Color(0xc9, 0, 0xc9), Color(0xaf, 0, 0xaa));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_COLUMN, IMAGE_BRICK_COL_NUKE, 0x118, 0x11c, true, 0x56667c, 0x566688, Color(0xff, 0, 0), Color(0xb4, 0, 0));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_BOOSTER, IMAGE_BRICK_BOOSTER, 0x118, 0x11c, true, 0x56661c, 0x566628, Color(0xff, 0x50, 0xff), Color(0xe1, 0x32, 0xe1));
	m210.push_back(aPanel);
	aPanel = NewPanel(IMAGE_TH_SUPER, IMAGE_TH_SUPER_STILL, 0x118, 0x11c, true, 0x5665c0, 0x5665d8, Color(0xb9, 0xdc, 0xdc), Color(0xc0, 0xd7, 0xd7));
	m210.push_back(aPanel);

	// the heights of the titles and the texts: they are written into nothing
	for (unsigned int i = 0; i < m210.size(); i++)
	{
		Graphics g((Image*) NULL);
		for (StageBg* aStage = m210[i]; aStage != NULL; aStage = aStage->mNext)
		{
			g.SetFont(GetFontById(0x121));
			aStage->m120 = WriteWordWrapped(&g, Rect(0x8c, 0x11e, 0x96, 0x3c), aStage->mTitle, -1, 0);
			g.SetFont(GetFontById(0xb1));
			aStage->m124 = WriteWordWrapped(&g, Rect(300, 0x11a, 0xd2, 0x3c), aStage->mText, -1, 0);
		}
	}

	// the pop-ups that describe the bricks: the picture and three lines (the entry is pushed once for each place that
	// shows it: three times for the first, twice for the rest)
	TipEntry aTip;
	aTip.mImage = IMAGE_FUELCELL;
	aTip.mLines.push_back(GT(0x5665b4));
	aTip.mLines.push_back(GT(0x5665a4));
	aTip.mLines.push_back(GT(0x566578));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	aTip.mImage = IMAGE_BRICK_BOOSTER;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x5665b4));
	aTip.mLines.push_back(GT(0x56661c));
	aTip.mLines.push_back(GT(0x56655c));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	aTip.mImage = IMAGE_BRICK_RADIAL;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x5665b4));
	aTip.mLines.push_back(GT(0x566868));
	aTip.mLines.push_back(GT(0x566530));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	aTip.mImage = IMAGE_BRICK_STONE;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x566524));
	aTip.mLines.push_back(GT(0x56682c));
	aTip.mLines.push_back(GT(0x566500));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	aTip.mImage = IMAGE_BRICK_ROW_NUKE;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x5665b4));
	aTip.mLines.push_back(GT(0x5667e4));
	aTip.mLines.push_back(GT(0x5664d4));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	aTip.mImage = IMAGE_BRICK_STEEL_1;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x566524));
	aTip.mLines.push_back(GT(0x5667a4));
	aTip.mLines.push_back(GT(0x5664b4));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	// (from here on the entry carries the 10 the original leaves in its second field)
	aTip.mImage = IMAGE_BRICK_FREEZE;
	aTip.mValue = 10;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x5665b4));
	aTip.mLines.push_back(GT(0x566758));
	aTip.mLines.push_back(GT(0x566494));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	aTip.mImage = IMAGE_BRICK_MUTATOR;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x566524));
	aTip.mLines.push_back(GT(0x5666f8));
	aTip.mLines.push_back(GT(0x566470));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	aTip.mImage = IMAGE_BRICK_KILLALL_PINK;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x5665b4));
	aTip.mLines.push_back(GT(0x5666c0));
	aTip.mLines.push_back(GT(0x566444));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);

	aTip.mImage = IMAGE_BRICK_COL_NUKE;
	aTip.mLines.clear();
	aTip.mLines.push_back(GT(0x5665b4));
	aTip.mLines.push_back(GT(0x56667c));
	aTip.mLines.push_back(GT(0x566418));
	m1d0.push_back(aTip);
	m1d0.push_back(aTip);
}

// 0x454410: a game that starts at a higher level has seen the hints of the levels before: their flags in the profile are
// set (and the profile saved when there is one)
void Board::MarkHintsSeen(int theLevel)
{
	std::vector<int> aHints;
	if (theLevel > 2)
	{
		aHints.push_back(0x19);
		aHints.push_back(0x29);
	}
	if (theLevel > 3)
		aHints.push_back(0x27);
	if (theLevel > 4)
		aHints.push_back(0x25);
	if (theLevel > 5)
		aHints.push_back(0x15);
	if (theLevel > 6)
		aHints.push_back(0x1d);
	if (theLevel > 7)
		aHints.push_back(0x17);
	if (theLevel > 8)
		aHints.push_back(0x1b);
	if (theLevel > 9)
		aHints.push_back(0x1f);
	if (theLevel > 10)
		aHints.push_back(0x23);
	if (theLevel > 11)
		aHints.push_back(0x21);

	if (aHints.empty())
		return;
	for (unsigned int i = 0; i < aHints.size(); i++)
		mApp->mUser->SetInt(aHints[i], 1);
	mApp->mUser->Save();
}

// The distances (columns, rows) of the kinds in the classic mode, in the order of the rules (.data of the original at
// 0x59c628 and 0x59c648)
static const int kClassicNearSame[8] = {2, 2, 10, 5, 10, 10, 10, 5};
static const int kClassicNearRow[8] = {2, 2, 6, 6, 8, 4, 10, 4};

// 0x44f670: the classic mode (and the other modes of the game that have no level file): the speed table by the time
// played, and the share of each kind in the three phases of a game
void Board::SetupClassicRules()
{
	m19cc = 400;
	m1ac4 = 6;
	mDropTicks = 900;

	RampRecord aRamp;
	aRamp.mTime = 0;
	aRamp.mV1 = 400;
	aRamp.mV2 = 6;
	aRamp.mV3 = 900;
	aRamp.mF4 = 0;
	aRamp.mF5 = FloatBits(0x3f4ccccd);
	m1be8 = FloatBits(0x3f4ccccd);
	m1bec = 0;
	m280.push_back(aRamp);

	aRamp.mTime = 17999;
	aRamp.mV1 = 200;
	aRamp.mV2 = 7;
	aRamp.mV3 = 700;
	aRamp.mF4 = FloatBits(0x3f000000);
	aRamp.mF5 = FloatBits(0x3e99999a);
	m280.push_back(aRamp);

	aRamp.mTime = 18000;
	aRamp.mV1 = 300;
	aRamp.mV2 = 8;
	aRamp.mV3 = 0x28a;
	aRamp.mF4 = FloatBits(0x3f266666);
	aRamp.mF5 = FloatBits(0x3e99999a);
	m280.push_back(aRamp);

	aRamp.mTime = 30000;
	aRamp.mV1 = 200;
	aRamp.mV2 = 10;
	aRamp.mV3 = 600;
	aRamp.mF4 = FloatBits(0x3f400000);
	aRamp.mF5 = FloatBits(0x3e4ccccd);
	m280.push_back(aRamp);

	aRamp.mTime = 60000;
	aRamp.mV1 = 100;
	aRamp.mV2 = 0x10;
	aRamp.mV3 = 0x15e;
	aRamp.mF4 = FloatBits(0x3f733333);
	aRamp.mF5 = FloatBits(0x3c23d70a);
	m280.push_back(aRamp);

	m1a20 = 500000000;
	m1bf0 = 0;
	m1bf4 = 0;
	m1a18 = 500000000;
	m1a1c = 0;
	m1ac8 = 4;
	m1a28 = 0;
	m19c0 = 500000000;
	m19c4 = 500000000;
	m19c8 = 0;

	for (int k = 0; k < 8; k++)
	{
		SpawnRule& aRule = m180[0].mRule[k];
		aRule.mNearSame = kClassicNearSame[k];
		aRule.mNearRow = kClassicNearRow[k];

		float aShare;
		if (k == 0 || k == 1 || k == 6)
			aShare = FloatBits(0x3eaaaaab);
		else if (k == 4)
			aShare = FloatBits(0x3b449ba6);
		else
			aShare = FloatBits(0x3e7f3b64);
		for (int aPhase = 0; aPhase < 3; aPhase++)
		{
			aRule.mRates[aPhase].mRate = aShare;
			aRule.mRates[aPhase].m4 = 0;
		}
	}

	m1a88 = -1;
	m940 = 1.0f;
}

// the keys of the kinds in the data files, in the order of the rules
static const char* const kRuleKeys[8] = {"ROCK\\", "STEEL\\", "KILLALL\\", "RADIAL\\", "FREEZE\\", "ROW_NUKE\\", "MUTATOR\\", "COL_NUKE\\"};

// 0x47f3d0: the first level of the level mode, from \LEVEL1 of data\descriptor.xcf
void Board::SetupLevel1Rules()
{
	XcfFile* aFile = mApp->mDescriptor;

	m1a20 = aFile->HasKey("\\GLOBAL\\MaxBricksToPass") ? (int) aFile->GetInt("\\GLOBAL\\MaxBricksToPass") : 1000;
	m1bf0 = aFile->HasKey("\\LEVEL1\\BASE_FREQUENCY\\BonusChanceChangeBy") ? Share(aFile, "\\LEVEL1\\BASE_FREQUENCY\\BonusChanceChangeBy") : 0.0f;
	m1bf4 = aFile->HasKey("\\LEVEL1\\BASE_FREQUENCY\\PenaltyChanceChangeBy") ? Share(aFile, "\\LEVEL1\\BASE_FREQUENCY\\PenaltyChanceChangeBy") : 0.0f;
	m1be8 = Share(aFile, "\\LEVEL1\\BASE_FREQUENCY\\BonusChance");
	m1bec = Share(aFile, "\\LEVEL1\\BASE_FREQUENCY\\PenaltyChance");
	m1a18 = aFile->HasKey("\\LEVEL1\\NumBricksToPass") ? (int) aFile->GetInt("\\LEVEL1\\NumBricksToPass") : 200;
	if (aFile->HasKey("\\LEVEL1\\MutationDelay"))
		mDropTicks = (int) aFile->GetInt("\\LEVEL1\\MutationDelay");
	m1a1c = aFile->HasKey("\\LEVEL1\\NumBricksToPassChangeBy") ? (int) aFile->GetInt("\\LEVEL1\\NumBricksToPassChangeBy") : 0x14;
	m1ac8 = aFile->HasKey("\\LEVEL1\\NumRowsAtStart") ? (int) aFile->GetInt("\\LEVEL1\\NumRowsAtStart") : 4;
	if (aFile->HasKey("\\LEVEL1\\MaxMutators"))
		m1ac4 = (int) aFile->GetInt("\\LEVEL1\\MaxMutators");
	if (m1ac4 < 0)
		m1ac4 = 500000000;
	if (aFile->HasKey("\\LEVEL1\\AddNewColor"))
		m1a28++;
	if (m1a28 > 1)
		m1a28 = 2;
	int aRows = aFile->HasKey("\\LEVEL1\\NumRowsToPass") ? (int) aFile->GetInt("\\LEVEL1\\NumRowsToPass") : 0x19;
	m19c0 = aRows;
	m19c4 = aRows;
	m19c8 = aFile->HasKey("\\LEVEL1\\NumRowsToPassChangeBy") ? (int) aFile->GetInt("\\LEVEL1\\NumRowsToPassChangeBy") : 0;
	m19cc = (int) aFile->GetInt("\\LEVEL1\\DropTime") * 100;

	for (int k = 0; k < 8; k++)
	{
		std::string aKey = std::string("\\LEVEL1\\") + kRuleKeys[k];
		SpawnRule& aRule = m180[0].mRule[k];
		aRule.mNearSame = (int) aFile->GetInt(aKey + "ColDist");
		aRule.mNearRow = (int) aFile->GetInt(aKey + "RowDist");
		aRule.mRates[0].mRate = Share(aFile, aKey + "FirstChance");
		aRule.mRates[1].mRate = Share(aFile, aKey + "SecondChance");
		aRule.mRates[2].mRate = Share(aFile, aKey + "ThirdChance");
		aRule.mRates[0].m4 = aFile->HasKey(aKey + "FirstChanceChangeBy") ? Share(aFile, aKey + "FirstChanceChangeBy") : 0.0f;
		aRule.mRates[1].m4 = aFile->HasKey(aKey + "SecondChanceChangeBy") ? Share(aFile, aKey + "SecondChanceChangeBy") : 0.0f;
		aRule.mRates[2].m4 = aFile->HasKey(aKey + "ThirdChanceChangeBy") ? Share(aFile, aKey + "ThirdChanceChangeBy") : 0.0f;
	}

	m1a88 = -1;
	// (computed in extended precision in the original: exact for these numbers)
	m940 = (float) ((double) m1940 * 0.125 + 1.0);
	if (m940 < 1.0f)
		m940 = 1.0f;
	if (6.0f < m940)
		m940 = 6.0f;
	BuildStoryText();
}

// The number after the five letters of the name of a section of the descriptor (LEVEL<n>)
static long LevelNumberOf(const std::string& theName)
{
	if (theName.size() < 5)
		return 0;
	return atol(theName.substr(5).c_str());
}

// 0x441980: the next level begins (theSave: the profile takes the results of the level that ended): the player's best
// level and score are kept, and the settings of the level come from its section of data\descriptor.xcf, or are stepped
// up from the level before when the file has none
void Board::AdvanceLevel(bool theSave)
{
	UserProfile* aUser = mApp->mUser;
	if (theSave)
	{
		if (m1a60 == 1)
		{
			int aValue = 0;
			aUser->GetInt(mMode + 8, &aValue);
			if (aValue < m1940)
				aUser->SetInt(mMode + 8, m1940);

			aValue = 0;
			aUser->GetInt(mMode, &aValue);
			if (aValue < mScore)
			{
				// (the original stores the value it has just read back: a no-op, kept)
				aUser->SetInt(mMode, aValue);
				aUser->SetInt(mMode + 4, mField193C);
			}

			aValue = 0;
			if (m1a24 < m7c4)
				m1a24 = m7c4;
			aUser->GetInt(0xc, &aValue);
			if (aValue < m7c4)
				aUser->SetInt(0xc, aValue);     // (no-op as well)

			aValue = 0;
			aUser->GetInt(0x2f, &aValue);
			aValue += m7c8;
			aUser->SetInt(0x2f, aValue);
		}
		aUser->Save();
	}

	XcfFile* aFile = mApp->mDescriptor;
	aFile->ResetCursor();
	long aNumber = LevelNumberOf(aFile->GetCurrentName());
	if (aNumber < mField193C)
	{
		int aResult;
		do
		{
			aResult = aFile->NextSibling();
			aNumber = LevelNumberOf(aFile->GetCurrentName());
		}
		while (aResult != XcfFile::CURSOR_END && aNumber < mField193C);
	}

	aNumber = LevelNumberOf(aFile->GetCurrentName());
	if (aNumber == mField193C)
	{
		// this level has a section of its own
		if (aFile->HasKey("BASE_FREQUENCY\\BonusChanceChangeBy"))
			m1bf0 = Share(aFile, "BASE_FREQUENCY\\BonusChanceChangeBy");
		if (aFile->HasKey("BASE_FREQUENCY\\PenaltyChanceChangeBy"))
			m1bf4 = Share(aFile, "BASE_FREQUENCY\\PenaltyChanceChangeBy");
		if (aFile->HasKey("BASE_FREQUENCY\\BonusChance"))
			m1be8 = Share(aFile, "BASE_FREQUENCY\\BonusChance");
		else
			m1be8 = (float) ((double) m1bf0 + (double) m1be8);
		if (aFile->HasKey("BASE_FREQUENCY\\PenaltyChance"))
			m1bec = Share(aFile, "BASE_FREQUENCY\\PenaltyChance");
		else
			m1bec = (float) ((double) m1bf4 + (double) m1bec);

		m1ac8 = aFile->HasKey("NumRowsAtStart") ? (int) aFile->GetInt("NumRowsAtStart") : 4;
		if (aFile->HasKey("MutationDelay"))
			mDropTicks = (int) aFile->GetInt("MutationDelay");
		m1a18 = aFile->HasKey("NumBricksToPass") ? (int) aFile->GetInt("NumBricksToPass") : m1a18 + m1a1c;
		if (aFile->HasKey("NumBricksToPassChangeBy"))
			m1a1c = (int) aFile->GetInt("NumBricksToPassChangeBy");
		if (aFile->HasKey("NumRowsToPassChangeBy"))
			m19c8 = (int) aFile->GetInt("NumRowsToPassChangeBy");
		if (aFile->HasKey("NumRowsToPass"))
			m19c4 = (int) aFile->GetInt("NumRowsToPass");
		else
			m19c4 = m19c4 + m19c8;
		m19c0 = m19c4;
		if (aFile->HasKey("MaxMutators"))
			m1ac4 = (int) aFile->GetInt("MaxMutators");
		if (m1ac4 < 0)
			m1ac4 = 500000000;
		if (aFile->HasKey("AddNewColor"))
			m1a28++;
		if (m1a28 > 1)
			m1a28 = 2;
		if (aFile->HasKey("DropTime"))
			m19cc = (int) aFile->GetInt("DropTime") * 100;
		if (m19cc < 1)
			m19cc = 1;

		for (int k = 0; k < 8; k++)
		{
			std::string aKey = kRuleKeys[k];
			SpawnRule& aRule = m180[0].mRule[k];
			if (aFile->HasKey(aKey + "ColDist"))
				aRule.mNearSame = (int) aFile->GetInt(aKey + "ColDist");
			if (aFile->HasKey(aKey + "RowDist"))
				aRule.mNearRow = (int) aFile->GetInt(aKey + "RowDist");
			if (aFile->HasKey(aKey + "FirstChanceChangeBy"))
				aRule.mRates[0].m4 = Share(aFile, aKey + "FirstChanceChangeBy");
			if (aFile->HasKey(aKey + "SecondChanceChangeBy"))
				aRule.mRates[1].m4 = Share(aFile, aKey + "SecondChanceChangeBy");
			if (aFile->HasKey(aKey + "ThirdChanceChangeBy"))
				aRule.mRates[2].m4 = Share(aFile, aKey + "ThirdChanceChangeBy");
			if (aFile->HasKey(aKey + "FirstChance"))
				aRule.mRates[0].mRate = Share(aFile, aKey + "FirstChance");
			else
				aRule.mRates[0].mRate = (float) ((double) aRule.mRates[0].m4 + (double) aRule.mRates[0].mRate);
			if (aFile->HasKey(aKey + "SecondChance"))
				aRule.mRates[1].mRate = Share(aFile, aKey + "SecondChance");
			else
				aRule.mRates[1].mRate = (float) ((double) aRule.mRates[1].m4 + (double) aRule.mRates[1].mRate);
			if (aFile->HasKey(aKey + "ThirdChance"))
				aRule.mRates[2].mRate = Share(aFile, aKey + "ThirdChance");
			else
				aRule.mRates[2].mRate = (float) ((double) aRule.mRates[2].m4 + (double) aRule.mRates[2].mRate);
		}
	}
	else
	{
		// no section: every fifth level the drop gets quicker, the rest steps up by the changes of the level before
		if (mField193C % 5 == 0)
			m19cc--;
		if (m19cc < 1)
			m19cc = 1;
		int aRows = m19c4 + m19c8;
		m1be8 = (float) ((double) m1bf0 + (double) m1be8);
		m19c0 = aRows;
		m19c4 = aRows;
		m1a18 = m1a18 + m1a1c;
		m1bec = (float) ((double) m1bf4 + (double) m1bec);
		for (int k = 0; k < 8; k++)
		{
			SpawnRule& aRule = m180[0].mRule[k];
			for (int aPhase = 0; aPhase < 3; aPhase++)
				aRule.mRates[aPhase].mRate = (float) ((double) aRule.mRates[aPhase].m4 + (double) aRule.mRates[aPhase].mRate);
		}
	}

	int aLevel = mField193C;
	if (m1bbc <= aLevel)
	{
		// the pieces come quicker every second level
		m19d4 -= (aLevel % 2 == 0) ? 1 : 0;
		if (m19d4 < 5)
			m19d4 = 5;
	}
	if (m19c4 > 0x46)
	{
		m19c0 = 0x46;
		m19c4 = 0x46;
	}
	if (m1a20 < m1a18)
		m1a18 = m1a20;
	m1a14 = 0;

	if (m1a60 == 1)
	{
		m1a04 = aLevel + 2;
		m1a00 = aLevel + 1;
		int aHalf = (aLevel + 2) / 2;
		int aRest = (aHalf + 1) % 4;
		m1a08 = (aRest == 0) ? -1 : (aHalf - aRest) * 2 + 8;
		if (m1a0c * 2 - 2 < m1a08 || m1a08 == -1)
		{
			m1a08 = -1;
			m1a70 = 0x17d - IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2;
			m1a74 = -1;
		}
		if (m1a08 < 1)
		{
			m1a70 = 0x17d - IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2;
			m1a74 = -1;
		}
		else
		{
			m1a70 = 0xb6;
			m1a74 = 0x192;
		}
	}

	if (m1bbc <= mField193C)
	{
		m1b0c = ((m1bbc - mField193C) + 0x3c) * 100;
		if (m1b0c < 0x1194)
			m1b0c = 0x1194;
		m1b08 = m1b0c;
	}
}

// 0x470130: the script the title's demo plays, from data\demo.xcf: twenty rows of ten brick codes for the field (m334),
// the rows that are added during the demo (m344) and its commands (m2d0: a wait of mArg updates or a click or move)
void Board::LoadDemo()
{
	XcfFile aFile;

	m954 = 0;
	aFile.AddKeyName("Command");
	aFile.AddKeyName("Idle");
	aFile.AddKeyName("NumAdded");
	aFile.AddKeyName("Brick");
	aFile.AddKeyName("NumCommands");
	int aResult = aFile.Parse("data\\demo.xcf");
	if (aResult != 0)
	{
		mApp->Popup(GT(0x566288));
		mApp->Shutdown();
		return;
	}

	m334.resize(20);
	for (unsigned int i = 0; i < m334.size(); i++)
		m334[i].resize(10);
	std::vector<std::vector<int> >().swap(m344);

	if (!aFile.EnterFirstChild())
	{
		mApp->Popup(StrFormat(GTC(0x566200), (int) XcfFile::CURSOR_END, aFile.mLine));
		mApp->Shutdown();
		return;
	}

	for (int aRow = 0; aRow < 20; aRow++)
	{
		for (int i = 0; i < 10; i++)
			m334[aRow][i] = (int) aFile.GetInt(StrFormat("Brick%d", i));
		// (the last row does not look further: the cursor stays where it is)
		if (aRow >= 19)
			continue;
		int aNext = aFile.NextSibling();
		if (aNext != 0)
		{
			mApp->Popup(StrFormat(GTC(0x566200), aNext, aFile.mLine));
			mApp->Shutdown();
			return;
		}
	}

	int aNumAdded = (int) aFile.GetInt("\\ADDEDROWS\\NumAdded");
	for (int aRowNumber = 0; aRowNumber < aNumAdded; aRowNumber++)
	{
		std::vector<int> aRow(10, 0);
		for (int i = 0; i < 10; i++)
			aRow[i] = (int) aFile.GetInt(StrFormat("\\ADDEDROWS\\ROW%d\\Brick%d", aRowNumber, i));
		m344.push_back(aRow);
	}

	int aFound = aFile.FindSection("COMMANDS");
	if (aFound != 0)
	{
		mApp->Popup(StrFormat(GTC(0x566200), aFound, aFile.mLine));
		mApp->Shutdown();
		return;
	}

	int aNumCommands = (int) aFile.GetInt("NumCommands");
	std::vector<DemoStep>().swap(m2d0);
	for (int aNumber = 1; aNumber <= aNumCommands; aNumber++)
	{
		DemoStep aStep;
		if (aFile.HasKey(StrFormat("Command%d", aNumber)))
		{
			aStep.mAction = (int) aFile.GetInt(StrFormat("Command%d", aNumber));
			aStep.mArg = 0;
		}
		else
		{
			aStep.mAction = 0;
			aStep.mArg = (int) aFile.GetInt(StrFormat("Idle%d", aNumber));
		}
		m2d0.push_back(aStep);
	}
}
