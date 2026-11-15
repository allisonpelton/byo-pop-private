#include "UserManager.h"

#include "Application.h"
#include "Buffer.h"
#include "Common.h"
#include "ProfileHash.h"
#include "XmlLite.h"

using namespace Sexy;

// A player's file, users\<id>.xml: SaveProfile writes it (0x46b1a0), Load reads it (0x46e500). Everything the game keeps of a
// player is in one <user> element, and every part that a game could be cheated through carries a "hash": the text of what the
// part holds (each number as it is printed, each string as it is) is made one string in the order of the file, and the hash is
// that string's MD5. A part whose hash does not come out the same on reading is dropped: a saved game as if it had never been
// stored, a list of restart points as if it were empty.

namespace
{

// 0x46ad60: the number of an attribute (0 where the element has none), whose text also joins what the hash is made of
int ReadHashedInt(const XmlNode* theElement, const char* theName, std::string& theHashText)
{
	int aValue = theElement->GetIntAttribute(theName);
	theHashText += StrFormat("%d", aValue);
	return aValue;
}

// The text of a number as the hash takes it: a negative zero is a zero (0x46ae30, 0x46af60)
std::string DoubleHashText(double theValue)
{
	std::string aText = StrFormat("%f", theValue);
	if (aText == "-0.000000")
		aText = "0.000000";
	return aText;
}

// 0x46ae30
double ReadHashedDouble(const XmlNode* theElement, const char* theName, std::string& theHashText)
{
	double aValue = theElement->GetDoubleAttribute(theName);
	theHashText += DoubleHashText(aValue);
	return aValue;
}

// 0x46af10: an attribute that is not there leaves the string as it was and adds nothing to the hash
void ReadHashedString(const XmlNode* theElement, const char* theName, std::string& theValue, std::string& theHashText)
{
	const std::string* aText = theElement->FindAttribute(theName);
	if (aText != nullptr)
	{
		theValue = *aText;
		theHashText += theValue;
	}
}

// 0x46b030
void WriteHashedInt(XmlNode* theElement, const std::string& theName, int theValue, std::string& theHashText)
{
	theElement->SetAttribute(theName, theValue);
	theHashText += StrFormat("%d", theValue);
}

// 0x46af60
void WriteHashedDouble(XmlNode* theElement, const std::string& theName, double theValue, std::string& theHashText)
{
	theElement->SetAttribute(theName, theValue);
	theHashText += DoubleHashText(theValue);
}

// 0x46b0f0
void WriteHashedString(XmlNode* theElement, const std::string& theName, const std::string& theValue, std::string& theHashText)
{
	theElement->SetAttribute(theName, theValue);
	theHashText += theValue;
}

// The flag elements of <user>: a name and the text "true" or "false"
void WriteFlagElement(XmlNode* theUserElement, const char* theName, bool theValue)
{
	theUserElement->AddElement(theName)->AddText(theValue ? "true" : "false");
}

// The text of a flag element is true when it starts with a "t" (an element with no text is false)
bool ReadFlagElement(const XmlNode* theElement)
{
	std::string aText = theElement->GetText();
	return !aText.empty() && aText[0] == 't';
}

// A list of restart points: one <resurrect state="..."/> for each, and the hash of the states put one after the other
void WriteRestartPoints(XmlNode* theUserElement, const char* theName, const std::vector<RestartPoint>& thePoints)
{
	XmlNode* aList = theUserElement->AddElement(theName);
	std::string aHashText;
	for (size_t i = 0; i < thePoints.size(); i++)
	{
		XmlNode* aPoint = aList->AddElement("resurrect");
		WriteHashedString(aPoint, "state", thePoints[i].ToString(), aHashText);
	}
	aList->SetAttribute("hash", HashProfileText(aHashText));
}

void WriteCubby(XmlNode* theParent, const CubbyState& theCubby, std::string& theHashText)
{
	XmlNode* aCubby = theParent->AddElement("cubby");
	WriteHashedString(aCubby, "name", theCubby.mName, theHashText);
	WriteHashedInt(aCubby, "isactive", theCubby.mActive ? 1 : 0, theHashText);
	WriteHashedInt(aCubby, "state", theCubby.mState, theHashText);
	WriteHashedInt(aCubby, "type", theCubby.mVariant, theHashText);
	WriteHashedDouble(aCubby, "contentpos", theCubby.mRise, theHashText);
	WriteHashedDouble(aCubby, "statedelay", theCubby.mTimer, theHashText);
	WriteHashedInt(aCubby, "numcreaturesspawn", theCubby.mCreatureCount, theHashText);
	WriteHashedInt(aCubby, "hascreature", theCubby.mHasCreature ? 1 : 0, theHashText);
	if (theCubby.mHasCreature)
	{
		WriteHashedInt(aCubby, "creature_state", theCubby.mCreatureState, theHashText);
		WriteHashedInt(aCubby, "creature_mustdelete", theCubby.mFinished ? 1 : 0, theHashText);
		WriteHashedInt(aCubby, "creature_movedown", theCubby.mSpent ? 1 : 0, theHashText);
		WriteHashedInt(aCubby, "creature_leavesequence", theCubby.mUntouchable ? 1 : 0, theHashText);
		WriteHashedInt(aCubby, "creature_type", theCubby.mKind, theHashText);
		WriteHashedDouble(aCubby, "creature_ypos", theCubby.mHeight, theHashText);
		WriteHashedDouble(aCubby, "creature_yvel", theCubby.mHeightVelocity, theHashText);
		WriteHashedDouble(aCubby, "creature_yaccel", theCubby.mGravity, theHashText);
		WriteHashedInt(aCubby, "creature_hitpoints", theCubby.mHealth, theHashText);
		WriteHashedInt(aCubby, "creature_freezetime", theCubby.mFrozenTimer, theHashText);
		WriteHashedInt(aCubby, "creature_isbombexploding", theCubby.mShatters ? 1 : 0, theHashText);
		WriteHashedString(aCubby, "creature_specificstate", theCubby.mExtra, theHashText);
	}
}

CubbyState ReadCubby(const XmlNode* theElement, std::string& theHashText)
{
	CubbyState aCubby = CubbyState();
	ReadHashedString(theElement, "name", aCubby.mName, theHashText);
	aCubby.mActive = ReadHashedInt(theElement, "isactive", theHashText) != 0;
	aCubby.mState = ReadHashedInt(theElement, "state", theHashText);
	aCubby.mVariant = ReadHashedInt(theElement, "type", theHashText);
	aCubby.mRise = ReadHashedDouble(theElement, "contentpos", theHashText);
	aCubby.mTimer = ReadHashedDouble(theElement, "statedelay", theHashText);
	aCubby.mCreatureCount = ReadHashedInt(theElement, "numcreaturesspawn", theHashText);
	aCubby.mHasCreature = ReadHashedInt(theElement, "hascreature", theHashText) != 0;
	if (aCubby.mHasCreature)
	{
		aCubby.mCreatureState = ReadHashedInt(theElement, "creature_state", theHashText);
		aCubby.mFinished = ReadHashedInt(theElement, "creature_mustdelete", theHashText) != 0;
		aCubby.mSpent = ReadHashedInt(theElement, "creature_movedown", theHashText) != 0;
		aCubby.mUntouchable = ReadHashedInt(theElement, "creature_leavesequence", theHashText) != 0;
		aCubby.mKind = ReadHashedInt(theElement, "creature_type", theHashText);
		aCubby.mHeight = ReadHashedDouble(theElement, "creature_ypos", theHashText);
		aCubby.mHeightVelocity = ReadHashedDouble(theElement, "creature_yvel", theHashText);
		aCubby.mGravity = ReadHashedDouble(theElement, "creature_yaccel", theHashText);
		aCubby.mHealth = ReadHashedInt(theElement, "creature_hitpoints", theHashText);
		aCubby.mFrozenTimer = ReadHashedInt(theElement, "creature_freezetime", theHashText);
		aCubby.mShatters = ReadHashedInt(theElement, "creature_isbombexploding", theHashText) != 0;
		ReadHashedString(theElement, "creature_specificstate", aCubby.mExtra, theHashText);
	}
	return aCubby;
}

void WriteBouncingGnome(XmlNode* theParent, const BouncingState& theGnome, std::string& theHashText)
{
	XmlNode* aGnome = theParent->AddElement("bouncinggnome");
	WriteHashedString(aGnome, "name", theGnome.mName, theHashText);
	WriteHashedInt(aGnome, "active", theGnome.mActive ? 1 : 0, theHashText);
	WriteHashedDouble(aGnome, "animationcount", theGnome.mFrame, theHashText);
	WriteHashedDouble(aGnome, "vel_x", theGnome.mSpeedX, theHashText);
	WriteHashedDouble(aGnome, "vel_y", theGnome.mSpeedY, theHashText);
	WriteHashedInt(aGnome, "bouncey", theGnome.mFloorY, theHashText);
	WriteHashedInt(aGnome, "countuntilactive", theGnome.mUnusedInt, theHashText);
	WriteHashedInt(aGnome, "pickrect_mx", theGnome.mBounds.mX, theHashText);
	WriteHashedInt(aGnome, "pickrect_my", theGnome.mBounds.mY, theHashText);
	WriteHashedInt(aGnome, "pickrect_mwidth", theGnome.mBounds.mWidth, theHashText);
	WriteHashedInt(aGnome, "pickrect_mheight", theGnome.mBounds.mHeight, theHashText);
	WriteHashedDouble(aGnome, "pos_x", theGnome.mX, theHashText);
	WriteHashedDouble(aGnome, "pos_y", theGnome.mY, theHashText);
	WriteHashedDouble(aGnome, "rotation", theGnome.mRotation, theHashText);
	WriteHashedDouble(aGnome, "rotationvel", theGnome.mRotationSpeed, theHashText);
	WriteHashedInt(aGnome, "hit", theGnome.mHit ? 1 : 0, theHashText);
	WriteHashedInt(aGnome, "numcoinslaunched", theGnome.mCount, theHashText);
	WriteHashedInt(aGnome, "saidhello", theGnome.mCalled ? 1 : 0, theHashText);
	WriteHashedInt(aGnome, "dontdestroy", theGnome.mHoldCount, theHashText);
}

BouncingState ReadBouncingGnome(const XmlNode* theElement, std::string& theHashText)
{
	BouncingState aGnome = BouncingState();
	ReadHashedString(theElement, "name", aGnome.mName, theHashText);
	aGnome.mActive = ReadHashedInt(theElement, "active", theHashText) != 0;
	aGnome.mFrame = ReadHashedDouble(theElement, "animationcount", theHashText);
	aGnome.mSpeedX = ReadHashedDouble(theElement, "vel_x", theHashText);
	aGnome.mSpeedY = ReadHashedDouble(theElement, "vel_y", theHashText);
	aGnome.mFloorY = ReadHashedInt(theElement, "bouncey", theHashText);
	aGnome.mUnusedInt = ReadHashedInt(theElement, "countuntilactive", theHashText);
	aGnome.mBounds.mX = ReadHashedInt(theElement, "pickrect_mx", theHashText);
	aGnome.mBounds.mY = ReadHashedInt(theElement, "pickrect_my", theHashText);
	aGnome.mBounds.mWidth = ReadHashedInt(theElement, "pickrect_mwidth", theHashText);
	aGnome.mBounds.mHeight = ReadHashedInt(theElement, "pickrect_mheight", theHashText);
	aGnome.mX = ReadHashedDouble(theElement, "pos_x", theHashText);
	aGnome.mY = ReadHashedDouble(theElement, "pos_y", theHashText);
	aGnome.mRotation = ReadHashedDouble(theElement, "rotation", theHashText);
	aGnome.mRotationSpeed = ReadHashedDouble(theElement, "rotationvel", theHashText);
	aGnome.mHit = ReadHashedInt(theElement, "hit", theHashText) != 0;
	aGnome.mCount = ReadHashedInt(theElement, "numcoinslaunched", theHashText);
	aGnome.mCalled = ReadHashedInt(theElement, "saidhello", theHashText) != 0;
	aGnome.mHoldCount = ReadHashedInt(theElement, "dontdestroy", theHashText);
	return aGnome;
}

// The aviator has no "bouncey", and its "state" comes last; which way it is turned (mLeft) is not stored
void WriteFlyingGnome(XmlNode* theParent, const FlyingState& theGnome, std::string& theHashText)
{
	XmlNode* aGnome = theParent->AddElement("flyinggnome");
	WriteHashedString(aGnome, "name", theGnome.mName, theHashText);
	WriteHashedInt(aGnome, "active", theGnome.mActive ? 1 : 0, theHashText);
	WriteHashedDouble(aGnome, "animationcount", theGnome.mFrame, theHashText);
	WriteHashedDouble(aGnome, "vel_x", theGnome.mSpeedX, theHashText);
	WriteHashedDouble(aGnome, "vel_y", theGnome.mSpeedY, theHashText);
	WriteHashedInt(aGnome, "countuntilactive", theGnome.mUnusedInt, theHashText);
	WriteHashedInt(aGnome, "pickrect_mx", theGnome.mBounds.mX, theHashText);
	WriteHashedInt(aGnome, "pickrect_my", theGnome.mBounds.mY, theHashText);
	WriteHashedInt(aGnome, "pickrect_mwidth", theGnome.mBounds.mWidth, theHashText);
	WriteHashedInt(aGnome, "pickrect_mheight", theGnome.mBounds.mHeight, theHashText);
	WriteHashedDouble(aGnome, "pos_x", theGnome.mX, theHashText);
	WriteHashedDouble(aGnome, "pos_y", theGnome.mY, theHashText);
	WriteHashedDouble(aGnome, "rotation", theGnome.mRotation, theHashText);
	WriteHashedDouble(aGnome, "rotationvel", theGnome.mRotationSpeed, theHashText);
	WriteHashedInt(aGnome, "hit", theGnome.mHit ? 1 : 0, theHashText);
	WriteHashedInt(aGnome, "numcoinslaunched", theGnome.mCount, theHashText);
	WriteHashedInt(aGnome, "saidhello", theGnome.mCalled ? 1 : 0, theHashText);
	WriteHashedInt(aGnome, "dontdestroy", theGnome.mHoldCount, theHashText);
	WriteHashedInt(aGnome, "state", theGnome.mState, theHashText);
}

FlyingState ReadFlyingGnome(const XmlNode* theElement, std::string& theHashText)
{
	FlyingState aGnome = FlyingState();
	ReadHashedString(theElement, "name", aGnome.mName, theHashText);
	aGnome.mActive = ReadHashedInt(theElement, "active", theHashText) != 0;
	aGnome.mFrame = ReadHashedDouble(theElement, "animationcount", theHashText);
	aGnome.mSpeedX = ReadHashedDouble(theElement, "vel_x", theHashText);
	aGnome.mSpeedY = ReadHashedDouble(theElement, "vel_y", theHashText);
	aGnome.mUnusedInt = ReadHashedInt(theElement, "countuntilactive", theHashText);
	aGnome.mBounds.mX = ReadHashedInt(theElement, "pickrect_mx", theHashText);
	aGnome.mBounds.mY = ReadHashedInt(theElement, "pickrect_my", theHashText);
	aGnome.mBounds.mWidth = ReadHashedInt(theElement, "pickrect_mwidth", theHashText);
	aGnome.mBounds.mHeight = ReadHashedInt(theElement, "pickrect_mheight", theHashText);
	aGnome.mX = ReadHashedDouble(theElement, "pos_x", theHashText);
	aGnome.mY = ReadHashedDouble(theElement, "pos_y", theHashText);
	aGnome.mRotation = ReadHashedDouble(theElement, "rotation", theHashText);
	aGnome.mRotationSpeed = ReadHashedDouble(theElement, "rotationvel", theHashText);
	aGnome.mHit = ReadHashedInt(theElement, "hit", theHashText) != 0;
	aGnome.mCount = ReadHashedInt(theElement, "numcoinslaunched", theHashText);
	aGnome.mCalled = ReadHashedInt(theElement, "saidhello", theHashText) != 0;
	aGnome.mHoldCount = ReadHashedInt(theElement, "dontdestroy", theHashText);
	aGnome.mState = ReadHashedInt(theElement, "state", theHashText);
	return aGnome;
}

// One <savegame> of <user>. The hash is of everything in the element, the attributes first and then the lists, in the order they
// are written. The state is 1 from here on (the file holds what it was when the game was stored, 3).
void WriteSavedGame(XmlNode* theUserElement, int theMode, SavedGame& theSaved)
{
	std::string aHashText;
	XmlNode* aGame = theUserElement->AddElement("savegame");
	WriteHashedInt(aGame, "mode", theMode, aHashText);
	WriteHashedInt(aGame, "version", theSaved.mVersion, aHashText);
	WriteHashedInt(aGame, "state", theSaved.mState, aHashText);
	WriteHashedInt(aGame, "levelnum", theSaved.mLevel, aHashText);
	WriteHashedDouble(aGame, "time", theSaved.mTime, aHashText);
	WriteHashedInt(aGame, "challengenum", theSaved.mBonusRounds, aHashText);
	for (int i = 0; i < GameScreen::NUM_SHUFFLED_BOARDS; i++)
		WriteHashedInt(aGame, StrFormat("challengeorder%d", i), theSaved.mBoardOrder[i], aHashText);
	WriteHashedInt(aGame, "gnomebagfills", theSaved.mCycle, aHashText);
	WriteHashedDouble(aGame, "timerstart", theSaved.mTimerTotal, aHashText);
	WriteHashedDouble(aGame, "timer", theSaved.mTimerLeft, aHashText);
	WriteHashedDouble(aGame, "timerdecrease", theSaved.mTimerRate, aHashText);
	WriteHashedInt(aGame, "waveinterludestate", theSaved.mTimerPhase, aHashText);
	WriteHashedInt(aGame, "inlevelintro", theSaved.mInLevelIntro ? 1 : 0, aHashText);
	WriteHashedInt(aGame, "inshop", theSaved.mInShop ? 1 : 0, aHashText);
	WriteHashedInt(aGame, "inchallenge", theSaved.mInChallenge ? 1 : 0, aHashText);
	WriteHashedInt(aGame, "inchallengeexplain", theSaved.mInChallengeExplain ? 1 : 0, aHashText);
	WriteHashedString(aGame, "challengestate", theSaved.mChallengeState, aHashText);
	WriteHashedInt(aGame, "gameplay_state", theSaved.mGameplayPhase, aHashText);
	WriteHashedDouble(aGame, "gameplay_statedelay", theSaved.mPhaseTimer, aHashText);
	WriteHashedDouble(aGame, "gameplay_countdownforcreate", theSaved.mNextPopTimer, aHashText);
	WriteHashedDouble(aGame, "gameplay_luckyclovertimer", theSaved.mCloverTimer, aHashText);
	WriteHashedDouble(aGame, "gameplay_strawberrytimer", theSaved.mStrawberryTimer, aHashText);
	WriteHashedDouble(aGame, "gameplay_poweruptimer", theSaved.mDoubleHitTimer, aHashText);
	WriteHashedDouble(aGame, "gameplay_multipliertimer", theSaved.mFiveTimesTimer, aHashText);
	WriteHashedInt(aGame, "gameplay_instantcreate", theSaved.mPopNow ? 1 : 0, aHashText);
	WriteHashedInt(aGame, "gameplay_firstgnomecreated", theSaved.mFirstCubbyStarted ? 1 : 0, aHashText);
	WriteHashedInt(aGame, "gameplay_saveitemhasbeencreated", theSaved.mSaveItemHasBeenCreated ? 1 : 0, aHashText);
	WriteHashedInt(aGame, "gameplay_mustcreatesaveitem", theSaved.mMustCreateSaveItem ? 1 : 0, aHashText);
	WriteHashedInt(aGame, "gameplay_pickcoinsandhearts", theSaved.mFlagCC ? 1 : 0, aHashText);
	WriteHashedInt(aGame, "avatar_clicksonlevel", theSaved.mClicksOnLevel, aHashText);

	XmlNode* aCounters = aGame->AddElement("counters");
	for (size_t i = 0; i < theSaved.mStats.size(); i++)
		WriteHashedDouble(aCounters->AddElement("counter"), "value", theSaved.mStats[i], aHashText);

	XmlNode* aCubbies = aGame->AddElement("cubbies");
	for (size_t i = 0; i < theSaved.mCubbies.size(); i++)
		WriteCubby(aCubbies, theSaved.mCubbies[i], aHashText);

	XmlNode* aBouncing = aGame->AddElement("bouncinggnomes");
	for (size_t i = 0; i < theSaved.mBouncing.size(); i++)
		WriteBouncingGnome(aBouncing, theSaved.mBouncing[i], aHashText);

	XmlNode* aFlying = aGame->AddElement("flyinggnomes");
	for (size_t i = 0; i < theSaved.mFlying.size(); i++)
		WriteFlyingGnome(aFlying, theSaved.mFlying[i], aHashText);

	XmlNode* aStickers = aGame->AddElement("stickers");
	for (size_t i = 0; i < theSaved.mStickers.size(); i++)
	{
		XmlNode* aSticker = aStickers->AddElement("sticker");
		WriteHashedString(aSticker, "name", theSaved.mStickers[i].mName, aHashText);
		WriteHashedInt(aSticker, "hit", theSaved.mStickers[i].mHit ? 1 : 0, aHashText);
	}

	XmlNode* aGnomeBag = aGame->AddElement("gnomebag");
	WriteHashedInt(aGnomeBag, "currentgnomeindex", theSaved.mKindIndex, aHashText);
	for (size_t i = 0; i < theSaved.mKindList.size(); i++)
		WriteHashedInt(aGnomeBag->AddElement("item"), "value", theSaved.mKindList[i], aHashText);

	theSaved.mState = 1;
	aGame->SetAttribute("hash", HashProfileText(aHashText));
}

}

// 0x46b1a0: the player's file is written whole: the flags, the trophies, the two lists of restart points and a <savegame> for
// each mode that has a game stored. The state of a saved game that is written becomes 1.
void User::SaveProfile()
{
	XmlNode aDocument;
	XmlNode* aUser = aDocument.AddElement("user");
	aUser->SetAttribute("version", std::string(PROFILE_VERSION));
	aUser->SetAttribute("name", mName);
	aUser->SetAttribute("uuid", mId);

	WriteFlagElement(aUser, "iscurrent", mIsCurrent);
	WriteFlagElement(aUser, "showhelpscreen", mShowHowToPlay);
	WriteFlagElement(aUser, "hascompletedclassic", mToughUnlocked);
	WriteFlagElement(aUser, "cookieexplanationshown", mToughAnnounced);

	std::string aTrophyText;
	for (int i = 0; i < NUM_TROPHIES; i++)
		aTrophyText += mTrophies[i] ? "1" : "0";
	aUser->AddElement("trophyflags")->AddText(aTrophyText);

	WriteRestartPoints(aUser, "resurrectsnormal", mClassicRestartPoints);
	WriteRestartPoints(aUser, "resurrectstough", mToughRestartPoints);

	int aMode = 0;
	do
	{
		SavedGame& aSaved = mSavedGames[aMode];
		if (aSaved.mState != 0)
			WriteSavedGame(aUser, aMode, aSaved);
		aMode++;
	}
	while (aMode < NUM_SAVED_GAMES);

	std::string aFileText = aDocument.ToFileText();
	gApp->WriteBytesToFile(StrFormat("users\\%d.xml", mId), aFileText.data(), (unsigned long)aFileText.size());
}

// 0x46e500: reads the file of the player's number. A player that is not in the file, or a file that is not whole XML, stays as
// the start made it (no name, no version: the roster does not take it).
void User::Load()
{
	mName.clear();
	mIsCurrent = false;
	mShowHowToPlay = true;
	mToughUnlocked = false;
	mToughAnnounced = false;
	for (int i = 0; i < NUM_TROPHIES; i++)
		mTrophies[i] = false;
	mClassicRestartPoints.clear();
	mToughRestartPoints.clear();

	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(StrFormat("users\\%d.xml", mId), &aBuffer))
		return;
	std::string aFileText((const char*)aBuffer.GetDataPtr(), (size_t)aBuffer.GetDataLen());
	XmlNode aDocument;
	if (!aDocument.Parse(aFileText))
		return;

	const XmlNode* aUser = aDocument.FirstChildElement("user");
	if (aUser == nullptr)
		return;

	// the original reads these two with no check that they are there
	const std::string* aName = aUser->FindAttribute("name");
	if (aName != nullptr)
		mName = *aName;
	const std::string* aVersion = aUser->FindAttribute("version");
	if (aVersion != nullptr)
		mVersion = *aVersion;

	for (size_t i = 0; i < aUser->mChildren.size(); i++)
	{
		const XmlNode* aChild = aUser->mChildren[i].get();
		if (!aChild->IsElement())
			continue;

		if (aChild->IsElement("iscurrent"))
			mIsCurrent = ReadFlagElement(aChild);
		if (aChild->IsElement("showhelpscreen"))
			mShowHowToPlay = ReadFlagElement(aChild);
		if (aChild->IsElement("hascompletedclassic"))
			mToughUnlocked = ReadFlagElement(aChild);
		if (aChild->IsElement("cookieexplanationshown"))
			mToughAnnounced = ReadFlagElement(aChild);
		if (aChild->IsElement("trophyflags"))
		{
			// a character '1' is a trophy won; a text shorter than the list leaves the rest unwon
			std::string aFlags = aChild->GetText();
			for (int j = 0; j < NUM_TROPHIES; j++)
				mTrophies[j] = j < (int)aFlags.size() && aFlags[j] == '1';
		}
		if (aChild->IsElement("resurrectsnormal"))
			ReadRestartPoints(aChild, mClassicRestartPoints);
		if (aChild->IsElement("resurrectstough"))
			ReadRestartPoints(aChild, mToughRestartPoints);
		if (aChild->IsElement("savegame"))
			ReadSavedGame(aChild);
	}
}

// The "resurrect" elements of a list are read in order; when the hash of their states is not the one in the file the whole list is
// dropped (0x46e500)
void User::ReadRestartPoints(const XmlNode* theElement, std::vector<RestartPoint>& theList)
{
	std::string aHashText;
	for (size_t i = 0; i < theElement->mChildren.size(); i++)
	{
		const XmlNode* aPoint = theElement->mChildren[i].get();
		if (!aPoint->IsElement("resurrect"))
			continue;

		// the original goes on with the last text when there is no "state" (a file that was cut)
		const std::string* aState = aPoint->FindAttribute("state");
		if (aState == nullptr)
			continue;
		aHashText += *aState;
		theList.push_back(RestartPoint(*aState));
	}

	std::string aFileHash;
	const std::string* aHash = theElement->FindAttribute("hash");
	if (aHash != nullptr)
		aFileHash = *aHash;
	if (aFileHash != HashProfileText(aHashText))
		theList.clear();
}

// A <savegame>: its mode says which of the three it fills. The state it ends in is 2 when the hash of what was read is the file's
// and 0 (nothing stored) when it is not (0x46e500).
void User::ReadSavedGame(const XmlNode* theElement)
{
	std::string aHashText;
	int aMode = ReadHashedInt(theElement, "mode", aHashText);
	if (aMode < 0 || aMode >= NUM_SAVED_GAMES)
		return;		// the original goes on with the slot of the savegame before, or with none at all
	SavedGame& aSaved = mSavedGames[aMode];

	aSaved.mStats.clear();
	aSaved.mCubbies.clear();
	aSaved.mKindList.clear();
	aSaved.mState = 0;

	aSaved.mVersion = ReadHashedInt(theElement, "version", aHashText);
	aSaved.mState = ReadHashedInt(theElement, "state", aHashText);
	aSaved.mLevel = ReadHashedInt(theElement, "levelnum", aHashText);
	aSaved.mTime = ReadHashedDouble(theElement, "time", aHashText);
	aSaved.mBonusRounds = ReadHashedInt(theElement, "challengenum", aHashText);
	for (int i = 0; i < GameScreen::NUM_SHUFFLED_BOARDS; i++)
		aSaved.mBoardOrder[i] = ReadHashedInt(theElement, StrFormat("challengeorder%d", i).c_str(), aHashText);
	aSaved.mCycle = ReadHashedInt(theElement, "gnomebagfills", aHashText);
	aSaved.mTimerTotal = ReadHashedDouble(theElement, "timerstart", aHashText);
	aSaved.mTimerLeft = ReadHashedDouble(theElement, "timer", aHashText);
	aSaved.mTimerRate = ReadHashedDouble(theElement, "timerdecrease", aHashText);
	aSaved.mTimerPhase = ReadHashedInt(theElement, "waveinterludestate", aHashText);
	aSaved.mInLevelIntro = ReadHashedInt(theElement, "inlevelintro", aHashText) != 0;
	aSaved.mInShop = ReadHashedInt(theElement, "inshop", aHashText) != 0;
	aSaved.mInChallenge = ReadHashedInt(theElement, "inchallenge", aHashText) != 0;
	aSaved.mInChallengeExplain = ReadHashedInt(theElement, "inchallengeexplain", aHashText) != 0;
	ReadHashedString(theElement, "challengestate", aSaved.mChallengeState, aHashText);
	aSaved.mGameplayPhase = ReadHashedInt(theElement, "gameplay_state", aHashText);
	aSaved.mPhaseTimer = ReadHashedDouble(theElement, "gameplay_statedelay", aHashText);
	aSaved.mNextPopTimer = ReadHashedDouble(theElement, "gameplay_countdownforcreate", aHashText);
	aSaved.mCloverTimer = ReadHashedDouble(theElement, "gameplay_luckyclovertimer", aHashText);
	aSaved.mStrawberryTimer = ReadHashedDouble(theElement, "gameplay_strawberrytimer", aHashText);
	aSaved.mDoubleHitTimer = ReadHashedDouble(theElement, "gameplay_poweruptimer", aHashText);
	aSaved.mFiveTimesTimer = ReadHashedDouble(theElement, "gameplay_multipliertimer", aHashText);
	aSaved.mPopNow = ReadHashedInt(theElement, "gameplay_instantcreate", aHashText) != 0;
	aSaved.mFirstCubbyStarted = ReadHashedInt(theElement, "gameplay_firstgnomecreated", aHashText) != 0;
	aSaved.mSaveItemHasBeenCreated = ReadHashedInt(theElement, "gameplay_saveitemhasbeencreated", aHashText) != 0;
	aSaved.mMustCreateSaveItem = ReadHashedInt(theElement, "gameplay_mustcreatesaveitem", aHashText) != 0;
	aSaved.mFlagCC = ReadHashedInt(theElement, "gameplay_pickcoinsandhearts", aHashText) != 0;
	aSaved.mClicksOnLevel = ReadHashedInt(theElement, "avatar_clicksonlevel", aHashText);

	for (size_t i = 0; i < theElement->mChildren.size(); i++)
	{
		const XmlNode* aList = theElement->mChildren[i].get();
		if (!aList->IsElement())
			continue;

		if (aList->IsElement("counters"))
		{
			aSaved.mStats.clear();
			for (size_t j = 0; j < aList->mChildren.size(); j++)
			{
				const XmlNode* aCounter = aList->mChildren[j].get();
				if (aCounter->IsElement("counter"))
					aSaved.mStats.push_back(ReadHashedDouble(aCounter, "value", aHashText));
			}
		}
		if (aList->IsElement("cubbies"))
		{
			for (size_t j = 0; j < aList->mChildren.size(); j++)
			{
				const XmlNode* aCubby = aList->mChildren[j].get();
				if (aCubby->IsElement("cubby"))
					aSaved.mCubbies.push_back(ReadCubby(aCubby, aHashText));
			}
		}
		if (aList->IsElement("bouncinggnomes"))
		{
			for (size_t j = 0; j < aList->mChildren.size(); j++)
			{
				const XmlNode* aGnome = aList->mChildren[j].get();
				if (aGnome->IsElement("bouncinggnome"))
					aSaved.mBouncing.push_back(ReadBouncingGnome(aGnome, aHashText));
			}
		}
		if (aList->IsElement("flyinggnomes"))
		{
			for (size_t j = 0; j < aList->mChildren.size(); j++)
			{
				const XmlNode* aGnome = aList->mChildren[j].get();
				if (aGnome->IsElement("flyinggnome"))
					aSaved.mFlying.push_back(ReadFlyingGnome(aGnome, aHashText));
			}
		}
		if (aList->IsElement("stickers"))
		{
			for (size_t j = 0; j < aList->mChildren.size(); j++)
			{
				const XmlNode* aStickerElement = aList->mChildren[j].get();
				if (aStickerElement->IsElement("sticker"))
				{
					StickerState aSticker;
					ReadHashedString(aStickerElement, "name", aSticker.mName, aHashText);
					aSticker.mHit = ReadHashedInt(aStickerElement, "hit", aHashText) != 0;
					aSaved.mStickers.push_back(aSticker);
				}
			}
		}
		if (aList->IsElement("gnomebag"))
		{
			aSaved.mKindIndex = ReadHashedInt(aList, "currentgnomeindex", aHashText);
			for (size_t j = 0; j < aList->mChildren.size(); j++)
			{
				const XmlNode* anItem = aList->mChildren[j].get();
				if (anItem->IsElement("item"))
					aSaved.mKindList.push_back(ReadHashedInt(anItem, "value", aHashText));
			}
		}
	}

	std::string aFileHash;
	const std::string* aHash = theElement->FindAttribute("hash");
	if (aHash != nullptr)
		aFileHash = *aHash;
	aSaved.mState = aFileHash != HashProfileText(aHashText) ? 0 : 2;
}
