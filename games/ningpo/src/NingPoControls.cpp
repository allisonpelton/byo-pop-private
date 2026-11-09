// The controls of the game screen (0x406cd0, the subclasses' functions 0x407660..0x407bb0): the buttons down the left
// side, the arrows of the title and the end screens, and the panel's labels. Each button is a PGControl subclass
// with its own vtable; the original makes one class for each arrow, which the port folds into a class with the step.

#include "NingPo.h"
#include "PGControl.h"

namespace ningpo
{

using pg::EV_MOUSE_MOVE;

namespace
{

// vtable 0x462590: a label of the panel (time, tiles left, moves), drawn while a game is up
class PanelLabel : public PGControl
{
public:
	// 0x407660
	void draw(const PGRect* theClip) override
	{
		NingPo* anApp = GetNingPo();
		if (anApp->mState != STATE_TITLE && anApp->mState != STATE_EXIT && !anApp->mPaused)
			PGControl::draw(theClip);
	}
};

// vtable 0x462b68
class ExitButton : public PGControl
{
public:
	// 0x4076c0
	bool handleEvent(const PGEvent* theEvent) override
	{
		PGControl::handleEvent(theEvent);
		if (isClick(theEvent))
		{
			NingPo* anApp = GetNingPo();
			anApp->setPaused(false);
			anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
			anApp->onExit();
			return true;
		}
		return false;
	}
};

// vtable 0x462ab8
class ShowMovesButton : public PGControl
{
public:
	// 0x407720
	bool handleEvent(const PGEvent* theEvent) override
	{
		if (isClick(theEvent))
		{
			GetNingPo()->showMoves();
			return true;
		}
		PGControl::handleEvent(theEvent);
		return false;
	}
};

// vtable 0x462a60
class HintButton : public PGControl
{
public:
	// 0x407760
	bool handleEvent(const PGEvent* theEvent) override
	{
		if (isClick(theEvent))
			GetNingPo()->hint(true);
		PGControl::handleEvent(theEvent);
		return false;
	}
};

// vtable 0x462a08 (0x40 bytes): the pause button, with a pair of sprites for each side
class PauseButton : public PGControl
{
public:
	PGSprite* mPauseNormal = nullptr;   // +0x30
	PGSprite* mPauseLit = nullptr;      // +0x34
	PGSprite* mResumeNormal = nullptr;  // +0x38
	PGSprite* mResumeLit = nullptr;     // +0x3c

	// 0x4077f0
	void draw(const PGRect* theClip) override
	{
		if (!GetNingPo()->mPaused)
			setSprites(mPauseNormal, mPauseLit, nullptr);
		else
			setSprites(mResumeNormal, mResumeLit, nullptr);
		PGControl::draw(theClip);
	}

	// 0x407790
	bool handleEvent(const PGEvent* theEvent) override
	{
		if (isClick(theEvent))
		{
			NingPo* anApp = GetNingPo();
			anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
			anApp->setPaused(!anApp->mPaused);
			return true;
		}
		PGControl::handleEvent(theEvent);
		return false;
	}
};

// vtable 0x462b10
class OptionsButton : public PGControl
{
public:
	// 0x407830
	bool handleEvent(const PGEvent* theEvent) override
	{
		PGControl::handleEvent(theEvent);
		if (isClick(theEvent))
		{
			NingPo* anApp = GetNingPo();
			anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
			anApp->showOptions();
			return true;
		}
		return false;
	}
};

// vtable 0x4629b0
class NewGameButton : public PGControl
{
public:
	// 0x407880
	bool handleEvent(const PGEvent* theEvent) override
	{
		if (isClick(theEvent))
		{
			NingPo* anApp = GetNingPo();
			anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
			anApp->toTitle(true);
		}
		PGControl::handleEvent(theEvent);
		return false;
	}
};

// vtable 0x462958
class RulesButton : public PGControl
{
public:
	// 0x4078d0
	bool handleEvent(const PGEvent* theEvent) override
	{
		if (isClick(theEvent))
		{
			NingPo* anApp = GetNingPo();
			anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
			anApp->showRules();
		}
		PGControl::handleEvent(theEvent);
		return false;
	}
};

// vtable 0x462900
class ShuffleButton : public PGControl
{
public:
	// 0x407920
	bool handleEvent(const PGEvent* theEvent) override
	{
		if (isClick(theEvent))
			GetNingPo()->shuffle();
		PGControl::handleEvent(theEvent);
		return false;
	}
};

// vtable 0x4628a8
class UndoButton : public PGControl
{
public:
	// 0x407950
	bool handleEvent(const PGEvent* theEvent) override
	{
		NingPo* anApp = GetNingPo();
		if (isClick(theEvent) && !anApp->mPaused)
		{
			anApp->playSound("nm_hint.ogg", 0x80, 0, true);
			anApp->undo();
		}
		PGControl::handleEvent(theEvent);
		return false;
	}
};

// (0x34 bytes) an arrow shown in one state of the game, the base of vtables 0x4625e8..0x462850
class StateArrow : public PGControl
{
public:
	int mShownIn;  // +0x30 the state it is shown in

	explicit StateArrow(int theShownIn) : mShownIn(theShownIn) {}

	// 0x4079a0
	void draw(const PGRect* theClip) override
	{
		NingPo* anApp = GetNingPo();
		if (anApp->mState == mShownIn && !anApp->mPaused)
			PGControl::draw(theClip);
	}

	// 0x4079c0
	bool handleEvent(const PGEvent* theEvent) override
	{
		PGControl::handleEvent(theEvent);
		NingPo* anApp = GetNingPo();
		if (isClick(theEvent) && anApp->mState == mShownIn && !anApp->mPaused)
		{
			onClick();
			return true;
		}
		return false;
	}
};

// vtable 0x462850: the title's start arrow
class StartArrow : public StateArrow
{
public:
	StartArrow() : StateArrow(STATE_TITLE) {}

	// 0x407a50
	void onClick() override
	{
		NingPo* anApp = GetNingPo();
		anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
		anApp->startGame();
	}
};

// vtable 0x4627f8: the end screens' arrow back to the title, past the best times
class ContinueArrow : public StateArrow
{
public:
	explicit ContinueArrow(int theShownIn) : StateArrow(theShownIn) {}

	// 0x407a10
	void onClick() override
	{
		NingPo* anApp = GetNingPo();
		anApp->playSound("nm_guiclick.wav", 0x80, 0, true);
		anApp->recordTime();
		anApp->toTitle(false);
	}
};

// the title's arrows that cycle a choice: the background (vtables 0x4627a0 back, 0x462748 on; 0x407ac0, 0x407af0),
// the tile set (0x4626f0, 0x462698; 0x407b20, 0x407b50) and the layout (0x462640, 0x4625e8; 0x407b80, 0x407bb0)
class CycleArrow : public StateArrow
{
public:
	void (NingPo::*mCycle)(int);
	int mStep;

	// 0x407ac0, 0x407af0, 0x407b20, 0x407b50, 0x407b80, 0x407bb0 are the six arrows' click handlers (onClick below)
	CycleArrow(void (NingPo::*theCycle)(int), int theStep) : StateArrow(STATE_TITLE), mCycle(theCycle), mStep(theStep) {}

	void onClick() override
	{
		NingPo* anApp = GetNingPo();
		anApp->playSound("nm_click.wav", 0x80, 0, true);
		(anApp->*mCycle)(mStep);
	}
};

} // namespace

// 0x406cd0: the "no match" sign's rectangle in the middle of the board, then the controls
void NingPo::createControls()
{
	PGSprite* aSign = &mSprites[SPRITE_NOMATCH];
	mNoMatchRect.left = ((mBoardRect.right - mBoardRect.left) - aSign->width()) / 2 + 0x7d;
	mNoMatchRect.right = aSign->width() + mNoMatchRect.left;
	mNoMatchRect.top = ((mBoardRect.bottom - mBoardRect.top) - aSign->height()) / 2 - 0x32;
	mNoMatchRect.bottom = aSign->height() + mNoMatchRect.top;

	// the trial added its buy button here (vtable 0x462bc0, at (0xb, 0x89), sprites 0x4a and 0x4b)

	PGPoint aPos;
	PGControl* aControl = new ExitButton();
	aPos = {0x23, 0x1bf};
	aControl->setPosition(&aPos);
	aControl->setSprites(getSprite(0x4c), getSprite(0x4d), nullptr);
	addControl(aControl);

	aControl = new OptionsButton();
	aPos = {0x11, 0xaf};
	aControl->setPosition(&aPos);
	aControl->setSprites(getSprite(0x54), getSprite(0x55), nullptr);
	addControl(aControl);

	aControl = new ShowMovesButton();
	aPos = {0, 0x124};
	aControl->setPosition(&aPos);
	aControl->setSprites(getSprite(0x4e), getSprite(0x4f), nullptr);
	addControl(aControl);
	aControl->setEnabled(false);
	mShowMovesButton = aControl;

	aControl = new HintButton();
	aControl->setSprites(getSprite(0x50), getSprite(0x51), nullptr);
	aPos = {6, 0x101};
	aControl->setPosition(&aPos);
	mHintButton = aControl;
	addControl(aControl);

	PauseButton* aPause = new PauseButton();
	aPause->mPauseNormal = getSprite(0x56);
	aPause->mPauseLit = getSprite(0x57);
	aPause->mResumeNormal = getSprite(0x58);
	aPause->mResumeLit = getSprite(0x59);
	aPos = {0x10, 0x147};
	aPause->setPosition(&aPos);
	mPauseButton = aPause;
	addControl(aPause);

	aControl = new NewGameButton();
	aControl->setSprites(getSprite(0x5c), getSprite(0x5d), nullptr);
	aPos = {1, 0x69};
	aControl->setPosition(&aPos);
	mNewGameButton = aControl;
	addControl(aControl);

	aControl = new RulesButton();
	aControl->setSprites(getSprite(0x5e), getSprite(0x5f), nullptr);
	aPos = {0x1b, 0xd2};
	aControl->setPosition(&aPos);
	addControl(aControl);

	mShuffleButton = new ShuffleButton();
	mShuffleButton->setSprites(getSprite(0x60), getSprite(0x61), nullptr);
	aPos = {0xc, 0x18d};
	mShuffleButton->setPosition(&aPos);
	addControl(mShuffleButton);

	aControl = new UndoButton();
	aControl->setSprites(getSprite(0x62), getSprite(0x63), nullptr);
	aPos = {0x23, 0x16a};
	aControl->setPosition(&aPos);
	addControl(aControl);
	aControl->setEnabled(false);
	mUndoButton = aControl;

	aControl = new StartArrow();
	aControl->setSprites(getSprite(0x47), getSprite(0x48), nullptr);
	aPos = {0x144, 0x1ad};
	aControl->setPosition(&aPos);
	addControl(aControl);

	// one for each end screen, lit only
	aControl = new ContinueArrow(STATE_GAME_OVER);
	aControl->setSprites(nullptr, getSprite(0x49), nullptr);
	aPos = {0xf8, 0x14e};
	aControl->setPosition(&aPos);
	addControl(aControl);

	aControl = new ContinueArrow(STATE_WON);
	aControl->setSprites(nullptr, getSprite(0x49), nullptr);
	aPos = {0xf8, 0x14e};
	aControl->setPosition(&aPos);
	addControl(aControl);

	aControl = new CycleArrow(&NingPo::cycleBackground, -1);
	aControl->setSprites(getSprite(SPRITE_ARROW_LEFT), getSprite(SPRITE_ARROW_LEFT_LIT), nullptr);
	aPos = {0xca, 0x57};
	aControl->setPosition(&aPos);
	addControl(aControl);

	aControl = new CycleArrow(&NingPo::cycleBackground, 1);
	aControl->setSprites(getSprite(SPRITE_ARROW_RIGHT), getSprite(SPRITE_ARROW_RIGHT_LIT), nullptr);
	aPos = {0xee, 0x57};
	aControl->setPosition(&aPos);
	addControl(aControl);

	aControl = new CycleArrow(&NingPo::cycleTileset, -1);
	aControl->setSprites(getSprite(SPRITE_ARROW_LEFT), getSprite(SPRITE_ARROW_LEFT_LIT), nullptr);
	aPos = {0x1f2, 0x56};
	aControl->setPosition(&aPos);
	addControl(aControl);

	aControl = new CycleArrow(&NingPo::cycleTileset, 1);
	aControl->setSprites(getSprite(SPRITE_ARROW_RIGHT), getSprite(SPRITE_ARROW_RIGHT_LIT), nullptr);
	aPos = {0x215, 0x56};
	aControl->setPosition(&aPos);
	addControl(aControl);

	aControl = new CycleArrow(&NingPo::cycleLayout, -1);
	aControl->setSprites(getSprite(SPRITE_ARROW_LEFT), getSprite(SPRITE_ARROW_LEFT_LIT), nullptr);
	aPos = {0x104, 0x97};
	aControl->setPosition(&aPos);
	addControl(aControl);

	aControl = new CycleArrow(&NingPo::cycleLayout, 1);
	aControl->setSprites(getSprite(SPRITE_ARROW_RIGHT), getSprite(SPRITE_ARROW_RIGHT_LIT), nullptr);
	aPos = {0x1da, 0x97};
	aControl->setPosition(&aPos);
	addControl(aControl);

	static const int LABELS[3][3] = {
		{SPRITE_LABEL_TIME, 0x86, 0x1bc}, {SPRITE_LABEL_MOVES, 0x12f, 0x1bc}, {SPRITE_LABEL_TILES_LEFT, 0x1cc, 0x1bc}};
	for (int i = 0; i < 3; i++)
	{
		aControl = new PanelLabel();
		aControl->setShownUnderDialogs(false);
		aControl->setSprites(getSprite(LABELS[i][0]), getSprite(LABELS[i][0]), nullptr);
		aPos = {LABELS[i][1], LABELS[i][2]};
		aControl->setPosition(&aPos);
		addControl(aControl);
	}
}

} // namespace ningpo
