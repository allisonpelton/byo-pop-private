// port: the bot behind the move test command (Bot.h). A word is made of letter balls that touch in a row or a column
// (WordBallSystem::scoreGroup); the bot builds rows. It picks a word the free letters can make (the level's own words in
// RELAX, the hint list's words otherwise), a resting ball with one of its letters as the anchor, and the places beside
// the anchor where the word's other letters go; each move drags one ball to its place and lets it fall the last few
// pixels, as a player does. Each call looks at where the balls are now, so a ball that rolled away is carried again; a
// plan that keeps failing is dropped and its word and anchor are not tried again in the level. When a word lies
// ready, a right click submits it (RELAX scores its words by itself). Outside play it answers the screens that stand
// between levels (the help's PLAY GAME, the level panel's CONTINUE and so on), so a driver can play a whole game.

#include "Bot.h"
#include "engine/App.h"
#include "WordHarmonyApp.h"
#include "game/Game.h"
#include "game/Hud.h"
#include "game/Obstacles.h"
#include "game/WordBallSystem.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

using namespace engine;
using Sexy::BotAction;
using Sexy::BotStep;

namespace
{

// The field in the ball system's coordinates (the floor of the freematch water's 440)
const float FIELD_W = 440.0f;
const float FIELD_H = 440.0f;

struct BotBall
{
	int mIndex;
	char mLetter;  // '?' for a wildcard
	float x, y, r;
	bool mUsable;  // a free letter ball the mouse can carry
	bool mChained; // a free letter ball of a chain: a letter of a word, never carried (the chain would come along)
	bool mResting;
	bool mFixed;   // a letter fixed in stone: never carried, but a letter of a word where it lies
	bool mLiftable; // the mouse can carry it somewhere: its centre is in a region of the grid
};

int regionAt(float x, float y);

struct Plan
{
	std::string mWord;
	int mAnchor = -1;               // the ball index
	int mAnchorAt = 0;              // its place in the word
	float mAnchorX = 0, mAnchorY = 0;
	std::vector<float> mSlotX;      // each letter's place (the anchor's own included)
	std::vector<float> mSlotY;      // each place's height: one for a row, down the column for a column (RELAX)
	bool mColumn = false;
	float mShift = 0;               // the row moved along from the anchor (a chain's word): the places stay put
	int mDrags = 0;
	int mReady = 0;                 // calls with every letter in place
	int mLastBall = -1;             // the ball of the last drag answered, and the frame its input and wait end: the
	int mLastUntil = 0;             // same answer again before then (a driver asking before it made it) counts once
};

struct BotMemory
{
	int mMode = -1;
	int mLevel = -1;
	int mScore = -1;
	Plan mPlan;
	bool mHavePlan = false;
	std::set<std::string> mFailed;  // "<word>@<anchor index>@<anchor x / 22>": tried again once the anchor has moved
	std::map<std::string, int> mFailures; // each word's failed plans: given up after eight
	int mHeldAsks = 0;              // asks in a row with a ball on the mouse
	std::map<int,int> mFlips;       // each chain's flips this level (by its first ball): given up after three
};

BotMemory gMemory;
}

// the bot's plans on stderr: WH_BOTDEBUG=1, or the test command "botdebug on"
bool Sexy::gBotDebug = std::getenv("WH_BOTDEBUG") != nullptr;

namespace
{
int gDebugBall = -1;
float gDebugX = 0, gDebugY = 0;

Point toScreen(WordBallSystem* theBalls, float x, float y)
{
	Point anOrigin(0.0f, 0.0f);
	theBalls->toLocal(anOrigin);
	return Point(x - anOrigin.x, y - anOrigin.y);
}

BotAction clickMove(const char* theWhat, int x, int y, int theUpdates)
{
	BotAction anAction(theWhat);
	anAction.mSteps = { { BotStep::MOVE, x, y, 0 }, { BotStep::WAIT, 0, 0, 2 }, { BotStep::CLICK, x, y, 0 } };
	anAction.mUpdates = theUpdates;
	return anAction;
}

std::vector<BotBall> readBalls(Game* theGame)
{
	WordBallSystem* aSystem = theGame->mBallSystem;
	std::vector<BotBall> aBalls;
	// every ball counts, under TSUNAMI's water too: the water only dims and slows a ball (0x41b760), it can still
	// be picked and make a word
	float aWaterTop = FIELD_H + 100.0f;
	for (unsigned int i = 0; i < aSystem->mCount; i++)
	{
		Ball* aBall = aSystem->getBall(i);
		if (aBall->mState == BALL_GONE)
			continue;
		BotBall b;
		b.mIndex = (int)i;
		b.x = aBall->getPosition()->x;
		b.y = aBall->getPosition()->y;
		b.r = aBall->mRadius;
		b.mLetter = (aBall->mSpecial & BALL_WILDCARD) != 0 ? '?' : (char)std::toupper((unsigned char)aBall->mLetter);
		unsigned int aHeavy = BALL_BLOCK | BALL_HEAVY | BALL_STONE | BALL_BREAKABLE | BALL_X2 | BALL_X3 | BALL_X4;
		b.mUsable = aBall->mState == BALL_FREE && !aBall->mFixed && aBall->mChain == nullptr && (aBall->mSpecial & aHeavy) == 0
			&& std::isalpha((unsigned char)b.mLetter | 0x20) && b.y <= aWaterTop && b.y > 0.0f;
		if (b.mLetter == '?')
			b.mUsable = aBall->mState == BALL_FREE && !aBall->mFixed && aBall->mChain == nullptr && b.y <= aWaterTop;
		b.mChained = aBall->mState == BALL_FREE && !aBall->mFixed && aBall->mChain != nullptr && b.y <= aWaterTop;
		float dx = b.x - aBall->mOld.x, dy = b.y - aBall->mOld.y;
		b.mResting = b.mFixed || dx * dx + dy * dy < (b.mChained ? 4.0f : 0.25f);
		// a letter fixed in place: a stone, or a block with a letter on it (RELAX's "fixed in stone")
		b.mFixed = aBall->mState == BALL_FREE && (aBall->mFixed || (aBall->mSpecial & (BALL_STONE | BALL_BREAKABLE)) != 0)
			&& (aBall->mSpecial & (BALL_BLOCK | BALL_HEAVY)) == 0 && std::isalpha((unsigned char)b.mLetter | 0x20);
		b.mLiftable = b.mUsable && regionAt(b.x, b.y) >= 0;
		aBalls.push_back(b);
	}
	return aBalls;
}

bool letterFits(char theWanted, char theLetter)
{
	return theWanted == '?' || theLetter == '?' || theWanted == theLetter;
}

// Something under a place for a ball of radius r at (x, y): a ball, an obstacle or the floor
bool supported(WordBallSystem* theSystem, const std::vector<BotBall>& theBalls, float x, float y, float r)
{
	if (y + r >= FIELD_H - 3.0f)
		return true;
	for (const BotBall& b : theBalls)
	{
		if (std::fabs(b.x - x) < r * 1.2f && b.y - y > r && b.y - y < r + b.r + 6.0f)
			return true;
	}
	if (theSystem->mObstacles != nullptr)
	{
		for (Obstacle* anObstacle : *theSystem->mObstacles)
		{
			Point p = *anObstacle->getPosition();
			if (std::fabs(p.x - x) < r + 4.0f && p.y - y > r && p.y - y < r + 34.0f)
				return true;
		}
	}
	return false;
}

bool inField(float x, float y, float r)
{
	// a place at most 4 px into the wall: a ball let go there rests against it, still within a place's 7 px
	return x >= r - 4.0f && x <= FIELD_W - r + 4.0f && y >= r + 30.0f;
}

// The ball in a place for a ball of radius r at (x, y) (the one nearest it), or null
const BotBall* occupant(const std::vector<BotBall>& theBalls, float x, float y, float r, int theAnchor)
{
	const BotBall* aFound = nullptr;
	float aNearest = 0.0f;
	for (const BotBall& b : theBalls)
	{
		if (b.mIndex == theAnchor)
			continue;
		float dx = b.x - x, dy = b.y - y;
		float d2 = dx * dx + dy * dy;
		if (d2 < (r + b.r - 3.0f) * (r + b.r - 3.0f) && (aFound == nullptr || d2 < aNearest))
		{
			aFound = &b;
			aNearest = d2;
		}
	}
	return aFound;
}

bool clearOfObstacles(WordBallSystem* theSystem, float x, float y, float r)
{
	if (theSystem->mObstacles != nullptr)
	{
		for (Obstacle* anObstacle : *theSystem->mObstacles)
		{
			Point p = *anObstacle->getPosition();
			if (std::fabs(p.x - x) < r + 18.0f && std::fabs(p.y - y) < r + 18.0f)
				return false;
		}
	}
	return true;
}

// Where a ball's centre can be: a grid of 4 px cells over the field (and a little above it), a cell free when a ball
// of the plan's radius there touches no block (a circle against each block's box, with 2 px to spare). Built once a
// move; each free cell carries the number of its region (the cells a ball can be carried between).
const float GRID = 4.0f;
const float GRID_TOP = -20.0f;  // the field's top edge: a ball is carried a little above it, over the highest ledge

struct Grid
{
	int mW = 0, mH = 0;
	float mR = 22.0f;
	std::vector<int> mRegion;  // -1 blocked
	bool valid(int cx, int cy) const { return cx >= 0 && cy >= 0 && cx < mW && cy < mH; }
	int at(int cx, int cy) const { return valid(cx, cy) ? mRegion[cy * mW + cx] : -1; }
	float x(int cx) const { return (float)cx * GRID; }
	float y(int cy) const { return GRID_TOP + (float)cy * GRID; }
};

Grid gGrid;

bool cellFree(WordBallSystem* theSystem, float x, float y, float r)
{
	if (x < r - 4.0f || x > FIELD_W - r + 4.0f || y > FIELD_H - r + 2.0f)
		return false;
	if (theSystem->mObstacles != nullptr)
	{
		for (Obstacle* anObstacle : *theSystem->mObstacles)
		{
			Point p = anObstacle->getOrigin();
			Point e = anObstacle->getExtents();
			float dx = std::max(std::fabs(p.x - x) - e.x, 0.0f), dy = std::max(std::fabs(p.y - y) - e.y, 0.0f);
			if (dx * dx + dy * dy < (r - 2.0f) * (r - 2.0f))
				return false;
		}
	}
	// balls that never move (RELAX's letters fixed in stone) wall a place in as much as a block does
	for (unsigned int i = 0; i < theSystem->mCount; i++)
	{
		Ball* aBall = theSystem->getBall(i);
		if (aBall->mState != BALL_FREE || !(aBall->mFixed || (aBall->mSpecial & (BALL_STONE | BALL_BREAKABLE)) != 0))
			continue;
		float dx = aBall->getPosition()->x - x, dy = aBall->getPosition()->y - y, aReach = aBall->mRadius + r - 2.0f;
		if (dx * dx + dy * dy < aReach * aReach)
			return false;
	}
	return true;
}

void buildGrid(WordBallSystem* theSystem, float r)
{
	Grid& g = gGrid;
	g.mR = r;
	g.mW = (int)(FIELD_W / GRID) + 1;
	g.mH = (int)((FIELD_H - GRID_TOP) / GRID) + 1;
	g.mRegion.assign((size_t)(g.mW * g.mH), -2);
	for (int cy = 0; cy < g.mH; cy++)
	{
		for (int cx = 0; cx < g.mW; cx++)
		{
			if (!cellFree(theSystem, g.x(cx), g.y(cy), r))
				g.mRegion[cy * g.mW + cx] = -1;
		}
	}
	int aRegion = 0;
	std::vector<int> aQueue;
	for (int i = 0; i < g.mW * g.mH; i++)
	{
		if (g.mRegion[i] != -2)
			continue;
		g.mRegion[i] = aRegion;
		aQueue.assign(1, i);
		for (size_t q = 0; q < aQueue.size(); q++)
		{
			int cx = aQueue[q] % g.mW, cy = aQueue[q] / g.mW;
			const int aDx[4] = { 1, -1, 0, 0 }, aDy[4] = { 0, 0, 1, -1 };
			for (int d = 0; d < 4; d++)
			{
				int nx = cx + aDx[d], ny = cy + aDy[d];
				if (g.valid(nx, ny) && g.mRegion[ny * g.mW + nx] == -2)
				{
					g.mRegion[ny * g.mW + nx] = aRegion;
					aQueue.push_back(ny * g.mW + nx);
				}
			}
		}
		aRegion++;
	}
}

// The free cell nearest (x, y), within 12 px, or -1
int cellNear(float x, float y)
{
	const Grid& g = gGrid;
	int aBest = -1;
	float aBestD = 0.0f;
	int cx0 = (int)std::lround(x / GRID), cy0 = (int)std::lround((y - GRID_TOP) / GRID);
	for (int dy = -3; dy <= 3; dy++)
	{
		for (int dx = -3; dx <= 3; dx++)
		{
			int cx = cx0 + dx, cy = cy0 + dy;
			if (g.at(cx, cy) < 0)
				continue;
			float ex = g.x(cx) - x, ey = g.y(cy) - y, d = ex * ex + ey * ey;
			if (aBest < 0 || d < aBestD)
				aBest = cy * g.mW + cx, aBestD = d;
		}
	}
	return aBest;
}

// The region a ball at (x, y) can be carried in, or -1
int regionAt(float x, float y)
{
	int aCell = cellNear(x, y);
	return aCell < 0 ? -1 : gGrid.mRegion[aCell];
}

// The way a ball is carried from one point to another in the field: the cheapest chain of cells, a cell near another
// ball (one it would shove) costing more, near a ball of the word's places much more; its corners, ending at the
// target. Empty when there is no way.
std::vector<Point> findWay(const std::vector<BotBall>& theBalls, const BotBall& theBall, float toX, float toY,
	const std::set<int>& thePlaced)
{
	const Grid& g = gGrid;
	std::vector<Point> aWay;
	int aFrom = cellNear(theBall.x, theBall.y), aTo = cellNear(toX, toY);
	if (aFrom < 0 || aTo < 0 || g.mRegion[aFrom] != g.mRegion[aTo])
		return aWay;
	int n = g.mW * g.mH;
	std::vector<int> aCost(n, 1 << 30), aBack(n, -1);
	// a cell's own cost: 1, more within reach of another ball
	auto cellCost = [&](int i)
	{
		float x = g.x(i % g.mW), y = g.y(i / g.mW);
		int c = 1;
		for (const BotBall& b : theBalls)
		{
			if (b.mIndex == theBall.mIndex)
				continue;
			float dx = b.x - x, dy = b.y - y, reach = b.r + theBall.r + 2.0f;
			if (dx * dx + dy * dy < reach * reach)
				c += thePlaced.count(b.mIndex) != 0 ? 60 : 8;
		}
		return c;
	};
	std::vector<int> aOwn(n, -1);
	typedef std::pair<int, int> Entry;
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> aOpen;
	aCost[aFrom] = 0;
	aOpen.push(Entry(0, aFrom));
	while (!aOpen.empty())
	{
		Entry e = aOpen.top();
		aOpen.pop();
		if (e.first != aCost[e.second])
			continue;
		if (e.second == aTo)
			break;
		int cx = e.second % g.mW, cy = e.second / g.mW;
		const int aDx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, aDy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
		for (int d = 0; d < 8; d++)
		{
			int nx = cx + aDx[d], ny = cy + aDy[d];
			if (g.at(nx, ny) < 0)
				continue;
			// a diagonal step only between two free cells
			if (d >= 4 && (g.at(cx + aDx[d], cy) < 0 || g.at(cx, cy + aDy[d]) < 0))
				continue;
			int i = ny * g.mW + nx;
			if (aOwn[i] < 0)
				aOwn[i] = cellCost(i);
			int c = e.first + aOwn[i] * (d >= 4 ? 14 : 10);
			if (c < aCost[i])
			{
				aCost[i] = c;
				aBack[i] = e.second;
				aOpen.push(Entry(c, i));
			}
		}
	}
	if (aBack[aTo] < 0 && aTo != aFrom)
		return aWay;
	std::vector<int> aCells;
	for (int i = aTo; i != aFrom; i = aBack[i])
		aCells.push_back(i);
	std::reverse(aCells.begin(), aCells.end());
	// its corners: a cell where the step's direction changes
	int aLastDx = 0, aLastDy = 0, aPrev = aFrom;
	for (size_t k = 0; k < aCells.size(); k++)
	{
		int i = aCells[k];
		int dx = i % g.mW - aPrev % g.mW, dy = i / g.mW - aPrev / g.mW;
		if (k > 0 && (dx != aLastDx || dy != aLastDy))
			aWay.push_back(Point(g.x(aPrev % g.mW), g.y(aPrev / g.mW)));
		aLastDx = dx, aLastDy = dy, aPrev = i;
	}
	aWay.push_back(Point(toX, toY));
	return aWay;
}

// A place a ball can be carried to: in a region with a free letter ball the mouse can carry
bool placeReachable(const std::vector<BotBall>& theBalls, float x, float y)
{
	int aRegion = regionAt(x, y);
	if (aRegion < 0)
		return false;
	for (const BotBall& b : theBalls)
	{
		if (b.mUsable && regionAt(b.x, b.y) == aRegion)
			return true;
	}
	return false;
}

// The ball sitting in place i of the plan, or -1
int ballAt(const Plan& thePlan, const std::vector<BotBall>& theBalls, int i)
{
	for (const BotBall& b : theBalls)
	{
		// a chain's ball within 20 px along the row (a chain sags), but in the row: a ball 15 px off it is in none
		float aNear = b.mChained && thePlan.mShift == 0.0f ? 20.0f : 7.0f;
		if ((b.mUsable || b.mChained || b.mFixed) && letterFits(thePlan.mWord[i], b.mLetter) && std::fabs(b.x - thePlan.mSlotX[i]) < aNear
			&& std::fabs(b.y - thePlan.mSlotY[i]) < (b.mChained ? 10.0f : 7.0f))
			return b.mIndex;
	}
	return -1;
}

bool makePlan(Game* theGame, const std::vector<BotBall>& theBalls, Plan& thePlan)
{
	WordBallSystem* aSystem = theGame->mBallSystem;
	std::string aLetters, anAllLetters;  // the free letters; and with the chains' too
	for (const BotBall& b : theBalls)
	{
		if ((b.mUsable && b.mLiftable) || b.mFixed)
			aLetters += b.mLetter;
		if (b.mUsable || b.mChained || b.mFixed)
			anAllLetters += b.mLetter;
	}
	std::vector<std::string> aWords;
	std::vector<std::string> aChains;
	if (aSystem->mMode == MODE_PUZZLE)
	{
		for (auto& aWord : aSystem->mWordIndex)
			aWords.push_back(aWord.first);
	}
	else
	{
		for (Chain* aChain : aSystem->mChains)
		{
			// the chain's letters as they lie, left to right (a submitted row is read by x, 0x415c20): a chain that
			// fell over reads backwards from its indices
			std::vector<std::pair<float, char>> aLying;
			for (int anIndex : aChain->mIndices)
			{
				Ball* aBall = aSystem->getBall(anIndex);
				if (aBall->mState == BALL_FREE || aBall->mState == BALL_STATE_4)
					aLying.push_back(std::make_pair(aBall->getPosition()->x, aBall->mLetter));
			}
			std::sort(aLying.begin(), aLying.end(),
				[](const std::pair<float, char>& a, const std::pair<float, char>& b) { return a.first < b.first; });
			std::string aPiece;
			for (const auto& aBallLying : aLying)
				aPiece += aBallLying.second;
			if (!aPiece.empty())
				aChains.push_back(aPiece);
		}
		theGame->mDictionary.findAnyWords(aLetters, aChains, aWords);
	}
	for (std::string& aWord : aWords)
	{
		for (char& c : aWord)
			c = (char)std::toupper((unsigned char)c);
	}
	// the longest words first, then in the list's order
	std::stable_sort(aWords.begin(), aWords.end(), [](const std::string& a, const std::string& b) { return a.size() > b.size(); });
	bool sDebug = Sexy::gBotDebug;
	int aNoAnchor = 0, aNotFree = 0, aNotSupported = 0, aBestCost = 1000;
	if (sDebug)
		std::fprintf(stderr, "plan: letters %s, %d words\n", aLetters.c_str(), (int)aWords.size());
	for (const std::string& aWord : aWords)
	{
		if ((int)aWord.size() < aSystem->mShortestWord || aWord.size() < 3)
			continue;
		// enough letters for the word
		std::string aLeft = anAllLetters;
		bool aEnough = true;
		for (char c : aWord)
		{
			size_t aAt = aLeft.find(c);
			if (aAt == std::string::npos && c != '?')
				aAt = aLeft.find('?');
			if (c == '?' && !aLeft.empty())
				aAt = 0;
			if (aAt == std::string::npos)
			{
				aEnough = false;
				break;
			}
			aLeft.erase(aAt, 1);
		}
		if (!aEnough)
			continue;
		// a word with a chain's piece in it is built around the chain: its anchor is one of the chain's balls
		bool aOnChain = false;
		for (const std::string& aPiece : aChains)
		{
			std::string anUpper = aPiece;
			for (char& c : anUpper)
				c = (char)std::toupper((unsigned char)c);
			if (aWord.find(anUpper) != std::string::npos)
				aOnChain = true;
		}
		for (const BotBall& anAnchor : theBalls)
		{
			if (!(anAnchor.mUsable || anAnchor.mChained || anAnchor.mFixed) || !anAnchor.mResting || (aOnChain && !anAnchor.mChained))
			{
				aNoAnchor++;
				continue;
			}
			// a row; in RELAX also a column (read from the top down), its places below the anchor already its letters
			for (int aColumn = 0; aColumn < (aSystem->mMode == MODE_PUZZLE && !aOnChain ? 2 : 1); aColumn++)
			for (size_t k = 0; k < aWord.size(); k++)
			{
				if (!letterFits(aWord[k], anAnchor.mLetter))
					continue;
				char aKey[64];
				std::snprintf(aKey, sizeof(aKey), "%s@%d@%d%s", aWord.c_str(), anAnchor.mIndex, (int)(anAnchor.x / 22.0f),
					aColumn ? "v" : "");
				if (gMemory.mFailed.count(aKey) != 0 || gMemory.mFailures[aWord] >= 8)
					continue;
				// each place costs a drag, two when another letter sits there and must be carried off first, none when
				// its letter is there already; a place in an obstacle, out of the field, over nothing or under a ball the
				// mouse cannot carry rules the anchor out
				float d = anAnchor.r * 2.0f + 0.5f;
				bool aGood = true;
				int aCost = 0;
				std::vector<float> aSlots, aSlotsY;
				int aHanging = 0;  // empty places over nothing
				std::vector<std::pair<char, int>> aNeeds;  // the letters to bring, each with its place's region
				for (size_t j = 0; j < aWord.size() && aGood; j++)
				{
					float x = aColumn ? anAnchor.x : anAnchor.x + ((float)j - (float)k) * d;
					float y = aColumn ? anAnchor.y + ((float)j - (float)k) * d : anAnchor.y;
					aSlots.push_back(x);
					aSlotsY.push_back(y);
					if (j == k)
						continue;
					const BotBall* anOccupant = occupant(theBalls, x, y, anAnchor.r, anAnchor.mIndex);
					bool aFits = anOccupant != nullptr && letterFits(aWord[j], anOccupant->mLetter)
						&& (anOccupant->mUsable || anOccupant->mFixed) && std::fabs(anOccupant->x - x) < 7.0f
						&& std::fabs(anOccupant->y - y) < 7.0f;
					if (!inField(x, y, anAnchor.r) || !clearOfObstacles(aSystem, x, y, anAnchor.r))
						aGood = false, aNotFree++;
					else if (aColumn && j > k && !aFits && !anAnchor.mFixed)
						aGood = false, aNotSupported++;  // a column's places under its anchor hold it up: already its letters
					else if (anOccupant == nullptr)
					{
						// over nothing: in a column the place below is filled first; in RELAX the last letter of a word can be
						// held in its place until the word counts (a word is scored while its balls touch, 0x41b2d0)
						if (!supported(aSystem, theBalls, x, y, anAnchor.r) && !(aColumn && j + 1 < aWord.size()))
							aHanging++;
						aGood = placeReachable(theBalls, x, y - 2.0f);
						aNeeds.push_back(std::make_pair(aWord[j], regionAt(x, y - 2.0f)));
						aCost += 1;
						if (!aGood)
							aNotSupported++;
					}
					else if (anOccupant->mChained && letterFits(aWord[j], anOccupant->mLetter) && std::fabs(anOccupant->x - x) < 20.0f
						&& std::fabs(anOccupant->y - y) < 20.0f)
						aCost += 0;
					else if (aFits)
						aCost += 0;
					else if (!anOccupant->mUsable)
						aGood = false, aNotFree++;
					else
					{
						aCost += 2;
						aNeeds.push_back(std::make_pair(aWord[j], regionAt(x, y - 2.0f)));
						if (regionAt(anOccupant->x, anOccupant->y) < 0 || !placeReachable(theBalls, x, y - 2.0f))
							aGood = false, aNotSupported++;
					}
				}
				if (aGood && aHanging > 0 && (aSystem->mMode != MODE_PUZZLE || aHanging > 1))
					aGood = false, aNotSupported++;
				// a word on a chain whose places are not all on something (a chain's ball hanging over a ledge's end): the
				// whole row moved along until they are, the chain's balls carried into it
				float aShift = 0.0f;
				if (aOnChain)
				{
					auto aFits = [&](float theShift)
					{
						for (size_t j = 0; j < aWord.size(); j++)
						{
							float x = anAnchor.x + ((float)j - (float)k) * d + theShift;
							if (!inField(x, anAnchor.y, anAnchor.r) || !clearOfObstacles(aSystem, x, anAnchor.y, anAnchor.r)
								|| !supported(aSystem, theBalls, x, anAnchor.y, anAnchor.r) || regionAt(x, anAnchor.y - 2.0f) < 0)
								return false;
						}
						return true;
					};
					if (!aGood || !aFits(0.0f))
					{
						aGood = false;
						for (int aStep = 1; aStep <= 66 && !aGood; aStep++)
						{
							float aTry = (float)((aStep + 1) / 2) * 4.0f * ((aStep % 2) != 0 ? -1.0f : 1.0f);
							if (aFits(aTry))
								aGood = true, aShift = aTry;
						}
						aSlots.clear();
						for (size_t j = 0; j < aWord.size(); j++)
							aSlots.push_back(anAnchor.x + ((float)j - (float)k) * d + aShift);
						aSlotsY.assign(aWord.size(), anAnchor.y);
						aCost = (int)aWord.size();
					}
				}
				// each letter to bring has a ball the mouse can carry there (a letter fixed in stone stays where it is)
				if (aGood && !aNeeds.empty())
				{
					std::set<int> aTaken;
					aTaken.insert(anAnchor.mIndex);
					for (const auto& aNeed : aNeeds)
					{
						const BotBall* aFound = nullptr;
						for (const BotBall& b : theBalls)
						{
							if (b.mUsable && b.mLiftable && aTaken.count(b.mIndex) == 0 && letterFits(aNeed.first, b.mLetter)
								&& regionAt(b.x, b.y) == aNeed.second)
							{
								aFound = &b;
								break;
							}
						}
						if (aFound == nullptr)
						{
							aGood = false, aNotSupported++;
							break;
						}
						aTaken.insert(aFound->mIndex);
					}
				}
				if (!aGood)
					continue;
				// the cheapest plan, the longer word on a tie
				if (aCost < aBestCost || (aCost == aBestCost && aWord.size() > thePlan.mWord.size()))
				{
					aBestCost = aCost;
					thePlan = Plan();
					thePlan.mWord = aWord;
					thePlan.mAnchor = anAnchor.mIndex;
					thePlan.mAnchorAt = (int)k;
					thePlan.mAnchorX = anAnchor.x;
					thePlan.mAnchorY = anAnchor.y;
					thePlan.mSlotX = aSlots;
					thePlan.mSlotY = aSlotsY;
					thePlan.mColumn = aColumn != 0;
					thePlan.mShift = aShift;
				}
			}
		}
	}
	if (sDebug)
	{
		for (const BotBall& b : theBalls)
			std::fprintf(stderr, "  ball %d %c %.1f,%.1f%s%s%s%s region %d special %x\n", b.mIndex, b.mLetter, b.x, b.y,
				b.mUsable ? " usable" : "", b.mChained ? " chained" : "", b.mFixed ? " fixed" : "", b.mResting ? " resting" : "",
				regionAt(b.x, b.y), aSystem->getBall((unsigned int)b.mIndex)->mSpecial);
		if (aSystem->mObstacles != nullptr)
		{
			for (Obstacle* anObstacle : *aSystem->mObstacles)
			{
				Point p = anObstacle->getOrigin(), e = anObstacle->getExtents();
				std::fprintf(stderr, "  obstacle %.1f,%.1f extents %.1f,%.1f\n", p.x, p.y, e.x, e.y);
			}
		}
		if (aBestCost < 1000)
			std::fprintf(stderr, "  anchor %d at %d\n", thePlan.mAnchor, thePlan.mAnchorAt);
	}
	if (sDebug)
		std::fprintf(stderr, "plan: %s cost %d (no anchor %d, place taken %d, unsupported %d)\n",
			aBestCost < 1000 ? thePlan.mWord.c_str() : "none", aBestCost, aNoAnchor, aNotFree, aNotSupported);
	return aBestCost < 1000;
}

void failPlan(int theWhy)
{
	if (Sexy::gBotDebug)
		std::fprintf(stderr, "fail %s: %d\n", gMemory.mPlan.mWord.c_str(), theWhy);
	char aKey[64];
	std::snprintf(aKey, sizeof(aKey), "%s@%d@%d", gMemory.mPlan.mWord.c_str(), gMemory.mPlan.mAnchor,
		(int)(gMemory.mPlan.mAnchorX / 22.0f));
	gMemory.mFailed.insert(aKey);
	gMemory.mFailures[gMemory.mPlan.mWord]++;
	gMemory.mHavePlan = false;
}

// One more drag for the plan, unless it is the drag answered last time (a driver that asks again before making it)
void countDrag(Plan& thePlan, const BotBall& theBall, const BotAction& theAction)
{
	// the framework's updates, which a move's waits count (in real time a frame takes three or four)
	int aFrame = (int)Sexy::gApp->mUpdateCount;
	if (theBall.mIndex != thePlan.mLastBall || aFrame >= thePlan.mLastUntil)
		thePlan.mDrags++;
	thePlan.mLastBall = theBall.mIndex;
	int aLength = theAction.mUpdates;
	for (const BotStep& st : theAction.mSteps)
		aLength += st.mKind == BotStep::WAIT ? st.mN : 0;
	thePlan.mLastUntil = aFrame + aLength;
}

// A drag of a ball from one point to another in screen coordinates: pressed on it, carried up to theTop at theUpX,
// across and down at theDownX, held there while it settles under the mouse, let go
BotAction dragMove(const char* theWhat, const Plan& thePlan, int theLetter, Point theFrom, Point theTo, float theTop,
	float theUpX, float theDownX = NAN)
{
	if (std::isnan(theDownX))
		theDownX = theTo.x;
	BotAction anAction(theWhat);
	anAction.mWord = thePlan.mWord;
	anAction.mLetter = theLetter;
	int x0 = (int)std::lround(theFrom.x), y0 = (int)std::lround(theFrom.y);
	anAction.mSteps = { { BotStep::MOVE, x0, y0, 0 }, { BotStep::WAIT, 0, 0, 2 }, { BotStep::DOWN, x0, y0, 0 },
		{ BotStep::WAIT, 0, 0, 4 } };
	Point aWay[5] = { Point(theUpX, theFrom.y), Point(theUpX, theTop), Point(theDownX, theTop), Point(theDownX, theTo.y),
		theTo };
	Point aAt = theFrom;
	for (const Point& aNext : aWay)
	{
		float aLength = std::sqrt((aNext.x - aAt.x) * (aNext.x - aAt.x) + (aNext.y - aAt.y) * (aNext.y - aAt.y));
		int aSteps = std::max(1, (int)(aLength / 30.0f));
		for (int i = 1; i <= aSteps; i++)
		{
			float x = aAt.x + (aNext.x - aAt.x) * (float)i / (float)aSteps;
			float y = aAt.y + (aNext.y - aAt.y) * (float)i / (float)aSteps;
			anAction.mSteps.push_back({ BotStep::MOVE, (int)std::lround(x), (int)std::lround(y), 0 });
			anAction.mSteps.push_back({ BotStep::WAIT, 0, 0, 3 });
		}
		aAt = aNext;
	}
	anAction.mSteps.push_back({ BotStep::WAIT, 0, 0, 24 });
	anAction.mSteps.push_back({ BotStep::UP, (int)std::lround(theTo.x), (int)std::lround(theTo.y), 0 });
	anAction.mUpdates = 45;
	return anAction;
}

// A drag of a ball to (toX, toY) in the field along the grid's way (findWay): pressed on it, moved along the way a few
// pixels an update so that the ball on its rope follows round the blocks' corners, held at the end while it settles,
// let go; false when there is no way
bool carryMove(const char* theWhat, const Plan& thePlan, int theLetter, WordBallSystem* theSystem,
	const std::vector<BotBall>& theBalls, const BotBall& theBall, float toX, float toY, const std::set<int>& thePlaced,
	BotAction& theAction)
{
	std::vector<Point> aWay = findWay(theBalls, theBall, toX, toY, thePlaced);
	if (aWay.empty())
		return false;
	theAction = BotAction(theWhat);
	theAction.mWord = thePlan.mWord;
	theAction.mLetter = theLetter;
	Point aFrom = toScreen(theSystem, theBall.x, theBall.y);
	int x0 = (int)std::lround(aFrom.x), y0 = (int)std::lround(aFrom.y);
	theAction.mSteps = { { BotStep::MOVE, x0, y0, 0 }, { BotStep::WAIT, 0, 0, 2 }, { BotStep::DOWN, x0, y0, 0 },
		{ BotStep::WAIT, 0, 0, 4 } };
	Point aAt(theBall.x, theBall.y);
	for (const Point& aNext : aWay)
	{
		float aLength = std::sqrt((aNext.x - aAt.x) * (aNext.x - aAt.x) + (aNext.y - aAt.y) * (aNext.y - aAt.y));
		int aSteps = std::max(1, (int)(aLength / 20.0f));
		for (int i = 1; i <= aSteps; i++)
		{
			Point p = toScreen(theSystem, aAt.x + (aNext.x - aAt.x) * (float)i / (float)aSteps,
				aAt.y + (aNext.y - aAt.y) * (float)i / (float)aSteps);
			theAction.mSteps.push_back({ BotStep::MOVE, (int)std::lround(p.x), (int)std::lround(p.y), 0 });
			theAction.mSteps.push_back({ BotStep::WAIT, 0, 0, 3 });
		}
		// a corner: a few updates for the ball to come round it
		theAction.mSteps.push_back({ BotStep::WAIT, 0, 0, 4 });
		aAt = aNext;
	}
	Point aTo = toScreen(theSystem, toX, toY);
	// held there while it settles; in RELAX longer, so that a word whose last letter has nothing under it scores while
	// the ball is held
	theAction.mSteps.push_back({ BotStep::WAIT, 0, 0, theSystem->mMode == MODE_PUZZLE ? 45 : 20 });
	theAction.mSteps.push_back({ BotStep::UP, (int)std::lround(aTo.x), (int)std::lround(aTo.y), 0 });
	theAction.mUpdates = 45;
	return true;
}

// Where an occupant of a place is put out of the way: the highest free cell of its region in a column away from the
// word (the far side of the field from it first), where it falls on whatever is there; false when there is none
bool parkPlace(const Plan& thePlan, const BotBall& theBall, float& theX, float& theY)
{
	const Grid& g = gGrid;
	int aRegion = regionAt(theBall.x, theBall.y);
	if (aRegion < 0)
		return false;
	float aLow = thePlan.mSlotX.front() - theBall.r * 2.0f, aHigh = thePlan.mSlotX.back() + theBall.r * 2.0f;
	bool aFound = false;
	float aBestScore = 0.0f;
	for (int cx = 0; cx < g.mW; cx++)
	{
		float x = g.x(cx);
		if (x > aLow && x < aHigh)
			continue;
		for (int cy = 0; cy < g.mH; cy++)
		{
			if (g.at(cx, cy) != aRegion)
				continue;
			// high up, and far from the word
			float aScore = g.y(cy) - std::min(std::fabs(x - aLow), std::fabs(x - aHigh)) * 0.5f;
			if (!aFound || aScore < aBestScore)
				aFound = true, aBestScore = aScore, theX = x, theY = g.y(cy);
			break;
		}
	}
	return aFound;
}

BotAction playMove(Game* theGame)
{
	WordBallSystem* aSystem = theGame->mBallSystem;
	if (gMemory.mMode != theGame->mMode || gMemory.mLevel != theGame->mLevel)
	{
		gMemory = BotMemory();
		gMemory.mMode = theGame->mMode;
		gMemory.mLevel = theGame->mLevel;
	}
	// a ball still on the mouse when the last drag is done: its release was lost (the window lost the focus during the
	// drag, and the framework forgets a button pressed before), so a click lets it go, as a player's next click does
	if (aSystem->mRope != nullptr && ++gMemory.mHeldAsks >= 2)
	{
		gMemory.mHeldAsks = 0;
		Point aMouse = toScreen(aSystem, aSystem->mMouse.x, aSystem->mMouse.y);
		return clickMove("release", (int)std::lround(aMouse.x), (int)std::lround(aMouse.y), 15);
	}
	if (aSystem->mRope != nullptr || aSystem->isBusy())
		return BotAction("wait", 15);
	gMemory.mHeldAsks = 0;
	bool sDebug = Sexy::gBotDebug;
	if (sDebug && gDebugBall >= 0)
	{
		Ball* aBall = aSystem->getBall(gDebugBall);
		std::fprintf(stderr, "landed: ball %d %c at %.1f,%.1f (aimed %.1f,%.1f)\n", gDebugBall, aBall->mLetter,
			aBall->getPosition()->x, aBall->getPosition()->y, gDebugX, gDebugY);
		gDebugBall = -1;
	}
	// a word lies ready: the right button submits it
	if (aSystem->mMode != MODE_PUZZLE && aSystem->mHud != nullptr && aSystem->mHud->isSubmitShown())
	{
		gMemory.mHavePlan = false;
		Point p = toScreen(aSystem, FIELD_W / 2.0f, 12.0f);
		BotAction anAction("submit", 40);
		anAction.mSteps = { { BotStep::MOVE, (int)p.x, (int)p.y, 0 }, { BotStep::WAIT, 0, 0, 2 }, { BotStep::RCLICK, (int)p.x, (int)p.y, 0 } };
		return anAction;
	}
	buildGrid(aSystem, 22.0f);
	std::vector<BotBall> aBalls = readBalls(theGame);
	for (int aTry = 0; aTry < 2; aTry++)
	{
		if (!gMemory.mHavePlan)
		{
			if (!makePlan(theGame, aBalls, gMemory.mPlan))
				break;
			gMemory.mHavePlan = true;
		}
		Plan& aPlan = gMemory.mPlan;
		// the anchor must still be where it was
		const BotBall* anAnchor = nullptr;
		for (const BotBall& b : aBalls)
		{
			if (b.mIndex == aPlan.mAnchor)
				anAnchor = &b;
		}
		// the anchor must still be near where it was (a drop beside it shoves it a little); the places follow it
		bool aShifted = aPlan.mShift != 0.0f;
		if (anAnchor == nullptr || !(anAnchor->mUsable || anAnchor->mChained || anAnchor->mFixed)
			|| aPlan.mDrags > (int)aPlan.mWord.size() * 3 + 2
			|| (!aShifted && (std::fabs(anAnchor->x - aPlan.mAnchorX) > 30.0f || std::fabs(anAnchor->y - aPlan.mAnchorY) > 8.0f)))
		{
			failPlan(1);
			continue;
		}
		if (!aShifted)
		{
			for (size_t i = 0; i < aPlan.mSlotX.size(); i++)
			{
				float aAlong = ((float)i - (float)aPlan.mAnchorAt) * (anAnchor->r * 2.0f + 0.5f);
				aPlan.mSlotX[i] = aPlan.mColumn ? anAnchor->x : anAnchor->x + aAlong;
				aPlan.mSlotY[i] = aPlan.mColumn ? anAnchor->y + aAlong : anAnchor->y;
			}
		}
		// the first place without its letter, from the anchor outwards; a moved row from its leading end (the end it
		// moves towards), the anchor's place too
		std::set<int> aPlaced;
		if (!aShifted)
			aPlaced.insert(aPlan.mAnchor);
		int aNext = -1, aHangingAt = -1;
		int aLength = (int)aPlan.mWord.size();
		// a place with nothing under it: no block or ball under it yet
		auto aHangs = [&](int i) {
			if (aShifted || aSystem->mMode != MODE_PUZZLE
				|| supported(aSystem, aBalls, aPlan.mSlotX[i], aPlan.mSlotY[i], anAnchor->r))
				return false;
			return true;
		};
		for (int aStep = aShifted ? 0 : 1; aStep < (aShifted ? aLength : aLength * 2); aStep++)
		{
			int i = !aShifted ? aPlan.mAnchorAt + ((aStep % 2) != 0 ? (aStep + 1) / 2 : -(aStep / 2))
				: aPlan.mShift < 0.0f ? aStep : aLength - 1 - aStep;
			if (i < 0 || i >= (int)aPlan.mWord.size())
				continue;
			int aBall = ballAt(aPlan, aBalls, i);
			if (aBall >= 0)
				aPlaced.insert(aBall);
			else if (aNext < 0 && aHangs(i))
				aHangingAt = i;
			else if (aNext < 0)
				aNext = i;
		}
		// a place over nothing (RELAX: held there until the word counts) comes last
		if (aNext < 0)
			aNext = aHangingAt;
		if (aNext < 0)
		{
			// every letter in place: the word scores (RELAX) or the submit button comes; if neither does, give it up
			if (++aPlan.mReady > 6)
			{
				failPlan(2);
				continue;
			}
			return BotAction("wait", 20);
		}
		// a chain's ball of the word hanging out of the row (over a ledge's end): carried up into its place, the chain
		// with it
		for (const BotBall& b : aBalls)
		{
			if (b.mChained && (aShifted || b.mIndex != aPlan.mAnchor) && aPlaced.count(b.mIndex) == 0
				&& letterFits(aPlan.mWord[aNext], b.mLetter)
				&& std::fabs(b.x - aPlan.mSlotX[aNext]) < (aShifted ? 160.0f : 30.0f) && std::fabs(b.y - aPlan.mSlotY[aNext]) < 60.0f)
			{
				Point aTo = toScreen(aSystem, aPlan.mSlotX[aNext], aPlan.mSlotY[aNext] - 2.0f);
				BotAction aLift = dragMove("lift", aPlan, aNext, toScreen(aSystem, b.x, b.y), aTo, aTo.y,
					toScreen(aSystem, b.x, 0.0f).x);
				countDrag(aPlan, b, aLift);
				return aLift;
			}
		}
		const BotBall* anOccupant = occupant(aBalls, aPlan.mSlotX[aNext], aPlan.mSlotY[aNext], anAnchor->r, aPlan.mAnchor);
		if (anOccupant != nullptr && (!anOccupant->mUsable || aPlaced.count(anOccupant->mIndex) != 0))
		{
			failPlan(3);
			continue;
		}
		if (anOccupant != nullptr)
		{
			// another letter in the place: carried out of the way (parkPlace), where it falls on whatever is there
			float aParkX, aParkY;
			BotAction aClear("clear");
			if (!parkPlace(aPlan, *anOccupant, aParkX, aParkY)
				|| !carryMove("clear", aPlan, aNext, aSystem, aBalls, *anOccupant, aParkX, aParkY, aPlaced, aClear))
			{
				failPlan(5);
				continue;
			}
			countDrag(aPlan, *anOccupant, aClear);
			return aClear;
		}
		// the letter from the top of the field, never one the plan's places rest on
		const BotBall* aBest = nullptr;
		for (const BotBall& b : aBalls)
		{
			if (!b.mUsable || !b.mLiftable || aPlaced.count(b.mIndex) != 0 || !letterFits(aPlan.mWord[aNext], b.mLetter))
				continue;
			bool aUnder = false;
			for (size_t i = 0; i < aPlan.mSlotX.size(); i++)
			{
				if (std::fabs(b.x - aPlan.mSlotX[i]) < b.r * 2.0f && b.y > aPlan.mSlotY[i] && b.y - aPlan.mSlotY[i] < b.r * 2.0f + 8.0f)
					aUnder = true;
			}
			if (aUnder)
				continue;
			if (regionAt(b.x, b.y) != regionAt(aPlan.mSlotX[aNext], aPlan.mSlotY[aNext] - 2.0f))
				continue;
			if (aBest == nullptr || b.y < aBest->y)
				aBest = &b;
		}
		if (aBest == nullptr)
		{
			if (sDebug)
				std::fprintf(stderr, "  no ball for %c at %.1f,%.1f (region %d)\n", aPlan.mWord[aNext], aPlan.mSlotX[aNext],
					aPlan.mSlotY[aNext], regionAt(aPlan.mSlotX[aNext], aPlan.mSlotY[aNext] - 2.0f));
			failPlan(4);
			continue;
		}
		gDebugBall = aBest->mIndex, gDebugX = aPlan.mSlotX[aNext], gDebugY = aPlan.mSlotY[aNext];
		BotAction aDrag("drag");
		if (!carryMove("drag", aPlan, aNext, aSystem, aBalls, *aBest, aPlan.mSlotX[aNext], aPlan.mSlotY[aNext] - 2.0f, aPlaced, aDrag))
		{
			if (sDebug)
				std::fprintf(stderr, "  no way for ball %d to %.1f,%.1f\n", aBest->mIndex, aPlan.mSlotX[aNext], aPlan.mSlotY[aNext]);
			failPlan(4);
			continue;
		}
		countDrag(aPlan, *aBest, aDrag);
		return aDrag;
	}
	// no word to make, and a chain lies fallen over (read by x it is its letters backwards, 0x415c20): its left end
	// carried up and over its right end (or the other way where the field ends), so that it reads forwards again
	if (aSystem->mMode != MODE_PUZZLE)
	{
		for (Chain* aChain : aSystem->mChains)
		{
			std::vector<const BotBall*> aPieces;
			for (int anIndex : aChain->mIndices)
			{
				for (const BotBall& b : aBalls)
				{
					if (b.mIndex == anIndex && b.mChained)
						aPieces.push_back(&b);
				}
			}
			if (aPieces.size() < 2 || aPieces.size() != aChain->mIndices.size()
				|| gMemory.mFlips[aChain->mIndices.front()] >= 3)
				continue;
			const BotBall* aFirst = aPieces.front();
			const BotBall* aLast = aPieces.back();
			bool aBackwards = aFirst->x > aLast->x, aAllResting = true;
			for (const BotBall* b : aPieces)
				aAllResting = aAllResting && b->mResting;
			if (!aBackwards || !aAllResting)
				continue;
			// aLast lies leftmost: carried over aFirst to its right; else aFirst carried left of aLast
			float d = aFirst->r * 2.0f + 0.5f, aTop = FIELD_H;
			for (const BotBall* b : aPieces)
				aTop = std::min(aTop, b->y);
			const BotBall* aMoved = aLast;
			float aToX = aFirst->x + d;
			if (aToX > FIELD_W - aFirst->r)
				aMoved = aFirst, aToX = aLast->x - d;
			if (aToX < aFirst->r)
				continue;
			float aToY = aMoved == aLast ? aFirst->y - 2.0f : aLast->y - 2.0f;
			aTop = std::max(aTop - d * (float)aPieces.size(), 30.0f);
			gMemory.mFlips[aChain->mIndices.front()]++;
			Plan aFlip;
			aFlip.mWord = "flip";
			Point aFrom = toScreen(aSystem, aMoved->x, aMoved->y), aTo = toScreen(aSystem, aToX, aToY);
			if (sDebug)
				std::fprintf(stderr, "flip: chain of %d, ball %d %c from %.1f,%.1f over to %.1f,%.1f\n", (int)aPieces.size(),
					aMoved->mIndex, aMoved->mLetter, aMoved->x, aMoved->y, aToX, aToY);
			return dragMove("flip", aFlip, 0, aFrom, aTo, toScreen(aSystem, 0.0f, aTop).y, aFrom.x);
		}
	}
	// no word to make: in TSUNAMI the next ball comes at once when clicked (more letters, and the water a little higher)
	if (aSystem->mMode == MODE_FREEMATCH && aSystem->mWaterLevel < 0.45f && aSystem->mHud != nullptr && aSystem->mHud->isSpawnReady())
		return clickMove("spawn", 678, 82, 30);
	return BotAction();
}

}

namespace Sexy
{

std::string BotAction::toJson() const
{
	if (mWhat.empty())
		return "null";
	std::string aJson = "{\"what\":\"" + mWhat + "\"";
	if (!mWord.empty())
		aJson += ",\"word\":\"" + mWord + "\",\"letter\":" + std::to_string(mLetter);
	aJson += ",\"input\":[";
	char aBuf[96];
	for (size_t i = 0; i < mSteps.size(); i++)
	{
		const BotStep& st = mSteps[i];
		const char* aKind = st.mKind == BotStep::MOVE ? "move" : st.mKind == BotStep::DOWN ? "press" : st.mKind == BotStep::UP ? "release" : "click";
		if (st.mKind == BotStep::WAIT)
			std::snprintf(aBuf, sizeof(aBuf), "{\"wait\":%d}", st.mN);
		else if (st.mKind == BotStep::RCLICK)
			std::snprintf(aBuf, sizeof(aBuf), "{\"click\":{\"x\":%d,\"y\":%d,\"button\":\"right\"}}", st.x, st.y);
		else
			std::snprintf(aBuf, sizeof(aBuf), "{\"%s\":{\"x\":%d,\"y\":%d}}", aKind, st.x, st.y);
		aJson += (i ? "," : "") + std::string(aBuf);
	}
	return aJson + "],\"updates\":" + std::to_string(mUpdates) + "}";
}

std::string BotMove(::Game* theGame)
{
	return BotNext(theGame).toJson();
}

BotAction BotNext(::Game* theGame)
{
	if (theGame->mSaverMode != 0)
		return BotAction();
	// the screens between levels: their own buttons
	Window* aRoot = App::instance()->getRootWindow();
	Node* aTop = aRoot->mChildren.empty() ? nullptr : aRoot->mChildren.back();
	if (aTop == (Node*)theGame->mPuzzleHelp || aTop == (Node*)theGame->mActionHelp || aTop == (Node*)theGame->mSolitaireHelp)
		return clickMove("play", 165, 415, 60);
	if (theGame->mState == 2 && !theGame->mContinue)
		return clickMove("continue", 430, 500, 60);
	if (theGame->mState == 7)
		return clickMove("continue", 430, 500, 60);
	// Save/Quit's box: Don't Save (doQuitNoSave), and the game is over (the score count, then the name entry)
	if (aTop != nullptr && aTop == (Node*)theGame->mQuitGameScreen)
		return clickMove("dontsave", 505, 325, 60);
	static int sIdle = 0;  // asks in a row with nothing to do in THINK
	if (theGame->mState == 5 && theGame->mBallSystem != nullptr)
	{
		BotAction anAction = playMove(theGame);
		// THINK never ends by itself (the next level comes only once the level's letters are used): with no word left
		// to make and nothing to flip, the game is left by its own Save/Quit
		if (anAction.mWhat.empty() && theGame->mBallSystem->mMode == MODE_SOLITAIRE && ++sIdle >= 12)
		{
			sIdle = 0;
			return clickMove("savequit", 155, 30, 60);
		}
		if (!anAction.mWhat.empty())
			sIdle = 0;
		else if (theGame->mBallSystem->mMode == MODE_SOLITAIRE)
			anAction = BotAction("wait", 30);
		return anAction;
	}
	sIdle = 0;
	return BotAction();
}

}
