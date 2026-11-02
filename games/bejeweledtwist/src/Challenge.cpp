// The challenges' data and goal (Challenge.h)
#include "Challenge.h"

#include "Board.h"
#include "GameText.h"
#include "Profile.h"
#include "Ranks.h"
#include "WinTwistApp.h"
#include "X86Math.h"
#include "XMLParser.h"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

using namespace Sexy;

namespace {

// 0x84ea98: the names a rule may count and their stats (the app's and the board's arrays are indexed by them). The
// texts are read from the exe; the first name that matches is taken (a second "MaxChain" row, 0x2b, never is)
struct StatName {
	unsigned int mName;  // the text's address
	int mStat;
};
const StatName kStatNames[] = {
	{0x7df720, 62}, {0x7df70c, 64}, {0x7df6f0, 65}, {0x7df6e0, 66}, {0x7df6cc, 74}, {0x7df6b8, 69}, {0x7df69c, 71},
	{0x7df68c, 75}, {0x7df678, 76}, {0x7df664, 77}, {0x7df64c, 78}, {0x7df634, 79}, {0x7df61c, 80}, {0x7df608, 81},
	{0x7df5f4, 67}, {0x7df5e0, 68}, {0x7df5d0, 73}, {0x7df5c4, 36}, {0x7df5b0, 70}, {0x7df594, 28}, {0x7df570, 29},
	{0x7df55c, 42}, {0x7df550, 35}, {0x7df538, 45}, {0x7df524, 72}, {0x7df518, 33}, {0x7df4fc, 21}, {0x7df4d8, 20},
	{0x7df55c, 43}, {0x7df4c0, 44}, {0x7df4ac, 82}, {0x7df494, 83}, {0x7df484, 41},
};
// 0x84eba0: the comparisons a rule may use (<=, <, >=, =) and their codes
const StatName kOpNames[] = {{0x7df47c, 3}, {0x7df478, 4}, {0x7df470, 2}, {0x811d7c, 1}};

// The stats the rules and the goal check name by number (the indices of the app's and the board's stat arrays)
enum {
	STAT_BIGGEST_MATCH = 0x1c,
	STAT_LEVEL = 0x24,
	STAT_MOVES = 0x21,
	STAT_TIME = 0x23,
	STAT_GEMS = 0x3e,
	STAT_POINTS = 0x49,
	STAT_FIRST_COLOR = 0x4b,  // Red; White, Green, Yellow, Violet, Orange and Blue follow
	STAT_COUNT = 84
};

// 0x6d9807: the first error of a file is kept with its line and file
class ChallengeInfoParser {
public:
	ChallengeInfo* mInfo;  // +0x04
	XMLParser* mParser;    // +0x08
	SexyString mError;     // +0x0c
	bool mFailed = false;  // +0x28

	explicit ChallengeInfoParser(ChallengeInfo* theInfo) : mInfo(theInfo), mParser(nullptr) {}  // 0x6d9497
	~ChallengeInfoParser() { delete mParser; }

	// port: the original keeps the message and shows nothing; the log gets it (and the keyword errors that the
	// original shows through the app, 0x6dc574)
	void Fail(const SexyString& theText);
	void ParseFile(const std::string& theFileName);       // 0x6dcfaa
	void ParseCategories();                               // 0x6dcb26
	bool ParseCategory(ChallengeCategory* theCategory);   // 0x6dc8d9
	bool ParseChallenge(Challenge* theChallenge);         // 0x6dbc98
	SexyString ReadText();                                // 0x6dbb7e
	bool NextElement(XMLElement& theElement);
};

void ChallengeInfoParser::Fail(const SexyString& theText)
{
	if (mFailed)
		return;
	mFailed = true;
	mError = theText;
	if (mParser != nullptr && 0 < mParser->GetCurrentLineNum())
		mError += StrFormat(" on Line %d", mParser->GetCurrentLineNum());
	if (mParser != nullptr && !mParser->GetFileName().empty())
		mError += StrFormat(" in File '%s'", mParser->GetFileName().c_str());
	SDL_Log("challenges: %s", mError.c_str());
}

bool ChallengeInfoParser::NextElement(XMLElement& theElement)
{
	theElement.mAttributeIteratorList.clear();  // (the parser empties the attributes but not their order)
	return mParser->NextElement(&theElement);
}

// 0x6dbb7e: the text of the tag that has just started, up to its end tag (the last piece of text if there are several)
SexyString ChallengeInfoParser::ReadText()
{
	SexyString aText;
	XMLElement anElement;
	while (NextElement(anElement)) {
		if (anElement.mType == XMLElement::TYPE_START) {
			Fail("Unexpected Section: '" + anElement.mValue + "'");
			return aText;
		}
		if (anElement.mType == XMLElement::TYPE_ELEMENT)
			aText = anElement.mValue;
		else if (anElement.mType == XMLElement::TYPE_END)
			return aText;
	}
	return aText;
}

// 0x6dbc98: the tags of one <challenge>, then the flags its rules make; false when the file is wrong
bool ChallengeInfoParser::ParseChallenge(Challenge* theChallenge)
{
	XMLElement anElement;
	while (NextElement(anElement)) {
		if (anElement.mType == XMLElement::TYPE_START) {
			SexyString aTag = Upper(anElement.mValue);
			if (aTag == GT(0x7dfff8)) {  // description
				theChallenge->mDescription = RemoveChar(ReadText(), '"');
			} else if (aTag == GT(0x7dffd8)) {  // donedescription
				theChallenge->mDoneDescription = RemoveChar(ReadText(), '"');
			} else if (aTag == GT(0x7dffc0)) {  // counttitle
				theChallenge->mCountTitle = RemoveChar(ReadText(), '"');
			} else if (aTag == GT(0x7dffac)) {  // bestscore
				theChallenge->mBestScore = atol(ReadText().c_str());
			} else if (aTag == GT(0x7dff98)) {  // lowscore
				theChallenge->mLowScore = atol(ReadText().c_str());
			} else if (aTag == GT(0x7dff8c)) {  // rules
				// Spaces and quotes go; the rules are separated by commas
				SexyString aRules = RemoveChar(RemoveChar(ReadText(), ' '), '"');
				for (;;) {
					SexyString aField = FirstField(aRules);
					size_t aCut = aField.size() + 1;
					if (aRules.size() < aCut)
						aCut = aRules.size();
					aRules = aRules.substr(aCut);
					if (aField.empty())
						break;

					// A field is a name, a comparison and a value: "Gems>=8", "Onemove", "pattern='RRR'"
					SexyString aName, aOp;
					size_t i = 0;
					while (i < aField.size() && isalnum((unsigned char)aField[i]))
						aName += aField[i++];
					if (i < aField.size()) {
						while (i < aField.size() && (aField[i] == '>' || aField[i] == '<' || aField[i] == '='))
							aOp += aField[i++];
					}
					SexyString aValue = aField.substr(i);

					if (sexystricmp(aName.c_str(), GT(0x7dff7c).c_str()) == 0) {  // onemove
						theChallenge->mFlags |= Challenge::FLAG_ONEMOVE;
					} else if (sexystricmp(aName.c_str(), GT(0x7dff64).c_str()) == 0) {  // nocascades
						theChallenge->mFlags |= Challenge::FLAG_NOCASCADES;
					} else if (sexystricmp(aName.c_str(), GT(0x7dff54).c_str()) == 0) {  // onboard
						theChallenge->mFlags |= Challenge::FLAG_ONBOARD;
					} else if (sexystricmp(aName.c_str(), GT(0x7dff30).c_str()) == 0) {  // constantcounters
						theChallenge->mFlags |= Challenge::FLAG_CONSTANTCOUNTERS;
					} else if (sexystricmp(aName.c_str(), GT(0x7dff14).c_str()) == 0) {  // constantdoom
						theChallenge->mFlags |= Challenge::FLAG_CONSTANTDOOM;
					} else if (sexystricmp(aName.c_str(), GT(0x7dff04).c_str()) == 0) {  // pattern
						// The colours in order: R W G Y V O B (the letters at 0x7dfef4), in apostrophes
						theChallenge->mFlags |= Challenge::FLAG_PATTERN;
						SexyString aPattern = RemoveChar(aValue, '\'');
						const SexyString& aLetters = GT(0x7dfef4);
						for (size_t j = 0; j < aPattern.size(); j++) {
							int aColor = (int)aLetters.find(aPattern[j]);
							if (aColor < 0 || (int)aLetters.size() <= aColor) {
								Fail("Illegal pattern '" + aPattern + "'");
								aColor = 0;
							}
							theChallenge->mPattern.push_back(aColor);
						}
					} else if (sexystricmp(aName.c_str(), GT(0x7dfe8c).c_str()) == 0) {  // config
						theChallenge->mConfig = RemoveChar(aValue, '\'');
					} else if (sexystricmp(aName.c_str(), GT(0x7dfe78).c_str()) == 0) {  // nightmode
						theChallenge->mFlags |= Challenge::FLAG_NIGHTMODE;
					} else if (sexystricmp(aName.c_str(), GT(0x7dfe68).c_str()) == 0) {  // nocount
						theChallenge->mFlags |= Challenge::FLAG_NOCOUNT;
					} else if (sexystricmp(aName.c_str(), GT(0x7dfe50).c_str()) == 0) {  // quitonfail
						theChallenge->mFlags |= Challenge::FLAG_QUITONFAIL;
					} else if (sexystricmp(aName.c_str(), GT(0x7dfe40).c_str()) == 0) {  // badgem
						// The gems of one colour must not break: its name (the colours' names at 0x84ea7c)
						theChallenge->mFlags |= Challenge::FLAG_BADGEM;
						theChallenge->mBadGem = -1;
						for (int j = 0; j < 7; j++) {
							if (aValue == GT(GameInt(GTP(0x84ea7c, 7) + j * 4))) {
								theChallenge->mBadGem = j;
								break;
							}
						}
					} else {
						// A stat with a comparison and a number
						int aStat = -1;
						for (const StatName& aStatName : kStatNames) {
							if (sexystricmp(aName.c_str(), GT(aStatName.mName).c_str()) == 0) {
								aStat = aStatName.mStat;
								break;
							}
						}
						if (!aName.empty() && aStat == -1)
							Fail("Illegal keyword '" + aName + "'");
						aOp = Trim(aOp);
						int anOp = 0;
						for (const StatName& anOpName : kOpNames) {
							if (sexystricmp(aOp.c_str(), GT(anOpName.mName).c_str()) == 0) {
								anOp = anOpName.mStat;
								break;
							}
						}
						int aNumber = atoi(aValue.c_str());
						if (aStat == STAT_TIME)
							aNumber *= 60;  // minutes
						// port: a rule with no stat would index the stat arrays by -1
						if (0 <= aStat) {
							ChallengeRule aRule;
							aRule.mStat = aStat;
							aRule.mOp = anOp;
							aRule.mValue = aNumber;
							theChallenge->mRules.push_back(aRule);
						}
					}
				}
			} else {
				Fail("Invalid Section '" + anElement.mValue + "'");
				return false;
			}
		} else if (anElement.mType == XMLElement::TYPE_ELEMENT) {
			Fail("Element Not Expected '" + anElement.mValue + "'");
			return false;
		} else if (anElement.mType == XMLElement::TYPE_END) {
			// The flags the rules make
			int aStats[STAT_COUNT] = {};
			for (const ChallengeRule& aRule : theChallenge->mRules) {
				aStats[aRule.mStat] = aRule.mValue;
				if (aRule.mStat == STAT_TIME) {
					theChallenge->mFlags |= Challenge::FLAG_TIME;
					theChallenge->mTimeGoal = aRule.mValue;
					if (aRule.mOp == 3 || aRule.mOp == 4)
						theChallenge->mFlags |= Challenge::FLAG_TIME_LIMIT;
				}
				if (aRule.mStat == STAT_MOVES && aRule.mValue != 1) {
					theChallenge->mFlags |= Challenge::FLAG_MOVES;
					theChallenge->mMoveGoal = aRule.mValue;
				}
				if (aRule.mOp != 3 && aRule.mStat == STAT_POINTS)
					theChallenge->mFlags |= Challenge::FLAG_POINTS;
			}
			if (!theChallenge->mRules.empty() && aStats[STAT_LEVEL] == 0 && aStats[STAT_BIGGEST_MATCH] == 0)
				theChallenge->mFlags |= Challenge::FLAG_COUNT;
			return true;
		}
	}
	return false;
}

// 0x6dc8d9: the <challenge> tags of one category; false when the file is wrong
bool ChallengeInfoParser::ParseCategory(ChallengeCategory* theCategory)
{
	XMLElement anElement;
	while (NextElement(anElement)) {
		if (anElement.mType == XMLElement::TYPE_START) {
			if (Upper(anElement.mValue) != GT(0x7e0010)) {  // challenge
				Fail("Invalid Section '" + anElement.mValue + "'");
				return false;
			}
			Challenge* aChallenge = new Challenge();
			aChallenge->mCategory = (int)mInfo->mCategories.size();
			aChallenge->mNumber = (int)theCategory->mChallenges.size() + 1;
			if (ParseChallenge(aChallenge)) {
				if ((aChallenge->mFlags & Challenge::FLAG_NIGHTMODE) == 0) {
					theCategory->mChallenges.push_back(aChallenge);
				} else {
					delete theCategory->mSpecial;
					theCategory->mSpecial = aChallenge;
				}
			} else {
				// port: the original leaves it unfreed
				delete aChallenge;
			}
		} else if (anElement.mType == XMLElement::TYPE_ELEMENT) {
			Fail("Element Not Expected '" + anElement.mValue + "'");
			return false;
		} else if (anElement.mType == XMLElement::TYPE_END) {
			return true;
		}
	}
	return false;
}

// 0x6dcb26: the <Category> tags of the file
void ChallengeInfoParser::ParseCategories()
{
	if (!mParser->HasFailed()) {
		XMLElement anElement;
		while (NextElement(anElement)) {
			if (anElement.mType == XMLElement::TYPE_START) {
				if (anElement.mValue != GT(0x7e0078)) {  // Category
					Fail("Invalid Section '" + anElement.mValue + "'");
					break;
				}
				ChallengeCategory* aCategory = new ChallengeCategory();
				for (std::list<XMLParamMap::iterator>::iterator anItr = anElement.mAttributeIteratorList.begin();
					 anItr != anElement.mAttributeIteratorList.end(); ++anItr) {
					const SexyString& aKey = (*anItr)->first;
					const SexyString& aValue = (*anItr)->second;
					if (aKey == GT(0x80f1d4)) {  // id
						aCategory->mId = aValue;
					} else if (aKey == GT(0x7e0070)) {  // viz
						aCategory->mViz = aValue;
					} else if (aKey == GT(0x7e0064)) {  // bkg2d
						aCategory->mBackground = atoi(aValue.c_str());
					} else if (aKey.substr(0, 5) == GT(0x7e0058)) {  // layer0, layer1
						aCategory->mLayers.push_back(aValue);
					} else if (aKey == GT(0x7e0044)) {  // additive
						aCategory->mAdditive = true;
					} else if (aKey == GT(0x7e0038)) {  // color
						int aRed = 0, aGreen = 0, aBlue = 0;
						sscanf(aValue.c_str(), "%d,%d,%d", &aRed, &aGreen, &aBlue);
						aCategory->mColor = Color(aRed, aGreen, aBlue, 255);
					}
				}
				if (!ParseCategory(aCategory)) {
					// port: the original leaves it unfreed
					delete aCategory;
					break;
				}
				mInfo->mCategories.push_back(aCategory);
			} else if (anElement.mType == XMLElement::TYPE_ELEMENT) {
				Fail("Element Not Expected '" + anElement.mValue + "'");
				break;
			}
		}
	}
	if (mParser->HasFailed())
		Fail(mParser->GetErrorText());
	delete mParser;
	mParser = nullptr;
}

// 0x6dcfaa
void ChallengeInfoParser::ParseFile(const std::string& theFileName)
{
	mParser = new XMLParser();
	mParser->OpenFile(theFileName);
	ParseCategories();
}

}  // namespace

ChallengeCategory::~ChallengeCategory()
{
	for (Challenge* aChallenge : mChallenges)
		delete aChallenge;
	delete mSpecial;
}

ChallengeInfo::ChallengeInfo()
{
	Load();
}

ChallengeInfo::~ChallengeInfo()
{
	Clear();
}

// 0x6dbb45
void ChallengeInfo::Clear()
{
	for (ChallengeCategory* aCategory : mCategories)
		delete aCategory;
	mCategories.clear();
}

// 0x6dcff3
void ChallengeInfo::Load()
{
	Clear();
	ChallengeInfoParser aParser(this);
	aParser.ParseFile("properties/challenges.xml");
}

// 0x6d9783
SexyString ChallengeInfo::GetCategoryId(unsigned int theCategory) const
{
	if (mCategories.size() <= theCategory)
		return SexyString();
	return mCategories[theCategory]->mId;
}

// 0x6d97c3
std::string ChallengeInfo::GetViz(unsigned int theCategory) const
{
	if (mCategories.size() <= theCategory)
		return std::string();
	return mCategories[theCategory]->mViz;
}

// 0x6d8c6d
int ChallengeInfo::GetChallengeCount(unsigned int theCategory) const
{
	if (mCategories.size() <= theCategory)
		return 0;
	return (int)mCategories[theCategory]->mChallenges.size();
}

// 0x6d8c9a
int ChallengeInfo::GetBackground(unsigned int theCategory) const
{
	if (mCategories.size() <= theCategory)
		return 0;
	return mCategories[theCategory]->mBackground;
}

// 0x6d8cc2
Challenge* ChallengeInfo::GetChallenge(unsigned int theCategory, int theNumber)
{
	if (mCategories.size() <= theCategory)
		return nullptr;
	ChallengeCategory* aCategory = mCategories[theCategory];
	Challenge* aChallenge;
	if (GetChallengeCount(theCategory) < theNumber)
		aChallenge = aCategory->mSpecial;
	else
		aChallenge = aCategory->mChallenges[theNumber - 1];
	// port: a category with no special challenge would have the original write through a null pointer
	if (aChallenge != nullptr)
		aChallenge->mProgress = 0;
	return aChallenge;
}

// 0x6d8d19: the profile's mInts1F4 hold, for each category, the challenge it stands at (from 1)
int ChallengeInfo::GetPercentDone(const Profile* theProfile) const
{
	if (theProfile == nullptr)
		return 0;
	int aDone = 0;
	int aTotal = 0;
	if (0 < (int)mCategories.size()) {
		for (int i = 0; i < (int)mCategories.size(); i++) {
			const std::vector<int>& aStages = theProfile->mInts1F4;
			if (i < (int)aStages.size() && -1 < aStages[i] - 1)
				aDone += aStages[i] - 1;
			aTotal += GetChallengeCount(i);
		}
		if (aTotal != 0)
			return Ftol((double)aDone * 100.0 / (double)aTotal);
	}
	return 0;
}

// 0x6d86c9: a move is over; a pattern whose matches did not go on starts again
void ChallengeInfo::EndMove(Challenge* theChallenge)
{
	if (mPatternMatches == 0)
		theChallenge->mProgress = 0;
	mPatternMatches = 0;
}

// 0x6db0ec
std::vector<ChallengeGoal> ChallengeInfo::GetGoals(const Challenge* theChallenge) const
{
	std::vector<ChallengeGoal> aGoals;
	bool anOneMove = (theChallenge->mFlags & Challenge::FLAG_ONEMOVE) != 0;
	// The stats come from the last move, the gems on the board now, or the whole game
	const int* aStats;
	if (anOneMove)
		aStats = gBoard->mUnknown2234;
	else if (gBoard != nullptr && (theChallenge->mFlags & Challenge::FLAG_ONBOARD) != 0)
		aStats = gBoard->mUnknown3560;
	else
		aStats = gApp->mUnknownC74;
	for (const ChallengeRule& aRule : theChallenge->mRules) {
		ChallengeGoal aGoal;
		aGoal.mStat = aRule.mStat;
		aGoal.mOp = aRule.mOp;
		aGoal.mTarget = aRule.mValue;
		int aValue;
		if (aRule.mStat == STAT_GEMS) {
			// port: the original needs the board here
			if (aStats == gApp->mUnknownC74)
				aValue = gBoard != nullptr ? gBoard->mUnknown1FC : 0;
			else
				aValue = aStats[STAT_GEMS + 1] + aStats[STAT_GEMS];
		} else if (aRule.mStat == STAT_MOVES || aRule.mStat == STAT_TIME) {
			aValue = gApp->mUnknownC74[aRule.mStat];
		} else {
			aValue = aStats[aRule.mStat];
		}
		if (anOneMove && aRule.mStat == STAT_POINTS)
			aValue = gBoard->mUnknown168 - gBoard->mUnknown16C;
		aGoal.mCurrent = aValue;
		aGoals.push_back(aGoal);
	}
	return aGoals;
}

// 0x6db273
int ChallengeInfo::Check(Challenge* theChallenge, int* theValue, bool theFinal)
{
	bool aNightMode = (theChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0;
	if (mResult == 1 && (!theFinal || (theChallenge->mFlags & Challenge::FLAG_ONBOARD) != 0))
		return 1;

	gBoard->CountGems();  // 0x6a56a5: the gems on the board now
	std::vector<ChallengeGoal> aGoals = GetGoals(theChallenge);
	bool anAllMet = true;
	bool aPast = false;  // some "=" or "<=" rule's stat is past its value
	int aValue = 0;
	if ((theChallenge->mFlags & Challenge::FLAG_PATTERN) != 0) {
		// The matches of this move that are the pattern's next colours
		unsigned int j = 0;
		while (theChallenge->mProgress < (int)theChallenge->mPattern.size() && j < gBoard->mUnknown2614.size()) {
			MatchInfo& aMatch = gBoard->mUnknown2614[j];
			if (!aMatch.mUnknown30 && aMatch.mColor == theChallenge->mPattern[theChallenge->mProgress]) {
				mPatternMatches++;
				theChallenge->mProgress++;
				aMatch.mUnknown30 = true;
			}
			j++;
		}
		aValue = theChallenge->mProgress;
		anAllMet = (int)theChallenge->mPattern.size() <= aValue;
	} else if (!aGoals.empty()) {
		if (aGoals.size() == 1 && aGoals[0].mStat == STAT_MOVES)
			aValue = (int)gApp->mUnknownC74[STAT_MOVES];
		else
			aValue = aGoals[0].mCurrent;
	}

	// The gems of the colour that must not break: the ones that did are taken off the count, and the check fails
	if (-1 < theChallenge->mBadGem) {
		int aBroken = gApp->mUnknownC74[STAT_FIRST_COLOR + theChallenge->mBadGem];
		if (0 < aBroken) {
			gBoard->mUnknown1FC -= aBroken;
			gBoard->mUnknown1F8 -= aBroken;
			gApp->mUnknownC74[STAT_FIRST_COLOR + theChallenge->mBadGem] = 0;
			mResult = 2;
			theFinal = true;
			if (!aGoals.empty() && aGoals[0].mStat == STAT_GEMS)
				aValue -= aBroken;
		}
	}

	if (theValue != nullptr)
		*theValue = aValue;
	if (theFinal)
		return mResult;

	int aFirstPast = -1;
	bool aTimeOut = false;
	for (unsigned int i = 0; i < aGoals.size(); i++) {
		const ChallengeGoal& aGoal = aGoals[i];
		bool aMet = true;
		if (aGoal.mOp == 1) {
			aPast |= aGoal.mCurrent != aGoal.mTarget;
			aMet = aGoal.mCurrent == aGoal.mTarget;
		} else if (aGoal.mOp == 2) {
			aMet = aGoal.mTarget <= aGoal.mCurrent;
		} else if (aGoal.mOp == 3) {
			aPast |= aGoal.mTarget <= aGoal.mCurrent;
			aMet = aGoal.mCurrent <= aGoal.mTarget;
		} else if (aGoal.mOp == 4) {
			aMet = aGoal.mCurrent < aGoal.mTarget;
		}
		if (aGoal.mStat == STAT_MOVES || aGoal.mStat == STAT_TIME) {
			// A limit that has run out, once the board has settled
			aTimeOut = aGoal.mOp != 2 && !aMet && gBoard->mUnknown25A4;
			if (anAllMet && 0 < i) {
				aMet = true;
			} else if (aTimeOut) {
				mResult = 2;
				return mResult;
			}
		}
		anAllMet = anAllMet && aMet;
		if (!anAllMet && aPast && aFirstPast == -1)
			aFirstPast = (int)i;
	}
	mResult = (anAllMet && (!aNightMode || aTimeOut)) ? 1 : 0;
	if (theValue != nullptr && mResult != 2 && aFirstPast != -1) {
		if ((theChallenge->mFlags & Challenge::FLAG_QUITONFAIL) != 0) {
			mResult = 2;
		} else {
			// The stats that came before the miss start again
			*theValue = 0;
			for (int j = 0; j < aFirstPast; j++) {
				gApp->mUnknownC74[aGoals[j].mStat] = 0;
				gBoard->mUnknown2234[aGoals[j].mStat] = 0;
			}
		}
	}
	return mResult;
}

// 0x6db73e
SexyString ChallengeInfo::GetText(const Challenge* theChallenge, bool theShowCount, bool theDone,
	float* theFraction) const
{
	if (theChallenge == nullptr)
		return SexyString();
	const SexyString& aText = theDone ? theChallenge->mDoneDescription : theChallenge->mDescription;
	unsigned int aFlags = theChallenge->mFlags;
	std::vector<ChallengeGoal> aGoals;  // read when they are first needed
	SexyString aResult;
	unsigned int aNext = 0;
	for (unsigned int i = 0; i < aText.size(); i++) {
		if (aText[i] == '#') {
			// The next rule's value now (the time as minutes and seconds)
			if (aGoals.empty())
				aGoals = GetGoals(theChallenge);
			if (aNext < aGoals.size()) {
				int aCurrent = aGoals[aNext].mCurrent;
				if (aGoals[aNext].mStat == STAT_TIME)
					aResult += StrFormat(GT(0x7dfdd4).c_str(), aCurrent / 60, aCurrent % 60);
				else
					aResult += StrFormat("%d", aCurrent);
				aNext++;
			}
		} else {
			aResult += aText[i];
		}
	}
	if (theShowCount && (aFlags & Challenge::FLAG_NIGHTMODE) == 0) {
		if (aGoals.empty())
			aGoals = GetGoals(theChallenge);
		if ((aFlags & Challenge::FLAG_PATTERN) != 0) {
			aResult += StrFormat(GT(0x7dfdb0).c_str(), theChallenge->mProgress, (int)theChallenge->mPattern.size());
		} else if ((aFlags & Challenge::FLAG_COUNT) != 0 &&
			(aFlags & (Challenge::FLAG_ONBOARD | Challenge::FLAG_ONEMOVE)) == 0) {
			aResult += StrFormat(GT(0x7dfdb0).c_str(), aGoals[0].mCurrent, aGoals[0].mTarget);
		}
	}
	if (theFraction != nullptr) {
		float aFraction = 0.0f;
		if ((theChallenge->mFlags & Challenge::FLAG_ONEMOVE) == 0) {
			if ((aFlags & Challenge::FLAG_PATTERN) != 0) {
				aFraction = (float)((double)theChallenge->mProgress / (double)(unsigned int)theChallenge->mPattern.size());
			} else {
				// What the rules have of what they ask (not counting Moves and Time)
				int aHave = 0;
				int aAsk = 0;
				if (aGoals.empty())
					aGoals = GetGoals(theChallenge);
				for (const ChallengeGoal& aGoal : aGoals) {
					if (aGoal.mStat != STAT_MOVES && aGoal.mStat != STAT_TIME) {
						aHave += aGoal.mTarget < aGoal.mCurrent ? aGoal.mTarget : aGoal.mCurrent;
						aAsk += aGoal.mTarget;
					}
				}
				if (aAsk != 0)
					aFraction = (float)((double)aHave / (double)aAsk);
			}
		}
		*theFraction = aFraction;
	}
	return aResult;
}
