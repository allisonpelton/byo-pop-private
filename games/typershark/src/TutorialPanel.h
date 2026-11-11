#ifndef __TYPERSHARK_TUTORIALPANEL_H__
#define __TYPERSHARK_TUTORIALPANEL_H__

// TutorialPanel.cpp: the "Tutorial Mode" page of the stats screen: a bar graph of the typing tutor's results for one lesson
// (lesson 0 is the typing test, the lessons 1 to 18 are the lessons) with three bars (the average, the best and the most
// recent attempt) for one of three measures (words per minute, adjusted words per minute, accuracy), a "Next Lesson" and
// a "Previous Lesson" button to change the lesson and three radio buttons to change the measure. The bars grow from 0 to
// their height 20 pixels at a time (whenever the page's timer reads more than 4 milliseconds), and the numbers are
// written on top of them.
// The StatsScreen sets mUser (SetUser) and then calls AdvanceToLesson with the profile's last lesson.
// Original: class Sexy::TutorialPanel : public TabPanel, public ButtonListener (the listener sub-object is at +0x8c with
// its own table 0x52fdb8), vftable 0x52fdd4, sizeof 0x174 (constructor 0x453ba0, destructor body 0x449c40, deleting
// destructor 0x450a50). The ButtonDepress of the listener is 0x450a80 (its `this` is the sub-object, the panel's +0x8c).
//
// Not translated: the allocator code of the std::vector<Widget*> and the strings (0x4511c0 and the string members).

#include "TabPanel.h"
#include "ButtonListener.h"

namespace Sexy
{

class ButtonWidget;
class DialogButton;
class UserProfile;
class WSTimer;

class TutorialPanel : public TabPanel, public ButtonListener
{
public:
	enum
	{
		NEXT_BUTTON_ID = 0x17f49,		// "Next Lesson"
		PREV_BUTTON_ID = 0x17f4a,		// "Previous Lesson"
		WPM_BUTTON_ID = 0x17f4b,		// radio button "Words Per Minute"
		ACCURACY_BUTTON_ID = 0x17f4c,	// radio button "Accuracy"
		ADJUSTED_BUTTON_ID = 0x17f4d	// radio button "Adjusted WPM"
	};

	enum
	{
		GRAPH_WPM = 0,					// the lesson record's best (+0) and last (+4) words per minute; the average from its total
		GRAPH_ADJUSTED = 1,				// the adjusted words per minute (+0x10, +0x14)
		GRAPH_ACCURACY = 2				// the accuracy in percent (+0x20, +0x24); the graph is 100 high with a tick every 10
	};

public:
	// the TabPanel part ends at +0x8c, the ButtonListener part is +0x8c (its virtual table pointer)
	DialogButton*			mNextButton;		// +0x90 id NEXT_BUTTON_ID, the TAB_BUTTON image; at (60, 75), 150 x 27
	DialogButton*			mPrevButton;		// +0x94 id PREV_BUTTON_ID; at (60, 105), 150 x 27; hidden on lesson 0
	ButtonWidget*			mWPMButton;			// +0x98 id WPM_BUTTON_ID; a radio button (down while its graph shows: disabled)
	ButtonWidget*			mAccuracyButton;	// +0x9c id ACCURACY_BUTTON_ID
	ButtonWidget*			mAdjustedButton;	// +0xa0 id ADJUSTED_BUTTON_ID
	WSTimer*				mTimer;				// +0xa4 owned (the original leaks it); added to the manager with the panel; paces the bars' growth
	int						mAxisMax;			// +0xa8 the best value of the graph (the top of the axis is mTickCount ticks of mStep)
	int						mUnitHeight;		// +0xac pixels per tick: 212 / mTickCount
	int						mStep;				// +0xb0 the value of one tick: 10, or 1 when the best value is below 10 (the accuracy graph: 10)
	int						mTickCount;			// +0xb4 ticks drawn: mAxisMax / mStep + 2 (accuracy: 11)
	SexyString				mAverageLabel;		// +0xb8 under the first bar ("Average")
	SexyString				mBestLabel;			// +0xd4 under the second bar ("Best")
	SexyString				mRecentLabel;		// +0xf0 under the third bar ("Most Recent Attempt")
	int						mAverageBarHeight;				// +0x10c the height of the average bar being drawn (Update grows it 20 pixels a step to mBar1Target; SetGraph restarts it at 0)
	int						mBar1Target;		// +0x110 its final height in pixels
	int						mAverageValue;			// +0x114 the lesson's average of the measure, written above the average bar
	int						mBestBarHeight;				// +0x118 the height of the best bar being drawn (grows to mBar2Target; SetGraph restarts it at 0)
	int						mBar2Target;		// +0x11c
	int						mBestValue;			// +0x120 the lesson's best value of the measure, written above the best bar
	int						mRecentBarHeight;				// +0x124 the height of the most recent attempt's bar being drawn (grows to mBar3Target; SetGraph restarts it at 0)
	int						mBar3Target;		// +0x128
	int						mRecentValue;			// +0x12c the most recent attempt's value of the measure, written above the most recent bar
	int						mLesson;			// +0x130 ctor -1 (AdvanceToLesson moves it to the profile's last lesson); 0 is the typing test
	int						mGraphType;			// +0x134 GRAPH_WPM, GRAPH_ADJUSTED or GRAPH_ACCURACY (ctor 0)
	SexyString				mHeader;			// +0x138 "LESSON n: <measure>" or "TYPING TEST: <measure>" above the graph
	SexyString				mCaption;			// +0x154 what the graph shows, or that the lesson has not been completed (word-wrapped, two lines)
	UserProfile*			mUser;				// +0x170 the profile whose results are shown (ctor unset; port: NULL); StatsScreen::SetUser sets it

public:
	TutorialPanel(WinWSApp* theApp);										// 0x453ba0 the title is "Tutorial Mode"
	virtual ~TutorialPanel() override;										// 0x449c40 (deleting 0x450a50), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x449df0 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4432e0 slot 2
	virtual void			Draw(Graphics* g) override;						// 0x44a060 slot 16
	virtual void			Update() override;								// 0x4417f0 slot 17
	virtual void			SetShown(bool theShow) override;				// 0x443330 slot 47
	virtual void			ButtonDepress(int theId) override;				// 0x450a80 the listener's slot 4

	void					SetGraph(int theGraph);							// 0x44a7b0 loads the three bars, the labels and the texts of a measure for mLesson
	void					NextLesson();									// 0x44ae30 mLesson + 1 if there is one, and the header
	void					PrevLesson();									// 0x44b070 mLesson - 1 if there is one, and the header
	void					AdvanceToLesson(int theLesson);					// 0x450bc0 theLesson + 1 times NextLesson (the lesson index to show, from -1)
};

}

#endif
