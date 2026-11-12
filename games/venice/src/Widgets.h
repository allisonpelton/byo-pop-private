// The widgets of the options and name screens: the name of the current player, the list of the players, the sliders for the
// music and the sounds and the check boxes of the options.

#ifndef VENICE_WIDGETS_H
#define VENICE_WIDGETS_H

#include "SceneObject.h"

const int WidgetTextSize = 64;

// 0x1cc bytes in the original: draws a fact about the current player. The only one in the data files is the player's name, and a
// display of another kind would have drawn whatever was left on the original's stack.
class UserDataDisplay : public SceneObject
{
public:
	char mDisplayType[WidgetTextSize] = ""; // 0x148: what to show ("current_user_name")
	char mFontName[WidgetTextSize] = ""; // 0x188

	void Load(const char* theText, SceneTag* theTag) override; // 0x4364f0
	void DrawSelf() override; // 0x436570
};

// 0x18c bytes in the original: the names of the players, a row each, with the current player's row lit; a click on a row makes
// the player current
class UserListBox : public SceneObject
{
public:
	char mFontName[WidgetTextSize] = ""; // 0x148
	float mTextScale = 0.0f; // 0x188: the size of the names ("size" in the data)

	void Load(const char* theText, SceneTag* theTag) override; // 0x436690
	bool Update() override; // 0x4366f0
	void Draw() override; // 0x4368b0

private:
	float RowPitch() const; // 0x424660's measure of an M, and five more
	double RowY(int theRow) const; // 0x436840: the row's y from the box's own y; the first row is six rows above it
};

// What a slider sets
enum SliderKind
{
	SliderKindNone,
	SliderKindMusicVolume,
	SliderKindSoundVolume,
	SliderKindStartLevel // the level the game starts at, 1 to 12 (no screen has one)
};

// 0x1d4 bytes in the original: a knob that slides from the left value to the right value (picture positions), and so from 0 to
// 100 per cent of what it sets
class Slider : public SceneObject
{
public:
	float mLeftValue = 0.0f; // 0x148: the x of the knob at 0 per cent
	float mRightValue = 0.0f; // 0x14c: at 100 per cent
	bool mIsSliding = false; // 0x154: the knob follows the pointer (it was pressed, and the button is still down)
	SliderKind mKind = SliderKindNone; // 0x194

	void Load(const char* theText, SceneTag* theTag) override; // 0x437270
	bool Update() override; // 0x437030

private:
	float GetPercent() const; // 0x437490
	void ApplyPercent(); // 0x437510: the setting follows the knob
};

// What a check box shows and sets
enum CheckboxKind
{
	CheckboxKindNone,
	CheckboxKindAccelerated, // "3d": the accelerated display
	CheckboxKindFullscreen,
	CheckboxKindInstructions // the player's "don't show this again" of the first level's instructions
};

// 0x190 bytes in the original: its picture (the tick) shows while it is checked, and a click while the pointer is on the box or
// up to "right_area" to its right (over its label) turns it over
class Checkbox : public SceneObject
{
public:
	int mChecked = 0; // 0x148
	CheckboxKind mKind = CheckboxKindNone; // 0x14c
	float mRightArea = 0.0f; // 0x18c: how far to the right of the box the pointer is still on it

	void Load(const char* theText, SceneTag* theTag) override; // 0x4378c0
	bool IsHot() override; // 0x4376d0
	bool Update() override; // 0x437790

private:
	void Toggle(); // 0x4379f0
};

#endif
