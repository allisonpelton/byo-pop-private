#include "MainMenuScreen.h"

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The buttons that start a game and the lines of help under them
const char* const NEW_GAME_ACTION = "newGame";
const char* const TIMED_GAME_ACTION = "timeGame";
const char* const NEW_GAME_HELPER = "newGameHelper";
const char* const TIMED_GAME_HELPER = "timedGameHelper";
// How long a line of help takes to show, and to go
const float HELPER_FADE_IN_SECONDS = 0.5f;
const float HELPER_FADE_OUT_SECONDS = 0.2f;
const float HELPER_OPACITY = 1.0f;
}

FadeContainer* MainMenuScreen::getHelper(const std::string& theButton)
{
	if (theButton == NEW_GAME_ACTION)
		return static_cast<FadeContainer*>(getNode(NEW_GAME_HELPER));
	if (theButton == TIMED_GAME_ACTION)
		return static_cast<FadeContainer*>(getNode(TIMED_GAME_HELPER));
	return nullptr;
}

// 0x423f20
void MainMenuScreen::hideHelpers()
{
	Node* aHelper = getNode(NEW_GAME_HELPER);
	if (aHelper != nullptr)
		aHelper->setVisible(false);
	aHelper = getNode(TIMED_GAME_HELPER);
	if (aHelper != nullptr)
		aHelper->setVisible(false);
}

// 0x423d00
void MainMenuScreen::buttonEnter(std::string theName)
{
	FadeContainer* aHelper = getHelper(theName);
	if (aHelper != nullptr)
		aHelper->fade(true, HELPER_FADE_IN_SECONDS, HELPER_OPACITY, false);
}

// 0x423e50
void MainMenuScreen::buttonExit(std::string theName)
{
	FadeContainer* aHelper = getHelper(theName);
	if (aHelper != nullptr)
		aHelper->fade(false, HELPER_FADE_OUT_SECONDS, HELPER_OPACITY, false);
}
