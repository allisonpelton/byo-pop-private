// The panels at the top of a level: the score panel (with the tries left in the Journey mode), the Survival mode's clock and the
// progress bar, which ends the level when the last plate is gone.

#ifndef VENICE_PANEL_H
#define VENICE_PANEL_H

#include "SceneObject.h"

// The score panel (0x150 bytes in the original): it shows the score, which swells for a moment when it changes. In the Journey
// mode (the original calls it relaxed) the panel has the tries beside the score. It also tells the shooter when the pointer is
// on a button of the panel, where a click must not shoot.
class Panel : public SceneObject
{
public:
	float mScoreScale = 1.0f; // 0x148: the size the score is drawn at, 1.2 after a change and back to 1
	int mIsRelaxed = 0; // 0x14c: the Journey mode's panel, which has the tries and the shooter picture

	Panel(); // 0x411d30
	void OnObjectClosed() override; // 0x411d90
	bool Update() override; // 0x411e90
	void Draw() override; // 0x412010
};

// The clock of the Survival mode (0x148 bytes in the original): the object's own tick count is the time of the game
class EndlessPanel : public SceneObject
{
public:
	EndlessPanel(); // 0x412190
	void Draw() override; // 0x4121e0
};

// The end of a level (0x148 bytes in the original): when no plate is left it makes the level a victory. It draws nothing.
class ProgressBar : public SceneObject
{
public:
	ProgressBar(); // 0x412260
	bool Update() override; // 0x412320
	void Draw() override; // 0x4127c0

private:
	void CheckVictory(); // 0x412340
};

#endif
