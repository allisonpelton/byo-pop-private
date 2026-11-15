// The Survival mode's bonus: when a game of the mode ends, one score of a thousand for each whole minute that the player lasted
// counts up on the summary screen, with a sparkle and a sound for each, and the time goes into the high score list.

#ifndef VENICE_ENDLESSBONUS_H
#define VENICE_ENDLESSBONUS_H

#include "SceneObject.h"

// 0x148 bytes in the original; its constructor, 0x409a50, only sets the virtual table over the scene object's
class EndlessBonus : public SceneObject
{
public:
	void OnObjectClosed() override; // 0x404b80: builds the stages and the numbers of the count
	void OnCustomEvent() override; // 0x404f50: the next minute marker of the level lights up
	bool Update() override; // 0x404b30
};

#endif
