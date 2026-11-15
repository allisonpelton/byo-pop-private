// The power-ups: a plate gives one when the first jewel fills its slots (and in the tutorial, when the third plate is on the
// screen). The power-up sits on the plate, rolls off it when the plate turns and falls with the sideways speed that gave it, and
// the shooter catches it when it touches it; the catch gives the shooter what the kind says.

#ifndef VENICE_POWERUP_H
#define VENICE_POWERUP_H

#include "SceneObject.h"

enum PowerupKind
{
	PowerupWings = 0, // the jewel in the shot flies on and is not lost for a while
	PowerupPowerShot = 1, // six shots that pass through the bumpers
	PowerupRewind = 2, // the level scrolls back (never in the Journey mode)
	PowerupWild = 3, // two wild jewels
	PowerupComboShot = 4, // the next shot fills the slots of its colour near the one it fills
	PowerupExtraTry = 5, // one more try (the Journey mode)
	PowerupScoreMultiplier = 6, // the score of everything is doubled (not in the Survival mode)
	PowerupCupid = 7 // the cupid comes to carry the jewels that would be lost
};

// 0x16c bytes in the original
class Powerup : public SceneObject
{
public:
	int mKind = 0; // 0x148: a PowerupKind
	char mTypeName[32] = {}; // 0x14c: the name a data file gives it (powerup_type)

	Powerup(); // 0x4165b0: a kind that the mode allows, at random
	void Load(const char* theText, SceneTag* theTag) override; // 0x416720
	void OnObjectClosed() override; // 0x4168b0
	bool Update() override; // 0x416ef0

	// 0x416af0: a power-up at the place of the object (a plate or a slot), unless there is only one plate or the level has a
	// single shot; in the Journey mode the kind is drawn again until it is one the level can use
	static void SpawnFrom(SceneObject* theSource);

	// 0x416cd0: it leaves its plate with a speed, and the level takes it over (a plate that has completed lets its power-up go)
	void StartFlying();

private:
	void SetKind(int theKind); // 0x416900: the picture and the name (which is the text that shows when it is caught)
	void Roll(); // 0x4170d0: a power-up on a turning plate rolls to the side the plate turns away from, and falls off the edge
	bool Fly(); // 0x416f20: it bounces off the sides of the screen, is lost under it and is caught by the shooter
	void Catch(); // 0x417620
	void ShowCatchScore(); // 0x417200: after the level is won a catch scores 500
	void GiveRewind(); // 0x417720
	void GivePowerShots(); // 0x417960
	void GiveWings(); // 0x417b90
	void GiveWild(); // 0x417fc0
	void GiveComboShot(); // 0x418270
	void GiveExtraTry(); // 0x418470
	void GiveCupid(); // 0x418770
	void GiveScoreMultiplier(); // 0x418810
};

#endif
