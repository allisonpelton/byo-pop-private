#ifndef __PIZZAFRENZY_GAMESCREEN_H__
#define __PIZZAFRENZY_GAMESCREEN_H__

// The screen the game is played on (0x220 bytes, constructor 0x434be0; the application keeps it at +0x114). It is
// translated next with the game; the city select draws the cities' postcards from it (CitySelectScreen.h), and the
// loading state does not make it yet, so the application's field is empty until then. The class is only what the
// city select reaches.

#include "GenericScreen.h"

namespace pizzafrenzy
{

class GameScreen : public engine::GenericScreen
{
public:
	GameScreen() {}  // 0x434be0

	// +0x20c: a node of the screen the city select hides while it draws the postcards, and shows again after
	engine::Ref<engine::Node> mHiddenWhileDrawing;
};

} // namespace pizzafrenzy

#endif
