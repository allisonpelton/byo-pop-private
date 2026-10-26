#include "SavedGame.h"
#include "Hud.h"
#include "Layout.h"
#include "Level.h"
#include "Physics.h"
#include "Settings.h"
#include "Stream.h"
#include "Xml.h"

#include <cstdio>

using namespace engine;

// 0x41efe0
SavedGame::SavedGame(int theMode)
	: mMode(theMode)
{
}

// 0x41f4e0
SavedGame::~SavedGame()
{
	for (Bubble* aBubble : mBubbles)
		delete aBubble;
	for (Chain* aChain : mChains)
		delete aChain;
	for (FoundWord* aWord : mWords)
		delete aWord;
}

// 0x41e760
void SavedGame::setHeader(int theLevel, int theScore, float theRespawnTimer)
{
	mLevel = theLevel;
	mScore = theScore;
	mRespawnTimer = theRespawnTimer;
}

// 0x41f0d0
void SavedGame::addBall(Ball* theBall)
{
	Bubble* aBubble = new Bubble();
	Point* aPosition = theBall->getPosition();
	aBubble->mOrigin.x = aPosition->x;
	aBubble->mOrigin.y = aPosition->y;
	aBubble->mLetters.append(1, theBall->mLetter);
	aBubble->mFlags = theBall->mSpecial;
	mBubbles.push_back(aBubble);
}

// 0x41f1c0
void SavedGame::addBubble(char theLetter, const std::string& theSpecial, float x, float y)
{
	Bubble* aBubble = new Bubble();
	aBubble->mOrigin.x = x;
	aBubble->mOrigin.y = y;
	aBubble->mLetters.append(1, theLetter);
	aBubble->setSpecial(theSpecial);
	if (theLetter == '?')
		aBubble->mFlags = aBubble->mFlags | BALL_WILDCARD;
	mBubbles.push_back(aBubble);
}

// 0x41f2b0
void SavedGame::addEmpty()
{
	mBubbles.push_back(nullptr);
}

// 0x41f310
Chain* SavedGame::addChain(const std::string& theWord)
{
	Chain* aChain = new Chain(nullptr, 0);
	mChains.push_back(aChain);
	aChain->mWord = theWord;
	aChain->mAlong = false;
	return aChain;
}

// 0x41f7e0
void SavedGame::addChain(Chain* theChain)
{
	Chain* aChain = new Chain(nullptr, 0);
	aChain->mWord = theChain->mWord;
	mChains.push_back(aChain);
	aChain->mIndices = theChain->mIndices;
}

// 0x41f3d0
void SavedGame::addWord(const std::string& theDisplayText, const std::string& theWord, bool theMatched, int theScore,
	int theMultiplier, const std::vector<std::string>& theChains)
{
	FoundWord* aWord = new FoundWord();
	aWord->mWord = theDisplayText;
	aWord->mLetters = theWord;
	aWord->mFound = theMatched;
	aWord->mPoints = theScore;
	aWord->mMultiplier = theMultiplier;
	aWord->mChainWords = theChains;
	mWords.push_back(aWord);
}

// 0x41f6c0
void SavedGame::addWord(FoundWord* theWord)
{
	FoundWord* aWord = new FoundWord();
	aWord->mLetters = theWord->mLetters;
	aWord->mWord = theWord->mWord;
	aWord->mMultiplier = theWord->mMultiplier;
	aWord->mPoints = theWord->mPoints;
	aWord->mFound = theWord->mFound;
	aWord->mChainWords = theWord->mChainWords;
	mWords.push_back(aWord);
}

// 0x41f7d0
void SavedGame::addLetters(const std::string& theLetters)
{
	mQueue.push_back(theLetters);
}

// 0x41e820 (the original formats each line into a 256-byte buffer)
std::string SavedGame::toXml()
{
	char aLine[1024];
	std::snprintf(aLine, sizeof(aLine),
		"<level mode=\"%d\" level=\"%d\" score=\"%d\" respawnTimer=\"%f\" wc=\"%d\" hug=\"%d\" hul=\"%d\" ha=\"%d\" "
		"lp=\"%d\" lba=\"%d\" bonus=\"%s\" ilg=\"%d\" time=\"%d\" >\n",
		mMode, mLevel, mScore, (double)mRespawnTimer, mWildcards, mHintsUsed, mLevelHintsUsed, mHints, mLevelPoints,
		mLettersUsed, mBonusWord.c_str(), mTemplate, mTime);
	std::string aText(aLine);
	for (std::vector<Chain*>::iterator it = mChains.begin(); it != mChains.end(); ++it)
	{
		Chain* aChain = *it;
		if (aChain == nullptr)
			continue;
		std::snprintf(aLine, sizeof(aLine), "\t<rope text=\"%s\" >\n", aChain->mWord.c_str());
		aText.append(aLine);
		if (aChain->mIndices.size() != 0)
		{
			for (std::list<int>::iterator i = aChain->mIndices.begin(); i != aChain->mIndices.end(); ++i)
			{
				int anIndex = *i;
				Bubble* aBubble = mBubbles[anIndex];
				if (aBubble != nullptr)
				{
					aText.append("\t\t", 2);
					aText.append(aBubble->toXml());
					delete aBubble;  // (the original drops it)
				}
				mBubbles[anIndex] = nullptr;
			}
		}
		aText.append("\t</rope>\n", 9);
	}
	for (unsigned int i = 0; i < mBubbles.size(); i++)
	{
		Bubble* aBubble = mBubbles[i];
		if (aBubble != nullptr)
		{
			aText.append("\t", 1);
			aText.append(aBubble->toXml());
		}
	}
	aText.append("\t<HUD>\n", 7);
	for (unsigned int i = 0; i < mWords.size(); i++)
	{
		FoundWord* aWord = mWords[i];
		if (aWord == nullptr)
			continue;
		std::string aChains;
		for (unsigned int j = 0; j < aWord->mChainWords.size(); j++)
		{
			std::string aChainWord = aWord->mChainWords[j];
			aChains.append(aChainWord);
			aChains.append(",", 1);
		}
		std::snprintf(aLine, sizeof(aLine),
			"\t\t<word displayText=\"%s\" word=\"%s\" isMatched=\"%s\" score=\"%d\" multiplier=\"%d\" chains=\"%s\" />\n",
			aWord->mWord.c_str(), aWord->mLetters.c_str(), aWord->mFound ? "true" : "false", aWord->mPoints,
			aWord->mMultiplier, aChains.c_str());
		aText.append(aLine);
	}
	aText.append("\t</HUD>\n", 8);
	aText.append("\t<spawnqueue>\n", 14);
	for (unsigned int i = 0; i < mQueue.size(); i++)
	{
		std::string aLetters = mQueue[i];
		std::snprintf(aLine, sizeof(aLine), "\t\t<letters text=\"%s\" />\n", aLetters.c_str());
		aText.append(aLine);
	}
	aText.append("\t</spawnqueue>\n", 15);
	aText.append("</level>\n", 9);
	return aText;
}

namespace
{

// 0x40c8f0 (0x28 bytes with its virtual bases): the settings (+0x14), the saved game being read (+0x18), the bubbles
// read (+0x1c), the first bubble of the open rope (+0x20) and its chain (+0x24)
class SavedGameReader : public XmlHandler
{
public:
	explicit SavedGameReader(Settings* theSettings)
		: mSettings(theSettings), mGame(nullptr), mBubbleCount(0), mRopeStart(0), mRope(nullptr) {}

	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;  // 0x40cf90
	void endElement(const std::string& theName) override;                                       // 0x40cdc0

private:
	Settings* mSettings;
	SavedGame* mGame;
	int mBubbleCount;
	int mRopeStart;
	Chain* mRope;
};

void SavedGameReader::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName.compare("level") == 0)
	{
		int aMode = getIntAttribute(theAttributes, "mode", 0);
		int aLevel = getIntAttribute(theAttributes, "level", 0);
		int aScore = getIntAttribute(theAttributes, "score", 0);
		float aRespawn = getFloatAttribute(theAttributes, "respawnTimer", 0);
		SavedGame* aGame = mSettings->newSavedGame(aMode);
		aGame->setHeader(aLevel, aScore, aRespawn);
		mGame = aGame;
		aGame->mWildcards = getIntAttribute(theAttributes, "wc", 0);
		aGame->mBonusWord = getStringAttribute(theAttributes, "bonus", "");
		aGame->mHints = getIntAttribute(theAttributes, "ha", 0);
		aGame->mHintsUsed = getIntAttribute(theAttributes, "hug", 0);
		aGame->mLevelHintsUsed = getIntAttribute(theAttributes, "hul", 0);
		aGame->mLettersUsed = getIntAttribute(theAttributes, "lba", 0);
		aGame->mLevelPoints = getIntAttribute(theAttributes, "lp", 0);
		aGame->mTemplate = getIntAttribute(theAttributes, "ilg", 0);
		aGame->mTime = getIntAttribute(theAttributes, "time", 0);
		return;
	}
	if (mGame != nullptr && theName.compare("bubble") == 0)
	{
		std::string aLetter = getStringAttribute(theAttributes, "letter", " ");
		Point anOrigin = getPointAttribute(theAttributes, "origin", Point(0, 0));
		std::string aSpecial = getStringAttribute(theAttributes, "special", "");
		mGame->addBubble(aLetter.c_str()[0], aSpecial, anOrigin.x, anOrigin.y);
		mBubbleCount++;
		return;
	}
	if (mGame != nullptr && theName.compare("rope") == 0)
	{
		mRopeStart = mBubbleCount;
		mRope = mGame->addChain(getStringAttribute(theAttributes, "text", ""));
		return;
	}
	if (mGame == nullptr)
		return;
	if (theName.compare("word") == 0)
	{
		std::string aDisplayText = getStringAttribute(theAttributes, "displayText", "");
		std::string aWord = getStringAttribute(theAttributes, "word", "");
		bool aMatched = getBoolAttribute(theAttributes, "isMatched", false);
		int aScore = getIntAttribute(theAttributes, "score", 0);
		int aMultiplier = getIntAttribute(theAttributes, "multiplier", 1);
		std::string aChainText = getStringAttribute(theAttributes, "chains", "");
		// the words before each ',' (the writer ends each with one)
		std::vector<std::string> aChains;
		size_t aStart = 0;
		size_t aComma = aChainText.find(',', 0);
		while (aComma != std::string::npos)
		{
			aChains.push_back(aChainText.substr(aStart, aComma - aStart));
			aStart = aComma + 1;
			aComma = aChainText.find(',', aStart);
		}
		mGame->addWord(aDisplayText, aWord, aMatched, aScore, aMultiplier, aChains);
		return;
	}
	if (mGame == nullptr)
		return;
	if (theName.compare("letters") == 0)
		mGame->addLetters(getStringAttribute(theAttributes, "text", ""));
}

// The end of a rope: the bubbles read inside it are its balls
void SavedGameReader::endElement(const std::string& theName)
{
	if (mGame != nullptr && mRope != nullptr && theName.compare("rope") == 0)
	{
		int i = mRopeStart;
		if (i < mBubbleCount)
		{
			do
			{
				mRope->mIndices.push_back(i);
				i++;
			} while (i < mBubbleCount);
		}
		mRope = nullptr;
	}
}

}

void readSavedGame(Settings* theSettings, const std::string& theText)
{
	SavedGameReader aReader(theSettings);
	std::vector<uint8_t> aData(theText.begin(), theText.end());
	Ref<InputStream> aStream = new MemoryStream(std::move(aData));
	parseXml(aReader, aStream);
}
