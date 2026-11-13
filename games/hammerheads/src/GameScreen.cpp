#include "GameScreen.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "Application.h"
#include "Avatar.h"
#include "ButtonWidget.h"
#include "Cubby.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Gameplay.h"
#include "GnomeBouncing.h"
#include "GnomeFlying.h"
#include "GnomeKing.h"
#include "Graphics.h"
#include "HammerDisplay.h"
#include "Hearts.h"
#include "LevelHud.h"
#include "LevelRules.h"
#include "NcButton.h"
#include "Coins.h"
#include "Res.h"
#include "Score.h"
#include "Stats.h"
#include "Sticker.h"
#include "Timer.h"
#include "WidgetManager.h"
#include "platform/VFS.h"

using namespace Sexy;

// 0x59241c: the kind that PickCreatureKind returned last (it lives on from one game to the next)
static int gLastKind = -1;

// 0x4527c0
GameScreen::GameScreen()
{
	mOver = false;
	mKindIndex = 0;
	mLevelTime = 0;
	mLevel = 1;
	mLevelsStarted = 0;
	mCycle = 0;
	mMode = 0;
	mMaxBoard = 0;
	mMenuButton = nullptr;

	// 0 to 10 in an order the generator picks
	std::vector<int> aNumbers;
	for (int i = 0; i < NUM_SHUFFLED_BOARDS; i++)
		aNumbers.push_back(i);
	int aCount = 0;
	while (!aNumbers.empty())
	{
		int anIndex = Rand() % (int)aNumbers.size();
		mShuffledBoards[aCount++] = aNumbers[anIndex];
		aNumbers.erase(aNumbers.begin() + anIndex);
	}

	// the cheat words that start a game at a later level
	int aJump = -1;
	if (gApp->GetCheatFlag(Application::CHEAT_JUMP_TO_5))
		aJump = 5;
	else if (gApp->GetCheatFlag(Application::CHEAT_JUMP_TO_5 + 1))
		aJump = 10;
	if (gApp->GetCheatFlag(Application::CHEAT_JUMP_TO_5 + 2))
		aJump = 15;
	else if (gApp->GetCheatFlag(Application::CHEAT_JUMP_TO_5 + 3))
		aJump = 20;
	else if (gApp->GetCheatFlag(Application::CHEAT_JUMP_TO_5 + 4))
		aJump = 25;
	if (aJump != -1)
		mLevel = aJump;

	mMenuButton = MakeNcButton(0, this, "", IMAGE_INGAMEUI_MENUBUTTON, 3);
	mMenuButton->mDarkensWithFade = true;
	FindMaxBoard();
}

// 0x451a30
GameScreen::~GameScreen()
{
	delete mMenuButton;
}

// 0x450b80
void GameScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMenuButton);
}

// 0x450bf0
void GameScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMenuButton);
	if (GetStats()->Get(STAT_HEALTH) > 0.0 && !mOver)
		gApp->ProcessPending();
}

// 0x450c50: the menu button stays in the top right corner
void GameScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mMenuButton->Move(gApp->mWidth - mMenuButton->mWidth - 6, theY + 6);
}

// 0x450bb0
bool GameScreen::IsGameInProgress() const
{
	return GetStats()->Get(STAT_HEALTH) > 0.0 && !mOver;
}

// 0x450ca0
bool GameScreen::IsInPlay() const
{
	if (mWidgetManager->mFocusWidget != this)
		return false;
	return gApp->GetDialog(DIALOG_GAME_MENU) == nullptr && gApp->GetDialog(DIALOG_LEAVE_GAME) == nullptr &&
		gApp->GetDialog(DIALOG_READY_GO) == nullptr && gApp->mLevelIntro == nullptr;
}

// 0x451040
bool GameScreen::IsPaused() const
{
	return gApp->IsPaused() && IsInPlay();
}

// 0x450d60
void GameScreen::Pause()
{
	gApp->PauseGame(true);
}

// 0x450d80
bool GameScreen::IsLevelRunning() const
{
	int aPhase = GetTimer()->mPhase;
	Creature* aSaveGnome = GetGameplay()->FindCreatureOfKind(KIND_SAVE);
	bool aSaveGnomeAlive = aSaveGnome != nullptr && aSaveGnome->GetHealth() > 0;
	int aKingsDestroyed = GetStats()->GetInt(STAT_KINGS_DESTROYED);
	int aKingsReleased = GetStats()->GetInt(STAT_KINGS_RELEASED);
	bool aLastKingStands = aKingsReleased == 1 && aKingsDestroyed == 0;
	return (aPhase != TIMER_PHASE_RUNNING_OUT || aSaveGnomeAlive || aLastKingStands) && aKingsDestroyed == 0;
}

// 0x450e10: the menu button
void GameScreen::ButtonPress(int theId)
{
	if (theId == 0 && !gApp->IsPaused())
		gApp->ShowGameMenu();
}

// 0x450ec0: the pointer is the hammer's crosshair
void GameScreen::ShowCrosshair()
{
	if (!gApp->IsShopOpen())
	{
		gApp->SetCursor(CURSOR_NONE);
		if (mAvatar.IsValid())
		{
			static_cast<Avatar*>(mAvatar.Get())->mCrosshairVisible = true;
			static_cast<Avatar*>(mAvatar.Get())->SetMouse(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
		}
	}
}

// 0x450f30: the plain pointer
void GameScreen::UpdateHover()
{
	if (!gApp->IsShopOpen())
	{
		gApp->SetCursor(CURSOR_POINTER);
		if (mAvatar.IsValid())
			static_cast<Avatar*>(mAvatar.Get())->mCrosshairVisible = false;
	}
}

// 0x450f80
void GameScreen::GotFocus()
{
	Widget::GotFocus();
	if (!mOver && !gApp->IsPaused())
		ShowCrosshair();
	else
		UpdateHover();
}

// 0x450fc0
void GameScreen::LostFocus()
{
	Widget::LostFocus();
	UpdateHover();
}

// 0x450d10
void GameScreen::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);
	if (!mOver && mAvatar.IsValid())
		static_cast<Avatar*>(mAvatar.Get())->SetMouse(x, y);
}

// 0x451200
void GameScreen::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	if (!mOver && mAvatar.IsValid())
		static_cast<Avatar*>(mAvatar.Get())->SetMouse(x, y);
}

// 0x451320
void GameScreen::MouseDown(int x, int y, int theClickCount)
{
	if (gApp->mTrophyRoom != nullptr)
		return;
	if (gApp->mFocusedUpdate == mUpdateCnt)
		return;			// the click that gave the game the focus again is eaten
	if (mOver)
		return;

	if (gApp->IsPaused() && IsInPlay())
	{
		gApp->PauseGame(false);
		return;
	}
	if (!gApp->IsPaused() && GetGameplay()->IsPlaying() && IsLevelRunning() && mAvatar.IsValid())
		static_cast<Avatar*>(mAvatar.Get())->AddClick(x, y);
}

// 0x451410
void GameScreen::KeyDown(KeyCode theKey)
{
	if (gApp->mTrophyRoom != nullptr)
		return;
	Widget::KeyDown(theKey);

	if (theKey == KEYCODE_ESCAPE)
	{
		if (!mOver && !gApp->IsPaused())
			gApp->ShowGameMenu();
	}
	else if (theKey == KEYCODE_SPACE || theKey == 'P')
	{
		if (gApp->IsPaused() && IsInPlay())
		{
			gApp->PauseGame(false);
			return;
		}
		if (IsInPlay())
			Pause();
	}
}

// 0x451070
void GameScreen::Update()
{
	Widget::Update();
	if (!gApp->IsPaused() && !mOver && IsLevelRunning())
		GetStats()->Set(STAT_SPEED_TIMER, GetStats()->Get(STAT_SPEED_TIMER) + 0.01);
}

// 0x4510d0
void GameScreen::UpdateF(float theFrac)
{
	if (!(gApp->IsPaused() && IsInPlay() && gApp->mFade == 1.0))
		MarkDirty();

	double aFade = gApp->mFade;
	if (!gApp->IsPaused())
	{
		if (aFade > 0.0)
		{
			aFade = aFade - theFrac * 0.02;
			if (aFade < 0.0)
				aFade = 0.0;
		}
		if (!mOver)
		{
			ObjectManager::Get()->Update(theFrac);
			mLevelTime = theFrac * 0.01 + mLevelTime;
		}
	}
	else if (aFade < 1.0)
	{
		aFade = theFrac * 0.02 + aFade;
		if (aFade > 1.0)
			aFade = 1.0;
	}
	gApp->mFade = aFade;
}

// 0x451d90
void GameScreen::Draw(Graphics* g)
{
	if (!mOver)
	{
		if (!gApp->IsShopOpen() && !gApp->IsChallengeOpen() && gApp->mTrophyRoom == nullptr)
		{
			g->DrawImage(GetGameplay()->GetBackdrop(), 0, 0);
			ObjectManager::Get()->Draw(g);

			if (gApp->mFade > 0.0)
			{
				Graphics aG(*g);
				aG.SetColor(Color(0, 0, 20, ToInt(sin(gApp->mFade * 1.5707963267948966) * 220.0)));
				aG.FillRect(0, 0, mWidth, mHeight);
			}

			if (IsPaused() && gApp->mTrophyRoom == nullptr && !gApp->mZooming)
			{
				g->SetColor(Color(0, 0, 0, 255));
				g->FillRect(0, 200, mWidth, 120);
				g->SetColorizeImages(false);
				g->SetFont(FONT_PTBANANASPLIT27);
				g->SetColor(Color(255, 255, 255, 255));
				std::string aTitle = "Paused";
				g->DrawString(aTitle, (mWidth - g->GetFont()->StringWidth(aTitle)) / 2, (mHeight - g->GetFont()->GetHeight() * 2) / 2);
				std::string aHint = "Click Here To Resume";
				g->DrawString(aHint, (mWidth - g->GetFont()->StringWidth(aHint)) / 2, mHeight / 2);
			}
		}
	}
	else
		g->DrawImage(GetGameplay()->GetBackdrop(), 0, 0);

	if (gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
	{
		if (GetGameplay()->GetBackdrop() == IMAGE_BACKDROP_WINTER_1)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0, 100, 255));
			g->DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->GetWidth(),
				mHeight - IMAGE_TOUGH_COOKIE_STAMP->GetHeight());
			g->SetColorizeImages(false);
		}
		else
			g->DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->GetWidth(),
				mHeight - IMAGE_TOUGH_COOKIE_STAMP->GetHeight());
	}
}

// 0x4525c0: a game of a mode starts: the board, then the objects of the board and of the HUD
void GameScreen::Start(int theMode)
{
	mMode = theMode;
	LoadBoard();
	ObjectManager::Get()->Clear();
	ResetObjectNames();

	// the objects of the board file, in the order of the file
	for (size_t i = 0; i < mBoardObjects.size(); i++)
	{
		const BoardObject& anObject = mBoardObjects[i];
		if (anObject.mKind == BOARD_OBJECT_CUBBY)
		{
			Gameplay* aGameplay = GetGameplay();
			Cubby* aCubby = new Cubby(anObject.mX, anObject.mY, Rand() % 6, aGameplay->GetCubbyBackImage());
			aCubby->SetName("");
			aGameplay->mCubbies.push_back(ObjectRef(aCubby));
		}
		else
		{
			Sticker* aSticker = new Sticker(anObject.mX, anObject.mY, anObject.mChoice);
			aSticker->SetName("");
		}
	}
	mBoardObjects.clear();

	mAvatar.Set(MakeAvatar());
	MakeHearts();
	MakeTimer();
	MakeCoins();
	MakeHammerDisplay();
	MakeLevelIndicator();
	MakeScore();
	if (mMode == GAME_MODE_MARATHON)
		MakeMarathonMeter();
}

// 0x452140
void GameScreen::LoadBoard()
{
	if (!LoadBoardFile(GetBoardFileName(), &mBoardObjects))
		mBoardObjects.clear();
}

// 0x451ce0: the board of the level; the levels past the last board run through the last eight over again
std::string GameScreen::GetBoardFileName() const
{
	char aBuffer[64];
	if (mMode == GAME_MODE_CLASSIC || mMode == GAME_MODE_TOUGH_COOKIE)
	{
		int aNumber = mLevel;
		if (aNumber > mMaxBoard)
			aNumber = ((aNumber - mMaxBoard - 1) % 8) + mMaxBoard - 7;
		snprintf(aBuffer, sizeof(aBuffer), mMode == GAME_MODE_CLASSIC ? "boards/board%03d.xml" : "boards/board_tough%03d.xml", aNumber);
		return aBuffer;
	}
	return "boards/board_marathon.xml";
}

// 0x4521a0: the highest number of the files of boards/ that have one in the last three letters before ".xml"
void GameScreen::FindMaxBoard()
{
	mMaxBoard = 0;
	std::string aFolder = Platform::VFS::Get().Resolve("boards/classic.csv");
	size_t aSlash = aFolder.find_last_of("/\\");
	if (aSlash == std::string::npos)
		return;
	aFolder.resize(aSlash);
	std::vector<std::string> aNames = Platform::ListDirectory(aFolder);
	for (size_t i = 0; i < aNames.size(); i++)
	{
		const std::string& aName = aNames[i];
		if (aName.size() < 7 || aName.compare(0, 5, "board") != 0 || Platform::IsDirectory(aFolder + "/" + aName))
			continue;
		int aNumber = atoi(aName.substr(aName.size() - 7, 3).c_str());
		if (aNumber > mMaxBoard)
			mMaxBoard = aNumber;
	}
}

// 0x4519b0: the next level
void GameScreen::NextLevel()
{
	mLevel++;
	mCycle = 0;
	mKindList.clear();
	mKindIndex = 0;
	if (mMode == GAME_MODE_MARATHON)
	{
		GetGameplay()->SetPlaying();
		GetTimer()->Setup(TIMER_SETUP_NEW_LEVEL);
	}
}

// 0x450b50
void GameScreen::SetLevel(int theLevel, int theCycle)
{
	mLevel = theLevel;
	mCycle = theCycle;
}

// 0x450b70
void GameScreen::AddLevelStart()
{
	mLevelsStarted++;
}

// 0x450e50: how many gnomes may be up at once, worked out between the sheet's start and end over the level's time
int GameScreen::GetMaxCreatures() const
{
	int aStart = GetRulesSheet()->GetInt(mLevel - 1, RULE_MAX_GNOMES_START);
	int anEnd = GetRulesSheet()->GetInt(mLevel - 1, RULE_MAX_GNOMES_END);
	return ToInt((double)(anEnd - aStart) * GetTimer()->GetProgress() + (double)aStart);
}

// 0x451630
double GameScreen::GetUpTime(int theKind) const
{
	double aSpeed = gApp->IsChallengeOpen() ? 1.2 : GetStats()->Get(STAT_COMBO);
	double aTopStart = GetRulesSheet()->GetDouble(mLevel - 1, RULE_TOP_START);
	double aTopEnd = GetRulesSheet()->GetDouble(mLevel - 1, RULE_TOP_END);

	int aColumn = -1;
	switch (theKind)
	{
	case KIND_GNOME: aColumn = RULE_TIME_FACTOR_GNOME_1; break;
	case KIND_WOMAN: aColumn = RULE_TIME_FACTOR_WOMAN; break;
	case KIND_STRONG: aColumn = RULE_TIME_FACTOR_STRONG; break;
	case KIND_BOMB: aColumn = RULE_TIME_FACTOR_BOMB; break;
	case KIND_BUBBLEHEAD: aColumn = RULE_TIME_FACTOR_BUBBLEHEAD; break;
	case KIND_HIDE: aColumn = RULE_TIME_FACTOR_HIDE; break;
	case KIND_FORKY_CHAP: aColumn = RULE_TIME_FACTOR_FORKY_CHAP; break;
	case KIND_SILVER: aColumn = RULE_TIME_FACTOR_SILVER; break;
	case KIND_MULTIPLIER: aColumn = RULE_TIME_FACTOR_MULTIPLIER_X5; break;
	case KIND_CUPIDO: aColumn = RULE_TIME_FACTOR_CUPIDO; break;
	case KIND_HEART: aColumn = RULE_TIME_FACTOR_HEART_1; break;
	case KIND_COIN:
	case KIND_BIG_COIN: aColumn = RULE_TIME_FACTOR_COIN_1; break;
	case KIND_SHOP: aColumn = RULE_TIME_FACTOR_SHOP; break;
	case KIND_CHALLENGE: aColumn = RULE_TIME_FACTOR_CHALLENGE; break;
	case KIND_CHEST: aColumn = RULE_TIME_FACTOR_CHEST; break;
	case KIND_CLOVER: aColumn = RULE_TIME_FACTOR_LUCKY_CLOVER; break;
	case KIND_BONUS_ICE_CREAM: aColumn = RULE_TIME_FACTOR_BONUS_ICECREAM; break;
	case KIND_BONUS_DUCK: aColumn = RULE_TIME_FACTOR_BONUS_DUCK; break;
	case KIND_STRAWBERRY: aColumn = RULE_TIME_FACTOR_BONUS_STRAWBERRY; break;
	case KIND_RABBIT: aColumn = RULE_TIME_FACTOR_RABBIT; break;
	case KIND_KING: aColumn = RULE_TIME_FACTOR_KING; break;
	case KIND_NAPOLEON: aColumn = RULE_TIME_FACTOR_NAPOLEON; break;
	case KIND_POWER_UP: aColumn = RULE_TIME_FACTOR_POWERUP; break;
	case KIND_SWITCHER: aColumn = RULE_TIME_FACTOR_SWITCHER; break;
	case KIND_SAVE:
	case KIND_SHOP_HEART:
	case KIND_SHOP_BIG_HEART:
	case KIND_SHOP_EXTRA_HEART:
	case KIND_SHOP_FLAME:
	case KIND_SHOP_HAMMER_2:
	case KIND_SHOP_HAMMER_3:
	case KIND_SHOP_HAMMER_4:
	case KIND_SHOP_ICE_CREAM:
	case KIND_SHOP_CLOVER:
	case KIND_SHOP_STRAWBERRY:
	case KIND_SHOP_GIFT:
		return -1.0;
	}

	double aFactor = 1.0;
	if (aColumn != -1)
		aFactor = GetRulesSheet()->GetDouble(mLevel - 1, aColumn);

	double aProgress = GetTimer()->GetProgress();
	double aTime = ((aTopEnd - aTopStart) * aProgress + aTopStart) / aSpeed * aFactor;

	// the second wave of the king's level lasts twice as long
	if (GetStats()->GetInt(STAT_KINGS_RELEASED) != 0 && GetWaveIndex() == 1)
	{
		if (gApp->mGameScreen->mMode == GAME_MODE_CLASSIC || gApp->mGameScreen->mMode == GAME_MODE_TOUGH_COOKIE)
		{
			if (GetRulesSheet()->IsLastRow(gApp->mGameScreen->mLevel - 1))
				return aTime + aTime;
		}
	}
	return aTime;
}

// 0x451ae0
double GameScreen::GetPopDelay() const
{
	double aStart = GetRulesSheet()->GetDouble(mLevel - 1, RULE_COUNTDOWN_START);
	double anEnd = GetRulesSheet()->GetDouble(mLevel - 1, RULE_COUNTDOWN_END);
	double aSpeed = gApp->IsChallengeOpen() ? 1.2 : GetStats()->Get(STAT_COMBO);
	double aFraction;
	GetWaveActivePart(&aFraction);
	return ((anEnd - aStart) * aFraction + aStart) / aSpeed;
}

// 0x451b80
double GameScreen::GetRiseSpeed() const
{
	double aSpeed = gApp->IsChallengeOpen() ? 1.2 : GetStats()->Get(STAT_COMBO);
	double aStart = GetRulesSheet()->GetDouble(mLevel - 1, RULE_SPEED_START);
	double anEnd = GetRulesSheet()->GetDouble(mLevel - 1, RULE_SPEED_END);
	double aFraction;
	GetWaveActivePart(&aFraction);
	return ((anEnd - aStart) * aFraction + aStart) * aSpeed * 0.001;
}

// 0x451c20
double GameScreen::GetPitchValue() const
{
	double aSpeed = GetRiseSpeed();
	if (0.0 < aSpeed)
		return 1.0 / aSpeed * 0.01;
	return 0.01;
}

// 0x452a80: a kind goes into the list as many times as the count says; a negative count is a set of cycles (the bits of
// the count's magnitude) in which it goes in once
void GameScreen::AddKindToList(int theKind, int theCount)
{
	unsigned int aTimes = (unsigned int)theCount;
	if (theCount < 0)
		aTimes = (mCycle < 32) ? (unsigned int)((1 << (mCycle & 0x1f)) & -theCount) != 0 : 0;
	while (aTimes != 0)
	{
		mKindList.push_back(theKind);
		aTimes--;
	}
}

// 0x452b10: the list of the kinds that come in this level, each as often as the sheet's maximum columns say
void GameScreen::MakeKindList()
{
	mKindList.clear();
	RulesSheet* aSheet = GetRulesSheet();
	int aRow = mLevel - 1;
	AddKindToList(KIND_GNOME, aSheet->GetInt(aRow, RULE_MAX_GNOME_1));
	AddKindToList(KIND_WOMAN, aSheet->GetInt(aRow, RULE_MAX_WOMAN));
	AddKindToList(KIND_STRONG, aSheet->GetInt(aRow, RULE_MAX_STRONG));
	AddKindToList(KIND_BOMB, aSheet->GetInt(aRow, RULE_MAX_BOMB));
	AddKindToList(KIND_BONUS_DUCK, aSheet->GetInt(aRow, RULE_MAX_BONUS_DUCK));
	AddKindToList(KIND_STRAWBERRY, aSheet->GetInt(aRow, RULE_MAX_BONUS_STRAWBERRY));
	AddKindToList(KIND_BONUS_ICE_CREAM, aSheet->GetInt(aRow, RULE_MAX_BONUS_ICECREAM));
	AddKindToList(KIND_KING, aSheet->GetInt(aRow, RULE_MAX_KING));
	AddKindToList(KIND_BUBBLEHEAD, aSheet->GetInt(aRow, RULE_MAX_BUBBLEHEAD));
	AddKindToList(KIND_HIDE, aSheet->GetInt(aRow, RULE_MAX_HIDE));
	AddKindToList(KIND_SHOP, aSheet->GetInt(aRow, RULE_MAX_SHOP));
	AddKindToList(KIND_CHALLENGE, aSheet->GetInt(aRow, RULE_MAX_CHALLENGE));
	AddKindToList(KIND_BOUNCING, aSheet->GetInt(aRow, RULE_MAX_BOUNCING));
	AddKindToList(KIND_FLYING, aSheet->GetInt(aRow, RULE_MAX_FLYING));
	AddKindToList(KIND_FORKY_CHAP, aSheet->GetInt(aRow, RULE_MAX_FORKY_CHAP));
	AddKindToList(KIND_MULTIPLIER, aSheet->GetInt(aRow, RULE_MAX_MULTIPLIER_X5));
	AddKindToList(KIND_RABBIT, aSheet->GetInt(aRow, RULE_MAX_RABBIT));
	AddKindToList(KIND_CUPIDO, aSheet->GetInt(aRow, RULE_MAX_CUPIDO));
	AddKindToList(KIND_SILVER, aSheet->GetInt(aRow, RULE_MAX_SILVER));
	AddKindToList(KIND_CHEST, aSheet->GetInt(aRow, RULE_MAX_CHEST));
	AddKindToList(KIND_CLOVER, aSheet->GetInt(aRow, RULE_MAX_LUCKY_CLOVER));
	AddKindToList(KIND_SWITCHER, aSheet->GetInt(aRow, RULE_MAX_SWITCHER));
	AddKindToList(KIND_POWER_UP, aSheet->GetInt(aRow, RULE_MAX_POWERUP));
	AddKindToList(KIND_NAPOLEON, aSheet->GetInt(aRow, RULE_MAX_NAPOLEON));
}

// 0x452ec0: the list is put in a new order, the generator picking one at a time from what is left
void GameScreen::ShuffleKindList()
{
	std::vector<int> aLeft = mKindList;
	mKindList.clear();
	while (!aLeft.empty())
	{
		int anIndex = Rand() % (int)aLeft.size();
		mKindList.push_back(aLeft[anIndex]);
		aLeft.erase(aLeft.begin() + anIndex);
	}
}

// 0x452fc0: the kind of the gnome that comes up next
int GameScreen::PickCreatureKind()
{
	if (GetGameplay()->IsChallengeWaiting())
	{
		GetGameplay()->SetChallengeDone();
		return KIND_SAVE;
	}

	if (GetStats()->GetInt(STAT_KINGS_RELEASED) == 0 && GetWaveIndex() == 1)
	{
		if (gApp->mGameScreen->mMode == GAME_MODE_CLASSIC || gApp->mGameScreen->mMode == GAME_MODE_TOUGH_COOKIE)
		{
			if (GetRulesSheet()->IsLastRow(gApp->mGameScreen->mLevel - 1))
				return KIND_KING;
		}
	}

	int aKind;
	Creature* aKing = GetGameplay()->FindCreatureOfKind(KIND_KING);
	if (aKing == nullptr)
	{
		if ((int)mKindList.size() <= mKindIndex)
		{
			MakeKindList();
			ShuffleKindList();
			mCycle++;
			mKindIndex = 0;
		}
		aKind = mKindList[mKindIndex++];
		if (aKind == KIND_SHOP)
		{
			// a shop sign is up already: the list is used up so that it is made again
			if (GetGameplay()->CountCubbiesWithKind(KIND_SHOP) > 0)
			{
				if (mKindList.empty())
					mKindIndex = 0;
				else
					mKindIndex = (int)mKindList.size();
				return PickCreatureKind();
			}
		}
		else if (aKind == KIND_BOUNCING)
		{
			if (gLastKind != KIND_BOUNCING)
				MakeGnomeBouncing("");
			gLastKind = KIND_BOUNCING;
			return PickCreatureKind();
		}
		else if (aKind == KIND_FLYING)
		{
			if (gLastKind != KIND_FLYING)
				MakeGnomeFlying("");
			gLastKind = KIND_FLYING;
			return PickCreatureKind();
		}
	}
	else
		aKind = static_cast<GnomeKing*>(aKing)->GetItemKind();

	gLastKind = aKind;
	return aKind;
}
