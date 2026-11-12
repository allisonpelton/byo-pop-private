// The tuning values (0x405800).

#include "Tuning.h"

#include "Profiles.h"
#include "SceneTag.h"

Tuning gTuning;

void LoadTuningTag(const SceneTag* theTag)
{
	if (theTag->Has("base_scroll_speed"))
		gTuning.mBaseScrollSpeed = (float)theTag->GetFloat("base_scroll_speed");
	else if (theTag->Has("scrolling_slow_point"))
		gTuning.mScrollingSlowPoint = (float)theTag->GetFloat("scrolling_slow_point");
	else if (theTag->Has("push_on_shot_fired"))
		gTuning.mPushOnShotFired = (float)theTag->GetFloat("push_on_shot_fired");
	else if (theTag->Has("push_on_filled_slot"))
		gTuning.mPushOnFilledSlot = (float)theTag->GetFloat("push_on_filled_slot");
	else if (theTag->Has("push_on_combo"))
		gTuning.mPushOnCombo = (float)theTag->GetFloat("push_on_combo");
	else if (theTag->Has("initial_difficulty")) {
		gTuning.mInitialDifficulty = (float)theTag->GetFloat("initial_difficulty");
		Profile& aProfile = CurrentProfile();
		aProfile.mDifficulty = aProfile.mDifficulty == 0.0f ? gTuning.mInitialDifficulty : aProfile.mDifficulty;
	} else if (theTag->Has("max_difficulty"))
		gTuning.mMaxDifficulty = (float)theTag->GetFloat("max_difficulty");
	else if (theTag->Has("min_difficulty"))
		gTuning.mMinDifficulty = (float)theTag->GetFloat("min_difficulty");
	else if (theTag->Has("difficulty_change_on_win"))
		gTuning.mDifficultyChangeOnWin = (float)theTag->GetFloat("difficulty_change_on_win");
	else if (theTag->Has("difficulty_change_on_loss"))
		gTuning.mDifficultyChangeOnLoss = (float)theTag->GetFloat("difficulty_change_on_loss");
	else if (theTag->Has("earthquake_point"))
		gTuning.mEarthquakePoint = (float)theTag->GetFloat("earthquake_point");
	else if (theTag->Has("push_filled_plate"))
		gTuning.mPushFilledPlate = (float)theTag->GetFloat("push_filled_plate");
	else if (theTag->Has("blocking_slots"))
		gTuning.mBlockingSlots = theTag->GetInt("blocking_slots");
	else if (theTag->Has("bounce_off_top"))
		gTuning.mBounceOffTop = theTag->GetInt("bounce_off_top");
	else if (theTag->Has("cascading_combos"))
		gTuning.mCascadingCombos = theTag->GetInt("cascading_combos");
	else if (theTag->Has("number_of_unique_slots"))
		gTuning.mNumberOfUniqueSlots = theTag->GetInt("number_of_unique_slots");
	else if (theTag->Has("bumper_fade_point"))
		gTuning.mBumperFadePoint = (float)theTag->GetFloat("bumper_fade_point");
	else if (theTag->Has("fire_while_still_pieces"))
		gTuning.mFireWhileStillPieces = theTag->GetInt("fire_while_still_pieces");
	else if (theTag->Has("scroll_while_still_pieces"))
		gTuning.mScrollWhileStillPieces = theTag->GetInt("scroll_while_still_pieces");
	else if (theTag->Has("plate_move_when_full_slot_hit"))
		gTuning.mPlateMoveWhenFullSlotHit = theTag->GetInt("plate_move_when_full_slot_hit");
	else if (theTag->Has("toss_on_filled_slot"))
		gTuning.mTossOnFilledSlot = theTag->GetInt("toss_on_filled_slot");
	else if (theTag->Has("toss_on_filled_plate"))
		gTuning.mTossOnFilledPlate = theTag->GetInt("toss_on_filled_plate");
	else if (theTag->Has("reload_time"))
		gTuning.mReloadTime = (float)theTag->GetFloat("reload_time");
	else if (theTag->Has("cupid_wait_ms"))
		gTuning.mCupidWaitMs = (float)theTag->GetFloat("cupid_wait_ms");
	else if (theTag->Has("bumps_to_explode"))
		gTuning.mBumpsToExplode = theTag->GetInt("bumps_to_explode");
}
