// MainMenu.cpp: see MainMenu.h. The original keeps its tweakable numbers in ModVal calls (each returns the number it was compiled
// with); they are named constants here, each with the line of MainMenu.cpp its call names. Left out because the numbers
// that gate them are 0 or the trial of the original is not in this port: the blinking of the Adventure button for a profile that has
// not played (line 516, 0 turns it off), the "Buy" button and everything that continues a registration (lines 480 to 482 and
// 605), and the trial's tests in ButtonDepress and StartGame.

#include "MainMenu.h"

#include <cmath>

#include "Color.h"
#include "Dialog.h"
#include "Font.h"
#include "Graphics.h"
#include "HyperlinkWidget.h"
#include "NotTranslated.h"
#include "PlayerInfo.h"
#include "Res.h"
#include "SexyAppBase.h"
#include "SexyMatrix.h"
#include "ThunderButton.h"
#include "ThunderCommon.h"
#include "ThunderDialog.h"
#include "ThunderballApp.h"
#include "WidgetManager.h"

using namespace Sexy;

// the buttons: where each goes (lines 495 to 514, 581 to 600), and the notes their hover sounds are raised to (lines 469 to 479)
static const int ADVENTURE_X = 588;
static const int ADVENTURE_Y = 27;
static const int SOLO_X = 577;
static const int SOLO_Y = 172;
static const int CHALLENGE_X = 588;
static const int CHALLENGE_Y = 456;
static const int DUEL_X = 577;
static const int DUEL_Y = 314;
static const int QUIT_X = 7;
static const int QUIT_Y = 542;
static const int DEMO_X = 108;
static const int REPLAYS_X = 417;
static const int OPTIONS_X = 264;
static const int BOTTOM_BUTTONS_Y = 554;
static const int NOT_YOU_LINK_X = 315;			// the link goes here after the constructor; the constructor first places it by its width
static const int NOT_YOU_LINK_Y = 136;
static const int NOT_YOU_LINK_CENTER_X = 290;	// line 489
static const int NOT_YOU_LINK_TOP = 29;			// line 490
static const int NOT_YOU_LINK_EXTRA_HEIGHT = 8;	// line 491
static const int NOT_YOU_LINK_COLOR = 100;		// line 484
static const int NOT_YOU_LINK_OVER_COLOR = 0xff;	// line 485
static const int NOT_YOU_LINK_UNDERLINE_SIZE = 0;	// line 487
static const int NOT_YOU_LINK_UNDERLINE_OFFSET = 1;	// line 488
static const char* const NOT_YOU_LINK_TEXT = "If this is not you, click here."; // line 486

// the hover sounds go up the notes of a major scale, a button each (the pictures' buttons are the notes of the hidden tune)
static const int ADVENTURE_PITCH = 0;
static const int SOLO_PITCH = 2;
static const int CHALLENGE_PITCH = 5;
static const int DUEL_PITCH = 4;
static const int REPLAYS_PITCH = 7;
static const int OPTIONS_PITCH = 9;
static const int DEMO_PITCH = 11;
static const int QUIT_PITCH = 12;

static const int BUBBLE_FADE_UPDATES = 20;		// DAT_00649f6c: the updates a bubble takes to change
static const int BUBBLE_DELAY = 15;				// the updates the mouse stays on a button before the bubble changes
static const int TROPHY_BUBBLE_DELAY = 50;		// line 602
static const int MOUSE_IGNORE_UPDATES = 10;		// line 515: after the menu is sized, mouse moves count only after this many updates
static const int QUIT_DARKEN_MAX = 15;			// line 577
static const int TROPHY_ANIM_LENGTH = 400;		// line 606
static const int TROPHY_AWARD_SOUND_TIME = 160;	// line 603
static const int TROPHY_APPLAUSE_SOUND_TIME = 10;	// line 604
static const int TROPHY_HOVER_X = 330;			// the place of the trophy the mouse must be over for its bubble
static const int TROPHY_HOVER_Y = 200;
static const int TROPHY_HOVER_WIDTH = 160;
static const int TROPHY_HOVER_HEIGHT = 225;
static const int NUM_EASTER_EGG_BUTTONS = 15;

// the hidden tune: the buttons the mouse goes over, in this order, open the story screen. They are the notes of "Ode to Joy" by
// their hover sounds (the table at 0x64a044).
static const int EASTER_EGG_SEQUENCE[NUM_EASTER_EGG_BUTTONS] = {2, 2, 3, 4, 4, 3, 2, 1, 0, 0, 1, 2, 2, 1, 1};

// the picture (lines 518 to 553)
static const int BUBBLE_X = 275;				// 518
static const int BUBBLE_Y = 56;					// 519
static const int BUBBLE_TEXT_LEFT = 10;			// 524: the text's area, from the bubble's corner
static const int BUBBLE_TEXT_TOP = 20;			// 525
static const int BUBBLE_TEXT_WIDTH = 235;		// 526
static const int BUBBLE_TEXT_HEIGHT = 80;		// 527
static const int BUBBLE_CENTER_Y_OFFSET = 10;	// the centre of the picture of the bubble is this far below the middle of its area
static const float BUBBLE_SCALE = 1.0f;			// 520: how big the bubble is at rest
static const float BUBBLE_SCALE_DIP_BASE = 10.0f;	// 522, 523: the bubble shrinks and grows back while it changes
static const float BUBBLE_SCALE_DIP = 0.0025f;	// 521: by this much for each update from the start or end of the change, to 10 updates
static const int BUBBLE_LINE_SPACING = 20;
static const int GREETING_LINE_OFFSET_Y = -10;	// 546, 549
static const int GREETING_NAME_AREA_WIDTH = 250;	// 550: the name is centred in this width from the bubble's left edge
static const int GREETING_COLOR = 0x86065d;		// 547, 551
static const int BUBBLE_TEXT_COLOR = 100;		// 529
static const int CRY_X = 194;					// 552
static const int CRY_Y = 178;					// 553
static const int MENU_X = 15;					// 554, 556
static const int MENU_Y = 11;					// 557
static const int SUN_GLOW_X = -170;				// 558
static const int SUN_GLOW_Y = -190;				// 559
static const float SUN_GLOW_DEGREES_PER_UPDATE = 0.05f;	// 560
static const int SUN_GLOW_CENTER_X = 394;		// 561: the glow turns about this point of its picture
static const int SUN_GLOW_CENTER_Y = 383;		// 562
static const int BLANK_SUN_X = 319;				// 563: from the glow's corner
static const int BLANK_SUN_Y = 307;				// 564
static const int FOREGROUND_X = 0;				// 565: from MENU_X
static const int FOREGROUND_BOTTOM = 60;		// 566
static const int MASCOT_X = 15;					// 567
static const int MASCOT_Y = 130;				// 568
static const int TROPHY_X = 312;				// 569, 572
static const int TROPHY_Y = 181;				// 570, 573
static const float TROPHY_RISE_SPEED = 0.75f;	// 571: pixels the trophy rises in an update
static const int TOP_X = 46;					// 574
static const int TOP_Y = 0;						// 575
static const int BOTTOM_X = 64;					// 576
static const int NUM_TROPHY_RANKS = 3;
static const double PI_OF_THE_ORIGINAL = 3.1415927;	// the constant the original turns degrees into radians with

// 0x4a79e0
MainMenu::MainMenu(ThunderballApp* theApp)
{
	mApp = theApp;
	mRandomLevel = false;
	mMouseIgnoreCountdown = 0;
	mMouseMoved = false;
	mFrame = 0;
	mEasterEggProgress = 0;

	// the pictures of the four main buttons hold their lit and pressed looks; one picture does for both (line 468 and the others)
	mAdventureButton = MakeEmbeddedButton(MAIN_MENU_BUTTON_ADVENTURE, this, IMAGE_MM_ADVENTURE, true);
	mAdventureButton->mHoverSound = SOUND_MOUSEOVER;
	mAdventureButton->mHoverSoundPitch = (float)ADVENTURE_PITCH;
	mSoloButton = MakeEmbeddedButton(MAIN_MENU_BUTTON_SOLO, this, IMAGE_MM_SOLO, true);
	mSoloButton->mHoverSound = SOUND_MOUSEOVER;
	mSoloButton->mHoverSoundPitch = (float)SOLO_PITCH;
	mChallengeButton = MakeEmbeddedButton(MAIN_MENU_BUTTON_CHALLENGE, this, IMAGE_MM_CHALLENGE, true);
	mChallengeButton->mHoverSound = SOUND_MOUSEOVER;
	mChallengeButton->mHoverSoundPitch = (float)CHALLENGE_PITCH;
	mDuelButton = MakeEmbeddedButton(MAIN_MENU_BUTTON_DUEL, this, IMAGE_MM_DUEL, true);
	mDuelButton->mHoverSound = SOUND_MOUSEOVER;
	mDuelButton->mHoverSoundPitch = (float)DUEL_PITCH;
	mReplaysButton = MakeButton(MAIN_MENU_BUTTON_REPLAYS, this, "Replays", IMAGE_DLG_BROWNBUTTON1);
	mReplaysButton->mHoverSound = SOUND_MOUSEOVER;
	mReplaysButton->mHoverSoundPitch = (float)REPLAYS_PITCH;
	mOptionsButton = MakeButton(MAIN_MENU_BUTTON_OPTIONS, this, "Options", IMAGE_DLG_BROWNBUTTON1);
	mOptionsButton->mHoverSound = SOUND_MOUSEOVER;
	mOptionsButton->mHoverSoundPitch = (float)OPTIONS_PITCH;
	mDemoButton = MakeButton(MAIN_MENU_BUTTON_DEMO, this, "Demo", IMAGE_DLG_BROWNBUTTON1);
	mDemoButton->mHoverSound = SOUND_MOUSEOVER;
	mDemoButton->mHoverSoundPitch = (float)DEMO_PITCH;
	mQuitButton = MakePurpleButton(MAIN_MENU_BUTTON_QUIT, this, "Quit");
	mQuitButton->mHoverSound = SOUND_MOUSEOVER;
	mQuitButton->mHoverSoundPitch = (float)QUIT_PITCH;

	// the link that opens the list of players, under the bubble's greeting
	mNotYouLink = new HyperlinkWidget(MAIN_MENU_BUTTON_NOT_YOU, this);
	mNotYouLink->SetFont(FONT_PLAIN2);
	mNotYouLink->mColor = Color(NOT_YOU_LINK_COLOR);
	mNotYouLink->mOverColor = Color(NOT_YOU_LINK_OVER_COLOR);
	mNotYouLink->mDoFinger = true;
	mNotYouLink->mLabel = NOT_YOU_LINK_TEXT;
	mNotYouLink->mUnderlineSize = NOT_YOU_LINK_UNDERLINE_SIZE;
	mNotYouLink->mUnderlineOffset = NOT_YOU_LINK_UNDERLINE_OFFSET;
	int aLinkWidth = mNotYouLink->mFont->StringWidth(mNotYouLink->mLabel);
	mNotYouLink->Resize(NOT_YOU_LINK_CENTER_X - aLinkWidth / 2, NOT_YOU_LINK_TOP, aLinkWidth,
						NOT_YOU_LINK_EXTRA_HEIGHT + mNotYouLink->mFont->GetHeight());
	mNotYouLink->SetVisible(false);

	AddWidget(mAdventureButton);
	AddWidget(mSoloButton);
	AddWidget(mDuelButton);
	AddWidget(mChallengeButton);
	AddWidget(mReplaysButton);
	AddWidget(mOptionsButton);
	AddWidget(mDemoButton);
	AddWidget(mQuitButton);
	AddWidget(mNotYouLink);

	mApp->PlayMusic(MUSIC_MENU, true);

	mCurBubble = BUBBLE_GREETING;
	mBubbleFade = 0;
	mPrevBubble = BUBBLE_GREETING;
	mBubbleHoverCount = 0;
	mTrophyRank = 0;
	mStartLevel = 0;
	mStartStage = 0;
	mMascotCrying = false;
	mQuitDarken = 0;
	mTrophyAnimLength = 0;
	mTrophyAnimTime = 0;

	PlaceButtons();
	SyncPlayerInfo();
}

MainMenu::~MainMenu()
{
}

// The place of every button, set in the constructor and again in every update (lines 495 to 514 and 581 to 600).
void MainMenu::PlaceButtons()
{
	mAdventureButton->Resize(ADVENTURE_X, ADVENTURE_Y, mAdventureButton->mWidth, mAdventureButton->mHeight);
	mSoloButton->Resize(SOLO_X, SOLO_Y, mSoloButton->mWidth, mSoloButton->mHeight);
	mChallengeButton->Resize(CHALLENGE_X, CHALLENGE_Y, mChallengeButton->mWidth, mChallengeButton->mHeight);
	mDuelButton->Resize(DUEL_X, DUEL_Y, mDuelButton->mWidth, mDuelButton->mHeight);
	mQuitButton->Move(QUIT_X, QUIT_Y);
	mDemoButton->Move(DEMO_X, BOTTOM_BUTTONS_Y);
	mOptionsButton->Move(OPTIONS_X, BOTTOM_BUTTONS_Y);
	mReplaysButton->Move(REPLAYS_X, BOTTOM_BUTTONS_Y);
	mNotYouLink->Move(NOT_YOU_LINK_X, NOT_YOU_LINK_Y);
}

// 0x494a00
void MainMenu::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mMouseIgnoreCountdown = MOUSE_IGNORE_UPDATES;
	mMouseMoved = false;
}

// 0x494a40
void MainMenu::SetCurBubble(int theBubble, int theDelay)
{
	if (mCurBubble != theBubble)
	{
		int aHoverCount = mBubbleHoverCount++;
		if (aHoverCount >= theDelay)
		{
			mPrevBubble = mCurBubble;
			mBubbleFade = BUBBLE_FADE_UPDATES;
			MarkDirty();
			mCurBubble = theBubble;
			mBubbleHoverCount = 0;
		}
	}
}

// 0x494aa0
void MainMenu::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	if (mMouseIgnoreCountdown == 0)
		mMouseMoved = true;
}

// 0x494ad0
void MainMenu::ButtonMouseEnter(int theId)
{
	// (the original goes on reading the table after its last button; the progress stops at the end here)
	if (mEasterEggProgress < NUM_EASTER_EGG_BUTTONS && theId == EASTER_EGG_SEQUENCE[mEasterEggProgress])
		mEasterEggProgress++;
	else
		mEasterEggProgress = 0;

	if (mEasterEggProgress >= NUM_EASTER_EGG_BUTTONS)
		mApp->ShowStoryScreen(true, true);
}

// 0x494b10
void MainMenu::StartTrophyAnimation()
{
	if (mTrophyRank > 0 && mTrophyRank < 4)
	{
		mTrophyAnimTime = 0;
		mTrophyAnimLength = TROPHY_ANIM_LENGTH;
	}
}

// 0x499c20
void MainMenu::SyncPlayerInfo()
{
	mStartStage = 0;
	mStartLevel = 0;
	mTrophyRank = 0;

	PlayerInfo* aPlayer = mApp->mCurrentPlayer;
	if (aPlayer != nullptr)
	{
		int aSavedRank = aPlayer->mTrophyRank;
		mStartStage = aPlayer->mAdventureStage;
		mStartLevel = aPlayer->mAdventureLevel;
		if (mStartStage < 0)
			mStartStage = 0;
		if (mStartLevel < 0)
			mStartLevel = 0;

		// the trophy: for the adventure, then all the trophies of the challenges, then every level cleared
		if (aPlayer->mAdventuresBeaten > 0)
		{
			mTrophyRank = 1;
			if (mApp->mTrophyMgr.mTrophyById.size() <= aPlayer->mTrophiesEarned.size())
			{
				mTrophyRank = 2;
				if (mApp->mStageMgr.mLevelsByName.size() <= aPlayer->mLevelsCleared.size())
					mTrophyRank = 3;
			}
		}

		if (aSavedRank < mTrophyRank)
		{
			aPlayer->mTrophyRank = mTrophyRank;
			StartTrophyAnimation();
			aPlayer->SaveDetails();
		}
	}
}

// 0x4a90a0
void MainMenu::DoNotYet(const std::string& theText)
{
	ThunderDialog* aDialog = static_cast<ThunderDialog*>(
		mApp->DoDialog(DIALOG_LOCKED, true, "Locked", theText, "Ok", Dialog::BUTTONS_FOOTER));
	aDialog->mHangsOnChain = true;
	aDialog->DoScroll(1);
}

// 0x4a9190
void MainMenu::StartGame()
{
	PlayerInfo* aPlayer = mApp->mCurrentPlayer;
	switch (mApp->mGameMode)
	{
	case GAME_MODE_ADVENTURE:
		if (aPlayer != nullptr && aPlayer->mAdventuresBeaten > 0)
		{
			if (aPlayer->JustBeatAdventure())
				aPlayer->RestartAdventure();
			mApp->ShowAdventureScreen();
			return;
		}
		mApp->StartAdventureGame();
		return;

	case GAME_MODE_QUICK_PLAY:
	case GAME_MODE_DUEL:
		if (mApp->mGameMode == GAME_MODE_DUEL && aPlayer != nullptr && aPlayer->mMaxStage < 1)
		{
			DoNotYet("^FFFF00^DUEL MODE LOCKED!^oldclr^\nThis mode lets you compete against a friend or the computer.\n\n"
					 "^FFFF00^To Unlock:^oldclr^\nFinish Stage 1 of Adventure Mode!");
			return;
		}
		if (mRandomLevel && aPlayer != nullptr)
		{
			NOT_TRANSLATED("StageMgr::GetRandomLevel (a right click on the second or third button)");
		}
		mApp->ShowLevelScreen(false);
		return;

	case GAME_MODE_CHALLENGE:
		if (aPlayer != nullptr && aPlayer->mAdventuresBeaten < 1)
		{
			DoNotYet("^FFFF00^CHALLENGE MODE LOCKED!^oldclr^\nThis mode presents 75 challenges for advanced players.\n\n"
					 "^FFFF00^To Unlock:^oldclr^\nComplete Adventure Mode!");
			return;
		}
		mApp->ShowTrophyScreen();
		return;
	}
}

// 0x4afd20 (the original starts by asking LogicMgr::KeyDown, a stub that answers no, which the Mac build's names make
// look like a test of the button; it does nothing)
void MainMenu::ButtonDepress(int theId)
{
	mRandomLevel = false;
	PlayerInfo* aPlayer = mApp->mCurrentPlayer;

	switch (theId)
	{
	case MAIN_MENU_BUTTON_ADVENTURE:
		mApp->mGameMode = GAME_MODE_ADVENTURE;
		mApp->mStartStage = mStartStage;
		mApp->mStartLevel = mStartLevel;
		// a profile that has not beaten the adventure may have a game saved: asks whether to go on with it
		if (aPlayer == nullptr || aPlayer->mAdventuresBeaten > 0 || !mApp->CheckSaveGame(true))
			StartGame();
		break;

	case MAIN_MENU_BUTTON_SOLO:
		mApp->mGameMode = GAME_MODE_QUICK_PLAY;
		mRandomLevel = mSoloButton->mRightClicked;
		if (!mApp->CheckSaveGame(true))
			StartGame();
		break;

	case MAIN_MENU_BUTTON_DUEL:
		mApp->mGameMode = GAME_MODE_DUEL;
		mRandomLevel = mDuelButton->mRightClicked;
		if (!mApp->CheckSaveGame(true))
			StartGame();
		break;

	case MAIN_MENU_BUTTON_CHALLENGE:
		mApp->mGameMode = GAME_MODE_CHALLENGE;
		if (!mApp->CheckSaveGame(true))
			StartGame();
		break;

	case MAIN_MENU_BUTTON_REPLAYS:
		mApp->ViewReplays();
		break;

	case MAIN_MENU_BUTTON_OPTIONS:
		mApp->DoOptionsDialog();
		break;

	case MAIN_MENU_BUTTON_DEMO:
		if (aPlayer != nullptr && aPlayer->mMaxStage < 2)
		{
			DoNotYet("^FFFF00^DEMO MODE LOCKED!^oldclr^\nThis mode shows how the Masters play.\n\n"
					 "^FFFF00^To Unlock:^oldclr^\nFinish Stage 2 of Adventure Mode!");
			return;
		}
		mApp->mGameMode = GAME_MODE_DEMO;
		mApp->ShowBoard(true, true);
		break;

	case MAIN_MENU_BUTTON_QUIT:
		mApp->DoConfirmQuitDialog();
		break;

	case MAIN_MENU_BUTTON_NOT_YOU:
		mApp->PlaySample(SOUND_BUTTON1);
		mApp->DoUserDialog();
		break;
	}
}

// 0x4af680
void MainMenu::Update()
{
	Widget::Update();

	// the picture goes darker while the mascot cries
	if (!mMascotCrying)
	{
		if (mQuitDarken > 0)
			mQuitDarken--;
	}
	else if (mQuitDarken < QUIT_DARKEN_MAX)
		mQuitDarken++;

	PlaceButtons();

	// what the mouse is over says what the bubble is to say (while no dialog is up and the mouse has moved)
	if (mApp->GetDialogCount() == 0 && mMouseMoved)
	{
		int aBubble;
		int aDelay = BUBBLE_DELAY;
		if (mAdventureButton->mIsOver || mAdventureButton->mIsDown)
			aBubble = BUBBLE_ADVENTURE;
		else if (mSoloButton->mIsOver || mSoloButton->mIsDown)
			aBubble = BUBBLE_SOLO;
		else if (mDuelButton->mIsOver || mDuelButton->mIsDown)
			aBubble = BUBBLE_DUEL;
		else if (mChallengeButton->mIsOver || mChallengeButton->mIsDown)
			aBubble = BUBBLE_CHALLENGE;
		else if (mDemoButton->mIsOver || mDemoButton->mIsDown)
			aBubble = BUBBLE_DEMO;
		else if (mReplaysButton->mIsOver || mReplaysButton->mIsDown)
			aBubble = BUBBLE_REPLAYS;
		else if (mOptionsButton->mIsOver || mOptionsButton->mIsDown)
			aBubble = BUBBLE_OPTIONS;
		else if (mQuitButton->mIsOver || mQuitButton->mIsDown)
			aBubble = BUBBLE_QUIT;
		else if (mTrophyRank > 0 && Rect(TROPHY_HOVER_X, TROPHY_HOVER_Y, TROPHY_HOVER_WIDTH, TROPHY_HOVER_HEIGHT)
										.Contains(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY))
		{
			aBubble = mTrophyRank + BUBBLE_QUIT; // the trophy's bubble for its rank
			aDelay = TROPHY_BUBBLE_DELAY;
		}
		else
			aBubble = BUBBLE_GREETING;
		SetCurBubble(aBubble, aDelay);
	}

	if (mMouseIgnoreCountdown != 0)
		mMouseIgnoreCountdown--;
	if (mBubbleFade > 0)
	{
		mBubbleFade--;
		MarkDirty();
	}

	// the link to change the player is there while the greeting is
	bool aGreeting = mApp->mCurrentPlayer != nullptr && mCurBubble == BUBBLE_GREETING;
	if (mNotYouLink->mVisible != aGreeting)
	{
		mNotYouLink->SetVisible(aGreeting);
		MarkDirty();
	}

	// the sun's glow turns in 3D mode, which has to draw the picture again every update
	if (mApp->Is3DAccelerated() && mApp->mActive)
	{
		mFrame++;
		MarkDirty();
	}

	if (mTrophyAnimTime < mTrophyAnimLength)
	{
		mTrophyAnimTime++;
		MarkDirty();
		if (mTrophyAnimTime == TROPHY_AWARD_SOUND_TIME)
			mApp->PlaySample(SOUND_AWARD);
		else if (mTrophyAnimTime == TROPHY_APPLAUSE_SOUND_TIME)
			mApp->PlaySample(SOUND_APPLAUSE);
	}
}

// 0x4a8470: the text of the bubble. theAlpha is how much of it shows while it changes, theDrawBubble whether to draw the
// bubble's own picture (only for one of the two bubbles that are drawn in a change).
void MainMenu::DrawSpeechBubble(Graphics* g, int theBubble, int theAlpha, bool theDrawBubble)
{
	PlayerInfo* aPlayer = mApp->mCurrentPlayer;
	std::string aName;
	int aStage = 0;
	int aLevel = 0;
	bool aBeatAdventure = false;
	if (aPlayer != nullptr)
	{
		aName = aPlayer->mName;
		aStage = aPlayer->mAdventureStage;
		aLevel = aPlayer->mAdventureLevel;
		aBeatAdventure = aPlayer->mAdventuresBeaten > 0;
	}

	if (theDrawBubble && (!aName.empty() || theBubble > 0))
	{
		// the bubble shrinks a little as it changes, most half way through
		Transform aTransform;
		float aScale = BUBBLE_SCALE;
		if (mBubbleFade > 0)
		{
			float aFromMiddle = std::fabs(BUBBLE_SCALE_DIP_BASE - (float)mBubbleFade);
			double aDip = BUBBLE_SCALE_DIP_BASE - aFromMiddle;
			aScale = (float)((double)aScale - (double)BUBBLE_SCALE_DIP * aDip);
		}
		aTransform.Scale(aScale, aScale);
		g->DrawImageTransform(IMAGE_MM_SPEECHBUBBLE, aTransform, (float)(IMAGE_MM_SPEECHBUBBLE->mWidth / 2 + BUBBLE_X),
							  (float)(IMAGE_MM_SPEECHBUBBLE->mHeight / 2 + BUBBLE_CENTER_Y_OFFSET + BUBBLE_Y));
	}

	Rect aTextRect(BUBBLE_TEXT_LEFT + BUBBLE_X, BUBBLE_TEXT_TOP + BUBBLE_Y, BUBBLE_TEXT_WIDTH, BUBBLE_TEXT_HEIGHT);
	mMascotCrying = false;

	if (theBubble < 1)
	{
		// the greeting: two lines, the name on the second, each in the middle
		if (!aName.empty())
		{
			std::string aLine = (aStage < 0) ? "Welcome to Peggle," : "Welcome Back,";
			Font* aFont = FONT_PLAIN2BIG;
			int aX = (aTextRect.mWidth - aFont->StringWidth(aLine)) / 2 + aTextRect.mX;
			int aY = aFont->GetHeight() + GREETING_LINE_OFFSET_Y + aTextRect.mY;
			g->SetColor(Color(GREETING_COLOR, theAlpha));
			g->SetFont(aFont);
			g->DrawString(aLine, aX, aY);

			aLine = aName + "!";
			aFont = FONT_PLAIN2BIG;
			aY = aFont->GetHeight() + aY + GREETING_LINE_OFFSET_Y;
			aX = (GREETING_NAME_AREA_WIDTH - aFont->StringWidth(aLine)) / 2 + BUBBLE_X;
			g->SetFont(aFont);
			g->SetColor(Color(GREETING_COLOR, theAlpha));
			g->DrawString(aLine, aX, aY);
		}
	}
	else
	{
		g->SetFont(FONT_PLAIN2);
		g->SetColor(Color(BUBBLE_TEXT_COLOR, theAlpha));

		std::string aText;
		switch (theBubble)
		{
		case BUBBLE_ADVENTURE:
			if (aStage == -1 && !aBeatAdventure)
				aText = "New to Peggle?\nClick here to start your adventure!";
			else if ((aStage >= 1 && aStage <= 10) || aLevel > 0)
				aText = StrFormat("Continue your adventure on\nStage %d-%d!", aStage + 1, aLevel + 1);
			else
				aText = "Start a new Adventure!\nCan you get a high score?";
			break;
		case BUBBLE_SOLO:
			aText = "Play individual levels that you have unlocked in Adventure!";
			break;
		case BUBBLE_DUEL:
			aText = "Play against a friend or the computer in this two player mode!";
			break;
		case BUBBLE_CHALLENGE:
			aText = "Complete all of these extra\ndifficult challenges to become a\nPeggle Grand Master!";
			break;
		case BUBBLE_DEMO:
			aText = "Watch the Peggle Masters play and see if you can learn their tricks!";
			break;
		case BUBBLE_REPLAYS:
			aText = "View instant replays\nthat you've saved!";
			break;
		case BUBBLE_OPTIONS:
			aText = "Set options like sound volume\nand colorblind mode.\nHelp is also available here.";
			break;
		case BUBBLE_QUIT:
			aText = "See you again soon!";
			mMascotCrying = true;
			break;
		case BUBBLE_TROPHY_ADVENTURE:
			aText = "This trophy is awarded for completing Adventure Mode!";
			break;
		case BUBBLE_TROPHY_CHALLENGES:
			aText = "This trophy is awarded for beating all of the challenges!";
			break;
		case BUBBLE_TROPHY_LEVELS:
			aText = "This trophy is awarded for beating all of the challenges and clearing all of the levels!";
			break;
		}

		// centred up and down in the text's area
		int aTextHeight = GetWordWrappedHeight(g, aTextRect.mWidth, aText, BUBBLE_LINE_SPACING);
		aTextRect.mY += (aTextRect.mHeight - aTextHeight) / 2;
		WriteWordWrapped(g, aTextRect, aText, BUBBLE_LINE_SPACING, 0);
	}

	// the mascot cries when the Quit button is the one the bubble is about; fading in and out with the bubble
	if (mMascotCrying && theBubble == BUBBLE_QUIT)
	{
		int anAlpha = 255;
		if (mBubbleFade != 0)
		{
			anAlpha = (mBubbleFade * 255) / BUBBLE_FADE_UPDATES;
			if (mCurBubble == BUBBLE_QUIT)
				anAlpha = 255 - anAlpha;
		}
		g->SetColorizeImages(true);
		g->SetColor(Color(0xffffff, anAlpha));
		g->DrawImage(IMAGE_MM_CRY, CRY_X, CRY_Y);
		g->SetColorizeImages(false);
	}
}

// 0x4a8c00
void MainMenu::Draw(Graphics* g)
{
	// the whole picture goes darker while the mascot cries
	if (mQuitDarken > 0)
	{
		int aShade = ((BUBBLE_FADE_UPDATES * 2 - mQuitDarken) * 127) / BUBBLE_FADE_UPDATES;
		g->SetColor(Color(aShade, aShade, aShade));
		g->SetColorizeImages(true);
	}

	g->DrawImage(IMAGE_MM_BACKGROUND, MENU_X, MENU_Y);

	// the glow of the sun turns in 3D mode
	g->SetColorizeImages(false);
	if (!g->Is3D())
		g->DrawImage(IMAGE_SUNGLOW, SUN_GLOW_X, SUN_GLOW_Y);
	else
	{
		float aDegrees = (float)((double)SUN_GLOW_DEGREES_PER_UPDATE * mFrame);
		float aRadians = (float)(((double)aDegrees * PI_OF_THE_ORIGINAL) / 180.0);
		g->DrawImageRotated(IMAGE_SUNGLOW, SUN_GLOW_X, SUN_GLOW_Y, (double)aRadians, SUN_GLOW_CENTER_X, SUN_GLOW_CENTER_Y);
	}
	g->DrawImage(IMAGE_MM_BLANKSUN, BLANK_SUN_X + SUN_GLOW_X, BLANK_SUN_Y + SUN_GLOW_Y);
	g->DrawImage(IMAGE_MM_FOREGROUND, FOREGROUND_X + MENU_X, mHeight - IMAGE_MM_FOREGROUND->mHeight - FOREGROUND_BOTTOM);
	g->DrawImage(IMAGE_MM_BJORN, MASCOT_X, MASCOT_Y);

	// the bubble: the one that was up fades out while the new one fades in
	if (mBubbleFade > 0)
	{
		int aFade = (mBubbleFade * 255) / BUBBLE_FADE_UPDATES;
		DrawSpeechBubble(g, mCurBubble, 255 - aFade, true);
		DrawSpeechBubble(g, mPrevBubble, aFade, false);
	}
	else
		DrawSpeechBubble(g, mCurBubble, 255, true);

	// the trophy, one of the three pictures of its row; while it rises, only the part that has come up
	if (mTrophyRank > 0)
	{
		int aCel = mTrophyRank - 1;
		if (aCel < 0)
			aCel = 0;
		else if (aCel > NUM_TROPHY_RANKS - 1)
			aCel = NUM_TROPHY_RANKS - 1;

		if (mTrophyAnimTime >= mTrophyAnimLength)
			g->DrawImageCel(IMAGE_MM_TROPHIES, TROPHY_X, TROPHY_Y, aCel);
		else
		{
			int aRisen = (int)((double)IMAGE_MM_TROPHIES->mHeight - (double)TROPHY_RISE_SPEED * mTrophyAnimTime);
			Graphics aClippedGraphics(*g);
			aClippedGraphics.ClipRect(TROPHY_X, aRisen + TROPHY_Y, IMAGE_MM_TROPHIES->mWidth / 3,
									  IMAGE_MM_TROPHIES->mHeight - aRisen);
			aClippedGraphics.DrawImageCel(IMAGE_MM_TROPHIES, TROPHY_X, TROPHY_Y, aCel);
		}
	}

	g->DrawImage(IMAGE_MM_LEFT, 0, 0);
	g->DrawImage(IMAGE_MM_TOP, TOP_X, TOP_Y);
	g->DrawImage(IMAGE_MM_RIGHT, mWidth - IMAGE_MM_RIGHT->mWidth, 0);
	g->DrawImage(IMAGE_MM_BOTTOM, BOTTOM_X, mHeight - IMAGE_MM_BOTTOM->mHeight);

	if (mApp->mCurrentPlayer != nullptr)
	{
		DrawTrophyEffect(g);
		DeferOverlay(0);
	}
}

// 0x494b50
void MainMenu::DrawTrophyEffect(Graphics* g)
{
	if (mTrophyAnimLength <= mTrophyAnimTime)
		return;
	NOT_TRANSLATED("MainMenu::DrawTrophyEffect (the animation of a trophy that has just been earned)");
}
