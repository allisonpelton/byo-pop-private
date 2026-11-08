// The screens the application opens and closes: the game selector and, until each is translated, stubs for the
// screens it leads to. A stub logs "not translated" so a test run shows that a button was reached.

#include "AstroPopApp.h"
#include "MainMenu.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include <SDL.h>

using namespace Sexy;

// 0x411490: the game selector replaces whatever screen is up
void AstroPopApp::ShowMainMenu()
{
	if (mBoard != NULL)
	{
		mEngineLoop->Stop();
		mHyperspaceLoop->Stop();
	}
	if (mScreen64c != NULL)
	{
		mWidgetManager->RemoveWidget(mScreen64c);
		SafeDeleteWidget(mScreen64c);
		mScreen64c = NULL;
		// The original also tells the resource manager's page counter (0x48f860, "MoreGames") that the page is gone;
		// that is PopCap's ad statistics and is left out.
	}

	mMainMenu = new MainMenu(this);
	mMainMenu->Resize(0, 0, 640, 480);
	mWidgetManager->AddWidget(mMainMenu);
	mWidgetManager->SetFocus(mMainMenu);
	PlaySong(5);

	if (mBoard != NULL)
	{
		mBoard->mHasAlpha = true;
		mBoard->mHasTransparencies = true;
		mWidgetManager->BringToFront(mBoard);
	}
}

void AstroPopApp::StartGame(bool theContinue)
{
	SDL_Log("AstroPop: StartGame(%d) not translated yet (0x428c70)", theContinue ? 1 : 0);
}

void AstroPopApp::ShowCharSelector(int theMode)
{
	SDL_Log("AstroPop: ShowCharSelector(%d) not translated yet (0x411660)", theMode);
}

void AstroPopApp::ShowHallOfFame()
{
	SDL_Log("AstroPop: ShowHallOfFame not translated yet (0x4288a0)");
}

void AstroPopApp::ShowQuitDialog()
{
	SDL_Log("AstroPop: ShowQuitDialog not translated yet (0x422c00)");
}

void AstroPopApp::ShowUserSelect()
{
	SDL_Log("AstroPop: ShowUserSelect not translated yet (0x422870)");
}

void AstroPopApp::ShowOptions(bool theFromGame)
{
	SDL_Log("AstroPop: ShowOptions(%d) not translated yet (0x40bd70)", theFromGame ? 1 : 0);
}

void AstroPopApp::ShowContinueDialog(const std::string& thePilot, int theLevel)
{
	SDL_Log("AstroPop: the CONTINUE? dialog (pilot %s, level %d) not translated yet (0x411ba0)", thePilot.c_str(), theLevel);
}
