#ifndef __FEEDINGFRENZY2_STAGEINTROSCREEN_H__
#define __FEEDINGFRENZY2_STAGEINTROSCREEN_H__

// The screen before a stage (StageIntroScreen, 0x1e0 bytes, vtable 0x55a35c (feedingfrenzy2); its constructor
// 0x48bb20 (feedingfrenzy2)), the application's +0x98, read from resource/shell/stageStartBase.xml with the shell's
// theme: the stage's title, caption, paragraphs and objective from its screen data, over the two layouts the data
// names (its text and its art, each a screen layout read into a child); in the timed mode the stage's new time and the
// clock's total instead of the objective. Its buttons' actions are the stage intro state's.

#include "GenericScreen.h"
#include "ScreenLayout.h"
#include "TextItem.h"

class Stage;

class StageIntroScreen : public engine::GenericScreen
{
public:
	StageIntroScreen() {}

	// 0x488de0 (feedingfrenzy2): the layouts of the stage's screen data read in place of the last stage's, the
	// components found again and given the data's texts, and the objective or the clock's times shown for the mode
	// (the objective for a bonus stage)
	void showFor(const std::string& theStageId, Stage* theStage);

private:
	// 0x41b5b0 (feedingfrenzy2): the screen's texts and groups found, emptied and the time groups hidden, then those
	// the text layout has taken from it
	void findComponents();
	// 0x492e10 and 0x4940f0 (feedingfrenzy2): a layout read from the file with the shell's theme and added on top,
	// kept (and not added) when it cannot be read
	void loadLayout(engine::Ref<engine::ScreenLayout>& theLayout, const std::string& thePath, const char* theKind);

	engine::Ref<engine::TextItem> mTitle;                         // +0x1a0
	engine::Ref<engine::TextItem> mCaption;                       // +0x1a4
	engine::Ref<engine::TextItem> mPara1Header;                   // +0x1a8: the text layout's, null for none
	engine::Ref<engine::TextItem> mPara2Header;                   // +0x1ac
	engine::Ref<engine::TextItem> mPara1;                         // +0x1b0
	engine::Ref<engine::TextItem> mPara2;                         // +0x1b4
	engine::Ref<engine::TextItem> mObjective;                     // +0x1b8
	engine::Ref<engine::Component> mTimeObjectiveGroup;           // +0x1bc: the new time and the clock's total
	engine::Ref<engine::Component> mStartingTimeObjectiveGroup;   // +0x1c0: the clock's total alone, for a new game
	engine::Ref<engine::TextItem> mNewTimeText;                   // +0x1c4
	engine::Ref<engine::TextItem> mTotalTimeText;                 // +0x1c8
	engine::Ref<engine::TextItem> mStartTotalTimeText;            // +0x1cc
	engine::Ref<engine::ScreenLayout> mTextLayout;                // +0x1d0
	engine::Ref<engine::ScreenLayout> mImageLayout;               // +0x1d4
};

#endif
