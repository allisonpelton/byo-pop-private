// Victory: the scene after the last level (0x473460..0x474f00 in the original; 0x2d0 bytes there). The level makes it
// when the player's saved progress is past the last world, shows it once that world is won (Show), and asks IsDone each
// frame. A banner sign drops in, a trophy bobs between two turning coronas, three monkeys bob at the bottom, hearts
// float up from the left, 120 pieces of confetti fall, and fireworks go off once the first 300 frames are over. A mouse
// press ends it: the whole scene fades away and IsDone says so.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class FireWorks;

const int VICTORY_MONKEYS = 3;
const int VICTORY_CORONAS = 2;
const int VICTORY_FIREWORKS = 12;

class Victory : public ObjBase
{
public:
	ObjBase* mMonkeys[VICTORY_MONKEYS];    // +0x27c: the picture of two monkeys (a cel each, the first smaller), bobbing
	ObjBase* mCoronas[VICTORY_CORONAS];    // +0x288: the lights behind the trophy (a guess at what the picture shows),
	                                       //   children of the trophy
	ObjBase* mTrophy;                      // +0x290
	bool mDone;                            // +0x294: the press came; the scene is fading away
	bool mMusicChanged;                    // +0x295: Show has changed the music, the destructor puts it back
	int mDelay;                            // +0x298: frames still to wait, from 300, before the fireworks and the press
	FireWorks* mFireWorks[VICTORY_FIREWORKS]; // +0x29c
	ObjBase* mSign;                        // +0x2cc: the banner that drops in; made inactive until Show

	Victory();                             // 0x473460
	~Victory();                            // 0x473590 (body 0x4735b0)
	void Notify(ObjBase* theObj) override; // 0x473630
	bool Update() override;                // 0x4736c0

	void Show();                           // 0x473840
	bool IsDone() { return mDone; }        // 0x4738a0

private:
	void BuildFadeTracks();                // 0x4738b0
	void BuildMonkeys();                   // 0x473de0
	void BuildTrophy();                    // 0x473fe0
	void BuildConfetti();                  // 0x4743d0
	void BuildHearts();                    // 0x474730
	void BuildSign();                      // 0x474cc0
};

} // namespace Sexy
