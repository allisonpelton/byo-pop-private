#ifndef __PIXELUS_PUZZLECOMPLETED_H__
#define __PIXELUS_PUZZLECOMPLETED_H__

// PuzzleCompleted.cpp: the screen after a puzzle is solved (Pixelus.exe 0x41b3c0 constructor, 0x419e60 Draw, 0x419a00
// Update, 0x419bc0 Resize; vftable 0x50e5f0, shown by 0x4344d0 from the board's Update). The picture of the puzzle flies
// from the board into a stone tablet, "Well Done!" and the number of moves are written on it, and two signs held by the
// characters at the sides tell what is new: a record (the best known solution, this player's own shortest one, how far
// from it), a new best time, or a temple that has opened. The constructor is where the result is entered in the
// player's record and in the table of the best solutions.
//
// port: the particle effects around the tablet (confetti and sparkles) and the blinking eyes are not ported; the medal
// grows beside the picture at a place of the port's own choosing; the sound of the "tips" the original chooses at the
// end (0x4f8, flags f8 and fc) is reduced to the stings of a finished temple or game.

#include "Widget.h"
#include "ButtonListener.h"
#include <string>

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class MemoryImage;
class WidgetManager;

class PuzzleCompletedScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		ID_CONTINUE = 1,
		FLY_UPDATES = 80,		// the picture comes in over 1 / 0.0125 updates
		END_X = 261,			// the picture's place in the tablet
		END_Y = 105,
		END_SIZE = 117
	};

	PixelusApp*		mApp;
	DialogButton*	mContinueButton;		// +0x98
	MemoryImage*	mPicture;				// +0x9c the solved puzzle, 20 pixels a cell
	std::string		mTitle;					// +0x110 the puzzle's title

	bool			mTutorial;				// +0x7d the puzzle is one of the tutorial's
	bool			mWorldRecord;			// +0xbc as good as the best known solution
	bool			mWasMaxed;				// +0xbd the player had that already: the time counts now
	bool			mOpenedTemple;			// +0xbe a temple has opened
	int				mOpenedIndex;			// which (an index of gTemples)
	bool			mShortest;				// +0xbf the player's shortest solution
	bool			mBronze;				// +0xc0 within 10 moves of the best known
	bool			mSilver;				// +0xc1 within 5
	bool			mTempleDone;			// the last puzzle of the temple
	bool			mGameDone;				// the last puzzle of the last temple
	int				mMode;					// +0xf8: 0 none, 1 temple, 2 game, 5 tutorial (0x41b3c0 sets those)
	int				mSub;					// +0xfc: 3 the temple is maxed, 4 the game is maxed
	bool			mTutorialDone;			// +0xf8 == 5: the tutorial is finished

	int				mTime;					// +0xc4 milliseconds spent on the puzzle
	int				mMoves;					// +0xc8
	int				mPreviousMoves;			// +0xf0 the player's shortest solution before this one (0: none)
	int				mPreviousTime;			// +0xec the player's best time before (0: none)

	int				mTicks;					// +0x4c
	int				mArrivedAt;				// +0xb8 the update at which the picture reached the tablet
	float			mProgress;				// +0xb4 the picture's flight, 0 to 1
	float			mMedal;					// +0xf4 the medal's growth, 0 to 1
	float			mStartX, mStartY;		// +0xa0, +0xa4
	float			mX, mY;					// +0xa8, +0xac the picture's place now
	int				mSize;					// +0xb0 and its width

public:
	PuzzleCompletedScreen(PixelusApp* theApp, MemoryImage* thePicture, int theStartX, int theStartY, int theTime,
		int theMoves, const std::string& theSolution, const std::string& theTitle, const std::string& theUuid);
	virtual ~PuzzleCompletedScreen();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;	// 0x419b60
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x419bc0
	virtual void	Update() override;											// 0x419a00
	virtual void	Draw(Graphics* g) override;									// 0x419e60
	virtual void	ButtonDepress(int theId) override;

	// "1 minute and 12 seconds" (0x431df0 mode 4), "1m 12s" (mode 3)
	static std::string	FormatTime(int theMilliseconds, int theMode);

private:
	void			DrawCentered(Graphics* g, const std::string& theText, int theCenterX, int theY);
	void			DrawOutlined(Graphics* g, const std::string& theText, int theCenterX, int theY, const Color& theColor,
						const Color& theOutline);
};

}

#endif
