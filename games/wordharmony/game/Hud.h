#ifndef __WORDHARMONY_HUD_H__
#define __WORDHARMONY_HUD_H__

// The playing screen's HUD (500 bytes, constructor 0x424040; one per mode, loaded from config\HUD.xml,
// HUD-solitaire.xml and HUD-freematch.xml by 0x423330). Its code (0x422220-0x423e40) is not ported yet: only what
// the ball system calls is declared, with the names it will keep once read.

#include "ScreenLayout.h"
#include <string>

class SaveWriter;

class Hud : public engine::ScreenLayout
{
public:
	Hud();  // 0x424040

	// 0x423330
	void load(std::string theFile);
	// 0x423db0
	void reset();
	// 0x423ba0: adds a word of the level to the list
	void addWord(std::string theWord);
	// 0x422450: the definition line
	void setDefinition(std::string theText);
	// 0x422740
	void setMode(int theMode);
	// 0x422220 (not read yet): a ball was grabbed (theReleased false) or let go
	void grabChanged(bool theReleased, bool theB, bool theC);
	// 0x422340 (not read yet; called whenever the balls or the queue change)
	void refresh();
	// 0x422d30 (not read yet): whether the hint button works
	void setHintReady(bool theReady);
	// 0x423210 (not read yet)
	void setPlaying(bool thePlaying);
	// 0x422400: its part of the saved game
	void save(SaveWriter& theWriter);
};

#endif
