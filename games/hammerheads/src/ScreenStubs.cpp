// Plain stand-ins for the screens and dialogs, so that the game links and its levels can be played while the original's
// classes are translated (Screens.h). Each block is deleted when the real class arrives in its own file. Nothing here is a
// translation: the stand-ins do the least that lets the rest of the game run (a menu that starts a game on a click, wipes
// that end at once).

#include "Application.h"

#include "Common.h"
#include "Dialog.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "Res.h"
#include "Screens.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

//////////////////////////////////////////////////////////////////////////
// The options and the quit box (the main menu's and the game menu's buttons)

void Sexy::MakeOptionsDialog() {}

void Sexy::MakeQuitDialog()
{
	gApp->Quit();
}
