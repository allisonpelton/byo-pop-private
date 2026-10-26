#include "Level.h"
#include "App.h"
#include "Layout.h"
#include "Obstacles.h"
#include "Physics.h"
#include "Stream.h"

using namespace engine;

// 0x41e380
Bubble::Bubble()
{
	mFlags = 0;
}

// 0x41e420
Bubble::Bubble(const Point& theOrigin, const std::string& theLetters, unsigned int theFlags)
{
	mOrigin = theOrigin;
	mFlags = theFlags;
	mLetters = theLetters;
}

// 0x41e280: the multipliers exclude each other, the first named winning
void Bubble::setSpecial(const std::string& theSpecial)
{
	if (theSpecial.find("bomb", 0, 4) != std::string::npos)
		mFlags = mFlags | BALL_BOMB;
	if (theSpecial.find("wildcard", 0, 8) != std::string::npos)
		mFlags = mFlags | BALL_WILDCARD;
	if (theSpecial.find("block", 0, 5) != std::string::npos)
		mFlags = mFlags | BALL_BLOCK;
	if (theSpecial.find("heavy", 0, 5) != std::string::npos)
		mFlags = mFlags | BALL_HEAVY;
	if (theSpecial.find("stone", 0, 5) != std::string::npos)
		mFlags = mFlags | BALL_STONE;
	if (theSpecial.find("obstacle", 0, 8) != std::string::npos)
		mFlags = mFlags | BALL_BREAKABLE;
	if (theSpecial.find("2x", 0, 2) != std::string::npos)
	{
		mFlags = mFlags | BALL_X2;
		return;
	}
	if (theSpecial.find("3x", 0, 2) != std::string::npos)
	{
		mFlags = mFlags | BALL_X3;
		return;
	}
	if (theSpecial.find("4x", 0, 2) != std::string::npos)
		mFlags = mFlags | BALL_X4;
}

// 0x41e460
std::string Bubble::specialText(unsigned int theFlags)
{
	std::string aText;
	if ((theFlags & BALL_BOMB) != 0)
		aText.append("bomb,", 5);
	if ((theFlags & BALL_WILDCARD) != 0)
		aText.append("wildcard,", 9);
	if ((theFlags & BALL_BLOCK) != 0)
		aText.append("block,", 6);
	if ((theFlags & BALL_HEAVY) != 0)
		aText.append("heavy,", 6);
	if ((theFlags & BALL_STONE) != 0)
		aText.append("stone,", 6);
	if ((theFlags & BALL_BREAKABLE) != 0)
		aText.append("obstacle,", 9);
	const char* aMultiplier;
	if ((theFlags & BALL_X2) == 0)
	{
		if ((theFlags & BALL_X3) == 0)
		{
			if ((theFlags & BALL_X4) == 0)
				return aText;
			aMultiplier = "4x,";
		}
		else
		{
			aMultiplier = "3x,";
		}
	}
	else
	{
		aMultiplier = "2x,";
	}
	aText.append(aMultiplier, 3);
	return aText;
}

// 0x41e5b0
std::string Bubble::toXml()
{
	std::string aText("<bubble letter=\"", 16);
	aText.append(mLetters);
	aText.append("\" ", 2);
	aText.append("origin=\"", 8);
	aText.append(pointText(Point(mOrigin)));
	aText.append("\" ", 2);
	aText.append("special=\"", 9);
	aText.append(specialText(mFlags));
	aText.append("\" ", 2);
	aText.append("/>\n", 3);
	return aText;
}

// 0x41c760
Level::Level(Level* thePrevious)
{
	mBalls = 0;
	mTime = 60;
	mShortestWord = 3;
	mFreematch = false;
	mBallCriteria = 10;
	mSpawnDelay = 5.0f;
	mMaxBalls = 0;
	mLongestPiece = 2;
	mPieces = 0;
	mGravity = Point(0, 500.0f);
	for (int i = 0; i < 14; i++)
		mBonuses[i] = 0;
	if (thePrevious != NULL)
	{
		mShortestWord = thePrevious->mShortestWord;
		mTime = thePrevious->mTime;
		mBallCriteria = thePrevious->mBallCriteria;
		mMaxBalls = thePrevious->mMaxBalls;
		mSpawnDelay = thePrevious->mSpawnDelay;
		mLongestPiece = thePrevious->mLongestPiece;
	}
}

// 0x41c910
Level::Level(const Level& theLevel)
{
	mBalls = theLevel.mBalls;
	mTime = theLevel.mTime;
	mShortestWord = theLevel.mShortestWord;
	mFreematch = theLevel.mFreematch;
	mBallCriteria = theLevel.mBallCriteria;
	mSpawnDelay = theLevel.mSpawnDelay;
	mMaxBalls = theLevel.mMaxBalls;
	mLongestPiece = theLevel.mLongestPiece;
	mPieces = 0;
	mGravity = theLevel.mGravity;
	int i = 3;
	std::string* aText = mIntro;
	do
	{
		*aText = theLevel.mIntro[aText - mIntro];
		aText++;
		i--;
	} while (i != 0);
	aText = mWords;
	i = 14;
	do
	{
		*aText = theLevel.mWords[aText - mWords];
		aText++;
		i--;
	} while (i != 0);
	aText = mDefinitions;
	i = 14;
	do
	{
		*aText = theLevel.mDefinitions[aText - mDefinitions];
		aText++;
		i--;
	} while (i != 0);
	int* aBonus = mBonuses;
	i = 14;
	do
	{
		*aBonus = theLevel.mBonuses[aBonus - mBonuses];
		aBonus++;
		i--;
	} while (i != 0);
	std::vector<Obstacle*>::const_iterator anObstacle = theLevel.mObstacles.begin();
	if (anObstacle != theLevel.mObstacles.end())
	{
		do
		{
			Obstacle* aCopy = (*anObstacle)->clone();
			aCopy->addRef();
			mObstacles.push_back(aCopy);
			++anObstacle;
		} while (anObstacle != theLevel.mObstacles.end());
	}
	std::vector<Bubble*>::const_iterator aBubble = theLevel.mBubbles.begin();
	if (aBubble != theLevel.mBubbles.end())
	{
		do
		{
			Bubble* aSource = *aBubble;
			mBubbles.push_back(new Bubble(aSource->mOrigin, aSource->mLetters, aSource->mFlags));
			++aBubble;
		} while (aBubble != theLevel.mBubbles.end());
	}
}

// 0x41c2f0: the obstacles are deleted whatever their references (as in the original)
Level::~Level()
{
	for (unsigned int i = 0; i < mObstacles.size(); i++)
	{
		if (mObstacles[i] != NULL)
			delete mObstacles[i];
	}
	for (unsigned int i = 0; i < mBubbles.size(); i++)
	{
		if (mBubbles[i] != NULL)
			delete mBubbles[i];
	}
}

// 0x41cc50
void Level::addObstacle(Obstacle* theObstacle)
{
	mObstacles.push_back(theObstacle);
	theObstacle->addRef();
}

// 0x41ce30
void LevelHandler::load(std::string theFile, std::vector<Level*>* theLevels)
{
	mLevels = theLevels;
	mLevel = NULL;
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(*this, aStream);
	mLevels = NULL;
	mLevel = NULL;
}

// 0x41cf70: "level" starts a level (its rules from the attributes, the missing ones at their defaults, and the time
// and the longest piece from the level before); "intro", "word", "obstacle" and "bubble" add to it. A tag before the
// first level makes a parse error message that the release build never shows.
void LevelHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare(0, theName.size(), "levels", 6) == 0)
		return;
	if (theName.compare(0, theName.size(), "level", 5) == 0)
	{
		Level* aLevel = new Level(mLevel);
		mLevels->push_back(aLevel);
		mLevel = aLevel;
		mLevel->mGravity = getPointAttribute(theAttributes, "gravity", Point(0, 500.0f));
		mLevel->mShortestWord = getIntAttribute(theAttributes, "shortestWord", 3);
		mLevel->mBalls = getIntAttribute(theAttributes, "balls", 0);
		mLevel->mMaxBalls = getIntAttribute(theAttributes, "maxBalls", 0);
		mLevel->mBallCriteria = getIntAttribute(theAttributes, "ballCriteria", 0);
		if (getIntAttribute(theAttributes, "freematch", 0) == 0)
			mLevel->mFreematch = false;
		else
			mLevel->mFreematch = true;
		mLevel->mSpawnDelay = getFloatAttribute(theAttributes, "spawnDelay", 5.0f);
		return;
	}
	if (mLevel == NULL)
	{
		std::string anError = "Level parse error, unexpected tag: ";
		anError.append(theName);
		return;
	}

	if (theName.compare(0, theName.size(), "intro", 5) == 0)
	{
		std::string aText0 = getStringAttribute(theAttributes, "text0", "");
		std::string aText1 = getStringAttribute(theAttributes, "text1", "");
		std::string aText2 = getStringAttribute(theAttributes, "text2", "");
		if (aText0.size() != 0)
			mLevel->mIntro[0] = getStringAttribute(theAttributes, "text0", "");
		if (aText1.size() != 0)
			mLevel->mIntro[1] = getStringAttribute(theAttributes, "text1", "");
		if (aText2.size() != 0)
			mLevel->mIntro[2] = getStringAttribute(theAttributes, "text2", "");
	}
	if (theName.compare(0, theName.size(), "word", 4) == 0)
	{
		// Into the first free slot of the 14
		int i = 0;
		do
		{
			if (mLevel->mWords[i].size() == 0)
			{
				mLevel->mWords[i] = getStringAttribute(theAttributes, "text", "");
				mLevel->mDefinitions[i] = getStringAttribute(theAttributes, "definition", "");
				mLevel->mBonuses[i] = getIntAttribute(theAttributes, "bonus", 0);
				break;
			}
			i++;
		} while (i < 14);
	}
	if (theName.compare(0, theName.size(), "obstacle", 8) == 0)
	{
		Point anOrigin = getPointAttribute(theAttributes, "origin", Point(0, 0));
		char aLetter = '\0';
		std::string aText = getStringAttribute(theAttributes, "letter", "");
		if (aText.size() == 1)
			aLetter = aText[0];
		aText = getStringAttribute(theAttributes, "type", "");
		Obstacle* anObstacle;
		if (aText.compare(0, aText.size(), "breakable", 9) == 0)
		{
			// A breakable obstacle is a bubble of one letter with the obstacle flag (its extents unused)
			getPointAttribute(theAttributes, "extents", Point(0, 0));
			Bubble* aBubble = new Bubble();
			aBubble->mLetters = std::string(1, aLetter);
			aBubble->mOrigin = anOrigin;
			aBubble->mFlags = aBubble->mFlags | BALL_BREAKABLE;
			mLevel->mBubbles.push_back(aBubble);
			return;
		}
		else if (aText.compare(0, aText.size(), "box", 3) == 0)
		{
			Point anExtents = getPointAttribute(theAttributes, "extents", Point(0, 0));
			anObstacle = new Box(anOrigin, 0);
			anObstacle->setExtents(anExtents);
			anObstacle->setLetter(aLetter);
		}
		else
		{
			if (aText != "ramp")
				return;
			Point anExtents = getPointAttribute(theAttributes, "extents", Point(0, 0));
			int aRotation = getIntAttribute(theAttributes, "rotation", 0);
			anObstacle = new Ramp(anOrigin, aRotation);
			anObstacle->setExtents(anExtents);
			anObstacle->setLetter(aLetter);
		}
		anObstacle->setName("obstacle");
		mLevel->addObstacle(anObstacle);
	}
	else
	{
		if (theName.compare(0, theName.size(), "bubble", 6) != 0)
			return;
		Bubble* aBubble = new Bubble();
		std::string aText = getStringAttribute(theAttributes, "letter", "");
		if (aText.compare(0, aText.size(), "?", 1) == 0)
			aBubble->mFlags = aBubble->mFlags | BALL_WILDCARD;
		aBubble->mLetters = aText;
		aBubble->mOrigin = getPointAttribute(theAttributes, "origin", Point(0, 0));
		aText = getStringAttribute(theAttributes, "special", "");
		if (aText.size() != 0)
			aBubble->setSpecial(aText);
		mLevel->mBubbles.push_back(aBubble);
	}
}
