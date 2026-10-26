#include "Dictionary.h"
#include "Stream.h"
#include <cctype>

using namespace engine;

// 0x41fcd0
LetterSet::LetterSet(const std::string& theLetters)
{
	for (int i = 0; i < 26; i++)
		mCounts[i] = 0;
	mWildcards = 0;
	unsigned int i = 0;
	if (theLetters.size() != 0)
	{
		do
		{
			if (theLetters[i] == '?')
			{
				mWildcards++;
			}
			else
			{
				int aLetter = (char)tolower(theLetters[i]) - 'a';
				if (-1 < aLetter && aLetter < 26)
					mCounts[aLetter]++;
			}
			i++;
		} while (i < theLetters.size());
	}
	mTotal = (signed char)theLetters.size();
}

// 0x41f8f0
bool LetterSet::findPiece(char theLetter, unsigned int& theIndex)
{
	theIndex = 0;
	while (mPieces.size() != 0 && theIndex < mPieces.size())
	{
		if (mPieces[theIndex].c_str()[0] == theLetter)
			return true;
		theIndex++;
	}
	return false;
}

// 0x41fd70
void LetterSet::removePiece(unsigned int theIndex)
{
	mTotal = (signed char)(mTotal - (signed char)mPieces[theIndex].size());
	mPieces.erase(mPieces.begin() + theIndex);
}

// 0x41ff40
void LetterSet::addPiece(std::string thePiece)
{
	mPieces.push_back(thePiece);
	mTotal = (signed char)(mTotal + (signed char)thePiece.size());
}

AnagramNode::AnagramNode(char theLetter)
{
	mLetter = theLetter;
	mSibling = nullptr;
	mChild = nullptr;
	mDepth = 0;
}

// 0x41fdd0
AnagramNode::~AnagramNode()
{
	if (mSibling != nullptr)
		delete mSibling;
	if (mChild != nullptr)
		delete mChild;
}

// 0x41ffb0
int AnagramNode::add(const char* theKey, const char* theWord)
{
	if (mLetter == *theKey)
	{
		if (*theKey == '\0')
		{
			mWords.push_back(std::string(theWord));
		}
		else
		{
			if (mChild == nullptr)
				mChild = new AnagramNode(theKey[1]);
			mChild->add(theKey + 1, theWord);
		}
	}
	else
	{
		if (mSibling == nullptr)
			mSibling = new AnagramNode(*theKey);
		mDepth = (signed char)(mSibling->add(theKey, theWord) + 1);
	}
	return mDepth;
}

// 0x420100 (the original also fetches the app and uses nothing of it). A letter outside A-Z in the word list would
// index outside the counts, as in the original.
void AnagramNode::findAllLetters(LetterSet& theLetters, bool* theDone, std::vector<std::string>* theWords,
	AnagramNode* theRoot, const char* theChain)
{
	char c = mLetter;
	if (c == '\0' && (theChain == nullptr || *theChain == '\0'))
	{
		theWords->push_back(mWords[0]);
		if (theLetters.mTotal < 1)
		{
			*theDone = true;
			return;
		}
		if (3 < theLetters.mTotal)
		{
			theRoot->findAllLetters(theLetters, theDone, theWords, theRoot, theChain);
			if (*theDone)
				return;
		}
		if (theWords->size() != 0)
			theWords->pop_back();
	}
	else if (theChain == nullptr || *theChain == '\0')
	{
		unsigned int anIndex;
		if (theLetters.findPiece(c, anIndex))
		{
			std::string aPiece = theLetters.mPieces[anIndex];
			theLetters.removePiece(anIndex);
			if (mChild != nullptr)
			{
				mChild->findAllLetters(theLetters, theDone, theWords, theRoot, aPiece.c_str() + 1);
				if (*theDone)
					return;
			}
			theLetters.addPiece(aPiece);
		}
		if (0 < theLetters.mCounts[toupper(mLetter) - 'A'])
		{
			theLetters.mCounts[toupper(mLetter) - 'A']--;
			theLetters.mTotal--;
			if (mChild != nullptr)
			{
				mChild->findAllLetters(theLetters, theDone, theWords, theRoot, theChain);
				if (*theDone)
					return;
			}
			theLetters.mCounts[toupper(mLetter) - 'A']++;
			theLetters.mTotal++;
		}
		if (theLetters.mWildcards != 0)
		{
			theLetters.mWildcards--;
			theLetters.mTotal--;
			if (mChild != nullptr)
			{
				mChild->findAllLetters(theLetters, theDone, theWords, theRoot, nullptr);
				if (*theDone)
					return;
			}
			theLetters.mWildcards++;
			theLetters.mTotal++;
		}
	}
	else if (*theChain == c)
	{
		mChild->findAllLetters(theLetters, theDone, theWords, theRoot, theChain + 1);
		if (*theDone)
			return;
	}
	if (mSibling != nullptr)
		mSibling->findAllLetters(theLetters, theDone, theWords, theRoot, theChain);
}

// 0x420390
void AnagramNode::findWords(LetterSet& theLetters, std::vector<std::string>* theWords, const char* theChain)
{
	char c = mLetter;
	if (c == '\0' && (theChain == nullptr || *theChain == '\0'))
	{
		unsigned int aLength = mWords[0].size();
		if (3 < aLength && aLength < 7)
			theWords->push_back(mWords[0]);
	}
	else if (theChain == nullptr || *theChain == '\0')
	{
		unsigned int anIndex;
		if (theLetters.findPiece(c, anIndex))
		{
			std::string aPiece = theLetters.mPieces[anIndex];
			theLetters.removePiece(anIndex);
			if (mChild != nullptr)
				mChild->findWords(theLetters, theWords, aPiece.c_str() + 1);
			theLetters.addPiece(aPiece);
		}
		if (0 < theLetters.mCounts[toupper(mLetter) - 'A'])
		{
			theLetters.mCounts[toupper(mLetter) - 'A']--;
			theLetters.mTotal--;
			if (mChild != nullptr)
				mChild->findWords(theLetters, theWords, theChain);
			theLetters.mCounts[toupper(mLetter) - 'A']++;
			theLetters.mTotal++;
		}
		if (theLetters.mWildcards != 0)
		{
			theLetters.mWildcards--;
			theLetters.mTotal--;
			if (mChild != nullptr)
				mChild->findWords(theLetters, theWords, theChain);
			theLetters.mWildcards++;
			theLetters.mTotal++;
		}
	}
	else if (*theChain == c)
	{
		mChild->findWords(theLetters, theWords, theChain + 1);
	}
	if (mSibling != nullptr)
		mSibling->findWords(theLetters, theWords, theChain);
}

// 0x4208d0
void Dictionary::add(const char* theWord)
{
	std::string aKey(theWord);
	if (mRoot == nullptr)
		mRoot = new AnagramNode(aKey.c_str()[0]);
	mRoot->add(aKey.c_str(), theWord);
}

// 0x4205b0
bool Dictionary::findWords(const std::string& theLetters, const std::vector<std::string>& theChains,
	std::vector<std::string>* theWords)
{
	theWords->clear();
	LetterSet aLetters(theLetters);
	std::vector<std::string>::const_iterator aChain = theChains.begin();
	if (aChain != theChains.end())
	{
		do
		{
			std::string aPiece = *aChain;
			aLetters.mPieces.push_back(aPiece);
			aLetters.mTotal = (signed char)(aLetters.mTotal + (signed char)aPiece.size());
			++aChain;
		} while (aChain != theChains.end());
	}
	bool isDone = false;
	mRoot->findAllLetters(aLetters, &isDone, theWords, mRoot, nullptr);
	return isDone;
}

// 0x420720 (the original logs how many it found)
bool Dictionary::findAnyWords(const std::string& theLetters, const std::vector<std::string>& theChains,
	std::vector<std::string>& theWords)
{
	LetterSet aLetters(theLetters);
	std::vector<std::string>::const_iterator aChain = theChains.begin();
	if (aChain != theChains.end())
	{
		do
		{
			std::string aPiece = *aChain;
			aLetters.mPieces.push_back(aPiece);
			aLetters.mTotal = (signed char)(aLetters.mTotal + (signed char)aPiece.size());
			++aChain;
		} while (aChain != theChains.end());
	}
	mRoot->findWords(aLetters, &theWords, nullptr);
	if (theWords.size() == 0)
		return false;
	return true;
}

// 0x420d40
WordNode::~WordNode()
{
	if (mSibling != nullptr)
		delete mSibling;
	if (mChild != nullptr)
		delete mChild;
}

// 0x420dd0
void WordNode::add(const char* theWord)
{
	WordNode* aNode = this;
	for (;;)
	{
		if (aNode->mLetter == *theWord)
		{
			if (*theWord == '\0')
				return;
			if (aNode->mChild == nullptr)
				aNode->mChild = new WordNode(theWord[1]);
			theWord++;
			aNode = aNode->mChild;
			continue;
		}
		if (aNode->mSibling == nullptr)
			aNode->mSibling = new WordNode(*theWord);
		aNode = aNode->mSibling;
	}
}

// 0x420b70: the other case of a capital is tried among its siblings, and the better status kept
void WordNode::find(const char* theWord, bool theExact, int* theStatus, bool theFirst, WordNode** theLast)
{
	WordNode* aNode = this;
	for (;;)
	{
		if (theLast != nullptr)
			*theLast = aNode;
		if (theFirst)
			*theStatus = 0;
		char c = aNode->mLetter;
		if (c == *theWord)
		{
			if (aNode->mLetter == '\0')
				return;
			if (aNode->mChild == nullptr)
			{
				*theStatus = 4;
				return;
			}
			aNode->mChild->find(theWord + 1, theExact, theStatus, false, theLast);
			if (*theStatus == 0)
				return;
			if (aNode->mLetter < 'A' || 'Z' < aNode->mLetter)
				return;
			aNode = aNode->mSibling;
			if (aNode == nullptr)
				return;
			if (theExact)
				return;
			// The original passes its own argument's slot as the status
			int anOther = theFirst ? 1 : 0;
			aNode->find(theWord, false, &anOther, theFirst, theLast);
			int aStatus = *theStatus;
			if (anOther <= aStatus)
				aStatus = anOther;
			*theStatus = aStatus;
			return;
		}
		if (tolower(c) == tolower(*theWord) && !theExact)
		{
			if (*theStatus == 0)
			{
				if (aNode->mSibling == nullptr)
					*theStatus = 3;
				else
					aNode->mSibling->find(theWord, false, theStatus, theFirst, theLast);
			}
			if (*theStatus < 1 || 3 < *theStatus)
				return;
			if (aNode->mChild == nullptr)
			{
				*theStatus = 4;
				return;
			}
			int aStatus;
			if (theFirst && aNode->mLetter == tolower(*theWord))
				aStatus = 1;
			else
				aStatus = 2;
			*theStatus = aStatus;
			aNode = aNode->mChild;
			theFirst = false;
			theWord++;
			continue;
		}
		aNode = aNode->mSibling;
		if (aNode == nullptr)
		{
			*theStatus = 3;
			return;
		}
	}
}

// 0x420ec0
void WordNode::match(const char* thePattern, std::vector<std::string>* theWords, std::string thePrefix)
{
	if (mLetter == *thePattern || tolower(mLetter) == tolower(*thePattern) || *thePattern == '?')
	{
		if (mLetter == '\0' && *thePattern == '\0')
		{
			theWords->push_back(thePrefix);
			return;
		}
		if (mChild != nullptr)
			mChild->match(thePattern + 1, theWords, thePrefix + mLetter);
		if (mSibling == nullptr)
			return;
	}
	else if (mSibling == nullptr)
	{
		return;
	}
	mSibling->match(thePattern, theWords, thePrefix);
}

// 0x420b50
WordTree::~WordTree()
{
	if (mRoot != nullptr)
		delete mRoot;
}

// 0x420e60
void WordTree::add(const char* theWord)
{
	if (mRoot == nullptr)
	{
		mRoot = new WordNode(*theWord);
		mRoot->add(theWord);
		return;
	}
	mRoot->add(theWord);
}

// 0x420d10
int WordTree::checkWord(const char* theWord, bool theExact)
{
	if (mRoot == nullptr)
		return 3;
	int aStatus = theExact;
	mRoot->find(theWord, theExact, &aStatus, true, nullptr);
	return aStatus;
}

// 0x420fe0
int WordTree::findMatches(const char* thePattern, std::vector<std::string>* theWords)
{
	theWords->clear();
	if (mRoot != nullptr)
	{
		mRoot->match(thePattern, theWords, std::string(""));
		return (int)theWords->size();
	}
	return 0;
}

// 0x420a80
int LineReader::readLine(std::string& theLine)
{
	bool isStarted = false;
	theLine.erase(theLine.begin(), theLine.end());
	do
	{
		for (;;)
		{
			if (mCount <= mPos)
			{
				int aRead = mStream->read(mBuffer, 0, 0x400);
				if (aRead == 0)
					return (int)theLine.size();
				mCount = aRead;
				mPos = 0;
			}
			char c = mBuffer[mPos];
			mPos++;
			if (c == '\n' || c == '\r')
				break;
			isStarted = true;
			theLine.append(1, c);
		}
	} while (!isStarted);
	return (int)theLine.size();
}
