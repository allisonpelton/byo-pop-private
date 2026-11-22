// Board.h: the game's board (the original's Sexy::Board, 0x250 bytes): the widget the game is played on. It holds the level's
// pieces (the objects and the balls in play), the managers that work on them (the bot, the collisions, the effects, the
// floating texts, the frame around the board, the sounds, the rules), the cannon, the pause and replay buttons and the dialogs
// that end a level. A level is put on it by LoadLevel (the file's pieces, the base pieces every level shares, the picture behind)
// and SetupLevel (the rules' start of the level).
//
// Only the making, the emptying and the putting of a level on the board are translated so far; the update, the drawing, the
// mouse and the rest of the original's 300 functions are not. The editor of levels is not part of the game: EditingLevel is
// always false and the editor's fields are left out. The fields keep the interim names mUnknownNN, the offset in the original,
// until their functions are translated.

#ifndef __BOARD_H__
#define __BOARD_H__

#include "ButtonListener.h"
#include "Gun.h"
#include "PhysObj.h"
#include "SharedPtr.h"
#include "Widget.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

namespace Sexy
{

class AIMgr;
class CharacterMgr;
class CollisionMgr;
class DebugMgr;
class EffectMgr;
class EndLevelDialog;
class FloatingTextMgr;
class InterfaceMgr;
class LogicMgr;
class ReplayDialog;
class SlotMachineDialog;
class SoundMgr;
class ThunderButton;
class ThunderballApp;
class TrophyInfo;

// One word the player may type to get a surprise (+0x8c ...): the word and how far the typing has got
struct TypingCheck
{
	std::string mWord;
	int mTyped;
};

class Board : public Widget, public ButtonListener
{
public:
	explicit Board(ThunderballApp* theApp); // 0x4238a0
	~Board() override;

	void Clear(bool theNewGame);			// 0x40d7d0: the level goes, the board is as new (a new game: the players go too)
	void Reset();							// 0x42da00: a level (re)starts: the mode's options, the music, and the level read again
	bool LoadLevel(std::string theName);				// 0x42aae0 (by value: Reset passes the board's own mLevelName, which Clear erases): puts a level on the board; false when its file cannot be read
	bool LoadLevel2(std::string thePath);	// 0x429d10
	void SetupLevel();						// 0x41ea50: the rules' start of the level
	void SubmitTotalStats();				// 0x406a20: the level's statistics join the player's totals (once)
	void CheckIncTip();						// 0x406c10
	void RemoveSlotMachineDialog();			// 0x402480
	void SyncModeOptions();					// 0x40f980: the options the game mode gives the board (the level it plays)
	void SyncCharacterOptions(bool theDrawAgain); // 0x40f400: false keeps the masters the rules already chose
	void Update() override;					// 0x410e10
	void UpdateMgrs();						// 0x402ad0: the managers' updates of one step
	void UpdateSlowMo();					// 0x402ba0: the slow motion's counters of one step
	void UpdateObjects(bool theMoreSteps);	// 0x410ae0
	void UpdateZoom();						// 0x40ad10
	void Reload();										// 0x4090d0: a new ball in the cannon
	void PlayMusic();						// 0x422640
	void AddObj(const SharedPtr<PhysObj>& theObj); // 0x422a90 (0x421070): a piece of the level joins the board's lists
	void SetSlowMo(bool theSlowMo, int theRate); // 0x4026f0
	void DeleteReplays(bool theKeep);		// 0x40cf70
	void CheckForBackground();				// 0x409920: the picture behind the level, when it is big, is the one drawn
	void SetWindowTitle(const std::string& theLevelName); // 0x41bb80
	bool EditingLevel() const { return false; } // the level editor is not part of the game

	int GetBoardMouseX() const;				// 0x407880: where the mouse is, in the board's own places
	int GetBoardMouseY() const;				// 0x4078a0

	// the offsets are the original's
	ThunderballApp* mApp;					// +0xb0
	int mGameMode;							// +0xb4: the app's mode when the level was started (Reset copies it)
	std::vector<TypingCheck> mTypingChecks;	// +0x8c: the words that do something when they are typed
	SharedPtr<Gun> mGun;					// +0xbc: the cannon
	bool mUnknownC0;						// +0xc0: true
	bool mSlowMo;							// +0xc1: the slow motion is on
	bool mUnknownC2;						// +0xc2
	bool mUnknownC3;						// +0xc3
	bool mFileNameUnderscore;				// +0xc4: the level's file name starts with "_" (the base file is not read for it)
	bool mUnknownC5;						// +0xc5
	bool mUnknownC6;						// +0xc6
	bool mUnknownC7;						// +0xc7
	int mUnknownC8;							// +0xc8
	int mUnknownCC;							// +0xcc
	int mSlowMoRate;						// +0xd0: 250
	int mUnknownD4;							// +0xd4
	int mUnknownD8;							// +0xd8: 20
	int mLives;								// +0xdc: the challenge's lives (1 otherwise)
	int mUnknownE0;							// +0xe0
	int mUnknownE4;							// +0xe4
	bool mUnknownE8;						// +0xe8: set when a level's picture is the full-size one, by CheckForBackground
	bool mUnknownE9;						// +0xe9
	bool mUnknownEA;						// +0xea
	int mResetCount;						// +0xec: how many times Reset was called
	int mUnknownF0;							// +0xf0
	int mUnknownF4;							// +0xf4
	int mUnknownF8;							// +0xf8
	int mUnknown104;						// +0x104
	int mUnknown108;						// +0x108
	int mUnknown10C;						// +0x10c
	bool mUnknown110;						// +0x110
	bool mUnknown11C;						// +0x11c
	bool mUnknown11D;						// +0x11d: true
	bool mUnknown11E;						// +0x11e
	bool mUnknown11F;						// +0x11f
	bool mUnknown120;						// +0x120: true
	bool mUnknown121;						// +0x121
	bool mUnknown122;						// +0x122
	bool mEasterEgg;						// +0x123: the level is an easter egg's
	int mEasterEggKind;						// +0x124: which (not set by the original's constructor: 0 here)
	bool mUnknown12C;						// +0x12c
	TrophyInfo* mChallenge;					// +0x128: the challenge being played (none outside the challenge mode)
	std::unique_ptr<AIMgr> mAIMgr;			// +0x134
	CharacterMgr* mCharacterMgr;			// +0x138: the app's
	std::unique_ptr<CollisionMgr> mCollisionMgr; // +0x13c
	std::unique_ptr<DebugMgr> mDebugMgr;	// +0x140
	std::unique_ptr<EffectMgr> mEffectMgr;	// +0x144
	std::unique_ptr<FloatingTextMgr> mFloatingTextMgr; // +0x148
	std::unique_ptr<InterfaceMgr> mInterfaceMgr; // +0x14c
	std::unique_ptr<LogicMgr> mLogicMgr;	// +0x154
	std::unique_ptr<SoundMgr> mSoundMgr;	// +0x158
	ReplayDialog* mReplayDialog;			// +0x15c
	EndLevelDialog* mEndLevelDialog;		// +0x160
	SlotMachineDialog* mSlotMachineDialog;	// +0x164
	ThunderButton* mMenuButton;				// +0x168: pauses the game (hidden)
	ThunderButton* mInstantReplayButton;	// +0x16c
	Image* mBackgroundImage;				// +0x170: the picture behind the level
	int mReplayCount;						// +0x184: how many replays are kept (the size of the list the original keeps from +0x17c)
	std::list<SharedPtr<PhysObj> > mObjects; // +0x190: every piece of the level, in the order they are drawn
	std::list<SharedPtr<PhysObj> > mBalls;	 // +0x19c: the balls in play
	int mOriginX;							// +0x1a8: where the level's own places start in the window (75)
	int mOriginY;							// +0x1ac: (40)
	int mUnknown1B0;						// +0x1b0: 650
	int mUnknown1B4;						// +0x1b4: 560
	int mUnknown1C0;						// +0x1c0: RemoveSlotMachineDialog sets it to 0
	int mUpdateCount;						// +0x1c4: updates of the level that were not skipped by the slow motion
	int mUnknown1D0;						// +0x1d0: updates to run on top of the next one's
	int mUnknown1CC;						// +0x1cc: counts down
	int mUnknown1C8;						// +0x1c8: counts down in the demo mode (the update stops while it does)
	int mUnknown1BC;						// +0x1bc: 0 to 10, grows while mUnknown1B8 is set and falls when not
	int mUnknown1B8;						// +0x1b8
	std::string mLevelName;					// +0x1d4: the level's path as the file is read (with "levels/" and ".dat")
};

} // namespace Sexy

#endif
