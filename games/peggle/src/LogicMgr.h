// LogicMgr.h: the rules of the game (the original's Sexy::LogicMgr, 99 functions): the state of a turn (aiming, a shot in play,
// the end of a shot, the level done), the pegs and which of them are the orange ones, the score and the scores' multipliers,
// the balls left, the fever, the power-ups, the slot machine, the replay's click times. The board owns one and calls it for
// everything that happens to a level.
//
// Only the making, the emptying and the start of a level (with the choosing of the goal pegs) are translated so far. The fields
// keep the interim names mUnknownNN, the offset in the original, until their functions are.

#ifndef __LOGICMGR_H__
#define __LOGICMGR_H__

#include "GameStats.h"
#include "SharedPtr.h"

#include <list>
#include <set>
#include <string>
#include <vector>

namespace Sexy
{

class Ball;
class Board;
class PhysObj;
class SoundInstance;

// The order of the sets of pegs. The original keeps a std::set of pointers, so its order is the order of the pieces' places in
// memory, which is the order they were made in as near as the heap goes; the port orders them by their serial number (the same
// thing, and the same on every run).
struct PegSerialLess
{
	bool operator()(const SharedPtr<PhysObj>& theLeft, const SharedPtr<PhysObj>& theRight) const;
};
typedef std::set<SharedPtr<PhysObj>, PegSerialLess> PegSet;

// The states of a turn (SetState)
enum LogicState
{
	LOGIC_STATE_UNKNOWN0 = 0,
	LOGIC_STATE_PRE_SHOT = 1,
	LOGIC_STATE_SHOT = 2,
	LOGIC_STATE_INIT_LEVEL = 8, // BeginInitLevel
	LOGIC_STATE_UNKNOWN10 = 10
};

// One record of the balls list (+0xd0), 20 bytes in the original: where a ball was in an update of the shot, for the replay
struct BallRecord
{
	int mTime;	 // the shot's update
	int mBallId; // which ball (0 or 1)
	float mX;
	float mY;
	bool mUsed; // +0x10: written already
};

// One click of the shot kept for the replay: when (the update of the state 2), pressed or let go, which button
struct ClickEvent
{
	int mTime;
	bool mIsUp;
	bool mRight;
};

// 16 bytes at +0xb4
struct LogicRecord16
{
	int mUnknown[4];
};

class LogicMgr
{
public:
	explicit LogicMgr(Board* theBoard); // 0x4610d0
	virtual ~LogicMgr();				// the original's deleting destructor

	void SetState(int theState);						// 0x436fb0
	void Clear(bool theNewGame, bool theKeepLists = false); // 0x45d900: the state of a level (a new game: the players' too)
	void Update();		// 0x4730d0
	bool IsComputerTurn() const;						// the flag of the player in turn at +0x244
	bool ClearedLevel() const;							// 0x43d580: no orange peg is left (with mUnknownFB, no purple one either)
	bool MouseMove();									// 0x43d5d0: the cannon follows the mouse again
	bool MouseDrag(int theX, int theY);					// 0x43d620
	bool MouseDown(int theX, int theY, int theButton, bool theReplayed, bool theForced); // 0x472810: theButton is below 0 for the right button
	bool MouseUp(int theX, int theY, int theButton, bool theReplayed); // 0x45ebb0
	void CalcCornerDisplay();						// 0x448370: what the corner of the board says
	void SpeedTransition();
	void DoTimeBomb();
	void FlipperClick(bool theDown);				// 0x45ea70
	bool CheckDoStealthHelperShot();					// 0x?
	void UpdateGun();								// 0x46d630
	void UpdateFreeBall();								// 0x440830
	void RemoveHitPegs();
	bool UpdateShotBalls();								// 0x467230
	void UpdateWrapAroundBall();						// 0x45f120
	void CheckBallStop();								// 0x466df0
	int GetNumPegsLeft() const;							// 0x43d5b0
	void CheckCollisions();								// 0x4704a0: what the balls touched in the update (the pegs they hit) is looked at
	void DoFeverSlow();									// 0x45e920: the slow motion of the fever starts
	void FinishShot();									// 0x470a50
	void HoleHit(PhysObj* theHole, Ball* theBall);		// 0x470230
	void PegHit(Ball* theBall, PhysObj* thePeg, bool theFlag); // 0x46f480
	void CheckDoFlippers();								// 0x472c70
	void AddFeverSparks(Ball* theBall, bool theZen);	// 0x451ec0
	void UpdatePreShot();								// 0x466f10
	void UpdateShot();									// 0x472f60
	void UpdatePostShot();
	void UpdateTotalMiss();								// 0x471500
	void UpdateLevelDone();
	void UpdatePostPostShot();							// 0x471600
	void UpdateShotExtender();							// 0x44b690
	void UpdateInitLevel();								// 0x4484a0
	void UpdateCharacterDialog();						// 0x4709f0
	void UpdateZenShot();
	void CheckDoCharacterDialog();					// 0x437360
	void BeginTurn(bool theNextPlayer);					// 0x46e880
	void ActivatePowerups();						// 0x461560: the powers that are on this turn, for the player in turn
	void ActivatePowerup(int thePower, bool theActive); // 0x4612e0
	void ActivateFreeBall(bool theActive);			// 0x440580
	void DoBeginTurnText();							// 0x469f30: '3 Balls Left' and the like
	void DoBeginTurnTip(int theTip);				// 0x46a370: a tip for a new player
	void SetWearHat(bool theWear);					// 0x43d4d0
	void BeginShot(bool theRecord);						// 0x46ac70: the ball leaves the cannon
	void IncNumBalls(int theBalls, int theDelay, bool theAtBottom); // 0x45d880
	void BeginTurn2();								// 0x44b5b0
	void DoLevelDone();								// 0x46a9c0
	void FinishInitLevelText();						// 0x46c220
	void MouseWheel(int theDelta);		// 0x43d6a0
	void DoFeverMissed();		// 0x4372e0
	void KillSlowMoSound();								// 0x4372c0
	void FadeOutSound();								// 0x458ab0 (no name in the original): the sound that fades goes to the sound manager
	int CalcScoreMult(int theNumPegsHit) const;			// 0x4370b0
	static int CalcMusicIntensity(int theNumPegsLeft);	// 0x437110
	void CalcGunAngle(bool theSetAtOnce);				// 0x43ece0: aims the cannon where the mouse is, with the lead the ball's fall needs
	void SetCharacters(int theFirst, int theSecond); // 0x43f020: the masters of the two players; the one of the player in turn is shown
	void SetGunAngle(float theAngle);					// 0x436fd0: the angle in [0, 2 pi), held to the band the level allows
	void BeginInitLevel();								// 0x440500
	void ActivateFreeBallCover(bool theActive);			// 0x440700
	void MakeGoalPegs();								// 0x457960: picks the orange pegs (and sets every other peg to its colour)
	void MakePowerupPeg();								// 0x457dc0: picks the green pegs
	void MakeScorePeg();								// 0x458110: picks the purple peg
	// 0x45dda0: theResetScores gives both players their balls and a score of 0; theRandomizeStart picks who shoots first of two;
	// theKeepLists keeps the lists of pegs the board has (a level that is only started again)
	void InitLevel(bool theResetScores, bool theRandomizeStart, bool theKeepLists);
	void ReInitLevel();									// 0x4612d0

	int mState;				 // +4: see LogicState
	int mStateTime;			 // +8: updates since the state was set
	int mUnknown0C;			 // +0xc
	int mUnknown10;			 // +0x10
	int mUnknown14;			 // +0x14
	bool mUnknown18;		 // +0x18
	bool mUnknown19;		 // +0x19
	bool mUnknown1A;		 // +0x1a
	bool mUnknown1B;		 // +0x1b
	bool mUnknown1C;		 // +0x1c
	bool mUnknown1D;		 // +0x1d
	bool mUnknown1E;		 // +0x1e
	bool mUnknown1F;		 // +0x1f
	bool mUnknown20;		 // +0x20
	bool mUnknown21;		 // +0x21
	int mUnknown24;			 // +0x24: 1 for two players (InitLevel copies mTwoPlayer)
	int mUnknown28;			 // +0x28
	int mUnknown2C;			 // +0x2c
	int mUnknown30;			 // +0x30
	int mUnknown34;			 // +0x34
	int mUnknown38;			 // +0x38
	int mUnknown3C;			 // +0x3c
	int mUnknown40;			 // +0x40
	int mUnknown44;			 // +0x44
	int mUnknown48;			 // +0x48
	int mUnknown4C;			 // +0x4c
	int mUnknown50;			 // +0x50
	bool mUnknown54;		 // +0x54
	bool mUnknown55;		 // +0x55
	int mUnknown58;			 // +0x58: one for each player (the next is +0x5c)
	int mUnknown5C;			 // +0x5c
	int mUnknown60;			 // +0x60: and these two
	int mUnknown64;			 // +0x64
	bool mUnknown68;		 // +0x68
	bool mUnknown69;		 // +0x69
	int mUnknown6C = 0;		 // +0x6c: -1000 when a shot starts (Clear leaves it)
	float mUnknown70;		 // +0x70: the mouse wheel's base move when the state was cleared
	int mUnknown74;			 // +0x74
	int mUnknown78;			 // +0x78
	std::string mCornerText; // +0x7c: the corner's text ("Fast Forward", "Triple Score", "ZenBall", "Fireball", "Magic Hat" or none)
	int mUnknown98;			 // +0x98: the place of the last drag (-10000 when it moved)
	int mUnknown9C;			 // +0x9c
	int mUnknownA0;			 // +0xa0
	int mUnknownA4;			 // +0xa4
	bool mUnknownA8;		 // +0xa8
	bool mUnknownA9;		 // +0xa9
	bool mUnknownAA;		 // +0xaa
	SoundInstance* mSlowMoSound; // +0xac: the sound of the slow motion, while it plays
	SoundInstance* mFadeSound;	 // +0xb0: the sound that fades when the level starts
	std::vector<LogicRecord16> mUnknownB4; // +0xb4
	std::list<ClickEvent> mClickEvents;	   // +0xc4: the clicks the shot has had (a replay plays them again)
	std::vector<BallRecord> mBallRecords; // +0xd0: where the balls were in each update of the shot, for the replay
	float mGunAngle;		 // +0xe0: where CalcGunAngle aims the cannon (the player's mouse)
	float mUnknownE4;		 // +0xe4
	int mUnknownE8;			 // +0xe8
	bool mUnknownEC;		 // +0xec
	bool mUnknownED;		 // +0xed
	bool mUnknownEE;		 // +0xee: CheckDoStealthHelperShot's answer
	int mUnknownF0;			 // +0xf0
	bool mUnknownF4;		 // +0xf4
	bool mUnknownF5;		 // +0xf5
	bool mUnknownF6;		 // +0xf6
	bool mUnknownF7;		 // +0xf7
	bool mUnknownF8;		 // +0xf8
	bool mUnknownF9;		 // +0xf9
	bool mUnknownFA;		 // +0xfa
	bool mUnknownFB;		 // +0xfb
	bool mUnknownFC;		 // +0xfc
	bool mUnknownFD;		 // +0xfd
	bool mUnknownFE;		 // +0xfe
	bool mUnknownFF;		 // +0xff
	bool mUnknown100;		 // +0x100
	bool mUnknown101;		 // +0x101
	Board* mBoard;			 // +0x104
	int mUnknown108;		 // +0x108
	int mUnknown10C;		 // +0x10c
	int mUnknown110;		 // +0x110
	int mScoreMult;			 // +0x114: what CalcScoreMult says of the pegs left
	int mMusicIntensity;	 // +0x118
	int mUnknown11C;		 // +0x11c
	int mUnknown120;		 // +0x120
	bool mTwoPlayer;		 // +0x124: a game of two players (kept when a level is cleared)
	int mCurPlayer;			 // +0x128: whose turn it is (0 or 1)
	int mUnknown12C;		 // +0x12c
	int mNumGoalPegs;		 // +0x130: how many orange pegs a level has; 0 for the usual 25
	int mUnknown134;		 // +0x134
	int mUnknown138;		 // +0x138
	int mUnknown13C;		 // +0x13c
	int mUnknown140;		 // +0x140
	int mUnknown144;		 // +0x144
	int mUnknown148;		 // +0x148
	int mUnknown14C;		 // +0x14c: -1 when cleared
	int mUnknown150;		 // +0x150: -1 when cleared
	int mUnknown154;		 // +0x154
	int mUnknown158;		 // +0x158
	int mUnknown15C;		 // +0x15c: the player's stage (InitLevel copies it)
	int mLevelType;			 // +0x160: 1, 2 or 3 for the levels the adventure's replay bonus pays for (InitLevel copies the player's)
	int mUnknown164;		 // +0x164
	int mUnknown168;		 // +0x168
	int mUnknown16C;		 // +0x16c
	SharedPtr<PhysObj> mScorePeg; // +0x170: the purple peg
	int mScore[2];			 // +0x174: each player's score
	int mNumBalls[2];		 // +0x17c: the balls each player has left
	int mCharacter[2];		 // +0x184: the master each player plays
	std::string mPlayerName[2]; // +0x18c: "Player1" and "Player2"
	int mPowerup[2];		 // +0x1c4: the power-up each player has
	int mUnknown1CC;		 // +0x1cc
	int mUnknown1D0;		 // +0x1d0
	int mUnknown1D4[14][2];	 // +0x1d4: fourteen pairs
	bool mUnknown244;		 // +0x244
	bool mUnknown245;		 // +0x245
	GameStats mPlayerStats[2]; // +0x248: the statistics of each player (0x6c bytes each in the original)
	int mUnknown320;		 // +0x320
	int mUnknown324;		 // +0x324
	int mScoreAtStart[2];	 // +0x328: the scores the level started with
	float mUnknown330;		 // +0x330
	float mUnknown334;		 // +0x334
	float mUnknown338;		 // +0x338
	float mUnknown33C;		 // +0x33c
	std::list<SharedPtr<PhysObj> > mPegs; // +0x340: every peg of the level
	std::list<SharedPtr<PhysObj> > mUnknown34C; // +0x34c
	PegSet mGoalPegs;		 // +0x358: the orange pegs
	PegSet mOtherPegs;		 // +0x364: the other pegs
	int mUnknown370[11];	 // +0x370: zeroed with Clear
};

} // namespace Sexy

#endif
