// LessonManagerConfig.cpp: the config parser of Typer Shark Deluxe 1.02 (the original's FUN_0042bfb0, 11.8 KB of code, with
// the manager in ECX), which reads data\<difficulty>-cfg.xml or data\abyss.xml into the LessonManager: the <Default> settings,
// the levels, their waves and their enemies. The format is described in LessonManager.h.
//
// Not translated: nothing (the original reads the file through the stdio functions and the framework's XMLParser, the port
// through PakInterface and the port's XMLParser).
//
// Quirks of the original that are kept: the file is read a character at a time through a decoding function (0x412360)
// that returns the character it is given (the key the first four bytes are read for is not used); a line is fed to the
// XMLParser once it is ended by CR or LF, so the last line of the file counts only if it is ended too; the last character
// of the file is handled twice (the loop only learns of the end of the file at the read after the last character; it
// matters for nothing, as the buffered half line is thrown away); the first line (" WordShark 1.0 -->" once the first four
// bytes are read) and every comment line parse as no element of interest.
// Deviations (port:): the line buffer is a std::string (the original's is 256 bytes); the elements the original makes with
// operator new for every value it reads (and mostly leaks) are locals.

#include "LessonManager.h"
#include "WinWSApp.h"
#include "GameText.h"
#include "XMLParser.h"
#include "PakInterface.h"
#include <stdlib.h>
#include <string.h>

using namespace Sexy;

// port: whether theElement is a tag of the given kind (START, END) with the name that is the game text at theTextVA
static bool IsTag(const XMLElement* theElement, int theType, unsigned int theTextVA)
{
	return (theElement->mType == theType) && (theElement->mValue == GT(theTextVA));
}

// port: the value of the element that follows a start tag (the original makes a new XMLElement for each, reads the next
// element into it and takes atol / atof of its text)
static long ReadLong(XMLParser& theParser)
{
	XMLElement aValue;
	theParser.NextElement(&aValue);
	return atol(aValue.mValue.c_str());
}

// port: the same for a number with a fraction
static double ReadDouble(XMLParser& theParser)
{
	XMLElement aValue;
	theParser.NextElement(&aValue);
	return atof(aValue.mValue.c_str());
}

// port: <MinSpeed> of an enemy: the file's number, changed by the kind of wave, the kind of enemy and the difficulty (4: Xtreme)
static double EnemyMinSpeed(double theValue, int theDifficulty, int theWaveType, int theEnemyType)
{
	if ((theEnemyType == ENEMY_BLACK) && (theDifficulty == 4))
		return theValue;
	else if ((theEnemyType == ENEMY_RED) && (theDifficulty == 4))
		return theValue - 0.05;
	else if ((theWaveType == WAVE_SHARKS) && ((theEnemyType == ENEMY_BLACK_STEALTH) || (theEnemyType == ENEMY_RED_STEALTH) || (theEnemyType == ENEMY_STEALTH)))
		return theValue * 0.7;
	else if ((theWaveType == WAVE_PIRANHAS) && ((theEnemyType == ENEMY_WHITE_STEALTH) || (theEnemyType == ENEMY_STEALTH)))
		return theValue * 0.75;
	else if ((theEnemyType == ENEMY_GHOST) && (theDifficulty == 4))
		return theValue + 0.05;
	else if (((theEnemyType == ENEMY_BLUE) && (theDifficulty == 4)) && (theWaveType == WAVE_SHARKS))
		return theValue + 0.15;
	else if (theWaveType == WAVE_PIRANHAS)
	{
		double anExtra = (theDifficulty == 4) ? 0.25 : 0.05;
		return theValue + anExtra + 0.15;
	}
	else if (theWaveType == WAVE_BONUS)
		return theValue + 0.2;
	else
	{
		double anExtra = (theDifficulty == 4) ? 0.0 : 0.05;
		return theValue + anExtra;
	}
}

// port: <MaxSpeed> of an enemy, the same way
static double EnemyMaxSpeed(double theValue, int theDifficulty, int theWaveType, int theEnemyType)
{
	if ((theEnemyType == ENEMY_BLACK) && (theDifficulty == 4))
		return theValue;
	else if ((theEnemyType == ENEMY_RED) && (theDifficulty == 4))
		return theValue - 0.05;
	else if ((theWaveType == WAVE_SHARKS) && ((theEnemyType == ENEMY_BLACK_STEALTH) || (theEnemyType == ENEMY_RED_STEALTH) || (theEnemyType == ENEMY_STEALTH)))
		return theValue * 0.75;
	else if ((theWaveType == WAVE_PIRANHAS) && ((theEnemyType == ENEMY_WHITE_STEALTH) || (theEnemyType == ENEMY_STEALTH)))
		return theValue * 0.9;
	else if ((theEnemyType == ENEMY_GHOST) && (theDifficulty == 4))
		return theValue + 0.05;
	else if (((theEnemyType == ENEMY_BLUE) && (theDifficulty == 4)) && (theWaveType == WAVE_SHARKS))
		return theValue + 0.15;
	else if (theWaveType == WAVE_PIRANHAS)
	{
		double anExtra = (theDifficulty == 4) ? 0.35 : 0.2;
		return theValue + anExtra + 0.15;
	}
	else if (theWaveType == WAVE_BONUS)
		return theValue + 0.2;
	else
	{
		double anExtra = (theDifficulty == 4) ? 0.0 : 0.1;
		return theValue + anExtra;
	}
}

// 0x42bfb0: theDifficulty is the app's difficulty (0 easy, 1 normal, 2 hard, 3 expert, 4 xtreme) or -1 for the abyss;
// returns 0 when the file was read, 1 when it could not be opened, 2 when a read failed
int LessonManager::LoadConfig(int theDifficulty)
{
	XMLParser aParser;
	XMLElement anElementObject;
	XMLElement* anElement = &anElementObject;     // port: the original makes a new one for each line and for each value

	std::string aFileName = GT(0x0052cd38);     // "data\"
	if (theDifficulty == 0)
		aFileName += GT(0x0052cd28);            // easy-cfg.xml
	else if (theDifficulty == 1)
		aFileName += GT(0x0052cd18);            // normal-cfg.xml
	else if (theDifficulty == 2)
		aFileName += GT(0x0052cd08);            // hard-cfg.xml
	else if (theDifficulty == 3)
		aFileName += GT(0x0052ccf8);            // expert-cfg.xml
	else if (theDifficulty == 4)
		aFileName += GT(0x0052cce8);            // xtreme-cfg.xml
	else if (theDifficulty == -1)
		aFileName += GT(0x0052ccdc);            // abyss.xml
	// (any other value leaves the folder name, which does not open)

	// the defaults start empty, but for the penalty numbers
	mAbyssMode = false;
	mDefaults.mNumForPenalty = 1;
	mDefaults.mPenaltySpeed = 1.0;
	mDefaults.mKnockBack = 0;
	mDefaults.mLetterDelay = 0;
	mDefaults.mMapDelay = 0;
	mDefaults.mMaxCol = 0;
	mDefaults.mMaxDiff = 0;
	mDefaults.mMaxEndDiff = 0;
	mDefaults.mMaxSpeed = 0;
	mDefaults.mMinCol = 0;
	mDefaults.mMinDiff = 0;
	mDefaults.mMinEndDiff = 0;
	mDefaults.mMinSpeed = 0;
	mDefaults.mOceanDepth = 0;
	mDefaults.mPenaltySpeedIncrease = 0;
	mDefaults.mTimeDelay = 0;
	mDefaults.mTreasureSecs = 0;

	int aWaveCount = 0;                     // the original's local_40: the index of the wave that is open (-1 before the first)
	LevelRecord* aLevel = NULL;             // local_34
	WaveRecord* aWave = NULL;               // local_38
	EnemyRecord* anEnemy = NULL;            // local_44
	bool aInDefault = false;                // local_15
	bool aInLevel = false;                  // local_16
	bool aInWave = false;                   // local_1d
	bool aInEnemy = false;                  // local_29
	double aWaveMinSpeed = -1.0;            // local_7c: the wave's own <MinSpeed> + 0.1 (the enemies without one use it)
	double aWaveMaxSpeed = -1.0;            // local_70

	PFILE* aFile = p_fopen(aFileName.c_str(), "r");
	if (aFile == NULL)
		return 1;

	// the first two bytes are a number (the width of the key), then that many more bytes, a number (the key); the
	// decoding function ignores it. For the shipped files these are "<!" and "--": both numbers are 0.
	char aBuf[512];
	p_fread(aBuf, 2, 1, aFile);
	aBuf[2] = 0;                            // port: the original's buffer is not ended
	long aKeyWidth = atol(aBuf);
	if ((aKeyWidth < 0) || (aKeyWidth > 500))
		aKeyWidth = 0;                      // port: the original would write past its buffer
	p_fread(aBuf, (int) aKeyWidth + 2, 1, aFile);
	aBuf[aKeyWidth + 2] = 0;
	long aKey = atol(aBuf);
	(void) aKey;

	char aChar = 0;
	std::string aLine;
	while (!p_feof(aFile))
	{
		size_t aRead = p_fread(&aChar, 1, 1, aFile);
		if ((aRead != 1) && !p_feof(aFile))
		{
			p_fclose(aFile);
			return 2;
		}

		// 0x412360 (aKey, aChar) returns aChar; the original also appends every character but CR and LF to a string it
		// never reads again

		if ((aChar == '\r') || (aChar == '\n'))
		{
			if (aLine.size() > 0)
			{
				aParser.SetStringSource(aLine);
				aLine = "";
				aParser.NextElement(anElement);

				if (IsTag(anElement, XMLElement::TYPE_START, 0x0052ccd4))       // <Config>: nothing to do
					continue;

				if (IsTag(anElement, XMLElement::TYPE_START, 0x0052cccc))       // <Level LevelNum="n">
				{
					aWaveCount = -1;
					(void) atol(anElement->mAttributes[GT(0x0052ccc0)].c_str());     // LevelNum, which the original does not keep
					aInLevel = true;

					aLevel = new LevelRecord();
					aLevel->mBonusLevel = false;
					aLevel->mFirstWave = 0;
					// (the original makes the level's wave vector, reserves five and takes the storage away again)
					aLevel->mSettings = mDefaults;
				}
				else
				{
					if (IsTag(anElement, XMLElement::TYPE_END, 0x0052cccc))     // </Level>
					{
						aInLevel = false;
						mLevels.push_back(aLevel);
					}

					if (IsTag(anElement, XMLElement::TYPE_START, 0x0052ccb8))   // <Default>
						aInDefault = true;
					if (IsTag(anElement, XMLElement::TYPE_END, 0x0052ccb8))     // </Default>
						aInDefault = false;

					if (IsTag(anElement, XMLElement::TYPE_START, 0x0052ccb0))   // <Wave WaveNum="n" Type="...">
					{
						aWaveCount++;
						aInWave = true;
						aWave = new WaveRecord();
						aWaveMinSpeed = -1.0;
						aWaveMaxSpeed = -1.0;

						if (anElement->mAttributes[GT(0x0052cca8)] == GT(0x0052cca0))          // Type = Sharks
							aWave->mType = WAVE_SHARKS;
						else if (anElement->mAttributes[GT(0x0052cca8)] == GT(0x0052cc94))     // Piranhas
							aWave->mType = WAVE_PIRANHAS;
						else if (anElement->mAttributes[GT(0x0052cca8)] == GT(0x0052cc8c))     // Bonus
							aWave->mType = WAVE_BONUS;
						else if (anElement->mAttributes[GT(0x0052cca8)] == GT(0x0052cc84))     // Boss
							aWave->mType = WAVE_BOSS;

						aWave->mTimeDelay = aLevel->mSettings.mTimeDelay;
						if (aWave->mType == WAVE_BONUS)
							aWave->mKnockBack = 0;
						else
							aWave->mKnockBack = aLevel->mSettings.mKnockBack;
						aWave->mMinCol = aLevel->mSettings.mMinCol;
						aWave->mMaxCol = aLevel->mSettings.mMaxCol;
						aWave->mLetterDelay = aLevel->mSettings.mLetterDelay;
					}
					if (IsTag(anElement, XMLElement::TYPE_END, 0x0052ccb0))     // </Wave>
					{
						aInWave = false;
						aLevel->mWaves.push_back(aWave);
						aWaveMinSpeed = -1.0;
						aWaveMaxSpeed = -1.0;
					}

					if (IsTag(anElement, XMLElement::TYPE_START, 0x0052cc7c))   // <Enemy Type="...">
					{
						aInEnemy = true;
						anEnemy = new EnemyRecord();
						anEnemy->mKnockBack = aWave->mKnockBack;
						anEnemy->mWordMaxLength = aLevel->mSettings.mMaxDiff;
						if (aWaveMinSpeed <= 0.0)
							anEnemy->mMinSpeed = aLevel->mSettings.mMinSpeed;
						else
							anEnemy->mMinSpeed = aWaveMinSpeed;
						if (aWaveMaxSpeed <= 0.0)
							anEnemy->mMaxSpeed = aLevel->mSettings.mMaxSpeed;
						else
							anEnemy->mMaxSpeed = aWaveMaxSpeed;
						anEnemy->mWordMinLength = aLevel->mSettings.mMinDiff;
						anEnemy->mLetterDelay = aWave->mLetterDelay;
						anEnemy->mThemeMin = -1;
						anEnemy->mThemeMax = -1;
						anEnemy->mThemeRandom = false;

						std::string anEnemyType = anElement->mAttributes[GT(0x0052cca8)];
						if (anEnemyType == GT(0x0052cc74))                  // Blue
							anEnemy->mType = ENEMY_BLUE;
						else if (anEnemyType == GT(0x0052cc6c))             // Black
							anEnemy->mType = ENEMY_BLACK;
						else if (anEnemyType == GT(0x0052cc64))             // Toxic
							anEnemy->mType = ENEMY_TOXIC;
						else if (anEnemyType == GT(0x0052cc58))             // CrazyToxic
							anEnemy->mType = ENEMY_CRAZY_TOXIC;
						else if (anEnemyType == GT(0x0052cc50))             // White
							anEnemy->mType = ENEMY_WHITE;
						else if (anEnemyType == GT(0x0052cc48))             // Stealth
							anEnemy->mType = ENEMY_STEALTH;
						else if (anEnemyType == GT(0x0052cc44))             // Red
							anEnemy->mType = ENEMY_RED;
						else if (anEnemyType == GT(0x0052cc34))             // WhiteStealth
							anEnemy->mType = ENEMY_WHITE_STEALTH;
						else if (anEnemyType == GT(0x0052cc2c))             // Purple
							anEnemy->mType = ENEMY_PURPLE;
						else if (anEnemyType == GT(0x0052cc20))             // RedStealth
							anEnemy->mType = ENEMY_RED_STEALTH;
						else if (anEnemyType == GT(0x0052cc10))             // BlackStealth
							anEnemy->mType = ENEMY_BLACK_STEALTH;
						else if (anEnemyType == GT(0x0052cc04))             // Standard
							anEnemy->mType = ENEMY_STANDARD;
						else if (anEnemyType == GT(0x0052cbfc))             // Torpedo
							anEnemy->mType = ENEMY_TORPEDO;
						else if (anEnemyType == GT(0x0052cbf4))             // Ghost
							anEnemy->mType = ENEMY_GHOST;
						else if (anEnemyType == GT(0x0052cbe8))             // GhostShip
							anEnemy->mType = ENEMY_GHOST_SHIP;
						else if (anEnemyType == GT(0x0052cbdc))             // MechaShark
							anEnemy->mType = ENEMY_MECHA_SHARK;
						else
							anEnemy->mType = ENEMY_MECHA_SQUID;             // MechaSquid, and what is not named
					}
					else if (IsTag(anElement, XMLElement::TYPE_END, 0x0052cc7c))    // </Enemy>
					{
						aInEnemy = false;
						aWave->mEnemies.push_back(anEnemy);
					}
					else if (anElement->mType == XMLElement::TYPE_START)
					{
						// a setting: the number that follows goes to the innermost record that is open. Most
						// tags do not test which one (a tag outside the records it belongs in writes through a null
						// pointer, as in the original)
						const std::string& aTag = anElement->mValue;

						if (aTag == GT(0x0052cbc8))                         // MinCol
						{
							long aValue = ReadLong(aParser);
							if (!aInWave)
							{
								if (!aInLevel)
								{
									if (aInDefault)
										mDefaults.mMinCol = aValue;
								}
								else
									aLevel->mSettings.mMinCol = aValue;
							}
							else
								aWave->mMinCol = aValue;
						}
						else if (aTag == GT(0x0052cbbc))                    // BonusLevel (no value)
							aLevel->mBonusLevel = true;
						else if (aTag == GT(0x0052cbb0))                    // LetterDelay
						{
							long aValue = ReadLong(aParser);
							if (!aInEnemy)
							{
								if (!aInWave)
								{
									if (!aInLevel)
									{
										if (aInDefault)
											mDefaults.mLetterDelay = aValue;
									}
									else
										aLevel->mSettings.mLetterDelay = aValue;
								}
								else
									aWave->mLetterDelay = aValue;
							}
							else
								anEnemy->mLetterDelay = aValue;
						}
						else if (aTag == GT(0x0052cba8))                    // MaxCol
						{
							long aValue = ReadLong(aParser);
							if (!aInWave)
							{
								if (!aInLevel)
								{
									if (aInDefault)
										mDefaults.mMaxCol = aValue;
								}
								else
									aLevel->mSettings.mMaxCol = aValue;
							}
							else
								aWave->mMaxCol = aValue;
						}
						else if (aTag == GT(0x0052cb98))                    // NumForPenalty
						{
							long aValue = ReadLong(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mNumForPenalty = aValue;
							}
							else
								aLevel->mSettings.mNumForPenalty = aValue;
						}
						else if (aTag == GT(0x0052cb8c))                    // AbyssMode (no value)
							mAbyssMode = true;
						else if (aTag == GT(0x0052cb80))                    // OceanDepth
						{
							long aValue = ReadLong(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mOceanDepth = aValue;
							}
							else
								aLevel->mSettings.mOceanDepth = aValue;
						}
						else if (aTag == GT(0x0052cb70))                    // PenaltySpeed
						{
							double aValue = ReadDouble(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mPenaltySpeed = aValue;
							}
							else
								aLevel->mSettings.mPenaltySpeed = aValue;
						}
						else if (aTag == GT(0x0052cb64))                    // FirstWave
						{
							XMLElement aValue;
							aParser.NextElement(&aValue);
							aLevel->mFirstWave = aWaveCount;
						}
						else if (aTag == GT(0x0052cb58))                    // MinSpeed
						{
							double aValue = ReadDouble(aParser);
							if (!aInEnemy)
							{
								if (!aInWave)
								{
									if (!aInLevel)
									{
										if (aInDefault)
											mDefaults.mMinSpeed = aValue + ((theDifficulty == 4) ? 0.0 : 0.03);
									}
									else
										aLevel->mSettings.mMinSpeed = aValue + ((theDifficulty == 4) ? 0.0 : 0.03);
								}
								else
									aWaveMinSpeed = aValue + 0.1;
							}
							else
								anEnemy->mMinSpeed = EnemyMinSpeed(aValue, theDifficulty, aWave->mType, anEnemy->mType);
						}
						else if (aTag == GT(0x0052cb30))                    // PenaltySpeedIncrease
						{
							double aValue = ReadDouble(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mPenaltySpeedIncrease = aValue;
							}
							else
								aLevel->mSettings.mPenaltySpeedIncrease = aValue;
						}
						else if (aTag == GT(0x0052cb24))                    // MaxSpeed
						{
							double aValue = ReadDouble(aParser);
							if (!aInEnemy)
							{
								if (!aInWave)
								{
									if (!aInLevel)
									{
										if (aInDefault)
											mDefaults.mMaxSpeed = aValue + ((theDifficulty == 4) ? 0.0 : 0.1);
									}
									else
										aLevel->mSettings.mMaxSpeed = aValue + ((theDifficulty == 4) ? 0.0 : 0.1);
								}
								else
									aWaveMaxSpeed = aValue + 0.1;
							}
							else
								anEnemy->mMaxSpeed = EnemyMaxSpeed(aValue, theDifficulty, aWave->mType, anEnemy->mType);
						}
						else if (aTag == GT(0x0052cb1c))                    // MinDiff
						{
							long aValue = ReadLong(aParser);
							if (!aInEnemy)
							{
								if (!aInLevel)
								{
									if (aInDefault)
										mDefaults.mMinDiff = aValue;
								}
								else
									aLevel->mSettings.mMinDiff = aValue;
							}
							else
								anEnemy->mWordMinLength = aValue;
						}
						else if (aTag == GT(0x0052cb14))                    // MaxDiff
						{
							long aValue = ReadLong(aParser);
							if (!aInEnemy)
							{
								if (!aInLevel)
								{
									if (aInDefault)
										mDefaults.mMaxDiff = aValue;
								}
								else
									aLevel->mSettings.mMaxDiff = aValue;
							}
							else
								anEnemy->mWordMaxLength = aValue;
						}
						else if (aTag == GT(0x0052cb08))                    // MinNumber
						{
							anEnemy->mMinNumber = ReadLong(aParser);
							if (aWave->mType == WAVE_BONUS)
								anEnemy->mMinNumber = 12;
						}
						else if (aTag == GT(0x0052cafc))                    // MaxNumber
						{
							anEnemy->mMaxNumber = ReadLong(aParser);
							if (aWave->mType == WAVE_BONUS)
								anEnemy->mMaxNumber = 15;
						}
						else if (aTag == GT(0x0052caf0))                    // TimeDelay
						{
							long aValue = ReadLong(aParser);
							if (!aInWave)
							{
								if (!aInLevel)
								{
									if (aInDefault)
										mDefaults.mTimeDelay = aValue;
								}
								else
									aLevel->mSettings.mTimeDelay = aValue;
							}
							else
								aWave->mTimeDelay = aValue;
						}
						else if (aTag == GT(0x0052cae8))                    // Theme
						{
							long aValue = ReadLong(aParser);
							anEnemy->mThemeMin = aValue;
							anEnemy->mThemeMax = aValue;
						}
						else if (aTag == GT(0x0052cadc))                    // ThemeRandom (no value)
							anEnemy->mThemeRandom = true;
						else if (aTag == GT(0x0052cad0))                    // ThemeMin
							anEnemy->mThemeMin = ReadLong(aParser);
						else if (aTag == GT(0x0052cac4))                    // ThemeMax
							anEnemy->mThemeMax = ReadLong(aParser);
						else if (aTag == GT(0x0052cab8))                    // DiverSpeed
						{
							double aValue = ReadDouble(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mDiverSpeed = aValue;
							}
							else
								aLevel->mSettings.mDiverSpeed = aValue;
						}
						else if (aTag == GT(0x0052caac))                    // MinEndDiff
						{
							long aValue = ReadLong(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mMinEndDiff = aValue;
							}
							else
								aLevel->mSettings.mMinEndDiff = aValue;
						}
						else if (aTag == GT(0x0052caa0))                    // MaxEndDiff
						{
							long aValue = ReadLong(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mMaxEndDiff = aValue;
							}
							else
								aLevel->mSettings.mMaxEndDiff = aValue;
						}
						else if (aTag == GT(0x0052ca90))                    // TreasureSecs
						{
							long aValue = ReadLong(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mTreasureSecs = aValue;
							}
							else
								aLevel->mSettings.mTreasureSecs = aValue;
						}
						else if (aTag == GT(0x0052ca84))                    // KnockBack
						{
							long aValue = ReadLong(aParser);
							if (!aInWave || (aWave->mType == WAVE_BONUS))
							{
								if (!aInLevel)
								{
									if (aInDefault)
										mDefaults.mKnockBack = aValue;
								}
								else
									aLevel->mSettings.mKnockBack = aValue;
							}
							else
								aWave->mKnockBack = aValue;
						}
						else if (aTag == GT(0x0052ca78))                    // MapDelay
						{
							long aValue = ReadLong(aParser);
							if (!aInLevel)
							{
								if (aInDefault)
									mDefaults.mMapDelay = aValue;
							}
							else
								aLevel->mSettings.mMapDelay = aValue;
						}
						else if (aTag == GT(0x0052ca6c))                    // TimeToFire
							anEnemy->mFireInterval = (double) ReadLong(aParser);
						else if (aTag == GT(0x0052ca64))                    // Health
							anEnemy->mHitPoints = ReadLong(aParser);
						else if (aTag == GT(0x0052ca58))                    // MinMissiles
							anEnemy->mMinMissiles = ReadLong(aParser);
						else if (aTag == GT(0x0052ca4c))                    // MaxMissiles
							anEnemy->mMaxMissiles = ReadLong(aParser);
						else if (aTag == GT(0x0052ca40))                    // BobSpeed
							anEnemy->mBobSpeed = ReadDouble(aParser);
						else if (aTag == GT(0x0052ca30))                    // MinMissileSpeed
							anEnemy->mShotSpeedMin = ReadDouble(aParser) + 0.3;
						else if (aTag == GT(0x0052ca20))                    // MaxMissileSpeed
							anEnemy->mShotSpeedMax = ReadDouble(aParser) + 0.3;
					}
				}

			}
		}
		else
			aLine += aChar;
	}

	p_fclose(aFile);

	if (!mAbyssMode)
	{
		// the levels from the 31st on, deep copies, for the endless play after level 35 to draw from (NextLevel, NextWave)
		for (size_t aLevelIndex = 0x1e; (mLevels.size() != 0) && (aLevelIndex < mLevels.size()); aLevelIndex++)
		{
			LevelRecord* aCopy = new LevelRecord();
			LevelRecord* aSource = mLevels[aLevelIndex];
			// the original copies the settings dword by dword (all but the unused field at +0x1c) and not the level's own
			// bonus flag and first wave, which stay uninitialised in its copy (the heap block is not cleared); the port's are 0
			aCopy->mSettings = aSource->mSettings;
			aCopy->mSettings.mAlignmentGap = 0;

			for (size_t aWaveIndex = 0; aWaveIndex < aSource->mWaves.size(); aWaveIndex++)
			{
				WaveRecord* aWaveSource = aSource->mWaves[aWaveIndex];
				WaveRecord* aWaveCopy = new WaveRecord();
				aWaveCopy->mTimeDelay = aWaveSource->mTimeDelay;
				aWaveCopy->mLetterDelay = aWaveSource->mLetterDelay;
				aWaveCopy->mMaxCol = aWaveSource->mMaxCol;
				aWaveCopy->mMinCol = aWaveSource->mMinCol;
				aWaveCopy->mKnockBack = aWaveSource->mKnockBack;
				aWaveCopy->mType = aWaveSource->mType;

				for (size_t anEnemyIndex = 0; anEnemyIndex < aWaveSource->mEnemies.size(); anEnemyIndex++)
				{
					EnemyRecord* anEnemySource = aWaveSource->mEnemies[anEnemyIndex];
					EnemyRecord* anEnemyCopy = new EnemyRecord();
					// port: the original copies these and leaves the boss numbers (fire interval, hit points, missile counts,
					// bob and shot speeds) and field 0x04 uninitialised; the port's are 0
					anEnemyCopy->mKnockBack = anEnemySource->mKnockBack;
					anEnemyCopy->mLetterDelay = anEnemySource->mLetterDelay;
					anEnemyCopy->mWordMaxLength = anEnemySource->mWordMaxLength;
					anEnemyCopy->mMaxNumber = anEnemySource->mMaxNumber;
					anEnemyCopy->mMaxSpeed = anEnemySource->mMaxSpeed;
					anEnemyCopy->mWordMinLength = anEnemySource->mWordMinLength;
					anEnemyCopy->mMinNumber = anEnemySource->mMinNumber;
					anEnemyCopy->mMinSpeed = anEnemySource->mMinSpeed;
					anEnemyCopy->mThemeMax = anEnemySource->mThemeMax;
					anEnemyCopy->mThemeMin = anEnemySource->mThemeMin;
					anEnemyCopy->mThemeRandom = anEnemySource->mThemeRandom;
					anEnemyCopy->mType = anEnemySource->mType;
					aWaveCopy->mEnemies.push_back(anEnemyCopy);
				}

				aCopy->mWaves.push_back(aWaveCopy);
			}

			mEndlessLevels.push_back(aCopy);
		}
	}

	return 0;
}
