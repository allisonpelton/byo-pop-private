// The lock on a mode's name on the mode-choice screen: the Flood, Trick Shot and Survival modes are shut until the player has
// played far enough, and a shut one is dimmed, has a lock on each side of its name and a tooltip that says what opens it.

#ifndef VENICE_LEVELLOCKER_H
#define VENICE_LEVELLOCKER_H

#include "SceneObject.h"

// 0x148 bytes in the original
class LevelLocker : public SceneObject
{
public:
	LevelLocker() {} // 0x409a70
	void OnObjectClosed() override; // 0x4051f0
};

#endif
