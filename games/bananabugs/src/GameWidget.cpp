#include "GameWidget.h"
#include "BananaBonus.h"
#include "BugImg.h"
#include "BugImgAnt.h"
#include "BugImgBeetle.h"
#include "BugImgSkeeter.h"
#include "Board.h"
#include "FireBall.h"

#include "Anim.h"
#include "Color.h"
#include "Cursor.h"
#include "GameApp.h"
#include "Graphics.h"
#include "LoadGame.h"
#include "MainMenu.h"
#include "Map.h"
#include "MetaGame.h"
#include "ObjBase.h"
#include "LevelObj.h"
#include "Pending.h"
#include "Player.h"
#include "PopCap.h"
#include "SoundQueue.h"
#include "Transition.h"

using namespace Sexy;

// 0x42f230: the cursor to the window's centre, and PopCap's logo first
GameWidget::GameWidget(GameApp* theApp)
{
	mApp = theApp;
	mScreen = SCREEN_POPCAP;
	mFrame = 0;
	mUnusedB0 = 0;
	mBoard = nullptr;
	mMetaGame = nullptr;
	mMainMenu = nullptr;
	mTransition = nullptr;
	mInputLock = 0;
	mLoadGame = nullptr;
	mPreloadStep = 0;
	CentreCursor();
	mPopCap = new PopCap();
}

// 0x42f330. The scene graph deletes the other screens with the root.
GameWidget::~GameWidget()
{
	if (mTransition != nullptr)
		delete mTransition;
	if (mBoard != nullptr)
		delete mBoard;
}

void GameWidget::CentreCursor()
{
	mApp->WarpCursor(mApp->mWidth / 2, mApp->mHeight / 2);
}

// 0x42f3b0: one group of each class's pictures a step, while the loading bar shows; false after the last
bool GameWidget::Preload(int theStep)
{
	switch (theStep) {
	case 0:
		PreloadNetFly();
		PreloadBananas();
		PreloadMenuTiles();
		PreloadWalkThru();
		PreloadSlots();
		PreloadFly();
		return true;
	case 1:
		PreloadInGameButtons();
		PreloadAnt();
		PreloadMoth();
		return true;
	case 2:
		PreloadBeetle();
		PreloadAskWin();
		return true;
	case 3:
		PreloadCentipede();
		PreloadWasp();
		return true;
	case 4:
		PreloadCaterpillar();
		PreloadFlyPaths();
		PreloadMothPaths();
		return true;
	case 5:
		// these three are loaded here only to be kept in the pictures' list
		PreloadSkeeter();
		Anim::Get("images/bonus_discoball", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 6, false);
		Anim::Get("images/splat01", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 3, false);
		Anim::Get("images/stat_back", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
		return true;
	case 6:
		PreloadSkeeter();
		return true;
	case 7:
		PreloadBonusPictures();
		InitBonusTable();
		return true;
	case 8:
		PreloadSpinStars();
		PreloadRolyPoly();
		return true;
	case 9:
		PreloadMap();
		PreloadFireBall();
		return true;
	case 10:
		PreloadPipes();
		PreloadRope();
		InitLevelTable();
		PreloadCockroach();
		Cursor::Create();
		Cursor::Hide();
		PauseWin::Enable();
		return true;
	default:
		return false;
	}
}

// 0x42f510: the screens' changes wait for the doors. A focus lost with no window open pauses the game; while the
// pictures are made again after a size change nothing else runs. The original sleeps 10 ms an update without the
// focus; the port does not wait.
void GameWidget::Update()
{
	if (!mApp->mHasFocus && BBWindow::sOpenCount < 1) {
		PauseWin::PauseForFocusLoss();
		mApp->mFocusLost = false;
	}
	if (AnimReload::sActive) {
		Widget::Update();
		MarkDirty();
		if (mApp->mSoundQueue != nullptr)
			mApp->mSoundQueue->Update();
		AnimReload::Step();
		return;
	}
	if (mInputLock != 0)
		mInputLock--;

	switch (mScreen) {
	case SCREEN_POPCAP:
		if (mPopCap->IsDone()) {
			mPopCap->Delete();
			mPopCap = nullptr;
			mScreen = SCREEN_LOADING;
			mPreloadStep = 0;
			mLoadGame = new LoadGame();
		}
		break;
	case SCREEN_LOADING:
		if (mLoadGame->IsReady()) {
			if (!Preload(mPreloadStep)) {
				mLoadGame->Finish();
				mTransition = new Transition();
				mTransition->Close(false, false);
				mScreen = SCREEN_TO_MENU;
			}
			mPreloadStep++;
			mLoadGame->SetProgress(mPreloadStep * 10);
		}
		break;
	case SCREEN_TO_MENU:
		if (Transition::GetPhase() == TRANSITION_CLOSED) {
			if (mBoard != nullptr) {
				delete mBoard;
				mBoard = nullptr;
			}
			if (mMetaGame != nullptr) {
				delete mMetaGame;
				mMetaGame = nullptr;
			}
			mMainMenu = new MainMenu();
			mTransition->Open(false);
			mScreen = SCREEN_MENU;
		}
		break;
	case SCREEN_MENU:
		if (Transition::GetPhase() == TRANSITION_OPEN) {
			int aChoice = mMainMenu->GetChoice();
			if (aChoice == MENU_CHOICE_PLAY) {
				mScreen = SCREEN_TO_PLAY;
				mTransition->Close(false, true);
				mMainMenu->StartLeaving();
			}
			// the original posts WM_CLOSE to its window
			if (mMainMenu->GetChoice() == MENU_CHOICE_QUIT)
				mApp->CloseRequestAsync();
		}
		break;
	case SCREEN_TO_PLAY:
		if (Transition::GetPhase() == TRANSITION_CLOSED) {
			if (mMainMenu != nullptr) {
				delete mMainMenu;
				mMainMenu = nullptr;
			}
			CentreCursor();
			mFrame = 0;
			mUnusedB0 = 0;
			if (!Player::sCurrent->mMetaGameStarted) {
				mBoard = new Board();
				mScreen = SCREEN_LEVEL;
				mTransition->Open(true);
			} else {
				mApp->ChangeMusic(MUSIC_META);
				mMetaGame = new MetaGame();
				mScreen = SCREEN_MAP;
				mTransition->Open(false);
			}
		}
		break;
	case SCREEN_LEVEL: {
		int aPhase = Transition::GetPhase();
		if (aPhase != TRANSITION_OPEN) {
			if (aPhase == TRANSITION_CLOSED) {
				CentreCursor();
				Player::sCurrent->Deactivate(Player::sCurrent);
				if (mBoard != nullptr)
					delete mBoard;
				mBoard = nullptr;
				mApp->ChangeMusic(MUSIC_META);
				mMetaGame = new MetaGame();
				mScreen = SCREEN_MAP;
				mFrame = 0;
				mTransition->Open(false);
			}
			break;
		}
		int aResult = mBoard->mLevel->GetResult();
		if (aResult == LEVEL_RESULT_WON) {
			CentreCursor();
			mBoard->StartLeaving();
			mTransition->Close(true, true);
			Player::sCurrent->Deactivate(Player::sCurrent);
			break;
		}
		if (aResult != LEVEL_RESULT_QUIT && aResult != LEVEL_RESULT_LOST)
			break;
		CentreCursor();
		Player::sCurrent->Deactivate(Player::sCurrent);
		mScreen = SCREEN_TO_MENU;
		mBoard->StartLeaving();
		mTransition->Close(true, true);
		break;
	}
	case SCREEN_MAP: {
		int aPhase = Transition::GetPhase();
		if (aPhase != TRANSITION_OPEN) {
			if (aPhase == TRANSITION_CLOSED) {
				if (mMetaGame != nullptr) {
					delete mMetaGame;
					mMetaGame = nullptr;
				}
				mBoard = new Board();
				mScreen = SCREEN_LEVEL;
				mFrame = 0;
				Player::sCurrent->Activate(Player::sCurrent, false);
				CentreCursor();
				mTransition->Open(true);
			}
			break;
		}
		if (mMetaGame->TakeLevelChosen()) {
			CentreCursor();
			mTransition->Close(false, true);
		}
		if (!mMetaGame->mWantsMenu)
			break;
		mScreen = SCREEN_TO_MENU;
		mMetaGame->mLeaving = true;
		mMetaGame->TakeLevelChosen();
		mTransition->Close(true, true);
		break;
	}
	}

	Widget::Update();
	ObjBase::UpdateAll(mFrame);
	if (mApp->mSoundQueue != nullptr)
		mApp->mSoundQueue->Update();
	MarkDirty();
	mFrame++;
}

// 0x42fc40: black behind PopCap's logo
void GameWidget::Draw(Graphics* g)
{
	if (AnimReload::sActive) {
		AnimReload::Draw(g);
		return;
	}
	if (mScreen == SCREEN_POPCAP) {
		g->SetColor(Color::Black);
		g->FillRect(0, 0, mApp->mWidth, mApp->mHeight);
	}
	ObjBase::DrawAll(g);
}

// 0x42fcc0
void GameWidget::MouseMove(int x, int y)
{
	if (mInputLock == 0)
		ObjBase::SetMousePos(x, y);
}

// 0x42fce0
void GameWidget::MouseDrag(int x, int y)
{
	if (mInputLock == 0)
		ObjBase::MouseDrag(x, y);
}

// 0x42fd00: a press no object takes goes to the board
void GameWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	(void)x;
	(void)y;
	(void)theClickCount;
	if (mInputLock == 0) {
		if (!ObjBase::MousePress() && mScreen == SCREEN_LEVEL)
			Player::BoardClick(theBtnNum);
	}
}

// 0x42fd30: a release while the pause window waits for it resumes the game
void GameWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	(void)x;
	(void)y;
	(void)theBtnNum;
	(void)theClickCount;
	if (mInputLock == 0) {
		if (PauseWin::IsBlocking()) {
			PauseWin::ResumeOnRelease();
			return;
		}
		ObjBase::MouseRelease();
	}
}

// 0x42fd60
void GameWidget::KeyChar(SexyChar theChar)
{
	ObjBase::SetKeyHeld(theChar);
}

// 0x42fd70
void GameWidget::KeyDown(KeyCode theKey)
{
	ObjBase::SetKeyPressed(theKey);
}
