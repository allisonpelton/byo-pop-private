#include "PauseScreen.h"
#include "Board.h"
#include "GameText.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x42bc50
PauseScreen::PauseScreen(Board* theBoard)
{
	mBoard = theBoard;
	theBoard->mSuspended = true;
	mBoard->mApp->mMusicInterface->SetVolume(0.0);
	mBoard->mApp->mSoundManager->SetVolume(0.0);
	Resize(0, 0, 640, 480);
}

// 0x42bce0
PauseScreen::~PauseScreen()
{
	mBoard->mApp->mMusicInterface->SetVolume(mBoard->mApp->GetMusicVolume());
	mBoard->mApp->mSoundManager->SetVolume(mBoard->mApp->GetSfxVolume());
	// The game stays still while the mouse is off the field
	if (mIsOver)
		mBoard->mSuspended = false;
	mBoard->mPaused = false;
	mBoard->mMouseDown = false;
	mBoard->mApp->mWidgetManager->RemoveWidget(this);
	mBoard->mApp->mWidgetManager->SetFocus(mBoard);
}

// 0x42be40
void PauseScreen::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, 192));
	g->FillRect(0, 0, 640, 480);
	g->SetFont(mBoard->mApp->mFonts[FONT_NORMAL]);
	WriteCenteredLine(g, 240, GT(0x0052ee30), Color(255, 255, 255, 255), Color(0, 0, 0, 255), Point(1, 2));
}

// 0x42bde0
void PauseScreen::KeyUp(KeyCode theKey)
{
	Widget::KeyUp(theKey);
	if (theKey == KEYCODE_SPACE)
		delete this;
}

// 0x42bdd0
void PauseScreen::MouseDown(int x, int y, int theClickCount)
{
	delete this;
}
