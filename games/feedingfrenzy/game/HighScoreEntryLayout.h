#ifndef __FEEDINGFRENZY_HIGHSCOREENTRYLAYOUT_H__
#define __FEEDINGFRENZY_HIGHSCOREENTRYLAYOUT_H__

// The screen where a name is typed for a score that has made the list (HighScoreEntryLayout, 0x184 bytes, constructor
// 0x4342b0; the file config\highScoreEntry.xml). It takes the keys while it is shown (it is a key
// handler the controller adds to the application, +0x14c).

#include "Node.h"
#include "ScreenLayout.h"

#include <string>

namespace engine
{
class TextItem;
}

namespace feedingfrenzy
{

class HighScoreEntryLayout : public engine::ScreenLayout, public engine::KeyHandler
{
public:
	HighScoreEntryLayout();  // 0x4342b0

	// 0x433ff0: the name that is put in the box to begin with, with the game's score shown above it
	void setUp(const std::string& theDefaultName);
	// 0x433ff0: the same for a score that is given
	void setUp(const std::string& theDefaultName, int theScore);
	// 0x42d3c0: the name that has been typed
	std::string getEnteredName();

	// 0x4340d0: a key that makes a letter goes on the end of the name (up to fifteen letters), backspace takes the last
	// letter off, and Enter ends the entry with the action "highScoreEntryDone"; any of them is taken
	bool keyDown(int theKey) override;
	// 0x433fd0: the keys that make a letter are taken
	bool keyUp(int theKey) override;

private:
	std::string mName;           // +0x154: the name as typed
	int mScore;                  // +0x170: the score that is entered
	engine::TextItem* mNameText; // +0x178: the node that shows the name; not counted
};

} // namespace feedingfrenzy

#endif
