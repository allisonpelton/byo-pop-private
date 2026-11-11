#ifndef __TYPERSHARK_PLAYER_H__
#define __TYPERSHARK_PLAYER_H__

// Player.cpp: the diver at the left edge of the board, hanging from his hose. He climbs down into the water (state 1),
// sends an EMP wave to the right when the board sets mEmpActive, is beaten (state 3: a cloud of dust and the beaten
// diver) and floats up (state 2). The player also keeps the score and the points at which mLives grows.
// Original: class Sexy::Player : GameObject (the only base, so the GameObject vptr is at +0 and the slots are direct),
// vftable 0x53198c (deleting destructor 0x461e80 shared with Bubble and BackgroundFish, Update 0x461740, Draw 0x461f00,
// SetState 0x461520, the rest GameObject's), sizeof 0x148; constructor 0x461320 (the board makes it in its constructor
// and keeps it at Board +0xd68). The field comments give the offsets in the original's object.
//
// States (GameObject::mState), as far as Update and Draw tell: 0 normal (the 8 frames of the diver), 1 climbing down from
// the top of the screen to y 235, 2 and 10 the beaten-diver picture (2 also floats up), 3 beaten (the dust cloud plays and
// fades; when it is faint enough the state goes to 2), 4 and 12 sinking slowly (to y 325), 5, 6, 7 and 11 the frame does
// not advance, 8 gone (nothing is updated or drawn). SetState also plays the sound or changes the music of some states.

#include "GameObject.h"

namespace Sexy
{

class Graphics;
class WinWSApp;

class Player : public GameObject
{
public:
	WinWSApp*				mApp;					// +0xf0 constructor argument
	int						mScore;					// +0xf4 the points of the game so far (AddScore, SetScore)
	double					mEmpCharge;				// +0xf8 constructor 0; the board adds 0.01 for each word it scores and tests it against 1.0
	bool					mEmpActive;				// +0x100 the EMP wave is travelling right (constructor false)
	double					mEmpX;					// +0x108 the x of the wave: 10 more each tick; constructor mX + 50
	int						mLives;				// +0x110 constructor 2; the diver's lives left (below 0 is game over: Board::PlayerEaten takes one, HudBar shows it); AddScore adds 1 whenever the score reaches mNextLevelScore; saved with the game and recorded in the results
	int						mLevelNumber;				// +0x114 constructor 0; the diver's level number counted from 0 (the HUD and the game-over dialog show it plus one, GetLevelBonus and the rating bonus use it): StartGame copies it from the stats word that WinWSApp counts up each time a level starts, GetStats gives it back, and the saved game and the profile's result lists keep it
	bool					mDiveActive;				// +0x118 constructor false; the dive is under way: Update sets it when the climb down ends, SetState(4) and the board's dive code clear it; the board scrolls the sea pictures and counts the depth only while it is set, and the boss-wave tests read it
	int						mHoseFrame;				// +0x11c the frame (0 to 7) of the hose picture, moves by 1 every 22 ticks up and down
	bool					mHoseFrameUp;			// +0x120 the hose frame is counting up (not set by the constructor)
	double					mHoseY;					// +0x128 the y the hose hangs to: follows mY while the diver climbs down and sinks (not set by the constructor)
	int						mAlpha;					// +0x130 the alpha of the dust cloud (state 3, 255 down by 8 a tick) and of state 2 (not set by the constructor)
	int						mCloudRounds;				// +0x134 the number of times the dust cloud has played (state 3: Update counts one each time the frame wraps, SetState(3) clears it; the cloud fades after four rounds)
	int						mNextLevelScore;		// +0x138 the score at which mLives grows next; constructor by the mode
	bool					mExtraLivesOff;				// +0x13c constructor false; AddScore does nothing past adding the points while it is set (an abyss game sets it, so no extra lives come from the score)
	bool					mExtraLifeFlashing;				// +0x13d constructor false; AddScore sets it whenever the score has reached mNextLevelScore (an extra life was won), HudBar flashes "EXTRA LIFE!" while it is set and clears it after four flashes
	int						mMode;					// +0x140 constructor: WinWSApp +0x690 (0 to 4), picks the thresholds
	bool					mBittenByShark;				// +0x144 which kind of creature beat the diver: the shark event loop sets it, the piranha event loop clears it, just before SetState(3); SetState(3) reads it and does nothing

public:
	Player(WinWSApp* theApp);														// 0x461320
	virtual ~Player();																// deleting destructor 0x461e80

	virtual void			Update() override;										// slot 1, 0x461740
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x461f00
	virtual void			SetState(int theState);									// GameObject slot 7, 0x461520

	void					AddScore(int theScore);									// 0x4619d0 (the export names it a Board function)
	void					SetDiveActive(bool theFlag) { mDiveActive = theFlag; }		// 0x461b40 sets mDiveActive
	void					SetScore(int theScore);									// 0x461b50 (the export names it a WinWSApp function)
};

}

#endif
