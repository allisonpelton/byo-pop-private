// The game's tuning values: the config file's global tags set them (one value to a tag), and the levels' scrolling, the shooter,
// the plates and the difficulty read them.

#ifndef VENICE_TUNING_H
#define VENICE_TUNING_H

class SceneTag;

struct Tuning
{
	float mBaseScrollSpeed = 0; // 0x5ba080: how fast a level scrolls, before the difficulty
	float mScrollingSlowPoint = 0; // 0x5ba084
	float mPushOnShotFired = 0; // 0x5ba088: how far a shot pushes the scroll back
	float mPushOnFilledSlot = 0; // 0x5ba08c
	float mPushOnCombo = 0; // 0x5ba090
	float mMaxDifficulty = 0; // 0x5ba094
	float mMinDifficulty = 0; // 0x5ba098
	float mDifficultyChangeOnWin = 0; // 0x5ba09c
	float mDifficultyChangeOnLoss = 0; // 0x5ba0a0
	float mEarthquakePoint = 0; // 0x5ba0a4
	float mPushFilledPlate = 0; // 0x5ba0a8
	int mBlockingSlots = 0; // 0x5ba0ac
	int mBounceOffTop = 0; // 0x5ba0b0
	int mCascadingCombos = 0; // 0x5ba0b4
	int mNumberOfUniqueSlots = 0; // 0x5ba0b8
	int mScrollWhileStillPieces = 0; // 0x5ba0bc
	int mFireWhileStillPieces = 0; // 0x5ba0c0
	int mPlateMoveWhenFullSlotHit = 0; // 0x5ba0c4
	int mTossOnFilledSlot = 0; // 0x5ba0c8
	int mTossOnFilledPlate = 0; // 0x5ba0cc
	int mBumpsToExplode = 0; // 0x5ba0d0
	float mInitialDifficulty = 0.1f; // 0x5b5140
	float mBumperFadePoint = 600.0f; // 0x5b5144
	float mReloadTime = 500.0f; // 0x5b5148: milliseconds
	float mCupidWaitMs = 10000.0f; // 0x5b514c
};

extern Tuning gTuning;

// 0x405800: the first of the tag's attributes that names a value sets it (the config file has one to a tag)
void LoadTuningTag(const SceneTag* theTag);

#endif
