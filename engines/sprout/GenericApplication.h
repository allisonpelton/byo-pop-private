#ifndef __SPROUT_GENERICAPPLICATION_H__
#define __SPROUT_GENERICAPPLICATION_H__

// The signal build's base of a game's application, the controller its run loop starts (Iggle Pop's Mac build names
// the class GenericApplication; constructor 0x46aed0 (pizzafrenzy), vtable 0x501cec (pizzafrenzy)). Its slots are the
// run loop's: configure (0x04), init (0x08), the screen saver's run and settings (0x0c and 0x10, which the games
// leave empty), shutdown (0x14); then the two volumes. The rest comes from the application's signals, which the
// game's init connects.

#include "App.h"
#include "Signal.h"

namespace engine
{

class GenericApplication : public Controller, public HasSlots
{
public:
	GenericApplication() : mHasSound(false), mHasMusic(false) {}  // 0x46aed0 (pizzafrenzy)
	~GenericApplication() override {}                             // 0x46af20 (pizzafrenzy)

	// 0x18: 0x46ae90 (pizzafrenzy): the sounds' volume, 1 on and 0 off (the sound system's category 0)
	virtual void setSoundEnabled(bool theEnabled);
	// 0x1c: 0x46ae50 (pizzafrenzy): the music's (category 1)
	virtual void setMusicEnabled(bool theEnabled);

protected:
	// 0x46ae00 (pizzafrenzy): the window's sound system (0x48ef60 (pizzafrenzy), DirectSound) given to the application;
	// without one the game has neither sounds nor music
	void initSound();

	bool mHasSound;  // +0x0c
	bool mHasMusic;  // +0x0d
};

} // namespace engine

#endif
