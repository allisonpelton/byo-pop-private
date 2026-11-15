#ifndef __DYNOMITEFOSSIL_H__
#define __DYNOMITEFOSSIL_H__

// The fossil challenge ("Fossil Puzzle" on the side bar; vtable 0x4f9a10, 0x328 bytes; constructor 0x408e40,
// destructor 0x409070; mode 3 of StartGame): fifteen fossils of three fragments each. A round's eggs are set out by
// its level string (SETUP, COLORS, ROWS...), the fragment sits in them as rock, and the round ends when it falls off
// the bottom; the museum then shows the fossils assembled so far. Kroney's sand pushes the eggs down a row when it
// runs out; in the panic kind (StartGame's fossil kind 1) the ceiling also comes down by itself, faster each time.
// The last fragment brings the tyrannosaurus.

#include "Dynomite.h"
#include <string>
#include <vector>

namespace Sexy
{

class Fossil;
class Kroney;
class Museum;

class DynomiteFossil : public Dynomite
{
public:
	float				mKick;				// +0x248 the ceiling's push up after a blast (3), less a twentieth a frame
	int					mFirstLevel;		// +0x24c the fossil the game started at (1)
	int					mLevel;				// +0x250 the fossil, 1-15
	int					mRound;				// +0x254 its fragment, 1-3
	int					mShownFossil;		// +0x258 the app's fossil picture showing the fragments found (-1 none)
	int					mShownChannels;		// +0x25c and which (1, 3 or 7)
	Kroney*				mKroney;			// +0x260
	int					mFirstShotFade;		// +0x264 up by 3 an update to 255 before the first shot, down after (saved; nothing reads it)
	int					mPushes;			// +0x268 frames the ceiling still comes down a pixel (33 after Kroney)
	Fossil*				mFossil;			// +0x26c
	bool				mFossilFell;		// +0x270 the fragment is off the bottom: the round's end
	int					mFallAlpha;			// +0x274 "cleared"'s alpha
	int					mFallTimer;			// +0x278 frames since "cleared" was fully in
	std::string			mBonusText;			// +0x27c "fallen fossil bonus" and the like
	bool				mResizing;			// +0x298 the walls move to the next round's width
	int					mWidth;				// +0x29c the round's width in pixels (WIDTH times 16)
	std::string			mFossilName;		// +0x2a0 gFossilNames[mLevel]
	int					mRoundCount;		// +0x2bc rounds played (1 the first)
	std::string			mRoundName;			// +0x2c0 gFossilRoundNames[mLevel]
	char				mFragmentText[28];	// +0x2dc FRAGMENT_NUM_1-3
	int					mTextAlpha;			// +0x2f8 the round's name and fragment over the board
	int					mTextTimer;			// +0x2fc frames they stay (500)
	int					mGlowAlpha;			// +0x300 the fragment's glow pulses between 128 and 255
	int					mGlowStep;			// +0x304
	int					mFossilX;			// +0x308 where the grid drew the fragment's cell this frame (-1 not)
	int					mFossilY;			// +0x30c
	Museum*				mMuseum;			// +0x310
	int					mMuseumScroll;		// +0x314 kept between visits
	int					mFossilsAssembled;	// +0x318
	bool				mPanicKind;			// +0x31c StartGame's fossil kind 1
	float				mFallSpeed;			// +0x320 the panic kind's ceiling speed (FALLSPEED)
	float				mFallSpeedUp;		// +0x324 added each time Kroney's sand runs out (FALLSPEEDUP)

	DynomiteFossil();						// 0x408e40
	virtual ~DynomiteFossil();				// 0x409070

	// 0x408bf0: the museum when it is open; else the screen shakes while a power-up moves the walls or the ceiling
	// (or the walls move to a new width), and at the game's end "game over" fades in
	virtual void		Draw(Graphics* g) override;
	// 0x40b420: the museum, the grinding walls, Kroney, the game's end, and the round: its play, its end when the
	// fragment fell, and the walls' move to the next width
	virtual void		Update() override;
	// 0x40ae70: space, escape, F1 and P pause; M opens and closes the museum
	virtual void		KeyDown(KeyCode theKey) override;
	// 0x4085d0: fires; the first shot of a game starts the music
	virtual void		MouseDown(int x, int y, int theClickCount) override;
	virtual void		GetPowerUpCenter(int* x, int* y) override;	// 0x4085a0: by Kroney
	// 0x408020: the board's, but Easy halves the points; after the fragment fell the popup stays in place longer
	virtual void		AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain) override;
	// 0x4084b0: the board's; a baby that left the board (mNextEggKind 20) makes a power-up egg
	virtual Egg*		NextEgg() override;
	// 0x40c5f0: theDifficulty is the fossil kind (1 panic); the game is always played at skill 2
	virtual void		Init(FossilizedApp* theApp, int theDifficulty) override;
	// 0x409df0: the fragment and its glow, the board's scenery, Kroney, "cleared" and the bonus, the round's name
	virtual void		DrawScenery() override;
	virtual void		Blasted() override;			// 0x407fa0: the kick
	// 0x40b8f0: the board's, with the best combo, the babies hatched, the fossils assembled and the final score
	virtual void		DrawGameOver(Graphics* g) override;
	virtual void		DrawPiece(int theColor, int x, int y) override;	// 0x407fb0: where the fragment's cell is
	virtual void		PieceFell(int theColor) override;	// 0x407fd0: the fragment off the bottom ends the round
	virtual void		Save() override;				// 0x408330 (slot 59): the board's, then the mode's
	virtual void		Load() override;				// 0x40aae0 (slot 60)

	void				SetupRound(int theLevel, int theRound);		// 0x4091f0: the width, the names
	void				SetFossil(int theLevel, int theRound);		// 0x4094b0: the fragment, where it is, its glow
	void				StartRound(int theLevel, int theRound);		// 0x40aed0: the eggs from the level string
	void				Fill(int theColors, int theRows);			// 0x408680: full rows
	void				Pipe(int theColors, int theWidth, int theRows);		// 0x4086d0: a column theWidth wide
	void				Triangle(int theColors, int theWidth, int theRows);	// 0x4087d0: widening downwards
	void				ITriangle(int theColors, int theWidth, int theRows);	// 0x4088e0: widening upwards
	void				UpdateRound();					// 0x40a770
	void				FossilFell();					// 0x40a410: "cleared", the bonus, then the museum
	void				Resizing();						// 0x40b360
	void				NextRound();					// 0x40a930: the museum's Continue
	void				ToggleMuseum();					// 0x40ad90: M and the side bar's sixth button
};

// 0x5304c4 and 0x5304b4, read by Init
extern std::vector<std::string> gFossilRoundNames;
extern std::vector<std::string> gFossilNames;

}

#endif
