// Sexy::QuestMenu (bejeweled3, vftable 0x86df4c, 81 slots, made by 0x693760, shown by 0x773b70 when the play menu's QUEST ball is pressed): the quest map. Five sections
// of eight quests (quest.cfg's quests in order: section n holds the quests 8n to 8n+7), each with a page of relics that come back as the quests are done; the quests of
// the section on show are buttons (QuestMenuBtn, vftable 0x86e4f4), and PREV and NEXT turn the pages. It is the listener (+0xbc) of its buttons. See docs/NOTES.md,
// "Quest", for what of it is made.
#pragma once

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Widget.h"

#include <string>
#include <vector>

namespace Sexy {

class QuestMenu : public Widget, public ButtonListener {
public:
	enum { SECTION_COUNT = 5, QUESTS_PER_SECTION = 8, QUEST_COUNT = SECTION_COUNT * QUESTS_PER_SECTION };
	// the buttons' ids (0x693760): PREV and NEXT are the first two, BACK and QUIT the next two, and the quests' are 1000 plus the quest's place on the page
	// (pending) or 2000 plus it (done)
	enum { ID_PREV = 0, ID_NEXT = 1, ID_BACK = 2, ID_QUIT = 3, ID_QUEST_PENDING = 1000, ID_QUEST_DONE = 2000 };
	// the sections' states (0x6734d0): 0 locked, 1 the one the player is at (fewer than 4 quests done and every section before it has 4 or more), 2 with 4 to 7 done,
	// 3 with all 8 done
	enum { SECTION_LOCKED = 0, SECTION_CURRENT = 1, SECTION_HALF_DONE = 2, SECTION_DONE = 3 };
	// a section is open once the one before it has this many quests done
	enum { QUESTS_TO_OPEN_NEXT = 4 };
	// the pictures of a quest's button (QuestMenu +0x129's map): the picture of the button and the one of its icon
	struct QuestLook {
		Image** mButton;
		Image** mIcon;
		int mButtonId;      // the resource ids (the numbers 0x693760 puts in the map)
		int mIconId;
	};

	int mSection = 0;                       // +0x14c: the section on show (the profile's +0x184 when the menu is made)
	int mSectionDone[SECTION_COUNT] = {};   // section +0x6ec: the quests done in each section
	int mSectionState[SECTION_COUNT] = {};  // 0x6734d0
	ButtonWidget* mPrevButton = nullptr;    // +0x118 (id 0)
	ButtonWidget* mNextButton = nullptr;    // +0x11c (id 1)
	ButtonWidget* mBackButton = nullptr;    // +0x120 (id 2)
	ButtonWidget* mQuitButton = nullptr;    // +0x124 (id 3)
	std::vector<ButtonWidget*> mQuestButtons;   // the 8 buttons of the page on show, in the order of the quests

	QuestMenu();        // 0x693760 (bejeweled3)
	~QuestMenu() override;
	void Draw(Graphics* g) override;               // slot 43 (0x6922d0)
	void ButtonDepress(int theId) override;        // 0x697640 (bejeweled3)
	void DrawOverlay(Graphics* g) override;        // port: the names of the quests
	void Update() override;                        // 0x697d40, a part

	// 0x6734d0 (bejeweled3): the sections' done counts and states from the profile
	void UpdateSections();
	// the quest at a place of the page on show, as its index in quest.cfg
	int QuestIndex(int thePlace) const { return mSection * QUESTS_PER_SECTION + thePlace; }
	bool IsQuestDone(int theIndex) const;
	// the pictures of a quest's button: its Challenge if the map has it, else its Goal (QuestMenu +0x129's map is looked up in that order), null when neither is there
	static const QuestLook* LookOf(int theQuestIndex);
	// the buttons of the page on show are made and put (the exe keeps all 16: the pending ones and the done ones, of which Update shows the ones that fit)
	void LayoutPage();
	// the section on show is turned to (0x697640's ids 0 and 1: the one before or after, round)
	void TurnTo(int theSection);
};

}  // namespace Sexy
