#ifndef __TYPERSHARK_LESSONMANAGER_H__
#define __TYPERSHARK_LESSONMANAGER_H__

// LessonManager.h / LessonManager.cpp / LessonManagerConfig.cpp: the level table of a game of Typer Shark Deluxe 1.02: the
// levels, waves and enemies read from data\<difficulty>-cfg.xml (or data\abyss.xml) and the cursor that walks through them
// (the level, the wave, the endless levels that come after level 35). The app owns one (WinWSApp::mLessonManager, +0x65c)
// and hands it to the Board and to the bosses.
//
// Original: a class with no name in the export (no virtual functions, so no RTTI name; the port keeps the name LessonManager that the first
// translation jobs gave it), constructor 0x4270e0, sizeof 0xc0, destructor body 0x421c50 (frees the records and the set).
// NOT to be confused with RTTI's Sexy::LessonManager (vftable 0x52c728, destructor body 0x427300, deleting destructor
// 0x429900): that is the typing tutor's lesson list (data\lessonN.xml), made by the TypingTutor constructor 0x460b30; it is
// translated with the TypingTutor.
//
// The file format (one element per line; the lines are fed to the XMLParser one at a time, so a start tag, its value and
// its end tag are on one line):
//   <Config> <AbyssMode></AbyssMode>? <Default> tags </Default>
//   <Level LevelNum="n"> tags <Wave WaveNum="n" Type="Sharks|Piranhas|Bonus|Boss"> tags <Enemy Type="Blue|..."> tags </Enemy>
//   </Wave> </Level>
// A tag sets a field of the innermost record that is open (enemy, wave, level, the default settings).
//
// Functions of the original that are STL instances and not translated (the port uses the standard library): the vectors'
// and strings' helpers 0x426ea0..0x427050 (0x426f90, 0x426fc0, 0x427000 among them), 0x42b8e0 and 0x42b930 (copies), 0x42bf30,
// 0x42bf80, 0x42f710, 0x42f820 (resize of a vector of vectors), 0x42f8e0..0x42fa10 (0x42f900, 0x42f9c0), 0x430100, 0x431510
// and 0x431560 (clear and resize of the same), 0x42fd10, 0x429690..0x4297d0 (0x4296b0, 0x429750), the maps' 0x42f760 and
// 0x42f930 (map<int, vector<string>>: insert and operator[]), 0x434640, 0x434700, 0x436660 (tree erase), and the sets' 0x414550
// (clear), 0x414580 (find), 0x41dc80 (insert). The exception funclets of the constructor, the parser and the loaders
// (0x4270a6, 0x429736, 0x429826, 0x42bac5, 0x42bb5f, 0x42fba5, 0x42fc3f) do nothing the port needs.

#include <set>
#include <string>
#include <vector>

namespace Sexy
{

class WinWSApp;

// The Type of a <Wave> (WaveRecord::mType)
enum WaveType
{
	WAVE_SHARKS = 0,
	WAVE_PIRANHAS = 1,
	WAVE_BONUS = 2,
	WAVE_BOSS = 3
};

// The Type of an <Enemy> (EnemyRecord::mType): the names are the file's
enum EnemyType
{
	ENEMY_BLUE = 0,
	ENEMY_BLACK = 1,
	ENEMY_RED = 2,
	ENEMY_STEALTH = 3,
	ENEMY_TOXIC = 4,
	ENEMY_CRAZY_TOXIC = 5,
	ENEMY_WHITE = 6,
	ENEMY_WHITE_STEALTH = 7,
	ENEMY_PURPLE = 8,
	ENEMY_RED_STEALTH = 9,
	ENEMY_BLACK_STEALTH = 10,
	ENEMY_STANDARD = 11,
	ENEMY_TORPEDO = 12,
	ENEMY_GHOST = 13,
	ENEMY_GHOST_SHIP = 14,
	ENEMY_MECHA_SHARK = 15,
	ENEMY_MECHA_SQUID = 16
};

// One <Enemy> of a wave: what creatures of that kind the wave sends (original: operator new(0x70), fields set by the
// config parser 0x42bfb0). The boss waves (Type "Boss") hold one record with the boss's numbers (ProjectileBoss,
// TorpedoBoss and their subclasses read them through LessonManager::GetEnemyRecord).
struct EnemyRecord
{
	int				mType;					// +0x00 EnemyType
	int				mAlignmentGap;				// +0x04 the four bytes between mType and the 8-aligned mMinSpeed; the parser never sets it and nothing reads it (the constructor zeroes it)
	double			mMinSpeed;				// +0x08 <MinSpeed> (adjusted by the wave's type, the enemy's type and the difficulty)
	double			mMaxSpeed;				// +0x10 <MaxSpeed> (adjusted the same way)
	int				mKnockBack;				// +0x18 the wave's knock-back at the time the enemy was opened
	int				mWordMinLength;			// +0x1c <MinDiff> (the word difficulty, which GetWord reads as a word length)
	int				mWordMaxLength;			// +0x20 <MaxDiff>
	int				mLetterDelay;			// +0x24 the wave's <LetterDelay> (an <Enemy> may set its own)
	int				mMinNumber;				// +0x28 <MinNumber>: how many of them come (12 in a bonus wave)
	int				mMaxNumber;				// +0x2c <MaxNumber> (15 in a bonus wave)
	bool			mThemeRandom;			// +0x30 <ThemeRandom> present: the theme is drawn at random
	int				mThemeMin;				// +0x34 <Theme> or <ThemeMin> (-1: none)
	int				mThemeMax;				// +0x38 <Theme> or <ThemeMax> (-1: none)
	double			mFireInterval;			// +0x40 <TimeToFire> (a boss's time between shots)
	int				mHitPoints;				// +0x48 <Health> (a boss)
	int				mMinMissiles;			// +0x4c <MinMissiles> (TorpedoBoss)
	int				mMaxMissiles;			// +0x50 <MaxMissiles>
	double			mBobSpeed;				// +0x58 <BobSpeed> (a boss)
	double			mShotSpeedMin;			// +0x60 <MinMissileSpeed> + 0.3
	double			mShotSpeedMax;			// +0x68 <MaxMissileSpeed> + 0.3

	// port: the original's operator new(0x70) is not cleared; the parser sets the fields it does not mention below
	EnemyRecord() : mType(0), mAlignmentGap(0), mMinSpeed(0), mMaxSpeed(0), mKnockBack(0), mWordMinLength(0), mWordMaxLength(0),
		mLetterDelay(0), mMinNumber(0), mMaxNumber(0), mThemeRandom(false), mThemeMin(0), mThemeMax(0), mFireInterval(0),
		mHitPoints(0), mMinMissiles(0), mMaxMissiles(0), mBobSpeed(0), mShotSpeedMin(0), mShotSpeedMax(0) {}
};

// One <Wave> of a level (original: operator new(0x28))
struct WaveRecord
{
	int							mType;			// +0x00 WaveType (<Wave Type="...">)
	int							mTimeDelay;		// +0x04 <TimeDelay>, from the level's
	int							mKnockBack;		// +0x08 <KnockBack>, from the level's (0 in a bonus wave)
	int							mMinCol;		// +0x0c <MinCol>, from the level's
	int							mMaxCol;		// +0x10 <MaxCol>, from the level's
	int							mLetterDelay;	// +0x14 <LetterDelay>, from the level's
	std::vector<EnemyRecord*>	mEnemies;		// +0x18 (pad), +0x1c begin, +0x20 end, +0x24 capacity

	// port: the original leaves mType unset when the wave's Type is not one of the four
	WaveRecord() : mType(0), mTimeDelay(0), mKnockBack(0), mMinCol(0), mMaxCol(0), mLetterDelay(0) {}
};

// The settings a level has, and the manager has as defaults (the <Default> section): the same 0x60 bytes at +0x00 of the
// manager and at +0x08 of every level. A level starts as a copy of the defaults and its own tags change them.
struct LevelSettings
{
	double			mMinSpeed;				// +0x00 <MinSpeed> (+0.03 unless the difficulty is 4)
	double			mMaxSpeed;				// +0x08 <MaxSpeed> (+0.1 unless the difficulty is 4)
	int				mKnockBack;				// +0x10 <KnockBack>
	int				mMinDiff;				// +0x14 <MinDiff>
	int				mMaxDiff;				// +0x18 <MaxDiff>
	int				mAlignmentGap;				// +0x1c the four bytes between mMaxDiff and the 8-aligned mDiverSpeed; the parser never reads or sets it (the copy for the endless levels sets it to 0)
	double			mDiverSpeed;			// +0x20 <DiverSpeed> (the Board reads it: the diver's speed)
	int				mMinEndDiff;			// +0x28 <MinEndDiff>
	int				mMaxEndDiff;			// +0x2c <MaxEndDiff>
	int				mTreasureSecs;			// +0x30 <TreasureSecs>
	int				mTimeDelay;				// +0x34 <TimeDelay>
	int				mMapDelay;				// +0x38 <MapDelay>
	int				mMinCol;				// +0x3c <MinCol>
	int				mMaxCol;				// +0x40 <MaxCol>
	int				mLetterDelay;			// +0x44 <LetterDelay>
	int				mNumForPenalty;			// +0x48 <NumForPenalty> (LoadConfig starts it at 1)
	int				mOceanDepth;			// +0x4c <OceanDepth> (the Board reads it)
	double			mPenaltySpeed;			// +0x50 <PenaltySpeed> (LoadConfig starts it at 1.0)
	double			mPenaltySpeedIncrease;	// +0x58 <PenaltySpeedIncrease>

	LevelSettings() : mMinSpeed(0), mMaxSpeed(0), mKnockBack(0), mMinDiff(0), mMaxDiff(0), mAlignmentGap(0), mDiverSpeed(0),
		mMinEndDiff(0), mMaxEndDiff(0), mTreasureSecs(0), mTimeDelay(0), mMapDelay(0), mMinCol(0), mMaxCol(0),
		mLetterDelay(0), mNumForPenalty(0), mOceanDepth(0), mPenaltySpeed(0), mPenaltySpeedIncrease(0) {}
};

// One <Level> (original: operator new(0x80))
struct LevelRecord
{
	bool						mBonusLevel;	// +0x00 <BonusLevel> present: a treasure level
	int							mAlignmentGap;		// +0x04 the bytes between mBonusLevel and the 8-aligned mSettings; never set or read (the constructor zeroes it)
	LevelSettings				mSettings;		// +0x08 .. +0x67
	int							mFirstWave;		// +0x68 <FirstWave>: the index of the wave that tag was in (0 until it is read)
	std::vector<WaveRecord*>	mWaves;			// +0x6c (pad), +0x70 begin, +0x74 end, +0x78 capacity

	LevelRecord() : mBonusLevel(false), mAlignmentGap(0), mFirstWave(0) {}
};

class LessonManager
{
public:
	// ---- data ----
	LevelSettings				mDefaults;				// +0x00 the <Default> section (set by LoadConfig)
	std::vector<LevelRecord*>	mLevels;				// +0x60 (pad), +0x64 begin, +0x68 end, +0x6c capacity: the levels of the file
	std::vector<LevelRecord*>	mEndlessLevels;			// +0x70 (pad), +0x74 begin, +0x78 end, +0x7c capacity: copies of the levels from index 30 on, which the endless play after level 35 draws from (not made in the abyss)
	int							mEndlessLevel;			// +0x80 the index in mEndlessLevels that is playing
	int							mPrevEndlessLevel;		// +0x84 the one before it (set by NextWave before a new one is drawn)
	WinWSApp*					mApp;					// +0x88 constructor argument
	int							mLevel;					// +0x8c the level that is playing (an index in mLevels; -1: the endless levels)
	int							mWave;					// +0x90 the wave that is playing
	int							mMinEndDiff;			// +0x94 the level's <MinEndDiff> (set by NextLevel)
	int							mMaxEndDiff;			// +0x98 the level's <MaxEndDiff>
	int							mCurrentWaveType;		// +0x9c the type of the wave NextWave chose (getter 0x412340)
	int							mCurrentWave;			// +0xa0 the index of that wave
	bool						mAbyssMode;				// +0xa5 the file had <AbyssMode>
	int							mEndlessCount;			// +0xa8 how many endless levels were started (getter 0x412370)
	std::set<int>				mUsedWaves;				// +0xac the waves of the level that were played (head +0xb0, size +0xb4); +0xb8 is the iterator the original keeps of the last find

public:
	LessonManager(WinWSApp* theApp);					// 0x4270e0
	~LessonManager();									// 0x421c50

	int							LoadConfig(int theDifficulty);	// 0x42bfb0 (LessonManagerConfig.cpp): 0 read, 1 file missing, 2 read error
	void						SetLevel(int theLevel);			// 0x4309d0
	void						NextLevel();					// 0x430640 (goes to the next level, drawing endless ones after level 35)
	void						NextWave(bool thePrevLevel);	// 0x41eb40 (draws the next wave to play)
	void						GoToBossWave();					// 0x4149d0 (when the level ends with a boss wave: the wave is that one)

	// the cursor and the current records
	bool						IsEndless();					// 0x4131a0 (the level is -1 or past the table: the endless levels are in use)
	int							GetWave() { return mWave; }					// 0x412330
	int							GetCurrentWaveType() { return mCurrentWaveType; }	// 0x412340
	int							GetLevel() { return mLevel; }				// 0x412350
	int							GetEndlessCount() { return mEndlessCount; }	// 0x412370
	bool						IsBonusLevel();					// 0x4137d0 (the J2 job called it IsTutorialActive: the flag is the level's <BonusLevel>)
	bool						LastWaveIsBoss();				// 0x4137f0
	LevelSettings*				GetLevelSettings();				// 0x413860 (the current level's settings: &level->mSettings)
	int							GetWaveType();					// 0x413890
	int							GetWaveMinCol();				// 0x4138d0
	int							GetWaveMaxCol();				// 0x413920
	int							GetWaveKnockBack();				// 0x413970
	EnemyRecord*				GetEnemyRecord(int theIndex);	// 0x4139d0 (an enemy of the current wave; the J2 job called it GetBossRecord)
	int							GetEnemyRecordCount();			// 0x413a10
	int							GetWaveCount();					// 0x413a60 (the waves of the current level)

private:
	LevelRecord*				GetCurrentLevel();				// the record 0x4131a0 chooses between (inlined at every use in the original)
	void						OrderEnemies();					// 0x430150 (the waves' enemies: stealth ones first)
};

}

#endif
