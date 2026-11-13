#ifndef __HAMMERHEADS_SCREENS_H__
#define __HAMMERHEADS_SCREENS_H__

// What Application.cpp needs of the screens and dialogs (0x44c170-0x462d00): the headers of the classes that are translated, one
// file each, and the declarations of the rest, which ScreenStubs.cpp holds plain stand-ins of until they are.

#include <string>
#include <vector>

#include "ButtonListener.h"
#include "Color.h"
#include "Dialog.h"
#include "CreateUserDialog.h"
#include "GameMenu.h"
#include "GameOverDialog.h"
#include "GameWonWidget.h"
#include "HighScoreDisplayDialog.h"
#include "HowToPlayWidget.h"
#include "LevelIntroWidget.h"
#include "LoadingScreen.h"
#include "MainMenu.h"
#include "NcMovingDialog.h"
#include "ReadyGoDialog.h"
#include "ResurrectScreenWidget.h"
#include "TrophyRoomWidget.h"
#include "UserDialog.h"
#include "ZoomWipeWidget.h"
#include "Widget.h"

namespace Sexy
{

class Image;
class GameScreen;

// 0x45c2e0: the options box (not translated yet: ScreenStubs.cpp has a stand-in that does nothing)
void MakeOptionsDialog();

// 0x45c730: the box that asks whether to quit (not translated yet: ScreenStubs.cpp has a stand-in that quits at once)
void MakeQuitDialog();

}

#endif
