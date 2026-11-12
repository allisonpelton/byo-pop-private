#ifndef __WORDHARMONY_LEVEL_H__
#define __WORDHARMONY_LEVEL_H__

// A level as read from the level file (games/wordharmony/docs/WORKING_NOTES.md, "The game code as read"): its texts,
// the words to find with their definitions and bonuses, the rules (balls, time, the shortest word...), the
// obstacles and the starting balls ("bubbles"). The level file's writer (0x41c450) is the editor's and left out.

#include "Geometry.h"
#include "Xml.h"
#include <string>
#include <vector>

class Obstacle;

// A starting ball (0x28 bytes); with more than one letter, a chained word of balls. (0x41e3a0, a bubble of one
// letter, is only the editor's.)
struct Bubble
{
	Bubble();  // 0x41e380
	Bubble(const engine::Point& theOrigin, const std::string& theLetters, unsigned int theFlags);  // 0x41e420

	// 0x41e280: the flags a "special" attribute names ("bomb", "wildcard", "2x"...) added
	void setSpecial(const std::string& theSpecial);
	// 0x41e460: the flags as a "special" attribute
	static std::string specialText(unsigned int theFlags);
	// 0x41e5b0: its element in the level file and the saved game
	std::string toXml();

	engine::Point mOrigin;  // +0x00
	unsigned int mFlags;    // +0x08: the ball's special flags
	std::string mLetters;   // +0x0c
};

// 0x3e8 bytes. (The assignment, 0x415460, was the editor's backup: left out.)
class Level
{
public:
	Level(Level* thePrevious);      // 0x41c760: the rules carry on from the previous level
	Level(const Level& theLevel);   // 0x41c910: the obstacles cloned and the bubbles copied
	~Level();                       // 0x41c2f0

	// 0x41cc50: kept with a reference
	void addObstacle(Obstacle* theObstacle);

	std::string mIntro[3];         // +0x000
	std::string mWords[14];        // +0x054
	std::string mDefinitions[14];  // +0x1dc
	int mBonuses[14];              // +0x364
	int mBalls;                    // +0x39c
	int mTime;                     // +0x3a0: 60
	int mShortestWord;             // +0x3a4: 3
	bool mFreematch;               // +0x3a8
	int mBallCriteria;             // +0x3ac: 10
	float mSpawnDelay;             // +0x3b0: 5
	int mMaxBalls;                 // +0x3b4
	int mLongestPiece;             // +0x3b8: 2; solitaire deals pieces of 2 to this many letters
	int mPieces;                   // +0x3bc: how many pieces solitaire deals (set by the controller; the original's
	                               // constructors leave it unset, the port's start it at 0)
	engine::Point mGravity;        // +0x3c0: (0, 500)
	std::vector<Obstacle*> mObstacles;  // +0x3c8
	std::vector<Bubble*> mBubbles;      // +0x3d8
};

// The level file's reader (constructor 0x41ccd0; its handler part, with virtual bases in the original, at +0x18)
class LevelHandler : public engine::XmlHandler
{
public:
	// 0x41ccd0
	LevelHandler() : mLevels(nullptr), mLevel(nullptr) {}

	// 0x41ce30: the levels of theFile added to theLevels
	void load(std::string theFile, std::vector<Level*>* theLevels);

	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override;  // 0x41cf70

private:
	std::vector<Level*>* mLevels;  // +0x04
	Level* mLevel;                 // +0x08: the level being read
};

#endif
