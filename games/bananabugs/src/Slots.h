// Slots: the casino's game of slots, played from the base (MetaGame) once the casino's button is out (0x4687e0..0x46c850
// in the original; 0x4e4 bytes there). A machine of three reels fills the screen. The first spin of a visit may be free
// (after the casino was unlocked); every other costs 100 bananas. The lever (the spin button) starts the reels, they
// stop one after the other on the symbols the roll gave, and three of a kind wins: the 100-banana symbol pays 100, the
// jackpot 999 (one banana a frame), the five others each unlock a bonus (flypaper, zapper, swatter, swinging monkeys,
// exploding mine). The done button leaves for the base. Nothing in it leads out of the game.
//
// The roll does not come from where the reels are: a draw of the game's own generator picks a category, and a symbol's
// category gives three of a kind only the first time (after that, each reel takes a place on a shuffled strip at
// random, by the C library's rand).
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Anim;
class MetaGame;
class TextFade;

const int SLOTS_REELS = 3;
const int SLOTS_ROWS_SHOWN = 3;              // the symbols a reel has made, one above, one at, and one below the middle
const int SLOTS_SYMBOLS = 7;                 // the cels of the symbols' picture
const int SLOTS_STRIP_ROWS = 32;             // the places on a reel's strip
const int SLOTS_CELL_HEIGHT = 0xc0;          // a symbol's height; the symbols of a reel are laid end to end
const int SLOTS_CELL_MIDDLE = 0x60;          // half of it
const int SLOTS_SHINES = 3;                  // the lights over the reels
const int SLOTS_SHUFFLED = 28;               // the length of two shuffled lists the original makes and never reads

// The symbols, by the prize of three of them
enum SlotSymbol
{
	SLOT_SYMBOL_BANANAS_100 = 0,     // pays 100 bananas
	SLOT_SYMBOL_FLY_PAPER = 1,       // unlocks the flypaper bonus
	SLOT_SYMBOL_ZAPPER = 2,          // unlocks the zapper
	SLOT_SYMBOL_SWAT = 3,            // unlocks the swatter
	SLOT_SYMBOL_TARZAN = 4,          // unlocks the swinging monkeys
	SLOT_SYMBOL_JACKPOT = 5,         // pays 999 bananas
	SLOT_SYMBOL_MINE = 6,            // unlocks the exploding mine
	SLOT_SYMBOL_NONE = -1,           // no three of a kind
};

// What ScoreRoll finds
enum SlotResult
{
	SLOT_RESULT_NOTHING = 0,
	SLOT_RESULT_UNLOCK = 1,          // a bonus unlocked
	SLOT_RESULT_BANANAS = 2,         // the 100 bananas
	SLOT_RESULT_JACKPOT = 3,         // the 999
};

// What the machine is doing (mState)
enum SlotsState
{
	SLOTS_IDLE = 0,                  // waiting for a spin
	SLOTS_ROLLING = 1,               // the reels turn and stop
	SLOTS_PAYING = 2,                // the prize is shown and the bananas counted in
	SLOTS_LEAVING = 3,               // the done button was pressed
};

// The ids of the two buttons (ObjBase::mButtonId)
enum SlotsButton
{
	SLOTS_BUTTON_SPIN = 0,
	SLOTS_BUTTON_DONE = 1,
};

class Slots : public ObjBase
{
public:
	bool mFreeSpin;                      // +0x27c: the first spin costs nothing (the player has just unlocked the casino)
	ObjBase* mDoneButton;                // +0x280
	int mLeaveDelay;                     // +0x284: frames the machine waits after the done button, then it is done
	TextFade* mMessage;                  // +0x288: the line that swells in over the machine
	TextFade* mSpareMessage;             // +0x28c: made and made a child, never used
	ObjBase* mWinBanner;                 // +0x290: "YOU WIN" over the machine, the jackpot's picture on its second cel
	int mWinSoundsLeft;                  // +0x298: plays of the win sound still to come
	int mWinSoundId;                     // +0x29c
	int mWinSoundInterval;               // +0x2a0: frames between them
	int mWinSoundTimer;                  // +0x2a4
	int mSpinAfterSounds;                // +0x2a8: the original spins again when the sounds are over if this is set; it is
	                                     // always 0 (StartWinSounds clears it and nothing sets it)
	int mSpinsSinceBigWin;               // +0x2ac: spins since the last 100 or 999 prize, or the special roll; past 14, the
	                                     // special roll gives the jackpot, not the 100
	bool mDone;                          // +0x2b0: the machine may be taken away (what MetaGame waits for)
	int mUnused2b4;                      // +0x2b4: set to 0, never read
	int mState;                          // +0x2b8: a SlotsState
	int mPayout;                         // +0x2c0: bananas still to be paid in
	int mWon;                            // +0x2c4: bananas paid in this win (counted, never read)
	MetaGame* mMetaGame;                 // +0x2c8
	ObjBase* mShines[SLOTS_SHINES];      // +0x2cc: lights over the reels, one cel of the shine picture, additive
	ObjBase* mSpinButton;                // +0x2d8: the lever (a guess: the picture is called "spin")
	ObjBase* mSpinRing;                  // +0x2e0: a ring behind it that turns while the reels do (a guess)
	ObjBase* mSpinGlow;                  // +0x2e4: a glow that breathes behind the lever and swells when the mouse is on it (a guess)
	ObjBase* mUnlockBubble;              // +0x2e8: the unlocked bonus's picture, flying up from the machine
	ObjBase* mSymbols[SLOTS_REELS][SLOTS_ROWS_SHOWN]; // +0x2ec: the three symbols a reel shows, top first
	ObjBase* mReelWindow;                // +0x310: what the symbols are children of
	int mStripLength;                    // +0x314: 7 symbols high
	int mPosition[SLOTS_REELS];          // +0x318: how far down the strip each reel has turned
	int mSpinsLeft[SLOTS_REELS];         // +0x324: whole turns still to make before the reel looks for its place
	int mTarget[SLOTS_REELS];            // +0x330: the position the reel stops at
	int mResult[SLOTS_REELS];            // +0x33c: the symbol it shows there
	int mStopped[SLOTS_REELS];           // +0x348: 1 once the reel has stopped
	int mSymbolCount[SLOTS_SYMBOLS];     // +0x354: how many of the reels show each symbol (ScoreRoll)
	bool mRolled[SLOTS_SYMBOLS];         // +0x374: the symbol's category has been rolled once (index 0 and 5 are never set)
	int mUnusedOrder[2][SLOTS_SHUFFLED]; // +0x388 and +0x3f8: two lists of the numbers 0 to 27 the original shuffles and
	                                     // never reads (kept, as the shuffles draw numbers)

	// The pictures (globals in the original; loaded by PreloadSlots)
	static Anim* sMachineAnim;           // 0x670210: "images/slot/slot_machine"
	static Anim* sSymbolsAnim;           // 0x670214: "images/slot/slot_symbols", seven cels
	static Anim* sWinAnim;               // 0x670218: "images/slot/slot_win", two cels
	static Anim* sSpinAnim;              // 0x67021c: "images/slot/slot_spin", four cels
	static Anim* sNumbersAnim;           // 0x670220: "images/slot/slot_numbers", ten cels (nothing uses it)
	static Anim* sShineAnim;             // 0x670224: "images/slot/slot_shine", four cels

	// The reels' strips, one column for each reel: 32 symbols, each reel's share of each as the table of the symbols'
	// counts gives, shuffled by the constructor (0x600068)
	static int sReelStrip[SLOTS_STRIP_ROWS][SLOTS_REELS];

	explicit Slots(MetaGame* theMetaGame);   // 0x4687e0
	// 0x468ae0 deletes it (the body 0x468b00 only calls ObjBase's)
	void Notify(ObjBase* theObj) override;   // 0x468b80
	bool Update() override;                  // 0x468c10
	bool MouseUp(ObjBase* theObj) override;  // 0x4699b0

	void Start();                            // 0x468b10
	bool IsDone() const { return mDone; }    // 0x469a30

private:
	void StartWinSounds(int theCount);       // 0x4693b0 (EAX this, EDX the count)
	int ScoreRoll(int* thePayout, int* theSymbol); // 0x4693e0 (EAX this): what the reels show is worth
	void Spin();                             // 0x4694d0 (ESI this): pays and rolls, then starts the reels
	void RollForced(int theSymbol);          // 0x4694d0, a case of its switch (five of them, one for each symbol that has one)
	void RollRandom();                       // 0x4694d0's default case
	void ShowBananas();                      // 0x469a40 (EAX this): the base shows the bananas the player has
	void UpdateShines();                     // 0x469a50 (ESI this): the lights' cels follow the bananas left to pay
	void PlaceReel(int thePosition, int theReel); // 0x46a3c0 (ECX the position, EDI the reel, the machine pushed)
	void ShowMessage(const char* theText, float theRed, float theGreen, float theBlue); // 0x46f740 as the machine calls it
	void BuildCabinet();                     // 0x469bd0: the machine's own picture and tracks, and the lights
	void BuildReels();                       // 0x46a230
	void BuildSpinButton();                  // 0x46a490
	void BuildSpinRing();                    // 0x46aa40
	void BuildSpinGlow();                    // 0x46adb0
	void BuildWinBanner();                   // 0x46b2a0
	void BuildUnlockBubble();                // 0x46b7d0
	void BuildDoneButton();                  // 0x46bc80
};

void PreloadSlots();                         // 0x46c710: the machine's pictures, loaded once

} // namespace Sexy
