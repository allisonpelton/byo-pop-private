// The pigeon: a bird that sits on a ledge in the background of a level. It flies off when a jewel fills a slot near it, or when
// the level has scrolled it too far down the screen.

#ifndef VENICE_PIGEON_H
#define VENICE_PIGEON_H

#include "SceneObject.h"

// 0x14c bytes in the original
class Pigeon : public SceneObject
{
public:
	ImageRecord* mFlightImage = nullptr; // 0x148: the picture of the wings, which the data file's flight_image tag names

	Pigeon(); // 0x404040
	void Load(const char* theText, SceneTag* theTag) override; // 0x404070
	void OnObjectClosed() override; // 0x4040c0
	bool Update() override; // 0x404140

	// 0x404250: the bird flies away when the point is within sixty of it, unless it is already doing so. It flies off to the
	// left if its name says it sits on the left and to the right if not, after a random wait.
	void Scare(float theX, float theY);
};

#endif
