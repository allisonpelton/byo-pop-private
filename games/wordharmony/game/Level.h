#ifndef __WORDHARMONY_LEVEL_H__
#define __WORDHARMONY_LEVEL_H__

// A level as read from the level file (games/wordharmony/docs/WORKING_NOTES.md, "The game code as read"): its texts,
// the words to find with their definitions and bonuses, the rules (balls, time, the shortest word...), the
// obstacles and the starting balls ("bubbles"). The reader (0x41cf70) is not ported yet.

#include "Geometry.h"
#include <string>
#include <vector>

class Obstacle;

// A starting ball (0x28 bytes, constructor 0x41e3a0); with more than one letter, a chained word of balls
struct Bubble
{
	Bubble(const engine::Point& theOrigin, unsigned int theFlags, char theLetter);  // 0x41e3a0

	engine::Point mOrigin;  // +0x00
	unsigned int mFlags;    // +0x08: the ball's special flags
	std::string mLetters;   // +0x0c
};

// 0x3e8 bytes. The copy (0x415460, the editor's backup) copies the obstacle and bubble pointers, not what they
// point to, as the default assignment does.
class Level
{
public:
	Level(Level* thePrevious);  // 0x41c760: the rules carry on from the previous level
	~Level();                   // 0x41c2f0

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
	int mPieces;                   // +0x3bc: how many pieces solitaire deals
	engine::Point mGravity;        // +0x3c0: (0, 500)
	std::vector<Obstacle*> mObstacles;  // +0x3c8
	std::vector<Bubble*> mBubbles;      // +0x3d8
};

#endif
