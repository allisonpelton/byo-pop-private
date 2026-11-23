// Sexy::QuestMenu: see the header
#include "QuestMenu.h"

#include "Bej3App.h"
#include "Board.h"
#include "Color.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Profile.h"
#include "QuestData.h"
#include "Res.h"
#include "WidgetManager.h"

#include <SDL.h>

#include <cstdio>

namespace Sexy {

namespace {

// The exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure)
{
	return gApp->mArtRes * theMeasure / 0x4b0;
}

// the text with its quotes taken off
std::string Unquoted(const std::string& theText)
{
	if (theText.size() >= 2 && theText.front() == '"' && theText.back() == '"')
		return theText.substr(1, theText.size() - 2);
	return theText;
}

// the setting of a quest, its quotes taken off
std::string QuestSetting(int theQuestIndex, const char* theName)
{
	if (gApp->mQuestData == nullptr || theQuestIndex < 0 || theQuestIndex >= (int)gApp->mQuestData->mQuests.size())
		return std::string();
	const std::map<std::string, std::string>& aSettings = gApp->mQuestData->mQuests[(size_t)theQuestIndex].mSettings;
	auto anIter = aSettings.find(theName);
	return anIter == aSettings.end() ? std::string() : Unquoted(anIter->second);
}

// 0x693760: the map from a name to the pictures of a quest's button and its icon, in the order the constructor makes it, with the ids it puts in the map. "Hidden Objects" is made twice
// (Buried Treasure's pictures, then Sandstorm's, which replace them), so Buried Treasure and Sandstorm both show Sandstorm's: an oddity of the exe
struct NamedLook {
	const char* mName;
	QuestMenu::QuestLook mLook;
};

#define QUEST_LOOK(theName, theButton, theIcon) {theName, {&IMAGE_QUESTMENU_##theButton, &IMAGE_QUESTMENU_##theIcon, IMAGE_QUESTMENU_##theButton##_ID, IMAGE_QUESTMENU_##theIcon##_ID}}

const NamedLook* FindNamedLook(const std::string& theName)
{
	static const NamedLook LOOKS[] = {
		QUEST_LOOK("Gold Lines", ALCHEMY, ICON_ALCHEMY),               // 0x57c, 0x590
		QUEST_LOOK("Gold Lines Flip", ALCHEMY, ICON_ALCHEMY),
		QUEST_LOOK("Filler", AVALANCHE, ICON_AVALANCHE),               // 0x57d, 0x591
		QUEST_LOOK("Balance", BALANCE, ICON_BALANCE),                  // 0x57e, 0x592
		QUEST_LOOK("Hidden Objects", SANDSTORM, ICON_SANDSTORM),       // 0x5ba, 0x598 (the first made, Buried Treasure's 0x58b and 0x593, is replaced)
		QUEST_LOOK("Butterflies", BUTTERFLIES, ICON_BUTTERFLY),        // 0x58c, 0x594
		QUEST_LOOK("Dig", GOLD_RUSH, ICON_GOLD_RUSH),                  // 0x58e, 0x595
		QUEST_LOOK("Inferno", ICE_STORM, ICON_ICE_STORM),              // 0x58f, 0x596
		QUEST_LOOK("Poker", POKER, ICON_POKER_COPY),                   // 0x5a1, 0x597
		QUEST_LOOK("Clear Gems", STRATAMAX, ICON_STRATAMAX),           // 0x5bb, 0x599
		QUEST_LOOK("Clear Bombs", TIME_BOMB, ICON_TIME_BOMB),          // 0x5bc, 0x59a
		QUEST_LOOK("Wall Blast", WALL_BLAST, ICON_WALL_BLAST),         // 0x5bd, 0x59b
	};
	for (const NamedLook& aLook : LOOKS) {
		if (theName == aLook.mName)
			return &aLook;
	}
	return nullptr;
}

#undef QUEST_LOOK

static_assert(IMAGE_QUESTMENU_ALCHEMY_ID == 0x57c && IMAGE_QUESTMENU_STRATAMAX_ID == 0x5bb && IMAGE_QUESTMENU_POKER_ID == 0x5a1, "the ids of 0x693760's map");

}  // namespace

// 0x693760 (bejeweled3): the menu is made at x -160, y 0, 1920 wide and as high as the art (0x773b70 sizes it), clipping off (+0x51). The five sections
// are made from the profile (0x67eb80: each with its quests done), their states are worked out (0x6734d0), the section on show is the profile's +0x184, and the
// buttons are made: the four round the page (PREV and NEXT of IMAGE_QUESTMENU_PREV and _NEXT, BACK of IMAGE_OVERWORLD_BACK_BUTTON, QUIT of
// IMAGE_MAIN_MENU_QUIT_BUTTON) and the quests'. port: the relics, the progress picture, the sparkles and the page's turn are not made (see docs/NOTES.md, "Quest")
QuestMenu::QuestMenu()
{
	mClip = false;   // +0x51
	mSection = gApp->mProfile != nullptr ? gApp->mProfile->mQuestSection : 0;
	if (mSection < 0 || mSection >= SECTION_COUNT)
		mSection = 0;
	UpdateSections();

	// the buttons are placed where the resources put their pictures (x= and y= of the image), a cel of the picture in size; a picture of two columns shows its
	// second column at rest and the first over and pressed
	auto aMakeButton = [&](int theId, Image* theImage, int theImageId, bool theTwoColumns) {
		ButtonWidget* aButton = new ButtonWidget(theId, this);
		aButton->Resize(Scaled(GetImageOffsetX(theImageId)), Scaled(GetImageOffsetY(theImageId)), theImage->GetCelWidth(), theImage->GetCelHeight());
		aButton->mButtonImage = theImage;
		if (theTwoColumns) {
			aButton->mNormalRect = theImage->GetCelRect(1, 0);
			aButton->mOverRect = theImage->GetCelRect(0, 0);
		}
		else {
			// BACK and QUIT are two cels, the second at rest (as in the play menu)
			aButton->mNormalRect = theImage->GetCelRect(1);
			aButton->mOverRect = theImage->GetCelRect(0);
		}
		aButton->mDownRect = aButton->mOverRect;
		aButton->mHasTransparencies = true;
		AddWidget(aButton);
		return aButton;
	};
	mPrevButton = aMakeButton(ID_PREV, IMAGE_QUESTMENU_PREV, IMAGE_QUESTMENU_PREV_ID, true);
	mNextButton = aMakeButton(ID_NEXT, IMAGE_QUESTMENU_NEXT, IMAGE_QUESTMENU_NEXT_ID, true);
	mBackButton = aMakeButton(ID_BACK, IMAGE_OVERWORLD_BACK_BUTTON, IMAGE_OVERWORLD_BACK_BUTTON_ID, false);
	mQuitButton = aMakeButton(ID_QUIT, IMAGE_MAIN_MENU_QUIT_BUTTON, IMAGE_MAIN_MENU_QUIT_BUTTON_ID, false);
	LayoutPage();
}

// the buttons go with the menu
QuestMenu::~QuestMenu()
{
	mQuestButtons.clear();
	RemoveAllWidgets(true, true);
}

// 0x67eb80 and 0x6734d0 (bejeweled3): a section's done count is the flags set among its eight quests; its state: 0 (locked) when the section before it has fewer than 4
// done, else 1 when it has fewer than 4 done itself, 2 with 4 to 7 done and 3 with all 8
void QuestMenu::UpdateSections()
{
	bool aPreviousOpen = true;
	for (int aSection = 0; aSection < SECTION_COUNT; aSection++) {
		int aDone = 0;
		for (int aPlace = 0; aPlace < QUESTS_PER_SECTION; aPlace++) {
			if (gApp->mProfile != nullptr && gApp->mProfile->mQuestsDone[aSection][aPlace] != 0)
				aDone++;
		}
		mSectionDone[aSection] = aDone;
		if (!aPreviousOpen)
			mSectionState[aSection] = SECTION_LOCKED;
		else if (aDone >= QUESTS_PER_SECTION)
			mSectionState[aSection] = SECTION_DONE;
		else if (aDone >= QUESTS_TO_OPEN_NEXT)
			mSectionState[aSection] = SECTION_HALF_DONE;
		else
			mSectionState[aSection] = SECTION_CURRENT;
		aPreviousOpen = aDone >= QUESTS_TO_OPEN_NEXT;
	}
}

bool QuestMenu::IsQuestDone(int theIndex) const
{
	if (gApp->mProfile == nullptr || theIndex < 0 || theIndex >= QUEST_COUNT)
		return false;
	return gApp->mProfile->mQuestsDone[theIndex / QUESTS_PER_SECTION][theIndex % QUESTS_PER_SECTION] != 0;
}

// the map's name for a quest: the Challenge if the map has it ("Filler", "Balance", "Butterflies", "Dig", "Inferno", "Poker"), else the Goal ("Clear Gems", "Gold Lines", ...).
// The map's keys are of both kinds, and a quest such as Avalanche (Filler with Clear Gems) must show its own picture, so the Challenge is tried first. port: the order of
// the lookup in 0x697d40 was not read
const QuestMenu::QuestLook* QuestMenu::LookOf(int theQuestIndex)
{
	const NamedLook* aLook = FindNamedLook(QuestSetting(theQuestIndex, "Challenge"));
	if (aLook == nullptr)
		aLook = FindNamedLook(QuestSetting(theQuestIndex, "Goal"));
	return aLook == nullptr ? nullptr : &aLook->mLook;
}

// 0x697d40 (bejeweled3), the part that puts a quest's button: it is as large as one cel of the picture and sits where the picture's x= and y= of resources.xml put
// it (the exe makes 16 buttons, a pending and a done one for each place, and shows the pending one for a quest not done, the other for a done one). A locked section has
// no buttons. The picture of a quest's button is three rows of the same gem: at rest, glowing and flashing
void QuestMenu::LayoutPage()
{
	for (ButtonWidget* aButton : mQuestButtons) {
		RemoveWidget(aButton);
		delete aButton;
	}
	mQuestButtons.clear();
	if (mSectionState[mSection] == SECTION_LOCKED)
		return;
	for (int aPlace = 0; aPlace < QUESTS_PER_SECTION; aPlace++) {
		const int anIndex = QuestIndex(aPlace);
		if (gApp->mQuestData == nullptr || anIndex >= (int)gApp->mQuestData->mQuests.size())
			break;
		const QuestLook* aLook = LookOf(anIndex);
		const bool isDone = IsQuestDone(anIndex);
		ButtonWidget* aButton = new ButtonWidget(isDone ? ID_QUEST_DONE + aPlace : ID_QUEST_PENDING + aPlace, this);
		Image* anImage = aLook == nullptr ? nullptr : *aLook->mButton;
		// port: the exe puts each quest's gem in a socket of the section's page of relics (0x67cc80 builds the page, a different one for each section); the port puts the
		// eight in two rows of four, their middles 320 (of 1200) apart
		const int aMiddleX = Scaled(960 + (aPlace % 4 - 1) * 320 - 160);
		const int aMiddleY = Scaled(420 + (aPlace / 4) * 320);
		if (anImage != nullptr) {
			aButton->Resize(aMiddleX - anImage->GetCelWidth() / 2, aMiddleY - anImage->GetCelHeight() / 2, anImage->GetCelWidth(), anImage->GetCelHeight());
			aButton->mButtonImage = anImage;
			aButton->mNormalRect = anImage->GetCelRect(0, 0);
			aButton->mOverRect = anImage->GetCelRect(0, 1);
			aButton->mDownRect = anImage->GetCelRect(0, 1);
			aButton->mHasTransparencies = true;
		}
		else
			aButton->Resize(aMiddleX - Scaled(100), aMiddleY - Scaled(100), Scaled(200), Scaled(200));
		AddWidget(aButton);
		mQuestButtons.push_back(aButton);
	}
}

// 0x697640 (bejeweled3), the ids 0 and 1: the page before or after (round the five); the menu's own flag (+0x6c) and the profile's +0x188 are cleared and the
// profile keeps the section (+0x184)
void QuestMenu::TurnTo(int theSection)
{
	if (theSection == mSection)
		return;
	mSection = theSection;
	if (gApp->mProfile != nullptr) {
		gApp->mProfile->mQuestSection = mSection;
		gApp->mProfile->mQuestMenuFlag = false;
	}
	LayoutPage();
}

// 0x697640 (bejeweled3): PREV and NEXT turn the page, BACK takes the map away (the play menu comes back, 0x7716e0, and the board that is under the map goes), QUIT asks
// whether to quit (as in the play menu, only logged here), and a quest's button (1000 plus its place, 2000 for a done one) starts the quest (0x774060 with the place
// plus 8 times the section, the app's quest.cfg and the secret flag 0). port: the click's sound (0x76aaf0) and the exe's wait for the relic animation (0x674060) are left out
void QuestMenu::ButtonDepress(int theId)
{
	if (theId == ID_PREV)
		TurnTo((mSection + SECTION_COUNT - 1) % SECTION_COUNT);
	else if (theId == ID_NEXT)
		TurnTo((mSection + 1) % SECTION_COUNT);
	else if (theId == ID_BACK)
		gApp->LeaveQuestMenu();
	else if (theId == ID_QUIT)
		SDL_Log("QuestMenu: QUIT");
	else if (theId >= ID_QUEST_PENDING) {
		int aPlace = theId - ID_QUEST_PENDING;
		if (theId >= ID_QUEST_DONE)
			aPlace = theId - ID_QUEST_DONE;
		if (gApp->mProfile != nullptr)
			gApp->mProfile->mQuestMenuFlag = false;
		gApp->StartQuest(QuestIndex(aPlace));
	}
}

void QuestMenu::Update()
{
	Widget::Update();
	MarkDirty();
}

// slot 43 (0x6922d0, bejeweled3): the exe draws the 2D background (IMAGE_2DONLY_QUESTMENU_BACKGROUND, the pillars behind and in front of the page and the floor), the
// page of the section with its relics and sockets (0x690110, 0x687f90), the text "Complete quests to restore the Lost Relics of Bejeweled" and the percentage of the quests
// done, the progress pictures and the navigation arrows. port: the background and its layers (tiled over the menu's width), the percentage and the quests' names are drawn;
// the relics and the animations are not made
void QuestMenu::Draw(Graphics* g)
{
	g->SetColor(Color(0, 0, 0, 255));
	g->FillRect(0, 0, mWidth, mHeight);
	Image* aLayers[] = {IMAGE_2DONLY_QUESTMENU_BACKGROUND, IMAGE_2DONLY_QUESTMENU_BACKPILLARS, IMAGE_2DONLY_QUESTMENU_FLOOR, IMAGE_2DONLY_QUESTMENU_FRONTPILLARS};
	const int aLayerIds[] = {IMAGE_2DONLY_QUESTMENU_BACKGROUND_ID, IMAGE_2DONLY_QUESTMENU_BACKPILLARS_ID, IMAGE_2DONLY_QUESTMENU_FLOOR_ID, IMAGE_2DONLY_QUESTMENU_FRONTPILLARS_ID};
	for (int aLayer = 0; aLayer < 4; aLayer++) {
		Image* anImage = aLayers[aLayer];
		if (anImage == nullptr || anImage->GetWidth() <= 0)
			continue;
		const int aY = Scaled(GetImageOffsetY(aLayerIds[aLayer]));
		for (int aX = 0; aX < mWidth; aX += anImage->GetWidth())
			g->DrawImage(anImage, aX, aY);
	}
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_OVERWORLD_HOVER);
	char aText[64];
	int aTotalDone = 0;
	for (int aSection = 0; aSection < SECTION_COUNT; aSection++)
		aTotalDone += mSectionDone[aSection];
	snprintf(aText, sizeof(aText), "%d%%", aTotalDone * 100 / QUEST_COUNT);
	g->WriteString(aText, mWidth / 2, Scaled(120), 0, 0, true, 0, -1, -1);
	if (mSectionState[mSection] == SECTION_LOCKED)
		g->WriteString("LOCKED", mWidth / 2, mHeight / 2, 0, 0, true, 0, -1, -1);
	DeferOverlay(0);
}

// port: the quests' icons (the exe puts them in the sockets of the relics) and names are drawn over the buttons (the children are drawn after the menu's own Draw, so this is the
// overlay pass); a quest that is done says so
void QuestMenu::DrawOverlay(Graphics* g)
{
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_OVERWORLD_HOVER);
	for (size_t aPlace = 0; aPlace < mQuestButtons.size(); aPlace++) {
		const ButtonWidget* aButton = mQuestButtons[aPlace];
		const int anIndex = QuestIndex((int)aPlace);
		std::string aName = Unquoted(gApp->mQuestData->mQuests[(size_t)anIndex].mName);
		const QuestLook* aLook = LookOf(anIndex);
		if (aLook != nullptr && *aLook->mIcon != nullptr)
			g->DrawImage(*aLook->mIcon, aButton->mX + (aButton->mWidth - (*aLook->mIcon)->GetWidth()) / 2, aButton->mY + (aButton->mHeight - (*aLook->mIcon)->GetHeight()) / 2);
		if (IsQuestDone(anIndex))
			aName += " (done)";
		g->WriteString(aName, aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2 + Scaled(130), 0, 0, true, 0, -1, -1);
	}
}

}  // namespace Sexy
