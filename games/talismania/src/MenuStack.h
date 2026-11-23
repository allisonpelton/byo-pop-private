#ifndef __TALISMANIA_MENUSTACK_H__
#define __TALISMANIA_MENUSTACK_H__

// The menu stack (MenuStack, constructor 0x5e98e0, push 0x5ea050, the darkening 0x5e9d40): the screens shown over the
// game in a scene's menu layers, three lists of them (normal, popup and modal), the dark layers beneath them fading in
// while any is up. The original has one stack for the sky and one for the land.
// Translated so far, from the push and its helpers: a screen put in the layer of its kind and shown, popped by being
// hidden, and the dark layers fading to their opacity over their time while a screen is up. Not yet: the modal list's
// own layer (taken to be the normal one), the second dark level's exact rule (0x5e9d40's two levels are taken as
// "a normal or modal screen is up" and "a popup is up"), the camera's input lock (0x492570) and the hooks the
// original calls on each screen (slots 0x148 and 0x160).

#include "AtlantisScene.h"
#include "GenericScreen.h"

#include <list>
#include <string>

class MenuStack
{
public:
	// The names of the scene layers: the one normal and modal screens go in, the one popups go in, and the dark layers
	// under each
	MenuStack(AtlantisScene* theScene, const std::string& theLayer, const std::string& thePopupLayer,
			  const std::string& theDarkenLayer, const std::string& theDarkenPopupLayer);

	// 0x5ea050: the screen put in its layer (once) and shown, on top of its list
	void push(engine::GenericScreen* theScreen, bool thePopup = false);
	// The screen hidden and taken from its list; it stays in its layer, invisible
	void pop(engine::GenericScreen* theScreen);
	// The dark layers moved toward where the lists put them
	void update(double theElapsed);
	bool isEmpty() const { return mScreens.empty() && mPopups.empty(); }
	bool contains(engine::GenericScreen* theScreen) const;

private:
	// A dark layer (a rectangle filled black) and where it is going
	struct Dark
	{
		engine::Component* mLayer = nullptr;
		float mTime = 0.6f;
		float mOpacity = 0.4f;
	};
	Dark makeDark(const std::string& theName) const;
	static void fade(Dark& theDark, bool theOn, double theElapsed);

	AtlantisScene* mScene;
	engine::Container* mLayer;
	engine::Container* mPopupLayer;
	Dark mDarken;
	Dark mDarkenPopup;
	std::list<engine::Ref<engine::GenericScreen>> mScreens;
	std::list<engine::Ref<engine::GenericScreen>> mPopups;
};

#endif
