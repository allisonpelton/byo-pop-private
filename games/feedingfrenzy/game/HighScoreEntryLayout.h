#ifndef __FEEDINGFRENZY_HIGHSCOREENTRYLAYOUT_H__
#define __FEEDINGFRENZY_HIGHSCOREENTRYLAYOUT_H__

// The screen where a name is typed for a score that has made the list (HighScoreEntryLayout, 0x184 bytes, constructor
// 0x4342b0; the file config\highScoreEntry.xml). It takes the keys while it is shown (it is a key
// handler the controller adds to the application, +0x14c).

#include "Node.h"
#include "ScreenLayout.h"

#include <string>

namespace feedingfrenzy
{

class HighScoreEntryLayout : public engine::ScreenLayout, public engine::KeyHandler
{
public:
	HighScoreEntryLayout();  // 0x4342b0

	// 0x433ff0: the name that is put in the box to begin with, and the box cleared for typing
	void setUp(const std::string& theDefaultName);
	// 0x42d3c0: the name that has been typed
	std::string getEnteredName();

	bool keyDown(int theKey) override;
	bool keyUp(int theKey) override;
};

} // namespace feedingfrenzy

#endif
