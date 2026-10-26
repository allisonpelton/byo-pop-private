#ifndef __WORDHARMONY_HUD_H__
#define __WORDHARMONY_HUD_H__

// The playing screen's HUD (500 bytes, constructor 0x424040, vtable 0x482f88; one per mode, loaded from
// config\HUD.xml, HUD-solitaire.xml and HUD-freematch.xml by 0x423330): the list of words (a puzzle level's words to
// find, or the words found, with a button each to give a found word's letters back), the definition line, the
// counters and the buttons, whose actions it carries out itself.

#include "ScreenLayout.h"
#include <string>
#include <vector>

namespace engine
{
class TextItem;
}

class SaveWriter;

// A line of the word list (0x54 bytes)
class SavedGame;

struct FoundWord
{
	std::string mLetters;                 // +0x00: the balls' letters (wildcards as '?')
	std::string mWord;                    // +0x1c
	int mMultiplier;                      // +0x38
	int mPoints;                          // +0x3c
	bool mFound;                          // +0x40: a puzzle level's word found (its brush shown)
	std::vector<std::string> mChainWords; // +0x44: the chained words the word freed
};

class Hud : public engine::ScreenLayout
{
public:
	Hud();  // 0x424040

	void update(engine::UpdateInfo& theInfo) override;  // 0x70: 0x4238b0
	void buttonPress(std::string theName) override;     // 0x422850

	// 0x423330: the layout file theFile, its nodes looked up
	void load(std::string theFile);
	// 0x423db0: the word list emptied
	void reset();
	// 0x423ba0: a puzzle level's word to find added to the list
	void addWord(std::string theWord);
	// 0x423ca0: a found word added to the list, with the chained words it freed
	void addFound(const std::string& theLetters, const std::string& theWord, std::vector<std::string>& theChainWords,
		int thePoints, int theMultiplier);
	// 0x423e40: the list replaced by copies of theWords (a saved game's)
	void setWords(const std::vector<FoundWord*>& theWords);
	// 0x422380: word theIndex found, its brush faded in
	void markFound(int theIndex);
	// 0x423130: the longest word of the list (the first of that length)
	std::string longestWord();
	// 0x422450: the definition line
	void setDefinition(std::string theText);
	// 0x423260: the bonus word on the definition line
	void showBonusWord();
	// 0x422740: the word buttons enabled in solitaire, freematch's next ball looked up
	void setMode(int theMode);
	// 0x422220: the buttons enabled or not; theFade fades the menu buttons too, theAtOnce without taking time
	void setControlsEnabled(bool theEnabled, bool theFade, bool theAtOnce);
	// 0x423210: the wildcard and spawn buttons enabled; when not, the submit and hint buttons go too
	void setPlaying(bool thePlaying);
	// 0x422c40: the submit button shown or hidden
	void showSubmit(bool theShow);
	// 0x422d30: whether the hint button works
	void setHintReady(bool theReady);
	// 0x422f00: whether the wildcard button works
	void setWildcardReady(bool theReady);
	// 0x422340: freematch's next ball hidden, shown again half a second later
	void restartNextBall();
	// 0x422400: its part of the saved game (copies of the found words)
	void saveWords(SavedGame* theGame);
	void save(SaveWriter& theWriter);

private:
	// 0x4224b0: the last 14 words of the list into the word lines
	void showWords();
	// 0x4221c0
	void enableWords();
	// 0x4221f0
	void disableWords();
	// 0x422bb0: the editor's ball count buttons hidden, the balls left reset
	void hideBallButtons();
	// 0x422ff0: freematch's next ball shown, growing
	void showNextBall();

	int mMode;                              // +0xd4
	engine::TextItem* mDefinition;          // +0xd8: "definition"
	std::vector<FoundWord*> mWords;         // +0xdc
	engine::TextItem* mWordTexts[14];       // +0xec: "word1" to "word14"
	engine::Node* mBrushes[14];             // +0x124: "brush1"...: behind a found word
	engine::Node* mRecycles[14];            // +0x15c: "word1recycle"...: gives a found word's letters back
	engine::TextItem* mBallsRemaining;      // +0x194
	engine::FadeContainer* mSpawnBallText;  // +0x19c
	engine::Node* mQuitBtn;                 // +0x1a0
	engine::Node* mRetryBtn;                // +0x1a4
	engine::Node* mHelpBtn;                 // +0x1a8
	engine::Node* mPauseBtn;                // +0x1ac
	engine::FadeContainer* mMenuGroup;      // +0x1b0: "menuButtonGroup"
	engine::Node* mSubmitBtn;               // +0x1b4
	engine::Node* mHintBtn;                 // +0x1b8
	engine::Node* mWildcardBtn;             // +0x1bc
	engine::TextItem* mLevelText;           // +0x1c0: "level"
	engine::TextItem* mWildcardText;        // +0x1c4
	engine::TextItem* mHintCounter;         // +0x1c8
	engine::TextItem* mHintText;            // +0x1cc
	bool mSubmitShown;                      // +0x1d0
	bool mHintReady;                        // +0x1d1
	bool mWildcardReady;                    // +0x1d2
	engine::Node* mNewBall;                 // +0x1d4: freematch's next ball
	engine::TextItem* mSpawnText;           // +0x1d8: its letters
	engine::Node* mSpawnBtn;                // +0x1dc
	float mNextBallTime;                    // +0x1e0: till the next ball shows (or grows)
	float mFoundFade;                       // +0x1e4: a found word's brush fading in
	int mFoundIndex;                        // +0x1e8: that word
};

#endif
