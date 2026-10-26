// port(variant): the watch's controls; see ReplayOverlay.h. Not part of the original game.

#include "ReplayOverlay.h"
#include "WinDMApp.h"
#include "WatchController.h"
#include "Board.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "KeyCodes.h"

using namespace Sexy;

ReplayOverlay::ReplayOverlay(WinDMApp* theApp)
{
	mApp = theApp;
	mHasAlpha = true;
	mHasTransparencies = true;
	mWantsFocus = true;
	Resize(OVERLAY_X, 0, theApp->mWidth - OVERLAY_X, theApp->mHeight);
}

SexyString ReplayOverlay::BannerText()
{
	WatchController* aWatch = mApp->mWatch;
	if (aWatch == NULL)
		return _S("");
	if (aWatch->HasEnded())
		return mApp->GetString("REPLAY_BANNER_END", _S("REPLAY END"));
	if (aWatch->IsStepping())
		return mApp->GetString("REPLAY_BANNER_NEXT_MOVE", _S("REPLAY NEXT MOVE"));
	if (aWatch->IsPaused())
		return mApp->GetString("REPLAY_BANNER_PAUSED", _S("REPLAY PAUSED"));
	return mApp->GetString("REPLAY_BANNER", _S("REPLAY")) + StrFormat(_S(" %dx"), 1 << aWatch->GetSpeed());
}

void ReplayOverlay::Update()
{
	Widget::Update();
	SexyString aText = BannerText();
	if (aText != mDrawnText)
	{
		mDrawnText = aText;
		MarkDirtyFull();
	}
}

// Dark strips over the board's frame, which they barely cross: the banner along the top, the keys along the bottom
// while paused
void ReplayOverlay::Draw(Graphics* g)
{
	if (mDrawnText.empty())
		return;
	Font* aFont = mApp->mMainFont;
	g->SetFont(aFont);
	int aCenterX = Board::BOARD_CENTER_X - mX;
	int aStripHeight = aFont->GetHeight() + 2;
	g->SetColor(Color(0, 0, 0, 170));
	int aWidth = aFont->StringWidth(mDrawnText);
	g->FillRect(aCenterX - aWidth / 2 - 10, BANNER_Y, aWidth + 20, aStripHeight);
	g->SetColor(Color(255, 255, 0));
	g->DrawString(mDrawnText, aCenterX - aWidth / 2, BANNER_Y + 1 + aFont->GetAscent());
	if ((mApp->mWatch != NULL) && mApp->mWatch->IsPaused() && !mApp->mWatch->HasEnded())
	{
		SexyString aKeys = mApp->GetString("REPLAY_KEYS", _S("SPACE: PLAY  1-4: SPEED  N: NEXT MOVE  ESC: BACK"));
		aWidth = aFont->StringWidth(aKeys);
		g->SetColor(Color(0, 0, 0, 170));
		g->FillRect(aCenterX - aWidth / 2 - 10, KEYS_Y, aWidth + 20, aStripHeight);
		g->SetColor(Color(255, 255, 255));
		g->DrawString(aKeys, aCenterX - aWidth / 2, KEYS_Y + 1 + aFont->GetAscent());
	}
}

void ReplayOverlay::KeyChar(SexyChar theChar)
{
	WatchController* aWatch = mApp->mWatch;
	if ((aWatch == NULL) || !aWatch->IsWatching())
		return;
	if (theChar == ' ')
		aWatch->TogglePause();
	else if ((theChar >= '1') && (theChar < '1' + WatchController::NUM_SPEEDS))
		aWatch->SetSpeed(theChar - '1');
	else if ((theChar == 'n') || (theChar == 'N'))
		aWatch->StepToMove();
}

void ReplayOverlay::KeyDown(KeyCode theKey)
{
	if ((theKey == KEYCODE_ESCAPE) && (mApp->mWatch != NULL) && mApp->mWatch->IsWatching())
		mApp->EndWatch();
}

void ReplayOverlay::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if ((theBtnNum == 0) && (mApp->mWatch != NULL) && mApp->mWatch->IsWatching())
		mApp->mWatch->TogglePause();
}
