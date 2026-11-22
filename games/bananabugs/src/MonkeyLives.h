// MonkeyLives: a monkey the second upgrade adds to the score board (0x44f450..0x4504d0 in the original; 0x294 bytes
// there). It dances on the board, one of three dances picked at random each time round. Asked to shoot (the pea
// shooters bonus), it starts its shooting at the end of the dance it is in: a blowgun shot that fires a pea (Pea,
// which flies at a bug) and a puff of light at its mouth. Dismissed, or with the player's own monkey close by, it
// plays a stepping-aside picture at the end of its dance, then goes back to dancing.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;
class Pea;

// What the monkey is to do at the end of the dance it is in (mPhase)
enum MonkeyPhase
{
	MONKEY_PHASE_DANCE = 0,      // dance on
	MONKEY_PHASE_SHOOT = 1,      // shoot (asked to by the board)
	MONKEY_PHASE_STAND_DOWN = 2, // step aside (dismissed, or the player's monkey is near)
};

class MonkeyLives : public ObjBase
{
public:
	ObjBase* mPuff;              // +0x27c: the puff of light at its mouth as it shoots, a child
	Pea* mPea;                   // +0x280: the pea it fires, under the root
	int mPhase;                  // +0x284: MonkeyPhase
	int mUnused288;              // +0x288: nothing writes or reads it
	int mReappearTrack;          // +0x28c: the track started when the vanishing track ends
	int mVanishTrack;            // +0x290: nothing starts it

	// The five pictures (0x60e6a4..0x60e6b4), loaded by each monkey's constructor
	static Anim* sDanceAnim[3];  // images/monkey_lives1..3: the three dances (28, 35 and 50 cels)
	static Anim* sShootAnim;     // images/monkey_lives4: the shot (75 cels)
	static Anim* sStandDownAnim; // images/monkey_lives5: stepping aside (36 cels; a guess at what it shows)

	MonkeyLives();                       // 0x44f450
	~MonkeyLives();                      // 0x44f630 (the body 0x44f650)
	void Notify(ObjBase* theObj) override; // 0x44f6c0
	bool Update() override;              // 0x44f800

	void StartShooting();                // 0x44f840: if the pea is not in flight and it is dancing, it will shoot
	                                     //   (ScoreBoard::WakeMonkeys; the original's name is unknown)
	void StandDown();                    // 0x44f870 (ScoreBoard::DismissMonkeys)

private:
	void BuildTracks();                  // 0x44fd50
	void BuildPuff();                    // 0x44f9e0
	void BurstSmoke();                   // 0x44f880
};

} // namespace Sexy
