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
};

struct Plan
{
	std::string mWord;
	int mAnchor = -1;               // the ball index
	int mAnchorAt = 0;              // its place in the word
	float mAnchorX = 0, mAnchorY = 0;
	std::vector<float> mSlotX;      // each letter's place (the anchor's own included)
	float mSlotY = 0;
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
	std::set<std::string> mFailed;  // "<word>@<anchor index>"
};

BotMemory gMemory;
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
	float aWaterTop = aSystem->mMode == MODE_FREEMATCH ? (float)((1.0 - aSystem->mWaterLevel) * 440.0) : FIELD_H + 100.0f;
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
		b.mResting = dx * dx + dy * dy < (b.mChained ? 4.0f : 0.25f);
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
	return x >= r + 2.0f && x <= FIELD_W - r - 2.0f && y >= r + 30.0f;
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

// The ball sitting in place i of the plan, or -1
int ballAt(const Plan& thePlan, const std::vector<BotBall>& theBalls, int i)
{
	for (const BotBall& b : theBalls)
	{
		// a chain's ball within 20 px along the row (a chain sags), but in the row: a ball 15 px off it is in none
		float aNear = b.mChained && thePlan.mShift == 0.0f ? 20.0f : 7.0f;
		if ((b.mUsable || b.mChained) && letterFits(thePlan.mWord[i], b.mLetter) && std::fabs(b.x - thePlan.mSlotX[i]) < aNear
			&& std::fabs(b.y - thePlan.mSlotY) < (b.mChained ? 10.0f : 7.0f))
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
		if (b.mUsable)
			aLetters += b.mLetter;
		if (b.mUsable || b.mChained)
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
			std::string aPiece;
			for (int anIndex : aChain->mIndices)
			{
				Ball* aBall = aSystem->getBall(anIndex);
				if (aBall->mState == BALL_FREE || aBall->mState == BALL_STATE_4)
					aPiece += aBall->mLetter;
			}
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
	static bool sDebug = std::getenv("WH_BOTDEBUG") != nullptr;
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
			if (!(anAnchor.mUsable || anAnchor.mChained) || !anAnchor.mResting || (aOnChain && !anAnchor.mChained))
			{
				aNoAnchor++;
				continue;
			}
			for (size_t k = 0; k < aWord.size(); k++)
			{
				if (!letterFits(aWord[k], anAnchor.mLetter))
					continue;
				char aKey[64];
				std::snprintf(aKey, sizeof(aKey), "%s@%d", aWord.c_str(), anAnchor.mIndex);
				if (gMemory.mFailed.count(aKey) != 0)
					continue;
				// each place costs a drag, two when another letter sits there and must be carried off first, none when
				// its letter is there already; a place in an obstacle, out of the field, over nothing or under a ball the
				// mouse cannot carry rules the anchor out
				float d = anAnchor.r * 2.0f + 0.5f;
				bool aGood = true;
				int aCost = 0;
				std::vector<float> aSlots;
				for (size_t j = 0; j < aWord.size() && aGood; j++)
				{
					float x = anAnchor.x + ((float)j - (float)k) * d;
					aSlots.push_back(x);
					if (j == k)
						continue;
					const BotBall* anOccupant = occupant(theBalls, x, anAnchor.y, anAnchor.r, anAnchor.mIndex);
					if (!inField(x, anAnchor.y, anAnchor.r) || !clearOfObstacles(aSystem, x, anAnchor.y, anAnchor.r))
						aGood = false, aNotFree++;
					else if (anOccupant == nullptr)
					{
						aGood = supported(aSystem, theBalls, x, anAnchor.y, anAnchor.r);
						aCost += 1;
						if (!aGood)
							aNotSupported++;
					}
					else if (anOccupant->mChained && letterFits(aWord[j], anOccupant->mLetter) && std::fabs(anOccupant->x - x) < 20.0f
						&& std::fabs(anOccupant->y - anAnchor.y) < 20.0f)
						aCost += 0;
					else if (!anOccupant->mUsable)
						aGood = false, aNotFree++;
					else if (letterFits(aWord[j], anOccupant->mLetter) && std::fabs(anOccupant->x - x) < 7.0f
						&& std::fabs(anOccupant->y - anAnchor.y) < 7.0f)
						aCost += 0;
					else
						aCost += 2;
				}
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
								|| !supported(aSystem, theBalls, x, anAnchor.y, anAnchor.r))
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
						aCost = (int)aWord.size();
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
					thePlan.mSlotY = anAnchor.y;
					thePlan.mShift = aShift;
				}
			}
		}
	}
	if (sDebug)
	{
		for (const BotBall& b : theBalls)
			std::fprintf(stderr, "  ball %d %c %.1f,%.1f%s%s%s\n", b.mIndex, b.mLetter, b.x, b.y, b.mUsable ? " usable" : "",
				b.mChained ? " chained" : "", b.mResting ? " resting" : "");
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
	if (std::getenv("WH_BOTDEBUG") != nullptr)
		std::fprintf(stderr, "fail %s: %d\n", gMemory.mPlan.mWord.c_str(), theWhy);
	char aKey[64];
	std::snprintf(aKey, sizeof(aKey), "%s@%d", gMemory.mPlan.mWord.c_str(), gMemory.mPlan.mAnchor);
	gMemory.mFailed.insert(aKey);
	gMemory.mHavePlan = false;
}

// One more drag for the plan, unless it is the drag answered last time (a driver that asks again before making it)
void countDrag(Plan& thePlan, const BotBall& theBall, const BotAction& theAction)
{
	int aFrame = Sexy::gApp->mFrames;
	if (theBall.mIndex != thePlan.mLastBall || aFrame >= thePlan.mLastUntil)
		thePlan.mDrags++;
	thePlan.mLastBall = theBall.mIndex;
	int aLength = theAction.mUpdates;
	for (const BotStep& st : theAction.mSteps)
		aLength += st.mKind == BotStep::WAIT ? st.mN : 0;
	thePlan.mLastUntil = aFrame + aLength;
}

// A drag of a ball from one point to another in screen coordinates: pressed on it, carried up to the top of the field,
// across and down (around the ledges, as a player carries it), held there while it settles under the mouse, let go
// The column a ball is lifted in: its own when no ball lies above it, else the nearest clear one beside it (lifted
// through a chain, a ball shoves it about)
float riseX(const std::vector<BotBall>& theBalls, const BotBall& theBall)
{
	for (int aStep = 0; aStep <= 16; aStep++)
	{
		float x = theBall.x + (float)((aStep + 1) / 2) * 12.0f * ((aStep % 2) != 0 ? 1.0f : -1.0f);
		if (x < theBall.r || x > FIELD_W - theBall.r)
			continue;
		bool aClear = true;
		for (const BotBall& b : theBalls)
		{
			if (b.mIndex != theBall.mIndex && b.y < theBall.y && std::fabs(b.x - x) < b.r + theBall.r + 4.0f)
				aClear = false;
		}
		if (aClear)
			return x;
	}
	return theBall.x;
}

BotAction dragMove(const char* theWhat, const Plan& thePlan, int theLetter, Point theFrom, Point theTo, float theTop,
	float theUpX)
{
	BotAction anAction(theWhat);
	anAction.mWord = thePlan.mWord;
	anAction.mLetter = theLetter;
	int x0 = (int)std::lround(theFrom.x), y0 = (int)std::lround(theFrom.y);
	anAction.mSteps = { { BotStep::MOVE, x0, y0, 0 }, { BotStep::WAIT, 0, 0, 2 }, { BotStep::DOWN, x0, y0, 0 },
		{ BotStep::WAIT, 0, 0, 4 } };
	Point aWay[4] = { Point(theUpX, theFrom.y), Point(theUpX, theTop), Point(theTo.x, theTop), theTo };
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

BotAction playMove(Game* theGame)
{
	WordBallSystem* aSystem = theGame->mBallSystem;
	if (gMemory.mMode != theGame->mMode || gMemory.mLevel != theGame->mLevel)
	{
		gMemory = BotMemory();
		gMemory.mMode = theGame->mMode;
		gMemory.mLevel = theGame->mLevel;
	}
	if (aSystem->mRope != nullptr || aSystem->isBusy())
		return BotAction("wait", 15);
	static bool sDebug = std::getenv("WH_BOTDEBUG") != nullptr;
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
		if (anAnchor == nullptr || !(anAnchor->mUsable || anAnchor->mChained) || aPlan.mDrags > (int)aPlan.mWord.size() * 3 + 2
			|| (!aShifted && (std::fabs(anAnchor->x - aPlan.mAnchorX) > 30.0f || std::fabs(anAnchor->y - aPlan.mAnchorY) > 8.0f)))
		{
			failPlan(1);
			continue;
		}
		if (!aShifted)
		{
			for (size_t i = 0; i < aPlan.mSlotX.size(); i++)
				aPlan.mSlotX[i] = anAnchor->x + ((float)i - (float)aPlan.mAnchorAt) * (anAnchor->r * 2.0f + 0.5f);
			aPlan.mSlotY = anAnchor->y;
		}
		// the first place without its letter, from the anchor outwards; a moved row from its leading end (the end it
		// moves towards), the anchor's place too
		std::set<int> aPlaced;
		if (!aShifted)
			aPlaced.insert(aPlan.mAnchor);
		int aNext = -1;
		int aLength = (int)aPlan.mWord.size();
		for (int aStep = aShifted ? 0 : 1; aStep < (aShifted ? aLength : aLength * 2); aStep++)
		{
			int i = !aShifted ? aPlan.mAnchorAt + ((aStep % 2) != 0 ? (aStep + 1) / 2 : -(aStep / 2))
				: aPlan.mShift < 0.0f ? aStep : aLength - 1 - aStep;
			if (i < 0 || i >= (int)aPlan.mWord.size())
				continue;
			int aBall = ballAt(aPlan, aBalls, i);
			if (aBall >= 0)
				aPlaced.insert(aBall);
			else if (aNext < 0)
				aNext = i;
		}
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
				&& std::fabs(b.x - aPlan.mSlotX[aNext]) < (aShifted ? 160.0f : 30.0f) && std::fabs(b.y - aPlan.mSlotY) < 60.0f)
			{
				Point aTo = toScreen(aSystem, aPlan.mSlotX[aNext], aPlan.mSlotY - 2.0f);
				BotAction aLift = dragMove("lift", aPlan, aNext, toScreen(aSystem, b.x, b.y), aTo, aTo.y,
					toScreen(aSystem, b.x, 0.0f).x);
				countDrag(aPlan, b, aLift);
				return aLift;
			}
		}
		const BotBall* anOccupant = occupant(aBalls, aPlan.mSlotX[aNext], aPlan.mSlotY, anAnchor->r, aPlan.mAnchor);
		if (anOccupant != nullptr && (!anOccupant->mUsable || aPlaced.count(anOccupant->mIndex) != 0))
		{
			failPlan(3);
			continue;
		}
		if (anOccupant != nullptr)
		{
			// another letter in the place: off to the top of the field away from the word, where it falls on whatever
			// is there
			float aLow = aPlan.mSlotX.front() - anAnchor->r * 2.0f, aHigh = aPlan.mSlotX.back() + anAnchor->r * 2.0f;
			float x = aLow > FIELD_W - aHigh ? aLow / 2.0f : (aHigh + FIELD_W) / 2.0f;
			x = std::max(anAnchor->r + 4.0f, std::min(FIELD_W - anAnchor->r - 4.0f, x));
			BotAction aClear = dragMove("clear", aPlan, aNext, toScreen(aSystem, anOccupant->x, anOccupant->y),
				toScreen(aSystem, x, 50.0f), toScreen(aSystem, 0.0f, 30.0f).y,
				toScreen(aSystem, riseX(aBalls, *anOccupant), 0.0f).x);
			countDrag(aPlan, *anOccupant, aClear);
			return aClear;
		}
		// the letter from the top of the field, never one the plan's places rest on
		const BotBall* aBest = nullptr;
		for (const BotBall& b : aBalls)
		{
			if (!b.mUsable || aPlaced.count(b.mIndex) != 0 || !letterFits(aPlan.mWord[aNext], b.mLetter))
				continue;
			bool aUnder = false;
			for (float x : aPlan.mSlotX)
			{
				if (std::fabs(b.x - x) < b.r * 2.0f && b.y > aPlan.mSlotY && b.y - aPlan.mSlotY < b.r * 2.0f + 8.0f)
					aUnder = true;
			}
			if (aUnder)
				continue;
			if (aBest == nullptr || b.y < aBest->y)
				aBest = &b;
		}
		if (aBest == nullptr)
		{
			failPlan(4);
			continue;
		}
		gDebugBall = aBest->mIndex, gDebugX = aPlan.mSlotX[aNext], gDebugY = aPlan.mSlotY;
		BotAction aDrag = dragMove("drag", aPlan, aNext, toScreen(aSystem, aBest->x, aBest->y),
			toScreen(aSystem, aPlan.mSlotX[aNext], aPlan.mSlotY - 2.0f),
			toScreen(aSystem, 0.0f, 30.0f).y, toScreen(aSystem, riseX(aBalls, *aBest), 0.0f).x);
		countDrag(aPlan, *aBest, aDrag);
		return aDrag;
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
		const char* aKind = st.mKind == BotStep::MOVE ? "move" : st.mKind == BotStep::DOWN ? "down" : st.mKind == BotStep::UP ? "up" : "click";
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
	if (theGame->mState == 5 && theGame->mBallSystem != nullptr)
		return playMove(theGame);
	return BotAction();
}

}
