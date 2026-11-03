#ifndef __SEVENSEAS_SCREENS_H__
#define __SEVENSEAS_SCREENS_H__

// The full-screen widgets of Seven Seas Deluxe: TitleScreen (WinSS.exe 0x4256f0), IntroWidget (the menu, 0x416900),
// MapWidget (0x4175d0), TalkWidget (the pirate's head, 0x4199f0) and HighScoreWidget (the game over screen,
// 0x40ff70). Behaviour spec: docs/GAME_NOTES.md section 4.
// Each is a top-level widget (the 2003 framework has no child widgets); the buttons they make are top-level too.

#include "Widget.h"
#include "ButtonListener.h"
#include "HyperlinkWidget.h"
#include "ButtonWidget.h"
#include "Button2003.h"
#include <string>
#include <vector>

namespace Sexy
{

class SevenSeasApp;

// 0x4256f0: the title picture, a bar that fills as the sounds load, and the link that starts the game
class TitleScreen : public Widget, public ButtonListener
{
public:
	SevenSeasApp*		mApp;                   // +0x5c
	double				mProgress;              // +0x60 the loading bar's fill, 0..1
	double				mDelayProgress;         // +0x68
	bool				mPlayShown;             // +0x70 the play link has appeared
	int					mPlaysLeft;             // +0x74
	bool				mRegisterClicked;       // +0x78
	bool				mDelay;                 // +0x79
	int					mDivisor;               // +0x7c
	int					mSparkles[25][3];       // +0x80: x, y, frame
	HyperlinkWidget*	mRegisterLink;          // +0x1ac id 0
	HyperlinkWidget*	mPlayLink;              // +0x1b0 id 1

public:
	TitleScreen(SevenSeasApp* theApp);
	virtual ~TitleScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x416900: the menu (the difficulty wheels, Play and Tutorial)
class IntroWidget : public Widget, public ButtonListener
{
public:
	SevenSeasApp*		mApp;                   // +0x5c
	int					mDifficulty;            // +0x60 1..3
	bool				mStarting;              // +0x64 Play or Tutorial was clicked
	ButtonWidget*		mPlayButton;            // +0x68 id 0
	ButtonWidget*		mTutorialButton;        // +0x6c id 1
	ButtonWidget*		mWheels[3];             // +0x70 ids 0xb, 0xc, 0xd

public:
	IntroWidget(SevenSeasApp* theApp);
	virtual ~IntroWidget();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		MouseMove(int x, int y) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;

	void				SetDifficulty(int theDifficulty);       // 0x4171c0
};

// 0x4175d0: the map that comes up every ten levels
class MapWidget : public Widget, public ButtonListener
{
public:
	SevenSeasApp*		mApp;                   // +0x5c
	DialogButton*		mNextButton;            // +0x60 id 100
	int					mChapter;               // +0x64 level / 10
	int					mDots;                  // +0x68 how many of the dots are lit

public:
	MapWidget(SevenSeasApp* theApp);
	virtual ~MapWidget();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		MouseMove(int x, int y) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x41b560: the panel at the left (the score, the lives, the level, the difficulty and the skull whose eyes follow
// the mouse)
class SidePanel : public Widget
{
public:
	SevenSeasApp*		mApp;                   // +0x58
	int					mShownScore;            // +0x5c counts up to the board's score
	int					mMouseX;                // +0x60 the last mouse position over the panel
	int					mMouseY;                // +0x64
	int					mPupilX;                // +0x68
	int					mPupilY;                // +0x6c
	int					mTargetX;               // +0x70
	int					mTargetY;               // +0x74

public:
	SidePanel(SevenSeasApp* theApp);

	virtual void		Update() override;
	virtual void		MouseMove(int x, int y) override;
	virtual void		Draw(Graphics* g) override;
};

// 0x4199f0: the pirate's head at the left of the screen; it talks while a sample plays
class TalkWidget : public Widget
{
public:
	SevenSeasApp*		mApp;                   // +0x58
	double				mProgress;              // +0x60 fade 0..1
	int					mMode;                  // +0x68 1 appearing, -1 disappearing, 0 still
	double				mBobX;                  // +0x70
	double				mBobY;                  // +0x78
	bool				mTalkingNow;            // +0x80 the mouth moves
	int					mSet;                   // +0x84 which line is spoken
	int					mStep;                  // +0x88
	int					mStepTime;              // +0x8c
	bool				mBlink;                 // +0x90
	int					mBlinkTick;             // +0x94

public:
	TalkWidget(SevenSeasApp* theApp);

	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		MouseMove(int x, int y) override;
	using Widget::MouseMove;

	void				SetMouth(int theSet);               // 0x419a70
};

// 0x40ff70: the end of a game: the totals, the rank and the best scores
class HighScoreWidget : public Widget, public ButtonListener
{
public:
	SevenSeasApp*		mApp;                   // +0x5c
	DialogButton*		mNewGameButton;         // +0x64 id 1 (the original's +0x68, "Submit to Internet High Score List", is not ported)
	std::string			mTodayName;             // +0x84 the best score of the day
	int					mTodayScore;            // +0x94
	std::string			mNames[5];              // +0x98 the five best
	int					mScores[5];             // +0xe8
	int					mShimmer;               // +0x7c counts up every ten updates; moves the colours of the list
	int					mReveal;                // +0x60 how much of the screen is shown: it grows by 6 an update

public:
	HighScoreWidget(SevenSeasApp* theApp);
	virtual ~HighScoreWidget();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		MouseMove(int x, int y) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;

	void				LoadScores();                       // 0x410550
	void				SaveScores();                       // 0x410870
	void				ShowScoreNote();                    // 0x410ae0
	int					PlaceFor(int theScore);             // 0x411160
	void				AddScore(const SexyString& theName);    // 0x412cc0
	static const char*	RankName(int theScore);
};

}

#endif
