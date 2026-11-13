// Plain stand-ins for the screens, dialogs and the player roster, so that the game links and its levels can be played while
// the original's classes are translated (Screens.h, UserManager.h, ShopScreen.h, ChallengeScreen.h). Each block is deleted
// when the real class arrives in its own file. Nothing here is a translation: the stand-ins do the least that lets the
// rest of the game run (a menu that starts a game on a click, a roster of one player, wipes that end at once).

#include "Application.h"

#include "ChallengeScreen.h"
#include "Common.h"
#include "Dialog.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "Res.h"
#include "SavedGames.h"
#include "Screens.h"
#include "ShopScreen.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

//////////////////////////////////////////////////////////////////////////
// The player roster: one player, nothing saved

namespace
{
User* gStubUser = nullptr;
}

bool User::HasTrophy(int) const { return false; }
void User::AwardTrophy(int) {}
bool User::HasSavedGame() const { return false; }
void User::ClearSavedGame() {}
int User::GetSavedGameLevel() const { return 0; }
int User::GetSavedGameScore() const { return 0; }
std::string User::GetSavedGameTime() const { return std::string(); }
void User::LoadSavedGame() {}
void User::StoreGameSnapshot() {}
void User::SaveProfile() {}
int User::GetRestartPointCount(int) const { return 0; }
void User::AddRestartPoint() {}
void User::ResumeFromRestartPoint(int) {}

UserManager::UserManager()
{
	gStubUser = new User();
	gStubUser->mName = "Player";
	gStubUser->mId = 1;
	gStubUser->mShowHowToPlay = false;
	gStubUser->mToughUnlocked = false;
	gStubUser->mToughAnnounced = false;
}

UserManager::~UserManager()
{
	delete gStubUser;
	gStubUser = nullptr;
}

User* UserManager::GetCurrentUser() { return gStubUser; }
User* UserManager::FindUser(const std::string&) { return gStubUser; }
void UserManager::SetCurrentUser(int) {}
void UserManager::DeleteUser(int) {}
void UserManager::SaveRoster() {}

void Sexy::StoreGameSnapshot() {}

//////////////////////////////////////////////////////////////////////////
// The loading screen and the main menu

LoadingScreen::LoadingScreen()
{
	mLoaded = false;
}

void LoadingScreen::Begin() {}

void LoadingScreen::LoadingFinished()
{
	mLoaded = true;
}

void LoadingScreen::Update()
{
	if (mLoaded)
		gApp->LoadingScreenDone();
}

void LoadingScreen::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0));
	g->FillRect(0, 0, mWidth, mHeight);
	double aProgress = gApp->GetLoadingThreadProgress();
	g->SetColor(Color(255, 172, 29));
	g->FillRect(100, mHeight - 100, (int) ((mWidth - 200) * aProgress), 20);
}

MainMenu::MainMenu() {}
void MainMenu::SetPlayButtonImage(Image*) {}
void MainMenu::UserChanged() {}

void MainMenu::Draw(Graphics* g)
{
	g->SetColor(Color(40, 40, 40));
	g->FillRect(0, 0, mWidth, mHeight);
	g->SetFont(FONT_PTBANANASPLIT27);
	g->SetColor(Color(255, 255, 255));
	g->DrawString("Click: classic   Right click: marathon   Shift click: tough cookie", 20, mHeight / 2);
}

void MainMenu::ButtonPress(int) {}

void MainMenu::MouseDown(int, int, int theClickCount)
{
	gApp->NewGame(GAME_MODE_CLASSIC);
}

//////////////////////////////////////////////////////////////////////////
// The pages between screens

LevelIntroWidget* Sexy::MakeLevelIntro(GameScreen*) { return nullptr; }

TrophyRoomWidget::TrophyRoomWidget(int, const std::vector<int>&) {}
GameWonWidget* Sexy::MakeGameWon()
{
	gApp->GameEndToMainMenu();
	return nullptr;
}
ResurrectScreenWidget::ResurrectScreenWidget(bool) {}

ZoomWipeWidget::ZoomWipeWidget()
{
	mCallback = nullptr;
}

void ZoomWipeWidget::Setup(Image*, Image*, const Color&, void (*theCallback)())
{
	mCallback = theCallback;
}

void ZoomWipeWidget::Draw(Graphics*) {}

void ZoomWipeWidget::Update()
{
	// the picture covers the screen at once: the callback runs, then the wipe ends
	void (*aCallback)() = mCallback;
	mCallback = nullptr;
	if (aCallback != nullptr)
	{
		aCallback();
		gApp->EndZoomWipe();
	}
}

//////////////////////////////////////////////////////////////////////////
// The dialogs

NcMovingDialog::NcMovingDialog(int, Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
	const std::string& theDialogHeader, const std::string& theDialogLines, const std::string& theDialogFooter, int theButtonMode) :
	Dialog(theComponentImage, theButtonComponentImage, theId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode)
{
	mEscapeResult = Dialog::ID_NO;
	mSettled = false;
}

void NcMovingDialog::SetEscapeResult(int theResult)
{
	mEscapeResult = theResult;
}

void NcMovingDialog::Update()
{
	Dialog::Update();
	if (!mSettled)
	{
		int aTarget = (gApp->mHeight - mHeight) / 2;
		mY += 25;
		if (mY >= aTarget)
		{
			mY = aTarget;
			mSettled = true;
		}
		MarkDirty();
	}
}

void Sexy::LayoutInfoDialog(Dialog*, int) {}

HighScoreDisplayDialog::HighScoreDisplayDialog() :
	Dialog(IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_HIGH_SCORE_TABLE, true, "High Scores", "", "OK", Dialog::BUTTONS_FOOTER)
{
	mShown = false;
}
void HighScoreDisplayDialog::Prepare() {}
void HighScoreDisplayDialog::RecordScore(const std::string&) {}
HighScoreDisplayDialog* Sexy::MakeHighScoreDisplay()
{
	HighScoreDisplayDialog* aDialog = new HighScoreDisplayDialog();
	gApp->AddDialog(DIALOG_HIGH_SCORE_TABLE, aDialog);
	return aDialog;
}

void Sexy::MakeGameMenu()
{
	gApp->DoDialog(DIALOG_GAME_MENU, true, "Paused", "", "Resume", Dialog::BUTTONS_FOOTER);
}
Dialog* Sexy::MakeUserDialog() { return nullptr; }
Dialog* Sexy::MakeCreateUserDialog(bool) { return nullptr; }
void Sexy::SetCreateUserText(Dialog*, const std::string&) {}
std::string Sexy::GetUserDialogSelection(Dialog*) { return "Player"; }
void Sexy::RefreshUserDialog(Dialog*, const std::string&) {}

void Sexy::MakeGameOverDialog()
{
	gApp->GameOverDone();
}

void Sexy::MakeReadyGoDialog(const std::string&, const std::string&)
{
	// the real box waits for a click, and the game goes on when it is closed
	gApp->PauseGame(false);
}

//////////////////////////////////////////////////////////////////////////
// The shop and the bonus round

ShopScreen::ShopScreen() {}
ShopScreen::~ShopScreen() {}
void ShopScreen::Opened() { gApp->LeaveShop(); }
void ShopScreen::Closing() {}
int ShopScreen::GetCreatureKind() { return 0; }
void ShopScreen::ShowRefusal(int) {}
void ShopScreen::ShowThanks(const std::string&) {}

ChallengeScreen::ChallengeScreen() {}
ChallengeScreen::~ChallengeScreen() {}
void ChallengeScreen::Opened(int) { gApp->LeaveChallenge(); }
void ChallengeScreen::Closing() {}
void ChallengeScreen::StartChallenge() {}
void ChallengeScreen::GotFocus() {}
void ChallengeScreen::LostFocus() {}
int ChallengeScreen::GetCreatureKind() { return 0; }
