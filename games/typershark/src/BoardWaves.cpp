// BoardWaves.cpp: the Board's wave spawner (job B3): SpawnWave builds every creature of the lesson's next wave from its enemy
// records, PlaceEnemies and PlaceBonusCreatures spread the new ones over the screen, AddPiranha is the penalty piranha that
// a typo in a piranha wave sends.
//
// SpawnWave shows the two hint boxes before a boss (ShowHintDialog).
//
// The lists hold the Enemy part of each creature (Board.h). The original's SpawnWave keeps a creature in one local per kind
// and converts it to the part it needs at the places where the code below names the class. The original's second argument of
// the Piranha constructors is the diver (mPlayer, a pointer); the Piranha keeps it in a field that nothing reads (the port
// declares that field an int, Piranha.h), so the port passes 0.
//
// Draws of random numbers all come from the game's MTRand (the original's 0x463a10 on the object at 0x565384).
//
// Four small helpers of the original's SpawnWave are written in line: 0x445df0 (BlackShark: mWord2 = the word) and 0x445e50
// (RedShark: mWord3 = the word) are field assignments here, and 0x44c900 (std::map insert of a key and a null value) and
// 0x450bf0 (std::map operator[] on the key) are mRecordsByType[kind] = record.

#include "Board.h"
#include "Enemy.h"
#include "Shark.h"
#include "BlackShark.h"
#include "RedShark.h"
#include "StealthShark.h"
#include "StealthBlackShark.h"
#include "StealthRedShark.h"
#include "ToxicShark.h"
#include "GhostShark.h"
#include "Piranha.h"
#include "RedPiranha.h"
#include "WhitePiranha.h"
#include "StealthPiranha.h"
#include "StealthWhitePiranha.h"
#include "BonusCreature.h"
#include "TorpedoBoss.h"
#include "GhostShip.h"
#include "MechaShark.h"
#include "MechaSquid.h"
#include "WSTimer.h"
#include "LessonManager.h"
#include "UserProfile.h"
#include "GameText.h"
#include "MTRand.h"
#include "Res.h"
#include "ImageFont.h"

using namespace Sexy;

// 0x443a70 (the export names it a WinWSApp member; only SpawnWave calls it): forgets the words and letters handed out so
// far and the theme of the game
static void ResetWordMemory(WinWSApp* theApp)
{
	theApp->mUsedLetters.clear();
	theApp->mTheme = -1;
	theApp->mUsedUpperCount = 0;
	theApp->mUsedLetterCount = 0;
}

// 0x442540: spreads the bonus creatures from theFirstNew on in rows of four or five, from the diver's right side: a row's
// first x is the diver's x plus the diver's frame width, its y the screen height plus 25 plus one frame height per row;
// each creature is moved a little at random in y (0 to 24) and the gap to the next one is its width plus 0 to 29. The first
// draw only passes the time (its value is not used; the original also asks the lesson for the wave's column counts and the
// widths of the first creatures and does nothing with the answers).
void Board::PlaceBonusCreatures(int theFirstNew)
{
	mLessonManager->GetWaveMaxCol();
	mLessonManager->GetWaveMinCol();
	mLessonManager->GetWaveMinCol();
	gGameRand->NextNoAssert();			// the value is not used

	if (theFirstNew < 0)
		theFirstNew = 0;
	int aCount = (int)mBonusCreatures.size();
	int aRowLength = (int)(gGameRand->NextNoAssert() & 1) + 4;
	int aRow = 0;
	int aPos = 0;
	double aX = 0.0;
	double aRowY = 0.0;
	int anIndex = theFirstNew;
	while (anIndex < aCount)
	{
		Enemy* aCreature = mBonusCreatures[anIndex];
		int aFrameHeight = aCreature->GetFrameHeight();
		aCreature->GetFrameWidth();		// the result is not used
		mPlayer->GetFrameWidth();		// nor is this one
		if (aPos % aRowLength == 0)
		{
			aPos = 0;
			if (anIndex > 0)
			{
				aRowLength = (int)(gGameRand->NextNoAssert() & 1) + 4;
				aRow++;
			}
			aX = (double)mPlayer->GetFrameWidth() + mPlayer->mX;
			aRowY = (double)(aFrameHeight * aRow + mHeight + 25);
		}
		aCreature->SetX(aX);
		aCreature->SetY((double)(unsigned int)(gGameRand->NextNoAssert() % 25) + aRowY);
		aX = (double)aCreature->GetFrameWidth() + aX;
		aX = (double)(unsigned int)(gGameRand->NextNoAssert() % 30) + aX;
		aCreature->SetVisible(true);
		anIndex++;
		aPos++;
	}
}

// 0x452050: spreads the sharks (wave type 0) or the piranhas (type 1) from theFirstNew on over the lanes: the wave has aCols
// creatures in a row (a draw between the lesson's column limits), a creature's lane is aSlot of aCols lanes of the height
// that is left above the floor, and it starts off the right edge. The first row starts at the width plus 0 to 14 (the draws
// are made even where their value is not used); every later creature starts behind the one aCols places before it
// (aPrev), as far to the right as it needs to stay behind it: from the previous one's x, its word width (aWordWidth) and the
// two speeds, so that the faster one does not catch up before it reaches the diver (x 400 at the start of the screen's left
// part). A stealth creature behind or in front of a normal one is placed by the other formulas. The creature also gets a
// speed below the previous one's (the record's limits keep it in), and aPrev gets the creature as its target.
// The vector of speeds is never read (it only collects the first row's speeds).
void Board::PlaceEnemies(int theFirstNew)
{
	int aFloorLimit = mFloorY - mApp->mFontRockwell13->GetHeight();
	std::vector<int> aSpeeds;			// never read
	bool aFirstRow = true;
	int aRange = mLessonManager->GetWaveMaxCol() - mLessonManager->GetWaveMinCol();
	if (aRange == 0)
		aRange = 1;
	int aCols = (int)(gGameRand->NextNoAssert() % (unsigned int)aRange) + mLessonManager->GetWaveMinCol();
	int aWaveType = mLessonManager->GetWaveType();
	if (theFirstNew < 0)
		theFirstNew = 0;

	int aCount = 0;						// port: the original leaves it uninitialised for any other wave type
	if (aWaveType == 0)
		aCount = (int)mSharks.size();
	else if (aWaveType == 1)
		aCount = (int)mPiranhas.size();

	int anIndex = theFirstNew;
	int aSlot = 0;
	while (anIndex < aCount)
	{
		int aTextWidth = mApp->mFontMyriadCondensed14Bold->StringWidth("A");		// 0x52f900
		if (aSlot % aCols == 0)
		{
			aSlot = 0;
			if (theFirstNew < anIndex)
				aFirstRow = false;
		}

		Enemy* aCur;
		Enemy* aPrev;
		double aSpan;
		if (aWaveType == 0)
		{
			aCur = mSharks[anIndex];
			aSpan = 3.0;
			aPrev = (anIndex - aCols < theFirstNew) ? NULL : mSharks[anIndex - aCols];
		}
		else
		{
			aCur = mPiranhas[anIndex];
			aSpan = 4.0;
			aPrev = (anIndex - aCols < theFirstNew) ? NULL : mPiranhas[anIndex - aCols];
		}

		int aFrameHeight = aCur->GetFrameHeight();
		int aWidth = aCur->GetFrameWidth();
		int anAvailable = aFloorLimit - aFrameHeight;
		int aLaneHeight = anAvailable / aCols;
		double aLaneHeightD = (double)aLaneHeight;
		unsigned int aDraw = (unsigned int)gGameRand->NextNoAssert();
		unsigned int aDivisor = (unsigned int)((int)aLaneHeightD - 40);
		double aY = (double)(aDraw % aDivisor) + (double)aSlot * aLaneHeightD + 25.0;
		if (aY > (double)anAvailable)
			aY = (double)anAvailable;
		double aXBase = (double)(unsigned int)(gGameRand->NextNoAssert() % 20) + 30.0;
		aCur->SetY(aY);
		if (aPrev != NULL)
		{
			aPrev->SetTarget(aCur);
			aCur->SetAttacker(aPrev);
		}

		if (aFirstRow)
		{
			aCur->SetVisible(true);
			gGameRand->NextNoAssert();			// the value is not used
			aCur->SetX((double)(unsigned int)((unsigned int)(gGameRand->NextNoAssert() % 15) + (unsigned int)mWidth));
			aSpeeds.push_back((int)aCur->mVelX);
		}
		else
		{
			bool aCurStealth = false;
			bool aPrevStealth = false;
			int aWordWidth;
			double aPrevVelX = aPrev->mVelX;
			double aPrevX = aPrev->mX;
			int aPrevType = aPrev->mField0e4;
			int aCurType = aCur->mField0e4;
			if (aWaveType == 0)
			{
				if (aPrevType == 10 || aPrevType == 3 || aPrevType == 9)
					aPrevStealth = true;
				if (aCurType == 10 || aCurType == 3 || aCurType == 9)
					aCurStealth = true;
				if (aPrevType == 1 || aPrevType == 10)
					aWordWidth = (int)(aPrev->mWord.size() + 1) * aTextWidth;
				else if (aPrevType == 2 || aPrevType == 9)
					aWordWidth = (int)(aPrev->mWord.size() + dynamic_cast<RedShark*>(aPrev)->mWord2.size() + 2) * aTextWidth;
				else
					aWordWidth = 0;
			}
			else
			{
				if (aPrevType == 3 || aPrevType == 7)
					aPrevStealth = true;
				if (aCurType == 3 || aCurType == 7)
					aCurStealth = true;
				if (aPrevType == 6 || aPrevType == 7)
					aWordWidth = 30;
				else
					aWordWidth = 0;
			}

			// the speed: below the previous one's, inside the record's limits
			EnemyRecord* aRecord = mRecordsByType[aCur->mField0e4];
			if (aPrevVelX > aRecord->mMaxSpeed)
				aPrevVelX = aRecord->mMaxSpeed;
			double aMinSpeed = aRecord->mMinSpeed;
			if (aMinSpeed >= aPrevVelX)
				aMinSpeed = aPrevVelX - 0.001;
			int aSteps = (int)((aPrevVelX - aMinSpeed) * 1000.0);
			if (aSteps == 0)
				aSteps = 1;
			double aVel = (double)(unsigned int)(gGameRand->NextNoAssert() % (unsigned int)aSteps) * 0.001 + aMinSpeed;
			aCur->SetVelX(aVel);

			// the place
			if (aCurStealth && aPrevStealth)
			{
				// both are stealth creatures
				double aPrevSpeed = aPrev->GetSpeed();
				aCur->SetX((double)(aWordWidth + aWidth) / aPrevSpeed * aSpan + aPrevX + aXBase);
			}
			else
			{
				if (aCurStealth)
				{
					// a stealth creature behind a normal one goes at its own base speed
					if (aWaveType == 0)
					{
						dynamic_cast<StealthShark*>(aCur)->ResetSpeed();
						aSpan = 3.0;
					}
					else if (aWaveType == 1)
					{
						dynamic_cast<StealthPiranha*>(aCur)->ResetSpeed();
						aSpan = 4.0;
					}
				}

				if (!aCurStealth && aPrevStealth)
				{
					// a normal creature behind a stealth one: it must not catch up before the stealth one shows (x 400)
					double aPrevSpeed = aPrev->GetSpeed();
					if (aCur->GetSpeed() > aPrevSpeed)
					{
						double aPlayerX = mPlayer->mX;
						double aTotal = (double)(aWordWidth + aWidth) + (400.0 - aPlayerX);
						double aTime1 = aTotal / aPrev->GetSpeed() + (aPrevX - 400.0) / aSpan;
						double aSum = (double)aWidth + (double)aWordWidth;
						double aTime2 = (aSum + aPrevX - mPlayer->mX) / aVel;
						if (aTime1 > aTime2)
							aCur->SetX((((aTime1 - aTime2) * aVel + (double)aWidth) + (double)aWordWidth) + aPrevX);
						else
							aCur->SetX(aSum + aPrevX);
					}
					else
						aCur->SetX((((double)aWordWidth + (double)aWidth) + aPrevX) + aXBase);
				}
				else
				{
					double aPrevSpeed = aPrev->GetSpeed();
					if (aCur->GetSpeed() > aPrevSpeed)
					{
						double aTime = (double)(mWidth + aWordWidth + aWidth) + aXBase - mPlayer->mX;
						aCur->SetX((((double)aWordWidth + (double)aWidth) + (aTime / aPrev->GetSpeed()) * aVel) + aPrevX);
					}
					else
						aCur->SetX((((double)aWordWidth + (double)aWidth) + aPrevX) + aXBase);
				}
			}
		}

		anIndex++;
		aSlot++;
	}
}

// 0x452a60: a penalty piranha (the typos of a piranha wave send it): only when a piranha that is not dying (state 3) is on
// the screen. A red piranha with a letter nobody has, at the right edge and a random height, as fast as the lesson's
// penalty speed; from the sixth on, the speed grows by 1.7 times each and the piranha takes 0.4 of it. The first piranha clears
// mFlag0f80 (the board waits for the wave to be over). (Board.h said "when no shark is in state 3"; the code looks at the
// piranhas.)
void Board::AddPiranha()
{
	size_t anIndex = 0;
	while (true)
	{
		if (anIndex >= mPiranhas.size())
			return;
		if (mPiranhas[anIndex]->mState != 3)
			break;
		anIndex++;
	}

	std::string aLetter = mApp->RandomLetter(true);
	if (aLetter == "")
		return;

	RedPiranha* aPiranha = new RedPiranha(mApp, 0);			// port: the original passes mPlayer (the field behind it is never read)
	aPiranha->SetX((double)(mWidth - 0x32));
	unsigned int aDraw = (unsigned int)gGameRand->NextNoAssert();
	unsigned int aDivisor = (unsigned int)(int)((double)mFloorY - 25.0 - 100.0);
	aPiranha->SetY((double)(unsigned int)(aDraw % aDivisor) + 25.0);
	aPiranha->SetWord(aLetter);
	aPiranha->SetVelX(mLessonManager->GetLevelSettings()->mPenaltySpeed);
	mPiranhaCount++;
	if (mPiranhaCount >= 6)
	{
		mPiranhaSpeed = mPiranhaSpeed * 1.7;
		aPiranha->SetVelX(mPiranhaSpeed * 0.4);
	}
	mPiranhas.push_back(aPiranha);
	mFlag0f80 = false;
}

// 0x455bf0: makes the creatures of the lesson's next wave. A bonus level has none. The spawner starts afresh (the word
// memory, the counters of the wave, the records by type). In the abyss mode a flag set by SetDepth / Update says that a boss
// comes (one of four by mField0d2c mod 4). Otherwise: when the last wave of the level is the boss wave and the diver is
// at the surface, the lesson goes to it; a player who has not seen the tutorial gets two sharks with fixed words; and the
// wave's type says what to build from the enemy records: 0 sharks (PlaceEnemies), 1 piranhas (PlaceEnemies), 2 bonus
// creatures (PlaceBonusCreatures), 3 a boss with its hint box. Each record gives a random count (between its minimum and
// maximum number) of creatures of one kind, a word (or words) of the record's length and theme, a speed inside the record's
// limits. Then the lesson draws the wave after this one (NextWave).
void Board::SpawnWave()
{
	if (mLessonManager->IsBonusLevel())
		return;

	mField0d64 = 0;
	int aFirstNew = -1;
	ResetWordMemory(mApp);
	mField0f60 = 0;
	mField0f64 = 0;
	mRecordsByType.clear();
	mApp->mUsedLetters = "";
	mApp->mUsedLetterCount = 0;
	mApp->mUsedUpperCount = 0;

	if (mLessonManager->mAbyssMode && mFlag0d28)
	{
		int aBossKind = mField0d2c % 4;
		mFlag0d28 = false;
		if (aBossKind == 0)
		{
			mBoss = new TorpedoBoss(mApp, mPlayer, mLessonManager);
			mBoss->mField0e4 = 0xc;
		}
		else if (aBossKind == 1)
		{
			mBoss = new MechaShark(mApp, mPlayer, mLessonManager);
			mBoss->mField0e4 = 0xf;
		}
		else if (aBossKind == 2)
		{
			mBoss = new MechaSquid(mApp, mPlayer, mLessonManager);
			mBoss->mField0e4 = 0x10;
		}
		else if (aBossKind == 3)
		{
			mBoss = new GhostShip(mApp, mPlayer, mLessonManager);
			mBoss->mField0e4 = 0xe;
		}
		mFlag0f80 = false;
		mPlayer->SetState(9);
		return;
	}

	if (mLessonManager->LastWaveIsBoss() && !mPlayer->mFlag118 && mPlayer->mState == 0 &&
		(double)mSurfaceHeight < mDivePos && !mLessonManager->IsEndless())
		mLessonManager->GoToBossWave();
	mFlag0f80 = false;
	mFlag0eb5 = false;

	if (!mApp->mUser->mFlag1dc)
	{
		// the tutorial's two sharks
		Shark* aShark = new Shark(mApp);
		aShark->SetWord(GT(0x00530ee0));
		aShark->SetX((double)(mWidth - 50));
		aShark->SetY(175.0);
		aShark->SetVelX(0.2);
		mSharks.push_back(aShark);

		aShark = new Shark(mApp);
		aShark->SetWord(GT(0x00530ed8));
		aShark->SetX((double)(mWidth - 50));
		aShark->SetY((double)aShark->GetFrameHeight() + 245.0);
		aShark->SetVelX(0.2);
		mSharks.push_back(aShark);
		return;
	}

	int aWaveType = mLessonManager->GetWaveType();
	if (aWaveType == 0)
	{
		// the sharks
		int aRecordIndex = 0;
		if (mLessonManager->GetEnemyRecordCount() > 0)
		{
			do
			{
				EnemyRecord* aRecord = mLessonManager->GetEnemyRecord(aRecordIndex);
				int aMinLength = aRecord->mWordMinLength;
				int aMaxLength = aRecord->mWordMaxLength;
				int aRepeat = aRecord->mMaxNumber - aRecord->mMinNumber;
				if (aRepeat <= 0)
					aRepeat = 1;
				int aCount = (int)(gGameRand->NextNoAssert() % (unsigned int)aRepeat) + aRecord->mMinNumber;
				if (aCount > 0)
				{
					int aMade = 0;
					do
					{
						int aKind = aRecord->mType;
						bool aThemed = false;
						bool aPlain = !aRecord->mThemeRandom && aRecord->mThemeMax == -1 && aRecord->mThemeMin == -1;
						Shark* aCreature = NULL;
						bool aKeep = true;
						if (aKind == 0)
						{
							mRecordsByType[0] = aRecord;
							aCreature = new Shark(mApp);
							if (aPlain)
								aCreature->SetWord(mApp->GetWord(aMinLength, aMaxLength));
							else
							{
								aCreature->SetWord(mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax));
								aThemed = true;
							}
						}
						else if (aKind == 1 || aKind == 8)
						{
							mRecordsByType[aKind] = aRecord;
							BlackShark* aBlack = new BlackShark(mApp);
							aCreature = aBlack;
							if (aPlain)
							{
								aBlack->SetWord(mApp->GetWord(aMinLength, aMaxLength));
								aBlack->mWord2 = mApp->GetWord(aMinLength, aMaxLength);
							}
							else
							{
								aBlack->SetWord(mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax));
								aBlack->mWord2 = mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax);
								aThemed = true;
							}
						}
						else if (aKind == 2)
						{
							mRecordsByType[2] = aRecord;
							RedShark* aRed = new RedShark(mApp);
							aCreature = aRed;
							if (aPlain)
							{
								aRed->SetWord(mApp->GetWord(aMinLength, aMaxLength));
								aRed->mWord2 = mApp->GetWord(aMinLength, aMaxLength);
								aRed->mWord3 = mApp->GetWord(aMinLength, aMaxLength);
							}
							else
							{
								aRed->SetWord(mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax));
								aRed->mWord2 = mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax);
								aRed->mWord3 = mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax);
								aThemed = true;
							}
						}
						else if (aKind == 4 || aKind == 5)
						{
							mRecordsByType[aRecord->mType] = aRecord;
							ToxicShark* aToxic = new ToxicShark(mApp);
							aCreature = aToxic;
							aToxic->SetWord(mApp->GetWord(aMinLength, aMaxLength));
							aToxic->mScramblePeriod = mLessonManager->GetLevelSettings()->mLetterDelay;
						}
						else if (aKind == 0xd)
						{
							// a ghost shark: a random letter nobody has and a few more
							mRecordsByType[aRecord->mType] = aRecord;
							aCreature = new GhostShark(mApp);
							std::string aWord = mApp->RandomLetter(true);
							int aSpread = aMaxLength - aMinLength;
							if (aSpread == 0)
								aSpread = 1;
							int aLength = (int)(gGameRand->NextNoAssert() % (unsigned int)aSpread) + aMinLength;
							if (aLength > 1)
							{
								int aMore = aLength - 1;
								do
								{
									char aLetter = (char)((int)(gGameRand->NextNoAssert() % 26) + 0x41);
									aWord = StrFormat("%s%c", aWord.c_str(), (int)aLetter);		// 0x530ec8
									aMore--;
								}
								while (aMore != 0);
							}
							aCreature->SetWord(aWord);
							if (aCreature->mWord == "")
							{
								// no letter was left: the creature is dropped
								delete aCreature;
								aCreature = NULL;
								aKeep = false;
							}
						}
						else if (aKind == 3)
						{
							mRecordsByType[aRecord->mType] = aRecord;
							StealthShark* aStealth = new StealthShark(mApp);
							aCreature = aStealth;
							if (aPlain)
								aStealth->SetWord(mApp->GetWord(aMinLength, aMaxLength));
							else
							{
								aStealth->SetWord(mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax));
								aThemed = true;
							}
						}
						else if (aKind == 0xa)
						{
							mRecordsByType[aRecord->mType] = aRecord;
							StealthBlackShark* aStealthBlack = new StealthBlackShark(mApp);
							aCreature = aStealthBlack;
							if (aPlain)
							{
								aStealthBlack->SetWord(mApp->GetWord(aMinLength, aMaxLength));
								aStealthBlack->mWord2 = mApp->GetWord(aMinLength, aMaxLength);
							}
							else
							{
								aStealthBlack->SetWord(mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax));
								aStealthBlack->mWord2 = mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax);
								aThemed = true;
							}
						}
						else if (aKind == 9)
						{
							mRecordsByType[aRecord->mType] = aRecord;
							StealthRedShark* aStealthRed = new StealthRedShark(mApp);
							aCreature = aStealthRed;
							if (aPlain)
							{
								aStealthRed->SetWord(mApp->GetWord(aMinLength, aMaxLength));
								aStealthRed->mWord2 = mApp->GetWord(aMinLength, aMaxLength);
								aStealthRed->mWord3 = mApp->GetWord(aMinLength, aMaxLength);
							}
							else
							{
								aStealthRed->SetWord(mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax));
								aStealthRed->mWord2 = mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax);
								aStealthRed->mWord3 = mApp->GetThemeWord(aMinLength, aMaxLength, aRecord->mThemeMin, aRecord->mThemeMax);
								aThemed = true;
							}
						}
						else
							aKeep = false;		// port: the original goes on with the creature of the pass before for a kind it does not know

						if (aKeep && aCreature != NULL)
						{
							aCreature->mField0e4 = aRecord->mType;
							// themed words make the shark a little slower, by difficulty (the score decides in the abyss mode)
							double aDelta = 0.0;
							if (aThemed)
							{
								int aDifficulty = mApp->mDifficulty;
								if (aDifficulty == 0 || aDifficulty == 1)
									aDelta = 0.1;
								else if (aDifficulty == 2)
									aDelta = 0.075;
								else if (aDifficulty == 3)
									aDelta = 0.025;
								else if (aDifficulty == -1)
								{
									if (mScore <= 1000)
										aDelta = 0.1;
									else if (mScore <= 2500)
										aDelta = 0.075;
									else if (mScore <= 4000)
										aDelta = 0.025;
								}
							}
							int aSteps = (int)((aRecord->mMaxSpeed - aRecord->mMinSpeed) * 1000.0);
							if (aSteps == 0)
								aSteps = 2;
							else if (aSteps > 500)
								aSteps = 500;
							double aDrawn = (double)(unsigned int)(gGameRand->NextNoAssert() % (unsigned int)aSteps) * 0.001;
							aCreature->SetVelX((aRecord->mMaxSpeed - aDrawn) - aDelta);
							mSharks.push_back(aCreature);
							if (aFirstNew == -1)
								aFirstNew = (int)mSharks.size() - 1;
						}
						aMade++;
					}
					while (aMade < aCount);
				}
				aRecordIndex++;
			}
			while (aRecordIndex < mLessonManager->GetEnemyRecordCount());
		}
		PlaceEnemies(aFirstNew);
	}
	else if (aWaveType == 1)
	{
		// the piranhas
		int aRecordIndex = 0;
		if (mLessonManager->GetEnemyRecordCount() > 0)
		{
			do
			{
				EnemyRecord* aRecord = mLessonManager->GetEnemyRecord(aRecordIndex);
				int aRepeat = aRecord->mMaxNumber - aRecord->mMinNumber;
				if (aRepeat <= 0)
					aRepeat = 1;
				int aCount = (int)(gGameRand->NextNoAssert() % (unsigned int)aRepeat) + aRecord->mMinNumber;
				if (aCount > 0)
				{
					int aLeft = aCount;
					do
					{
						Piranha* aPiranha = NULL;		// port: the original keeps the one of the pass before for a kind it does not know
						if (aRecord->mType == 0)
						{
							mRecordsByType[0] = aRecord;
							aPiranha = new Piranha(mApp, 0);
							aPiranha->SetWord(mApp->RandomLetter(true));
						}
						else if (aRecord->mType == 6)
						{
							mRecordsByType[6] = aRecord;
							WhitePiranha* aWhite = new WhitePiranha(mApp, 0);
							aPiranha = aWhite;
							aWhite->SetWord(mApp->RandomLetter(true));
							aWhite->SetWord2(mApp->RandomLetter(true));
						}
						else if (aRecord->mType == 7)
						{
							mRecordsByType[7] = aRecord;
							StealthWhitePiranha* aStealthWhite = new StealthWhitePiranha(mApp, 0);
							aPiranha = aStealthWhite;
							aStealthWhite->SetWord(mApp->RandomLetter(true));
							aStealthWhite->SetWord2(mApp->RandomLetter(true));
						}
						else if (aRecord->mType == 3)
						{
							mRecordsByType[aRecord->mType] = aRecord;
							StealthPiranha* aStealth = new StealthPiranha(mApp, 0);
							aPiranha = aStealth;
							aPiranha->SetWord(mApp->RandomLetter(true));
						}

						if (aPiranha != NULL)
						{
							aPiranha->mField0e4 = aRecord->mType;
							int aSteps = (int)((aRecord->mMaxSpeed - aRecord->mMinSpeed) * 1000.0);
							if (aSteps == 0)
								aSteps = 2;
							aPiranha->SetVelX((double)(unsigned int)(gGameRand->NextNoAssert() % (unsigned int)aSteps) * 0.001 + aRecord->mMinSpeed);
							mPiranhas.push_back(aPiranha);
							if (aFirstNew == -1)
								aFirstNew = (int)mPiranhas.size() - 1;
						}
						aLeft--;
					}
					while (aLeft != 0);
				}
				aRecordIndex++;
			}
			while (aRecordIndex < mLessonManager->GetEnemyRecordCount());
		}
		PlaceEnemies(aFirstNew);
	}
	else if (aWaveType == 2)
	{
		// the bonus creatures: one speed (down) for all of them
		double aSpeedY = -10.0;
		int aRecordIndex = 0;
		if (mLessonManager->GetEnemyRecordCount() > 0)
		{
			do
			{
				EnemyRecord* aRecord = mLessonManager->GetEnemyRecord(aRecordIndex);
				int aRepeat = aRecord->mMaxNumber - aRecord->mMinNumber;
				if (aRepeat <= 0)
					aRepeat = 1;
				int aCount = (int)(gGameRand->NextNoAssert() % (unsigned int)aRepeat) + aRecord->mMinNumber;
				if (aCount > 0)
				{
					int aLeft = aCount;
					do
					{
						BonusCreature* aBonus = NULL;		// port: the original keeps the one of the pass before for a kind it does not know
						if (aRecord->mType == 0xb)
						{
							aBonus = new BonusCreature(mApp);
							aBonus->mField0e4 = aRecord->mType;
							if (!aRecord->mThemeRandom && aRecord->mThemeMax == -1 && aRecord->mThemeMin == -1)
								aBonus->SetWord(mApp->GetWord(3, 4));
							else
								aBonus->SetWord(mApp->GetThemeWord(3, 4, aRecord->mThemeMin, aRecord->mThemeMax));
						}

						if (aBonus != NULL)
						{
							if (aSpeedY < 0.0)
							{
								double aFirst = (double)(unsigned int)(gGameRand->NextNoAssert() % 70) * 0.001;
								aSpeedY = (double)(unsigned int)(gGameRand->NextNoAssert() % 400) * 0.001 + aFirst + 0.6;
							}
							aBonus->SetVelY(aSpeedY);
							mBonusCreatures.push_back(aBonus);
							if (aFirstNew == -1)
								aFirstNew = (int)mBonusCreatures.size() - 1;
						}
						aLeft--;
					}
					while (aLeft != 0);
				}
				aRecordIndex++;
			}
			while (aRecordIndex < mLessonManager->GetEnemyRecordCount());
		}
		PlaceBonusCreatures(aFirstNew);
	}
	else if (aWaveType == 3)
	{
		// the boss, with its hint box
		int aBossKind = mLessonManager->GetEnemyRecord(0)->mType;
		if (aBossKind == 0xc)
		{
			mBoss = new TorpedoBoss(mApp, mPlayer, mLessonManager);
			mBoss->mField0e4 = 0xc;
			mApp->ShowHintDialog(GT(0x00530e20), GT(0x00530eb4));
		}
		else if (aBossKind == 0xe)
		{
			mBoss = new GhostShip(mApp, mPlayer, mLessonManager);
			mBoss->mField0e4 = 0xe;
			mApp->ShowHintDialog(GT(0x00530da0), GT(0x00530eb4));
		}
		else if (aBossKind == 0xf)
		{
			mBoss = new MechaShark(mApp, mPlayer, mLessonManager);
			mBoss->mField0e4 = 0xf;
			mApp->ShowHintDialog(GT(0x00530d10), GT(0x00530eb4));
		}
		else if (aBossKind == 0x10)
		{
			mBoss = new MechaSquid(mApp, mPlayer, mLessonManager);
			mBoss->mField0e4 = 0x10;
			mApp->ShowHintDialog(GT(0x00530c68), GT(0x00530eb4));
		}
		mPlayer->SetState(9);
	}

	mLessonManager->NextWave(false);
}
