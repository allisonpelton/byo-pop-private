// The names the scene files use for attributes, filters, triggers and actions (0x42b4d0, 0x42b770, 0x42ba80, 0x42bc80).

#include "SceneNames.h"

#include "SceneObject.h"

#include <cstdlib>
#include <cstring>

namespace
{
	struct NameEntry
	{
		const char* mName;
		int mValue;
	};

	int LookUp(const NameEntry* theTable, int theCount, const char* theName, int theDefault)
	{
		int aResult = theDefault;
		for (int i = 0; i < theCount; i++) {
			if (strcmp(theName, theTable[i].mName) == 0)
				aResult = theTable[i].mValue;
		}
		return aResult;
	}

	const NameEntry kAttributeNames[] = {
		{"x", AttributeX}, {"y", AttributeY}, {"z", AttributeZ}, {"rotation", AttributeRotation},
		{"x_scale", AttributeXScale}, {"y_scale", AttributeYScale}, {"r", AttributeRed}, {"g", AttributeGreen},
		{"b", AttributeBlue}, {"alpha", AttributeAlpha}, {"frame_number", AttributeFrameNumber},
		{"all", AttributeAll}};

	const NameEntry kFilterNames[] = {
		{"super_cos", FilterSuperCos}, {"squared", FilterSquared}, {"cubed", FilterCubed},
		{"square_root", FilterSquareRoot}, {"sined", FilterSined}, {"decay_sined", FilterDecaySined},
		{"cosined", FilterCosined}, {"decay_cosined", FilterDecayCosined}, {"loop_anim", FilterLoopAnim},
		{"pingpong_anim", FilterPingPongAnim}, {"hot_spot", FilterHotSpot}, {"velocity", FilterVelocity}};

	const NameEntry kTriggerNames[] = {
		{"click", TriggerClick}, {"click_anywhere", TriggerClickAnywhere}, {"hot", TriggerHot},
		{"not_hot", TriggerNotHot}, {"start", TriggerStart}, {"end", TriggerEnd}, {"less_than", TriggerLessThan},
		{"code", TriggerCode}, {"interval", TriggerInterval}};

	const NameEntry kActionNames[] = {
		{"set_mode", ActionSetMode}, {"enable_3d_hardware", ActionEnable3DHardware},
		{"insert_level", ActionInsertLevel}, {"send_level", ActionSendLevel}, {"move", ActionMove},
		{"play_sound", ActionPlaySound}, {"go_to_stage", ActionGoToStage},
		{"send_parent_to_stage", ActionSendParentToStage}, {"next_stage", ActionNextStage},
		{"change_user_name", ActionChangeUserName}, {"new_user_name", ActionNewUserName},
		{"delete_user", ActionDeleteUser}, {"get_initial_user", ActionGetInitialUser},
		{"dim_previous", ActionDimPrevious}, {"pause_previous", ActionPausePrevious},
		{"set_level_number", ActionSetLevelNumber}, {"kill_all_previous", ActionKillAllPrevious},
		{"kill_all", ActionKillAll}, {"dim_previous_no_pause", ActionDimPreviousNoPause},
		{"brighten_previous", ActionBrightenPrevious}, {"stop_music", ActionStopMusic},
		{"fade_music", ActionFadeMusic}, {"fade_previous_music", ActionFadePreviousMusic},
		{"make_sparkles", ActionMakeSparkles}, {"make_particles", ActionMakeParticles},
		{"enter_high_score", ActionEnterHighScore}, {"clear_score", ActionClearScore},
		{"reset_progress", ActionResetProgress}, {"bump_tick_counter", ActionBumpTickCounter},
		{"change_image", ActionChangeImage}, {"custom", ActionCustom}, {"save_score", ActionSaveScore},
		{"bump_score", ActionBumpScore}, {"clear_button_state", ActionClearButtonState}};
}

// 0x42b4d0: a name that is no attribute's (or none) is "all"
int AttributeFromName(const char* theName)
{
	if (theName == nullptr)
		return AttributeAll;
	return LookUp(kAttributeNames, sizeof(kAttributeNames) / sizeof(kAttributeNames[0]), theName, AttributeAll);
}

// 0x42b770: a name that is no filter's (or none) is the first filter
int FilterTypeFromName(const char* theName)
{
	if (theName == nullptr)
		return FilterSuperCos;
	return LookUp(kFilterNames, sizeof(kFilterNames) / sizeof(kFilterNames[0]), theName, FilterSuperCos);
}

// 0x42ba80
int TriggerFromName(const char* theName)
{
	return LookUp(kTriggerNames, sizeof(kTriggerNames) / sizeof(kTriggerNames[0]), theName, TriggerNone);
}

// 0x42bc80
int ActionFromName(const char* theName)
{
	return LookUp(kActionNames, sizeof(kActionNames) / sizeof(kActionNames[0]), theName, ActionNone);
}

// 0x42ba20
float ParseNumber(const char* theText)
{
	if (theText == nullptr)
		return 0.0f;
	if (strcmp(theText, "button") == 0)
		return 7.0f;
	return (float)atof(theText);
}
