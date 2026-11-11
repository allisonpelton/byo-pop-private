// Board: the stories of the four pilots. BuildStoryText (0x47a940) makes the pages of the stories of the player's pilot when
// the Board is set up, OpenStoryScreen (0x468e70) shows the one that belongs before the level that is coming, with its
// pictures loaded by LoadStoryPages (0x45ef90), and CloseStory (0x469030) takes the screen away and starts the level.

#include "Board.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Image.h"
#include "Res.h"
#include "ResourceManager.h"
#include "StoryScreen.h"
#include "UserProfile.h"
#include "WidgetManager.h"
#include "SoundInstance.h"

using namespace Sexy;

namespace
{

// The texts of the pages: where each line is in the game's image (StoryText.inc: the generator reads the addresses from the
// export of 0x47a940; the texts are read from the player's own exe)
struct StoryRow
{
	int				mPilot;
	int				mStory;
	int				mPage;          // 0: the upper page, 1: the lower page
	unsigned int	mAddress;       // 0 is the empty string
};

const StoryRow kStoryRows[] =
{
#define STORYLINE(pilot, story, page, va) { pilot, story, page, va },
#define STORYTITLE(pilot, number, va)
#define FINALETEXT(scene, text, va)
#include "StoryText.inc"
#undef STORYLINE
#undef STORYTITLE
#undef FINALETEXT
	{ -1, 0, 0, 0 }
};

// 0x429340: the number of the picture of a name (RESOURCE_ID_MAX when there is none)
int GetIdByStringId(const std::string& theName)
{
	for (int anId = 0; anId < RESOURCE_ID_MAX; anId++)
		if (theName == GetStringIdById(anId))
			return anId;
	return RESOURCE_ID_MAX;
}

// the value of an attribute of a picture in the resource descriptor ("" when it has none)
std::string GetAttribute(const XMLParamMap& theAttributes, const char* theName)
{
	XMLParamMap::const_iterator anItr = theAttributes.find(theName);
	return (anItr != theAttributes.end()) ? anItr->second : std::string();
}

// the number in an attribute, or theDefault when it has none
int GetNumberAttribute(const XMLParamMap& theAttributes, const char* theName, int theDefault)
{
	std::string aValue = GetAttribute(theAttributes, theName);
	int aNumber = theDefault;
	if (!aValue.empty())
		StringToInt(aValue, &aNumber);
	return aNumber;
}

}

// 0x47a940: the lines of the pages of the stories of this pilot (the first pilot has nine stories, the others eight), in
// the order the original adds them. The pages know the level before which they come.
void Board::BuildStoryText()
{
	int aNumStories = (mMode == 0) ? 9 : 8;
	if (mMode < 0 || mMode > 3)
		aNumStories = 0;

	for (int aStory = 0; aStory < aNumStories; aStory++)
	{
		StoryEntry* anEntry = new StoryEntry;
		anEntry->mPageA = new StoryPage;
		anEntry->mPageB = new StoryPage;
		anEntry->mPageA->mSide = -1;
		anEntry->mPageB->mSide = 1;

		for (const StoryRow* aRow = kStoryRows; aRow->mPilot >= 0; aRow++)
		{
			if (aRow->mPilot != mMode || aRow->mStory != aStory)
				continue;
			StoryLine* aLine = new StoryLine;
			aLine->mText = (aRow->mAddress == 0) ? std::string() : GT(aRow->mAddress);
			(aRow->mPage == 0 ? anEntry->mPageA : anEntry->mPageB)->mLines.push_back(aLine);
		}
		mPilotStories.push_back(anEntry);
	}

	// the level before which each story comes (the first pilot's first story is early: before level 3)
	static const int kFirstPilotLevels[9] = { 3, 5, 9, 0xd, 0x11, 0x15, 0x19, 0x1d, 0x21 };
	static const int kOtherLevels[8] = { 5, 9, 0xd, 0x11, 0x15, 0x19, 0x1d, 0x21 };
	for (size_t i = 0; i < mPilotStories.size(); i++)
	{
		int aLevel = (mMode == 0) ? kFirstPilotLevels[i] : kOtherLevels[i];
		mPilotStories[i]->mPageA->mLevel = aLevel;
		mPilotStories[i]->mPageB->mLevel = aLevel;
	}
}

// 0x468e70: the story that belongs before this level, when it has one. The number of stories that the player has seen is a
// setting of the profile (0x4b + the pilot); a story seen before can be left at once.
bool Board::OpenStoryScreen()
{
	for (size_t i = 0; i < mPilotStories.size(); i++)
	{
		StoryEntry* anEntry = mPilotStories[i];
		if (anEntry->mPageA->mLevel != mLevelNumber)
			continue;

		int aSeen = 0;
		mApp->mUser->GetInt(mMode + 0x4b, &aSeen);
		int aNumber = (int) i + 1;
		if (aSeen < aNumber)
		{
			mApp->mUser->SetInt(mMode + 0x4b, aNumber);
			mApp->mUser->Save();
		}

		mStoryScreen = new StoryScreen(mApp, aNumber, mMode, aNumber <= aSeen, false);
		LoadStoryPages();
		mStoryScreen->SetPageA(anEntry->mPageA);
		mStoryScreen->SetPageB(anEntry->mPageB);
		mStoryScreen->Resize(0, 0, 0x280, 0x1e0);
		Pause(true, false);
		mApp->mWidgetManager->AddWidget(mStoryScreen);
		mApp->SetCursor(0);
		mApp->mWidgetManager->SetFocus(mStoryScreen);
		mApp->PlaySong(5);
		return true;
	}
	return false;
}

// 0x45ef90: the pictures of the story that is coming. Each story has a group of its own in the resources, which the pages
// get their pictures from: the animation of a page is the pictures from its first one (FIRST_TOP_IMAGE or
// FIRST_BOTTOM_IMAGE) to the one with the attribute lastanimframe. The first picture of a page also says how the page
// looks (static, analysis, border, pingpong, cols). The page counter calls of the original (PopCap's ad statistics) are
// left out.
void Board::LoadStoryPages()
{
	int aLevel = mLevelNumber;
	StoryEntry* anEntry = NULL;
	int aStoryNumber = -1;
	for (size_t i = 0; i < mPilotStories.size() && anEntry == NULL; i++)
	{
		if (mPilotStories[i]->mPageA->mLevel == aLevel)
		{
			anEntry = mPilotStories[i];
			aStoryNumber = (int) i;
		}
	}
	// (OpenStoryScreen has found the story first; the original would go on with the number -1 of none)
	if (anEntry == NULL)
		return;

	const char* aGroupFormat;
	const char* aPrefixFormat;
	switch (mMode)
	{
	case 0:
		aGroupFormat = "VectorPage%d";
		aPrefixFormat = "VP%d_";
		break;
	case 1:
		aGroupFormat = "SprocketPage%d";
		aPrefixFormat = "SP%d_";
		break;
	case 3:
		aGroupFormat = "VixxPage%d";
		aPrefixFormat = "VXP%d_";
		break;
	default:
		aGroupFormat = "TurbotPage%d";
		aPrefixFormat = "TP%d_";
		break;
	}
	mStoryResourceGroup = StrFormat(aGroupFormat, aStoryNumber + 1);
	std::string aPrefix = std::string("IMAGE_STORY_") + StrFormat(aPrefixFormat, aStoryNumber + 1);

	mApp->mResourceManager->StartLoadResources(mStoryResourceGroup);
	while (mApp->mResourceManager->LoadNextResource())
	{
	}
	if (mApp->mResourceManager->HadError() || !ExtractResourcesByName(mApp->mResourceManager, mStoryResourceGroup.c_str()))
	{
		mApp->ShowResourceError(false);
		mApp->Shutdown();
		return;
	}

	for (int aPageNumber = 0; aPageNumber < 2; aPageNumber++)
	{
		StoryPage* aPage = (aPageNumber == 0) ? anEntry->mPageA : anEntry->mPageB;
		std::string aFirstName = aPrefix + (aPageNumber == 0 ? "FIRST_TOP_IMAGE" : "FIRST_BOTTOM_IMAGE");
		int aFirstId = GetIdByStringId(aFirstName);

		// the last picture of the animation is the first one from here on that has lastanimframe (the original shows a
		// message when it finds none up to number 3000, and goes on to look)
		int aLastId = -1;
		for (int anId = aFirstId; aLastId < 0 && anId < RESOURCE_ID_MAX; anId++)
		{
			const XMLParamMap& anAttributes = mApp->mResourceManager->GetImageAttributes(GetStringIdById(anId));
			if (!GetAttribute(anAttributes, "lastanimframe").empty())
				aLastId = anId;
		}
		if (aLastId < 0)
			aLastId = aFirstId;

		aPage->mAnim.SetNumFrames(aLastId - aFirstId + 1);
		aPage->mAnim.mHoldLast = false;
		aPage->mAnim.SetStopped(false);

		for (int anId = aFirstId, aFrame = 0; anId <= aLastId; anId++, aFrame++)
		{
			aPage->mImages.push_back(GetImageById(anId));
			const XMLParamMap& anAttributes = mApp->mResourceManager->GetImageAttributes(GetStringIdById(anId));
			aPage->mAnim.SetFrameDelay(GetNumberAttribute(anAttributes, "delay", 0x19), aFrame);

			if (anId != aFirstId)
				continue;
			aPage->mStatic = !GetAttribute(anAttributes, "static").empty();
			aPage->mGauge = !GetAttribute(anAttributes, "analysis").empty();
			aPage->mBorder = GetNumberAttribute(anAttributes, "border", 2);
			// ("radar" is an error of the original, not used by any picture)
			if (!GetAttribute(anAttributes, "pingpong").empty())
				aPage->mAnim.mPingPong = true;
			int aColumns = GetNumberAttribute(anAttributes, "cols", 0);
			if (aColumns > 0)
				aPage->mAnim.SetNumFrames(aColumns);
		}
	}
}

// 0x469030: the story is over and the level starts
void Board::CloseStory()
{
	mApp->mWidgetManager->RemoveWidget(mStoryScreen);
	mApp->SafeDeleteWidget(mStoryScreen);
	bool aSaved = m1c09;
	mStoryScreen = NULL;
	m1c09 = true;
	Pause(false, false);
	mApp->PlaySong(9);
	m1c09 = aSaved;
	mApp->PlaySample(SOUND_GET_READY);
	mApp->mEngineLoop->SetVolume(1.0);
	mApp->mHyperspaceLoop->SetVolume(1.0);
	mApp->mEngineLoop->Play(true, false);
	LoadLevel(true);
	mLevelUpPanelY = -mLevelUpPanelHeight;
}
