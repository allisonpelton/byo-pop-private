// The tutorial: the cupid who explains the first level. It is a scene object with the stages of the lessons (it comes in, says the
// rules, waits for the first shot, the second and so on), a scroll that shows what it says and an arrow that shows where to
// shoot. The level scrolls on from one lesson to the next when the plates of the lesson have gone by.

#ifndef VENICE_TUTORIAL_H
#define VENICE_TUTORIAL_H

#include "SceneObject.h"

#include <string>
#include <vector>

// 0x164 bytes in the original
class Tutorial : public SceneObject
{
public:
	std::vector<std::string> mTips; // 0x148: what the scroll says, a tip at a time (tip1, tip2 ... of the data file)
	size_t mTipIndex = 0; // 0x148's cursor: the next tip to show
	SceneObject* mTargetSlot = nullptr; // 0x154: the slot of the first lesson that the arrow points at

	Tutorial(); // 0x420340: the stages of the lessons, the scroll and the arrow
	void Load(const char* theText, SceneTag* theTag) override; // 0x420dd0
	void OnCustomEvent() override; // 0x421930: the stages' custom events show the next tip
	bool Update() override; // 0x420e80

private:
	void ShowTip(); // 0x420f20
	void UpdateFirstLesson(); // 0x4211c0
	void UpdateSecondLesson(); // 0x4214f0
	void UpdateThirdLesson(); // 0x421670
	void UpdateFourthLesson(); // 0x4217d0
	// the level scrolls on to the next plates (the middle layer's new stage), and the cupid goes on to the stage after the lesson
	void StartNextLesson(const char* theScrollStageName, float theScrollTo, const char* theCupidStageName);
	static bool PlateIsGone(SceneObject* thePlate); // there is no plate, or the lowest one is above the top of the screen
};

#endif
