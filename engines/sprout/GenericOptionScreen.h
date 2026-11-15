#ifndef __SPROUT_GENERICOPTIONSCREEN_H__
#define __SPROUT_GENERICOPTIONSCREEN_H__

// The signal build's options screen (Pizza Frenzy's; 0x1dc bytes, constructor 0x466570 (pizzafrenzy)): check boxes
// for the sound, the music and the full screen mode, which change the application and the player's settings, and the
// version text. A game's screen derives from it for the options of its own.
//
// Left out: the original's link texts (the settings' link, which only served the outbound links).

#include "GenericApplication.h"
#include "GenericGameOptions.h"
#include "GenericScreen.h"

namespace engine
{

class GenericOptionScreen : public GenericScreen
{
public:
	// 0x17c: 0x4669d0 (pizzafrenzy): the check boxes show the settings (a box of a kind the application has none of
	// is off and dimmed), the actions go to action, and the version shows
	virtual void setup(GenericApplication* theApp, GenericGameOptions* theSettings);
	// The end of 0x4669d0 (pizzafrenzy): the text of the node "VersionText"
	void setVersionText(const std::string& theText);
	// 0x180: the game's action handler (0x466690 (pizzafrenzy) is the generic one): the sound, music and full screen
	// check boxes flip the setting, which is saved
	virtual void action(std::string theAction);

protected:
	void onAction(std::string theAction) { action(theAction); }  // 0x43b460 (pizzafrenzy), the thunk to slot 0x180

	Ref<GenericGameOptions> mSettings;  // +0x1d4
	GenericApplication* mApp;           // +0x1d8
};

} // namespace engine

#endif
