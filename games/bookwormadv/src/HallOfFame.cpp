#include "HallOfFame.h"

#include <SDL.h>

#include <algorithm>
#include <iterator>
#include <set>

#include "Common.h"
#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "JeffLib.h"
#include "LuaLinkedClass.h"
#include "PopLua.h"
#include "Profile.h"
#include "Res.h"
#include "ResourceManager.h"
#include "WidgetManager.h"
#include "XLayout.h"
#include "lua.h"

using namespace Sexy;

namespace {

// ---- std::sort as the original's compiler made it (Visual C++ 2005's: a quicksort that leaves to an insertion
// sort what is 32 or fewer, which keeps equal rows in the order they came in), so that equal rows rank as they
// did there ----

template <class It, class Pr>
void SortMed3(It a, It b, It c, Pr thePred)
{
	if (thePred(*b, *a))
		std::iter_swap(b, a);
	if (thePred(*c, *b)) {
		std::iter_swap(c, b);
		if (thePred(*b, *a))
			std::iter_swap(b, a);
	}
}

template <class It, class Pr>
void SortMedian(It theFirst, It theMid, It theLast, Pr thePred)
{
	if (40 < theLast - theFirst) {
		int aStep = (int)((theLast - theFirst + 1) / 8);
		SortMed3(theFirst, theFirst + aStep, theFirst + 2 * aStep, thePred);
		SortMed3(theMid - aStep, theMid, theMid + aStep, thePred);
		SortMed3(theLast - 2 * aStep, theLast - aStep, theLast, thePred);
		SortMed3(theFirst + aStep, theMid, theLast - aStep, thePred);
	} else {
		SortMed3(theFirst, theMid, theLast, thePred);
	}
}

template <class It, class Pr>
std::pair<It, It> SortPartition(It theFirst, It theLast, Pr thePred)
{
	It aMid = theFirst + (theLast - theFirst) / 2;
	SortMedian(theFirst, aMid, theLast - 1, thePred);
	It aPfirst = aMid;
	It aPlast = aPfirst + 1;
	while (theFirst < aPfirst && !thePred(*(aPfirst - 1), *aPfirst) && !thePred(*aPfirst, *(aPfirst - 1)))
		--aPfirst;
	while (aPlast < theLast && !thePred(*aPlast, *aPfirst) && !thePred(*aPfirst, *aPlast))
		++aPlast;
	It aGfirst = aPlast;
	It aGlast = aPfirst;
	for (;;) {
		for (; aGfirst < theLast; ++aGfirst) {
			if (thePred(*aPfirst, *aGfirst))
				;
			else if (thePred(*aGfirst, *aPfirst))
				break;
			else
				std::iter_swap(aPlast++, aGfirst);
		}
		for (; theFirst < aGlast; --aGlast) {
			if (thePred(*(aGlast - 1), *aPfirst))
				;
			else if (thePred(*aPfirst, *(aGlast - 1)))
				break;
			else
				std::iter_swap(--aPfirst, aGlast - 1);
		}
		if (aGlast == theFirst && aGfirst == theLast)
			return std::pair<It, It>(aPfirst, aPlast);
		if (aGlast == theFirst) {
			if (aPlast != aGfirst)
				std::iter_swap(aPfirst, aPlast);
			++aPlast;
			std::iter_swap(aPfirst++, aGfirst++);
		} else if (aGfirst == theLast) {
			if (--aGlast != --aPfirst)
				std::iter_swap(aGlast, aPfirst);
			std::iter_swap(aPfirst, --aPlast);
		} else {
			std::iter_swap(aGfirst++, --aGlast);
		}
	}
}

template <class It, class Pr>
void SortInsertion(It theFirst, It theLast, Pr thePred)
{
	if (theFirst == theLast)
		return;
	for (It aNext = theFirst; ++aNext != theLast;) {
		typename std::iterator_traits<It>::value_type aVal = *aNext;
		if (thePred(aVal, *theFirst)) {
			std::copy_backward(theFirst, aNext, aNext + 1);
			*theFirst = aVal;
		} else {
			It aHole = aNext;
			It aPrev = aHole;
			while (thePred(aVal, *--aPrev)) {
				*aHole = *aPrev;
				aHole = aPrev;
			}
			*aHole = aVal;
		}
	}
}

template <class It, class Pr>
void SortRange(It theFirst, It theLast, long theIdeal, Pr thePred)
{
	long aCount;
	for (; 32 < (aCount = (long)(theLast - theFirst)) && 0 < theIdeal;) {
		std::pair<It, It> aMid = SortPartition(theFirst, theLast, thePred);
		theIdeal /= 2;
		theIdeal += theIdeal / 2;
		if (aMid.first - theFirst < theLast - aMid.second) {
			SortRange(theFirst, aMid.first, theIdeal, thePred);
			theFirst = aMid.second;
		} else {
			SortRange(aMid.second, theLast, theIdeal, thePred);
			theLast = aMid.first;
		}
	}
	if (32 < aCount) {
		std::make_heap(theFirst, theLast, thePred);
		std::sort_heap(theFirst, theLast, thePred);
	} else if (1 < aCount) {
		SortInsertion(theFirst, theLast, thePred);
	}
}

template <class T, class Pr>
void SortRows(std::vector<T>& theRows, Pr thePred)
{
	SortRange(theRows.begin(), theRows.end(), (long)theRows.size(), thePred);
}

// ---- the lists' entries (one per ranked thing, ahead of the rows made from them) ----

// 0x40 bytes (constructor 0x45e5e0)
struct WordEntry {
	std::string mName;   // +0x00
	std::string mWord;   // +0x1c
	int mValue;          // +0x38
	bool mNew;           // +0x3c
};
// 0x24 bytes (0x45e690)
struct PowerEntry {
	std::string mName;   // +0x00
	int mPower;          // +0x1c
	bool mChanged;       // +0x20
};
// 0x28 bytes (0x45e6f0): a timed run, a number of bonuses or a mini-game's score
struct RunEntry {
	std::string mName;   // +0x00
	int mFirst;          // +0x1c: the time, the experience or the words
	int mSecond;         // +0x20: the stage, the number of bonuses or the score
	bool mNew;           // +0x24
};

// 0x45e670
bool WordBefore(const WordEntry& a, const WordEntry& b) { return a.mValue > b.mValue; }
// 0x45e6d0
bool PowerBefore(const PowerEntry& a, const PowerEntry& b) { return a.mPower > b.mPower; }
// 0x45e7a0: the higher stage; for the same, the name that sorts first
bool TimeBefore(const RunEntry& a, const RunEntry& b)
{
	if (b.mSecond < a.mSecond)
		return true;
	if (b.mSecond > a.mSecond)
		return false;
	return JeffLib::StringLessNatural(a.mName, b.mName);
}
// 0x45e730: the experience; for the same, the bonuses
bool BonusBefore(const RunEntry& a, const RunEntry& b)
{
	if (a.mFirst != b.mFirst)
		return a.mFirst > b.mFirst;
	return a.mSecond > b.mSecond;
}
// 0x45e810: the score; for the same, the words
bool ScoreBefore(const RunEntry& a, const RunEntry& b)
{
	if (a.mSecond != b.mSecond)
		return a.mSecond > b.mSecond;
	return a.mFirst > b.mFirst;
}

// 0x45e130: a name that fits theWidth in the bold medium font, else cut short with "..." after
std::string FitText(const std::string& theText, int theWidth)
{
	Font* aFont = FONT_MEDIUM_BOLD;
	if (aFont->StringWidth(theText) <= theWidth)
		return theText;
	std::string anEllipsis = "...";
	int aWidth = aFont->StringWidth(anEllipsis);
	std::string aResult;
	for (size_t i = 0; i < theText.size(); i++) {
		std::string aChar = theText.substr(i, 1);
		aWidth += aFont->StringWidth(aChar);
		if (aWidth >= theWidth)
			break;
		aResult += aChar;
	}
	return aResult + anEllipsis;
}

// 0x45e350: a time counted in hundredths of a second, as hours:minutes:seconds
std::string FormatTime(int theTime)
{
	int aSeconds = (theTime / 100) % 60;
	int aMinutes = (theTime / 6000) % 60;
	int anHours = (theTime / 6000) / 60;
	std::string aResult = StrFormat("%d", anHours);
	aResult += StrFormat(aMinutes < 10 ? ":0%d" : ":%d", aMinutes);
	aResult += StrFormat(aSeconds < 10 ? ":0%d" : ":%d", aSeconds);
	return aResult;
}

// ---- reading a profile's tables (a list of entries, each a table of named values) ----

const Profile::Value* GetTable(const Profile* theProfile, const std::string& theKey)
{
	const Profile::Value* aValue = theProfile->GetValue(theKey);
	return aValue != nullptr && aValue->mType == Profile::TYPE_TABLE ? aValue : nullptr;
}

int ValueInt(const Profile::Value& theValue)
{
	switch (theValue.mType) {
	case Profile::TYPE_INT: return theValue.mInt;
	case Profile::TYPE_FLOAT: return (int)theValue.mFloat;
	case Profile::TYPE_BOOL: return theValue.mBool ? 1 : 0;
	default: return 0;
	}
}

// A profile is the player's when it has the name of the player of the app
bool IsCurrentProfile(GameApp* theApp, const Profile* theProfile)
{
	return theApp->mProfile != nullptr && theApp->mProfile->mName == theProfile->mName;
}

// The first ten of a sorted list, the way the original read them (it never checked: its lists begin with ten
// entries of their own)
template <class T>
size_t TopTen(const std::vector<T>& theList)
{
	return theList.size() < 10 ? theList.size() : 10;
}

// The entries the authors began each list with (the text read from the player's exe)
struct DefaultWord {
	unsigned int mName, mWord;
	int mValue;
};
const DefaultWord kDefaultWords[10] = {
	{GTA(0x7ae1f4), GTA(0x7ae1ec), 6}, {GTA(0x7ae208), GTA(0x7ae200), 5}, {GTA(0x7ae218), GTA(0x7ae210), 5}, {GTA(0x7ae228), GTA(0x7ae220), 5},
	{GTA(0x7ae238), GTA(0x7ae230), 5}, {GTA(0x7ae248), GTA(0x7ae240), 5}, {GTA(0x7ae258), GTA(0x7ae250), 5}, {GTA(0x7ae268), GTA(0x7ae260), 4},
	{GTA(0x7ae274), GTA(0x7ae270), 3}, {GTA(0x7ae280), GTA(0x7ae27c), 3},
};
struct DefaultPower {
	unsigned int mName;
	int mPower;
};
const DefaultPower kDefaultPowers[10] = {
	{GTA(0x7ae2d0), 600}, {GTA(0x7ae2d8), 550}, {GTA(0x7ae2e0), 500}, {GTA(0x7ae2e8), 450}, {GTA(0x7ae2ec), 400},
	{GTA(0x7ae2f4), 350}, {GTA(0x7ae2fc), 300}, {GTA(0x7ae304), 200}, {GTA(0x7ae30c), 100}, {GTA(0x7ae314), 0},
};
struct DefaultRun {
	unsigned int mName;
	int mFirst, mSecond;
};
const DefaultRun kDefaultTimes[10] = {  // the time, the stage
	{GTA(0x7ae364), 120000, 11}, {GTA(0x7ae30c), 102000, 10}, {GTA(0x7ae36c), 89000, 9}, {GTA(0x7ae374), 72000, 8},
	{GTA(0x7ae37c), 60000, 7},   {GTA(0x7ae384), 39000, 5},   {GTA(0x7ae388), 30000, 4}, {GTA(0x7ae390), 21000, 3},
	{GTA(0x7ae398), 12000, 2},   {GTA(0x7ae3a0), 6000, 1},
};
const DefaultRun kDefaultBonuses[10] = {  // the experience, the bonuses
	{GTA(0x7ae458), 100, 5}, {GTA(0x7ae464), 90, 2}, {GTA(0x7ae470), 80, 0}, {GTA(0x7ae478), 70, 1}, {GTA(0x7ae480), 60, 6},
	{GTA(0x7ae488), 50, 2},  {GTA(0x7ae490), 40, 1}, {GTA(0x7ae498), 30, 0}, {GTA(0x7ae4a0), 20, 4}, {GTA(0x7ae4a8), 10, 3},
};

// The cycling colour of the player's own new rows, and the other colours of the lists
const Color kNameColor(220, 140, 0);
const Color kTextColor(160, 100, 0);
const Color kPowerNameColor(50, 130, 180);
const Color kLabelColor(0, 160, 20);

}  // namespace

// 0x45e840: the layout is in data/hof_*.xml, by the tab (XLayout, 0x4c8970 with the app's resource lookups), the
// resources of the group "HallOfFame" are loaded, and the lists made from every profile
HallOfFame::HallOfFame(GameApp* theApp, int theMode, bool theGameOver, bool theTimed,
                       const std::string& theMiniGameName, bool theEndOfGame)
	: mApp(theApp), mMainMenuButton(nullptr), mPlayAgainButton(nullptr), mPrevButton(nullptr), mNextButton(nullptr),
	  mMode(theMode), mNumMiniGames(0), mCounter(0), mGameOver(theGameOver), mTimed(theTimed),
	  mEndOfGame(theEndOfGame), mFromArena(false)
{
	mTabs[0] = mTabs[1] = mTabs[2] = nullptr;
	mLayout = new XLayout();
	if (!mApp->mResourceManager->LoadResources("HallOfFame"))
		mApp->ShowResourceError(false);
	if (!ExtractHallOfFameResources(mApp->mResourceManager))
		mApp->ShowResourceError(false);
	mMiniGameName = theMiniGameName;
	mNumMiniGames = LuaLib::CallLua("main.GetNumMiniGames", 0).Int();
	LoadLists();
	LayOut();
	mPriority = 3;
	SetTab(theMode);
}

// 0x45eba0: the buttons go with it, and the resources
HallOfFame::~HallOfFame()
{
	RemoveAllWidgets(true, true);
	mApp->mResourceManager->DeleteResources("HallOfFame");
	delete mLayout;
}

// 0x460510: the layout's animations, and the colour of the player's own rows one step on
void HallOfFame::Update()
{
	mLayout->Update();
	MarkDirty();
	mCounter = (mCounter + 5) % 0xff;
}

// 0x466370
void HallOfFame::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_CLICK2);
}

// 0x460950
void HallOfFame::SetTab(int theMode)
{
	mMode = theMode;
	// the tabs: the cels of the strip are the selected one (0), normal, lit and pressed; the chosen tab shows its
	// selected cel and cannot be pressed
	for (int i = 0; i < 3; i++) {
		FadeButton* aTab = mTabs[i];
		aTab->mNormalRect = aTab->mButtonImage->GetCelRect(1);
		aTab->mOverRect = aTab->mButtonImage->GetCelRect(2);
		aTab->mDownRect = aTab->mButtonImage->GetCelRect(3);
		aTab->mDisabled = false;
	}
	FadeButton* aChosen = mTabs[theMode];
	Rect aSelected = aChosen->mButtonImage->GetCelRect(0);
	aChosen->mDisabledRect = aSelected;
	aChosen->mOverRect = aSelected;
	aChosen->mDownRect = aSelected;
	aChosen->mNormalRect = aSelected;
	aChosen->mDisabled = true;
	if (mMode == MODE_MINI_GAMES && mPrevButton != nullptr)
		mPrevButton->SetVisible(false);
}

// 0x45edc0: the layout of the tab, then the buttons put where it says, and for the mini-games the page's
// banners given to the layout's places
void HallOfFame::LayOut()
{
	mLayout->Clear();
	if (mMode == MODE_ADVENTURE)
		mLayout->Load("data/hof_adventure.xml");
	else if (mMode == MODE_ARENA)
		mLayout->Load("data/hof_arena.xml");
	else
		mLayout->Load("data/hof_mg.xml");

	auto aMove = [this](Widget* theWidget, const char* theNode) {
		XLayout* aNode = mLayout->FindNode(theNode);
		if (aNode == nullptr) {
			SDL_Log("HallOfFame: the layout has no \"%s\"", theNode);
			return;
		}
		theWidget->Move(aNode->GetX(), aNode->GetY());
	};

	if (mMainMenuButton == nullptr) {
		mMainMenuButton = new FadeButton(BUTTON_MAIN_MENU, static_cast<ButtonListener*>(this));
		mMainMenuButton->SetupImage(mEndOfGame ? IMAGE_UI_BOOK_TS_CONTINUE_BUTTON : IMAGE_HOF_BTN_MAINMENU);
		AddWidget(mMainMenuButton);
	}
	if (mPlayAgainButton == nullptr && mGameOver) {
		mPlayAgainButton = new FadeButton(BUTTON_PLAY_AGAIN, static_cast<ButtonListener*>(this));
		mPlayAgainButton->SetupImage(IMAGE_HOF_BTN_PLAYAGAIN);
		AddWidget(mPlayAgainButton);
	}
	if (mGameOver) {
		aMove(mMainMenuButton, "MultiMMBtn");
		aMove(mPlayAgainButton, "MultiPlayBtn");
	} else {
		aMove(mMainMenuButton, "SingleMMBtn");
	}

	static const int kTabIds[3] = {BUTTON_TAB_ADVENTURE, BUTTON_TAB_ARENA, BUTTON_TAB_MINI_GAMES};
	static Image* const* const kTabImages[3] = {&IMAGE_HOF_TAB_ADVENTURE, &IMAGE_HOF_TAB_ARENA,
	                                             &IMAGE_HOF_TAB_MINIGAMES};
	static const char* const kTabNodes[3] = {"AdvTab", "ArenaTab", "MiniGamesTab"};
	for (int i = 0; i < 3; i++) {
		if (mTabs[i] == nullptr) {
			mTabs[i] = new FadeButton(kTabIds[i], static_cast<ButtonListener*>(this));
			mTabs[i]->SetupImage(*kTabImages[i]);
			AddWidget(mTabs[i]);
		}
		aMove(mTabs[i], kTabNodes[i]);
	}
	if (mPrevButton == nullptr && mNumMiniGames > 3) {
		mPrevButton = new FadeButton(BUTTON_PREV, static_cast<ButtonListener*>(this));
		mPrevButton->SetupImage(IMAGE_HOF_BTN_MG_PREV);
		AddWidget(mPrevButton);
	}
	if (mNextButton == nullptr && mNumMiniGames > 3) {
		mNextButton = new FadeButton(BUTTON_NEXT, static_cast<ButtonListener*>(this));
		mNextButton->SetupImage(IMAGE_HOF_BTN_MG_NEXT);
		AddWidget(mNextButton);
	}

	if (mMode < MODE_MINI_GAMES) {
		if (mNextButton != nullptr)
			mNextButton->SetVisible(false);
	} else {
		if (mPrevButton != nullptr)
			aMove(mPrevButton, "PrevBtn");
		if (mNextButton != nullptr)
			aMove(mNextButton, "NextBtn");
		// the three places of the page: emptied, then each given the banner of the game that is in it
		for (const char* aSlot : {"Slot1", "Slot2", "Slot3"}) {
			XLayout* aNode = mLayout->FindNode(aSlot);
			if (aNode != nullptr) {
				aNode->mContent = XLayout::CONTENT_NONE;
				aNode->mImage = nullptr;
			}
		}
		size_t aGame = (size_t)(mMode - 2) * 3;
		for (int i = 0; i < 3 && aGame < mMiniGameRows.size() && aGame < mMiniGames.size(); i++, aGame++) {
			XLayout* aNode = mLayout->FindNode(mMiniGames[aGame].mSlot);
			if (aNode != nullptr)
				aNode->SetImage(mMiniGames[aGame].mBanner, 0, Rect(0, 0, 0, 0));
		}
		if (mNextButton != nullptr)
			mNextButton->SetVisible(true);
	}
	if (mPrevButton != nullptr)
		mPrevButton->SetVisible(false);
}

// 0x460b10: every profile in users/*.bwa that loads, then the lists made from them
void HallOfFame::LoadLists()
{
	std::vector<Profile*> aProfiles;
	std::vector<std::string> aFiles = JeffLib::GetFilesInDirectory("users/*.bwa");
	for (size_t i = 0; i < aFiles.size(); i++) {
		size_t aPos = aFiles[i].find(".bwa");
		std::string aName = aPos == std::string::npos ? aFiles[i] : aFiles[i].substr(0, aPos);
		Profile* aProfile = new Profile(aName, false);
		if (aProfile->Load(true))
			aProfiles.push_back(aProfile);
		else
			delete aProfile;
	}
	MakeWordList(aProfiles);
	MakePowerList(aProfiles);
	MakeTimeList(aProfiles);
	MakeBonusList(aProfiles);
	MakeMiniGameLists(aProfiles);
	for (size_t i = 0; i < aProfiles.size(); i++)
		delete aProfiles[i];
}

// 0x460ea0: the top words. Each profile's TopWordsEverSpelled (a list of word and value), each word once a profile,
// ranked by value after the authors' ten. The player's own words that are new since the screen was last seen (the
// profile's HOFNewTopWord1 to HOFNumNewTopWords, which this clears) are marked.
void HallOfFame::MakeWordList(const std::vector<Profile*>& theProfiles)
{
	std::vector<WordEntry> aList;
	for (int i = 0; i < 10; i++) {
		WordEntry anEntry = {GT(kDefaultWords[i].mName), GT(kDefaultWords[i].mWord), kDefaultWords[i].mValue, false};
		aList.push_back(anEntry);
	}
	for (size_t aProfileNum = 0; aProfileNum < theProfiles.size(); aProfileNum++) {
		Profile* aProfile = theProfiles[aProfileNum];
		if (!aProfile->KeyExists("TopWordsEverSpelled"))
			continue;
		std::vector<std::string> aNewWords;
		bool isCurrent = IsCurrentProfile(mApp, aProfile);
		if (isCurrent) {
			Profile* aPlayer = mApp->mProfile;
			int aNumNew = aPlayer->KeyExists("HOFNumNewTopWords") ? aPlayer->GetInt("HOFNumNewTopWords") : 0;
			aPlayer->SetInt("HOFNumNewTopWords", 0);
			for (int i = 1; i <= aNumNew; i++) {
				std::string aKey = StrFormat("HOFNewTopWord%d", i);
				if (aPlayer->KeyExists(aKey)) {
					aNewWords.push_back(aPlayer->GetString(aKey));
					aPlayer->Delete(aKey);
				}
			}
			aPlayer->Save();
		}
		std::set<std::string> aSeen;
		int aValue = 0;
		const Profile::Value* aTable = GetTable(aProfile, "TopWordsEverSpelled");
		if (aTable == nullptr)
			continue;
		for (size_t i = 0; i < aTable->mTable.size(); i++) {
			const Profile::Value& anItem = aTable->mTable[i].mValue;
			if (anItem.mType != Profile::TYPE_TABLE)
				continue;
			std::string aWord;
			bool isNew = false;
			for (size_t j = 0; j < anItem.mTable.size(); j++) {
				const Profile::Entry& aField = anItem.mTable[j];
				if (!aField.mStringKey)
					continue;
				if (aField.mKey == "mValue")
					aValue = ValueInt(aField.mValue);
				else if (aField.mKey == "mWord")
					aWord = aField.mValue.mString;
			}
			if (isCurrent) {
				for (size_t j = 0; j < aNewWords.size(); j++) {
					if (aNewWords[j] == aWord) {
						isNew = true;
						aNewWords.erase(aNewWords.begin() + j);
						break;
					}
				}
			}
			if (aSeen.insert(aWord).second) {
				WordEntry anEntry = {aProfile->mName, aWord, aValue, isNew};
				aList.push_back(anEntry);
			}
		}
	}
	SortRows(aList, WordBefore);
	mWordRows.clear();
	for (size_t i = 0; i < TopTen(aList); i++) {
		WordRow aRow;
		aRow.mName = aList[i].mName;
		aRow.mWord = aList[i].mWord;
		aRow.mNew = aList[i].mNew;
		mWordRows.push_back(aRow);
	}
}

// 0x4623f0: the Lex power ratings: 100 times the average length of the words a profile spelled (its
// WordLengthsEverSpelled: a count for each length), and its experience level, less ten for each death. The
// player's own is saved (HOFPower) and marked when it differs from the one the screen last showed.
void HallOfFame::MakePowerList(const std::vector<Profile*>& theProfiles)
{
	std::vector<PowerEntry> aList;
	for (int i = 0; i < 10; i++) {
		PowerEntry anEntry = {GT(kDefaultPowers[i].mName), kDefaultPowers[i].mPower, false};
		aList.push_back(anEntry);
	}
	for (size_t aProfileNum = 0; aProfileNum < theProfiles.size(); aProfileNum++) {
		Profile* aProfile = theProfiles[aProfileNum];
		int aCounts[17] = {0};
		int aTotal = 0;
		int aFrequency = 0;
		int aLength = 0;
		const Profile::Value* aTable = GetTable(aProfile, "WordLengthsEverSpelled");
		if (aTable != nullptr) {
			for (size_t i = 0; i < aTable->mTable.size(); i++) {
				const Profile::Value& anItem = aTable->mTable[i].mValue;
				if (anItem.mType != Profile::TYPE_TABLE)
					continue;
				for (size_t j = 0; j < anItem.mTable.size(); j++) {
					const Profile::Entry& aField = anItem.mTable[j];
					if (!aField.mStringKey)
						continue;
					if (aField.mKey == "mFreq")
						aFrequency = ValueInt(aField.mValue);
					else if (aField.mKey == "mLength")
						aLength = ValueInt(aField.mValue);
				}
				// port: the original added to a table of 17 with no check of the length
				if (aLength >= 0 && aLength < 17)
					aCounts[aLength] += aFrequency;
				aTotal += aFrequency;
			}
		}
		int aSum = 0;
		for (int i = 0; i < 17; i++)
			aSum += aCounts[i] * i;
		float anAverage = aTotal == 0 ? 0.0f : (float)((double)aSum / (double)aTotal);
		int aLevel = aProfile->KeyExists("XPLevel") ? aProfile->GetInt("XPLevel") : 1;
		int aDeaths = aProfile->KeyExists("TotalDeaths") ? aProfile->GetInt("TotalDeaths") : 0;
		int aPower = (int)((double)anAverage * 100.0 + aLevel - aDeaths * 10);
		bool aChanged = false;
		if (IsCurrentProfile(mApp, aProfile)) {
			Profile* aPlayer = mApp->mProfile;
			int anOld = aPlayer->KeyExists("HOFPower") ? aPlayer->GetInt("HOFPower") : -1;
			if (anOld != aPower)
				aChanged = true;
			aPlayer->SetInt("HOFPower", aPower);
			aPlayer->Save();
		}
		PowerEntry anEntry = {aProfile->mName, aPower, aChanged};
		aList.push_back(anEntry);
	}
	SortRows(aList, PowerBefore);
	mPowerRows.clear();
	for (size_t i = 0; i < TopTen(aList); i++) {
		int aPower = aList[i].mPower;
		// the picture's cel for the rating: a step for each hundred, with halves from 300 to 600
		int aCel;
		if (aPower < 100)
			aCel = 0;
		else if (aPower < 200)
			aCel = 1;
		else if (aPower < 300)
			aCel = 2;
		else if (aPower < 350)
			aCel = 3;
		else if (aPower < 400)
			aCel = 4;
		else if (aPower < 450)
			aCel = 5;
		else if (aPower < 500)
			aCel = 6;
		else if (aPower < 550)
			aCel = 7;
		else if (aPower < 600)
			aCel = 8;
		else if (aPower < 650)
			aCel = 9;
		else
			aCel = (699 < aPower) + 10;
		PowerRow aRow;
		aRow.mName = FitText(aList[i].mName, 147);
		aRow.mCel = aCel;
		aRow.mChanged = aList[i].mChanged;
		mPowerRows.push_back(aRow);
	}
}

// 0x4632a0: the best timed runs: each profile's TM_Top10Times (a time and a stage), ranked by the stage reached
// after the authors' ten. The run the player just made (TM_Top10TimesMostRecentIndex, which this clears) is marked.
void HallOfFame::MakeTimeList(const std::vector<Profile*>& theProfiles)
{
	std::vector<RunEntry> aList;
	for (int i = 0; i < 10; i++) {
		RunEntry anEntry = {GT(kDefaultTimes[i].mName), kDefaultTimes[i].mFirst, kDefaultTimes[i].mSecond, false};
		aList.push_back(anEntry);
	}
	for (size_t aProfileNum = 0; aProfileNum < theProfiles.size(); aProfileNum++) {
		Profile* aProfile = theProfiles[aProfileNum];
		if (!aProfile->KeyExists("TM_Top10Times"))
			continue;
		const Profile::Value* aTable = GetTable(aProfile, "TM_Top10Times");
		bool isCurrent = IsCurrentProfile(mApp, aProfile);
		int aMostRecent = -1;
		if (isCurrent) {
			Profile* aPlayer = mApp->mProfile;
			if (aPlayer->KeyExists("TM_Top10TimesMostRecentIndex"))
				aMostRecent = aPlayer->GetInt("TM_Top10TimesMostRecentIndex");
			aPlayer->SetInt("TM_Top10TimesMostRecentIndex", -1);
			aPlayer->Save();
		}
		if (aTable == nullptr)
			continue;
		for (size_t i = 0; i < aTable->mTable.size(); i++) {
			const Profile::Value& anItem = aTable->mTable[i].mValue;
			if (anItem.mType != Profile::TYPE_TABLE)
				continue;
			int aTime = 0, aStage = 0;
			for (size_t j = 0; j < anItem.mTable.size(); j++) {
				const Profile::Entry& aField = anItem.mTable[j];
				if (!aField.mStringKey)
					continue;
				if (aField.mKey == "mTime")
					aTime = ValueInt(aField.mValue);
				else if (aField.mKey == "mStage")
					aStage = ValueInt(aField.mValue);
			}
			RunEntry anEntry = {aProfile->mName, aTime, aStage, isCurrent && aMostRecent == (int)i};
			aList.push_back(anEntry);
		}
	}
	SortRows(aList, TimeBefore);
	mTimeRows.clear();
	lua_State* L = mApp->mLua->L;
	for (size_t i = 0; i < TopTen(aList); i++) {
		int aStage = aList[i].mSecond;
		TimeRow aRow;
		aRow.mName = aList[i].mName;
		aRow.mTime = "(" + FormatTime(aList[i].mFirst) + ")";
		// the stage's arena picture and cel, and its boss, from the scripts
		lua_pushint(L, aStage);
		std::string anImageName = LuaLib::CallLua("main.GetArenaImageStrFromStageNum", 1).String();
		SharedImageRef anImage = mApp->mResourceManager->GetImage(anImageName);
		aRow.mImage = anImage;
		lua_pushint(L, aStage);
		aRow.mCel = LuaLib::CallLua("main.GetArenaCelFromStageNum", 1).Int();
		lua_pushint(L, aStage);
		aRow.mBoss = LuaLib::CallLua("main.GetArenaBossNameFromStageNum", 1).String();
		aRow.mNew = aList[i].mNew;
		mTimeRows.push_back(aRow);
	}
}

// 0x4644d0: the most bonuses: each profile's TM_Top10Bonuses (experience and bonuses), ranked by experience after
// the authors' ten. The one the player just made (TM_Top10BonusesMostRecentIndex, cleared) is marked.
void HallOfFame::MakeBonusList(const std::vector<Profile*>& theProfiles)
{
	std::vector<RunEntry> aList;
	for (int i = 0; i < 10; i++) {
		RunEntry anEntry = {GT(kDefaultBonuses[i].mName), kDefaultBonuses[i].mFirst, kDefaultBonuses[i].mSecond,
		                    false};
		aList.push_back(anEntry);
	}
	for (size_t aProfileNum = 0; aProfileNum < theProfiles.size(); aProfileNum++) {
		Profile* aProfile = theProfiles[aProfileNum];
		bool isCurrent = IsCurrentProfile(mApp, aProfile);
		if (!aProfile->KeyExists("TM_Top10Bonuses"))
			continue;
		const Profile::Value* aTable = GetTable(aProfile, "TM_Top10Bonuses");
		int aMostRecent = -1;
		if (isCurrent) {
			Profile* aPlayer = mApp->mProfile;
			if (aPlayer->KeyExists("TM_Top10BonusesMostRecentIndex"))
				aMostRecent = aPlayer->GetInt("TM_Top10BonusesMostRecentIndex");
			aPlayer->SetInt("TM_Top10BonusesMostRecentIndex", -1);
			aPlayer->Save();
		}
		if (aTable == nullptr)
			continue;
		for (size_t i = 0; i < aTable->mTable.size(); i++) {
			const Profile::Value& anItem = aTable->mTable[i].mValue;
			if (anItem.mType != Profile::TYPE_TABLE)
				continue;
			int anExperience = 0, aNumBonuses = 0;
			for (size_t j = 0; j < anItem.mTable.size(); j++) {
				const Profile::Entry& aField = anItem.mTable[j];
				if (!aField.mStringKey)
					continue;
				if (aField.mKey == "mXP")
					anExperience = ValueInt(aField.mValue);
				else if (aField.mKey == "mNumBonuses")
					aNumBonuses = ValueInt(aField.mValue);
			}
			RunEntry anEntry = {aProfile->mName, anExperience, aNumBonuses, isCurrent && aMostRecent == (int)i};
			aList.push_back(anEntry);
		}
	}
	SortRows(aList, BonusBefore);
	mBonusRows.clear();
	for (size_t i = 0; i < TopTen(aList); i++) {
		ScoreRow aRow;
		aRow.mName = FitText(aList[i].mName, 105);
		aRow.mFirst = StrFormat("%d", aList[i].mFirst);
		aRow.mSecond = StrFormat("%d", aList[i].mSecond);
		aRow.mNew = aList[i].mNew;
		mBonusRows.push_back(aRow);
	}
}

// 0x465290: the mini-games' best scores. For each game of main.GetNumMiniGames: what the scripts say of how its
// page looks (a banner, whether it keeps scores, its class), the authors' ten, and each profile's
// Best<class>Score (a score and the words, by the index of the score); the player's latest
// (MostRecent<class>ScoreIndex, cleared) is marked.
void HallOfFame::MakeMiniGameLists(const std::vector<Profile*>& theProfiles)
{
	mMiniGameRows.clear();
	mMiniGames.clear();
	lua_State* L = mApp->mLua->L;
	for (int aGame = 0; aGame < mNumMiniGames; aGame++) {
		lua_pushint(L, aGame);
		LuaLib::LuaClassRet anInfo = LuaLib::CallLua("main.GetHOFMiniGameFormatInfo", 1);
		MiniGame aMiniGame;
		SharedImageRef aBanner = mApp->mResourceManager->GetImage(anInfo.v[0].str);
		aMiniGame.mBanner = aBanner;
		aMiniGame.mClass = anInfo.v[2].str;
		aMiniGame.mSlot = StrFormat("Slot%d", aGame % 3 + 1);
		aMiniGame.mScoreAndWords = anInfo.v[1].b;
		mMiniGames.push_back(aMiniGame);
		bool aScoreAndWords = aMiniGame.mScoreAndWords;

		std::vector<RunEntry> aList;
		int aScore = 100;
		for (int i = 0; aScore < 1100; i++, aScore += 100) {
			RunEntry anEntry = {GT(0x7ae518), i / 2 + 1, aScoreAndWords ? aScore : 0, false};
			aList.push_back(anEntry);
		}
		for (size_t aProfileNum = 0; aProfileNum < theProfiles.size(); aProfileNum++) {
			Profile* aProfile = theProfiles[aProfileNum];
			bool isCurrent = IsCurrentProfile(mApp, aProfile);
			int aScoreIndex = -1;
			if (isCurrent) {
				Profile* aPlayer = mApp->mProfile;
				std::string aKey = "MostRecent" + aMiniGame.mClass + "ScoreIndex";
				aScoreIndex = aPlayer->KeyExists(aKey) ? aPlayer->GetInt(aKey) : -1;
				aPlayer->Delete(aKey);
				aPlayer->Save();
			}
			const Profile::Value* aTable = GetTable(aProfile, "Best" + aMiniGame.mClass + "Score");
			if (aTable == nullptr)
				continue;
			int aWords = 0, aBest = 0;
			for (size_t i = 0; i < aTable->mTable.size(); i++) {
				const Profile::Entry& anEntry = aTable->mTable[i];
				if (anEntry.mValue.mType == Profile::TYPE_TABLE) {
					for (size_t j = 0; j < anEntry.mValue.mTable.size(); j++) {
						const Profile::Entry& aField = anEntry.mValue.mTable[j];
						if (!aField.mStringKey)
							continue;
						if (aField.mKey == "mScore")
							aBest = ValueInt(aField.mValue);
						else if (aField.mKey == "mWords")
							aWords = ValueInt(aField.mValue);
					}
				}
				bool isNew = !anEntry.mStringKey && anEntry.mIntKey == aScoreIndex;
				RunEntry aRun = {aProfile->mName, aWords, aScoreAndWords ? aBest : 0, isNew};
				aList.push_back(aRun);
			}
		}
		SortRows(aList, ScoreBefore);
		std::vector<ScoreRow> aRows;
		for (size_t i = 0; i < TopTen(aList); i++) {
			ScoreRow aRow;
			aRow.mName = FitText(aList[i].mName, aScoreAndWords ? 0xaf - 0x37 : 0xaf);
			aRow.mFirst = aScoreAndWords ? StrFormat("%d", aList[i].mSecond) : std::string("0");
			aRow.mSecond = StrFormat("%d", aList[i].mFirst);
			aRow.mNew = aList[i].mNew;
			aRows.push_back(aRow);
		}
		mMiniGameRows.push_back(aRows);
	}
}

// 0x45f9a0: the backdrop and the layout's pictures, then the tab's lists, down from the panel's top left
void HallOfFame::Draw(Graphics* g)
{
	Color aPulse((int)(mApp->HSLToRGB(mCounter, 255, 128) | 0xff000000));
	g->DrawImage(IMAGE_HOF_BG, 0, 0);
	mLayout->Draw(g);
	g->SetFont(FONT_MEDIUM_BOLD);
	XLayout* aPanel = mLayout->FindNode("Panel");
	if (aPanel == nullptr)
		return;
	int aLeft = aPanel->GetX();
	int aTop = aPanel->GetY();

	if (mMode == MODE_ADVENTURE) {
		// the top words: the name, and the word at the right
		for (size_t i = 0; i < mWordRows.size(); i++) {
			const WordRow& aRow = mWordRows[i];
			int aRowY = (int)i * 25;
			g->SetColor(aRow.mNew ? aPulse : kNameColor);
			g->DrawString(aRow.mName, aLeft + 30, aTop + 110 + aRowY);
			g->SetColor(kTextColor);
			g->WriteWordWrapped(Rect(aLeft + 140, aTop + 97 + aRowY, 305, 300), aRow.mWord, -1, 1, nullptr, -1);
		}
		// the power ratings: the name, and the rating's picture
		for (size_t i = 0; i < mPowerRows.size(); i++) {
			const PowerRow& aRow = mPowerRows[i];
			int aRowY = (int)i * 25;
			g->SetColor(aRow.mChanged ? aPulse : kPowerNameColor);
			g->DrawString(aRow.mName, aLeft + 480, aTop + 110 + aRowY);
			g->DrawImageCel(IMAGE_HOF_POWER_RATING, aLeft + 635, aTop + 93 + aRowY, aRow.mCel);
		}
	} else if (mMode == MODE_ARENA) {
		// the timed runs: the name, the arena's picture, the time (and in black the boss, further left)
		for (size_t i = 0; i < mTimeRows.size(); i++) {
			const TimeRow& aRow = mTimeRows[i];
			int aRowY = (int)i * 25;
			g->SetColor(aRow.mNew ? aPulse : kNameColor);
			g->DrawString(aRow.mName, aLeft + 30, aTop + 110 + aRowY);
			g->DrawImageCel(aRow.mImage, aLeft + 435, aTop + 97 + aRowY, aRow.mCel);
			g->SetColor(kTextColor);
			g->WriteWordWrapped(Rect(aLeft + 120, aTop + 97 + aRowY, 305, 300), aRow.mTime, -1, 1, nullptr, -1);
			g->SetColor(Color::Black);
			g->WriteWordWrapped(Rect(aLeft + 60, aTop + 97 + aRowY, 305, 300), aRow.mBoss, -1, 1, nullptr, -1);
		}
		// the most bonuses: the name, the experience, the bonus token and the bonuses (the numbers start a font's
		// ascent higher than the names, and are drawn in black)
		int anOffset = -FONT_MEDIUM_BOLD->GetAscent();
		for (size_t i = 0; i < mBonusRows.size(); i++) {
			const ScoreRow& aRow = mBonusRows[i];
			int aRowY = (int)i * 25;
			g->SetColor(aRow.mNew ? aPulse : kPowerNameColor);
			g->DrawString(aRow.mName, aLeft + 485, aTop + 110 + aRowY);
			g->SetColor(Color::Black);
			g->WriteWordWrapped(Rect(aLeft + 580, aTop + 112 + anOffset + aRowY, 50, 100), aRow.mFirst, -1, 1, nullptr, -1);
			g->DrawImage(IMAGE_TM_LEVELUP_TOKEN, aLeft + 640, aTop + 94 + aRowY);
			g->WriteWordWrapped(Rect(aLeft + 650, aTop + 112 + anOffset + aRowY, 50, 100), aRow.mSecond, -1, 0, nullptr, -1);
		}
	} else if (mMode > MODE_ARENA) {
		// three mini-games a page, each in its place of the layout: the labels of its columns, then its rows
		size_t aGame = (size_t)(mMode - 2) * 3;
		for (int aSlot = 0; aSlot < 3 && aGame < mMiniGameRows.size() && aGame < mMiniGames.size(); aSlot++, aGame++) {
			const std::vector<ScoreRow>& aRows = mMiniGameRows[aGame];
			const MiniGame& aMiniGame = mMiniGames[aGame];
			XLayout* aNode = mLayout->FindNode(aMiniGame.mSlot);
			if (aNode == nullptr)
				continue;
			int aNodeX = aNode->GetX();
			int aNodeY = aNode->GetY();
			g->SetColor(kLabelColor);
			g->DrawString(GT(0x7ae1dc), aNodeX + 0xb9, aNodeY + 0x41);
			if (aMiniGame.mScoreAndWords)
				g->DrawString(GT(0x7ae1e4), aNodeX + 0x87, aNodeY + 0x41);
			for (size_t i = 0; i < aRows.size(); i++) {
				const ScoreRow& aRow = aRows[i];
				int aRowY = (int)i * 25;
				g->SetColor(aRow.mNew ? aPulse : kNameColor);
				g->DrawString(aRow.mName, aNodeX + 10, aNodeY + 0x57 + aRowY);
				g->SetColor(kTextColor);
				g->WriteWordWrapped(Rect(aNodeX + 0xa8, aNodeY + 0x49 + aRowY, 75, 100), aRow.mSecond, -1, 0, nullptr, -1);
				if (aMiniGame.mScoreAndWords)
					g->WriteWordWrapped(Rect(aNodeX + 0x72, aNodeY + 0x49 + aRowY, 75, 100), aRow.mFirst, -1, 0, nullptr, -1);
			}
		}
	}
}

namespace {

// 0x477cd0: the curtain's callback that lets an object (the hall of fame, here) act once the curtain has closed
void ShowMiniGamesMenuCallback(void* theObject)
{
	static_cast<HallOfFame*>(theObject)->ShowMiniGamesMenu();
}

// 0x460550's, as the curtain's callback
void CurtainClosedCallback(void* theObject)
{
	static_cast<HallOfFame*>(theObject)->CurtainClosed();
}

}  // namespace

// 0x460550: the curtain over the screen is closed (after a timed game the main menu then comes); the screen goes
void HallOfFame::CurtainClosed()
{
	if (mFromArena)
		mApp->ShowMainMenu(false, false);
	mApp->CloseHallOfFame();
}

// 0x466340: the mini-games' menu for the game just played, and this screen goes
void HallOfFame::ShowMiniGamesMenu()
{
	if (!mTimed && !mMiniGameName.empty()) {
		mApp->ShowMiniGameMenu();
		mApp->CloseHallOfFame();
	}
}

// 0x460580: the buttons. The main menu's closes the curtain, and the main menu comes under the screen (or after
// the end of the adventure, the credits); "Play Again" starts again what was played; the tabs and the arrows
// change the lists.
void HallOfFame::ButtonDepress(int theId)
{
	if (theId == mMainMenuButton->mId) {
		mMainMenuButton->SetDisabled(true);
		if (mPlayAgainButton != nullptr)
			mPlayAgainButton->SetDisabled(true);
		mApp->mMiniGameFromMenu = false;
		if (!mEndOfGame) {
			mApp->ShowMainMenu(false, true);
			mApp->DoFullCurtainTransition("");
			mApp->mCurtainCallbackObject = this;
			mApp->mCurtainCallback = CurtainClosedCallback;
			return;
		}
		mApp->StartEndOfGameCredits();
		return;
	}
	if (mPlayAgainButton != nullptr && theId == mPlayAgainButton->mId) {
		mPlayAgainButton->SetDisabled(true);
		if (mMainMenuButton != nullptr)
			mMainMenuButton->SetDisabled(true);
		if (!mTimed) {
			if (!mMiniGameName.empty()) {
				// a mini-game: the curtain closes, then its menu comes
				mApp->DoFullCurtainTransition("");
				mApp->mCurtainCallbackObject = this;
				mApp->mCurtainCallback = ShowMiniGamesMenuCallback;
				return;
			}
			mApp->PlayMainMenuCutScene();
			return;
		}
		// the timed mode again
		lua_pushboolean(mApp->mLua->L, 1);
		mApp->mLua->Call("main.SetTimedMode", 1, 0);
		mApp->mTimedMode = true;
		mApp->mProfile->ResetTimedMode();
		mApp->StartNewGameTransition();
		return;
	}
	// a tab, or an arrow
	int aFirstTab = mTabs[0]->mId;
	if (theId == mTabs[0]->mId) {
		if (mMode == 0)
			return;
	} else if (theId == mTabs[1]->mId) {
		if (mMode == 1)
			return;
	} else if (theId == mTabs[2]->mId) {
		if (mMode > 1)
			return;
		SetTab(theId - aFirstTab);
		LayOut();
		return;
	} else if (mPrevButton != nullptr && theId == mPrevButton->mId) {
		mMode--;
		LayOut();
		mPrevButton->SetVisible(mMode != 2);
		if (mNextButton != nullptr)
			mNextButton->SetVisible(true);
		return;
	} else if (mNextButton != nullptr && theId == mNextButton->mId) {
		mMode++;
		LayOut();
		if (mPrevButton != nullptr)
			mPrevButton->SetVisible(true);
		if (mMode - 2 <= mNumMiniGames / 3 - 1)
			return;
		if (mNextButton != nullptr)
			mNextButton->SetVisible(false);
		return;
	} else {
		return;
	}
	SetTab(theId - aFirstTab);
	LayOut();
}
