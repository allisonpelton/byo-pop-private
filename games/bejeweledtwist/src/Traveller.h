// Sexy::Traveller (vtable 0x7f1a38, 0x74f284-0x74ff9b) and Sexy::SmokeTraveller (0x98 bytes, vtable 0x7f1a48,
// 0x74f457-0x7502ad): things that travel across the board. A SmokeTraveller is the smoke a match's points leave on
// their way to the tube; the points reach the score when it ends. Being ported: so far the SmokeTraveller and the
// parts of the Traveller it uses (the Traveller's own update, 0x74f30d, and draw, 0x74ff9b, come with its uses).
#pragma once

#include "Color.h"

namespace Sexy {

class Image;

class Traveller {
public:
	Image* mImage = nullptr;         // +0x4: IMAGE_EMBER
	float mT = 0;                    // +0x8
	float mSpeed = 0;                // +0xc
	float mX = 0;                    // +0x10: a piece's mX when made from one
	float mY = 0;                    // +0x14
	float mUnknown18 = 0;            // +0x18
	float mUnknown1C = 1.0f;         // +0x1c
	float mUnknown20[8] = {};        // +0x20: four points (0x757616 clears them)
	int mUnknown44 = 0;              // +0x44: a cel
	int mUnknown48 = 0;              // +0x48: the updates on that cel
	int mUnknown4C = 1;              // +0x4c
	float mDelay = 0;                // +0x50: counts down by 0.01 before it moves
	bool mSparkles = true;           // +0x54
	bool mDone = false;              // +0x55: the board deletes it

	Traveller();                     // 0x74f2a4 (with no piece)
	virtual ~Traveller() {}          // 0x74f284
	virtual void Update() {}         // 0x74f30d
	virtual void Draw() {}           // 0x74ff9b
};

class SmokeTraveller : public Traveller {
public:
	float mProgress = 0;             // +0x58: 0 to 1 along the way
	float mFromX = 0;                // +0x5c
	float mFromY = 0;                // +0x60: wanders on the way
	float mToX = 0;                  // +0x64
	float mToY = 0;                  // +0x68
	float mStep = 0.038f;            // +0x6c: the progress an update
	float mHurry = 0;                // +0x70: 0 to 1, faster after the game's first good twists
	float mWanderY = 0;              // +0x74: where mFromY wanders to
	float mWanderDY = 0;             // +0x78
	Color mColor;                    // +0x7c
	int mPoints = 0;                 // +0x8c
	float mScale = 1.0f;             // +0x90
	bool mBurst = true;              // +0x94: sparks at the end
	bool mNoPoints = false;          // +0x95
	bool mSound = false;             // +0x96: a sound at the end
	bool mAdditive = true;           // +0x97

	// 0x750185
	SmokeTraveller(const Color& theColor, int theFromX, int theFromY, int theToX, int theToY, int thePoints,
		bool theSound);
	~SmokeTraveller() override;      // 0x74f457: the points reach the score, and the sparks
	void Update() override;          // 0x74f67f
	void Draw() override {}          // 0x74f7e9
	virtual void EmitSmoke();        // 0x74f51f (slot 3)
};

}  // namespace Sexy
