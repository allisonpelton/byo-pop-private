// The obstacles: the bumpers that the jewels bounce off. They share the fade (a bumper turns half transparent while the jewel in
// flight is over it) and the removal of a bumper that has scrolled off the bottom of the screen.

#ifndef VENICE_OBSTACLE_H
#define VENICE_OBSTACLE_H

#include "SceneObject.h"

// 0x150 bytes in the original
class Obstacle : public SceneObject
{
public:
	float mPreviousY = 99999.0f; // 0x148: the world y of the last update, to tell which way the obstacle moves
	float mBounceCooldown = 0; // 0x14c: updates since the last bump sound (the sound plays once in 21 updates)

	Obstacle(); // 0x4018a0
	bool Update() override; // 0x4018f0

protected:
	void PlayBumpSound(); // 0x401b20
};

// A rectangle: the jewel bounces off its four sides. When the stage it is in has an event that runs on code, the bounce fires it
// and the jewel is marked as having hit a bumper that runs code (which gives the shot back).
class RectBumper : public Obstacle
{
public:
	RectBumper() {} // 0x4099b0
	bool Update() override; // 0x401b70
};

// A ball: the jewel's velocity is reflected off it, as off the ball of a slot
class RoundBumper : public Obstacle
{
public:
	RoundBumper() {} // 0x4099d0
	bool Update() override; // 0x401e60
};

// A triangle with its point down: the jewel bounces off its three sides
class TriBumper : public Obstacle
{
public:
	TriBumper() {} // 0x4099f0
	bool Update() override; // 0x402130
};

#endif
