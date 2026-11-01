#ifndef __BONNIES_APP_H__
#define __BONNIES_APP_H__

// The engine's app (0x420da0-0x421960, the vtable 0x458c38, the object 0x480b0c): the platform with a list of screens
// drawn in order of priority, the modal stack, the widget with the focus and a status line. The game's app derives
// from it, and gApp and gPlatform are the same object.

#include "Platform.h"

namespace bonnies
{

struct Font;
struct FontDesc;
class Screen;
struct Sample;
class Widget;

class App : public Platform
{
public:
	// 0x420da0
	App();
	// 0x420df0
	~App() override;

	// slot 0, 0x420e50
	void init(int theWidth, int theHeight, int theFullScreen) override;
	// slot 1, 0x420f20: deletes the screens and frees the font and sounds
	void shutdown() override;
	// slot 2, 0x421810: the screens in order, then the status line
	void draw() override;
	// slot 3, 0x4218d0: the screens' updates (last first) and a modal end that waited for this
	void update() override;
	// slot 4, 0x421160: the screens from the last, until one consumes theEvent or a modal screen's level is reached
	int handleEvent(Event* theEvent) override;
	// slot 19, 0x421240: whether the top of the modal stack blocks
	int isModal() override;
	// slot 22, 0x421070: adds theScreen before the first of a higher priority
	virtual void addScreen(Screen* theScreen);
	// slot 23, 0x421140: removes theScreen, shuts it down and frees it
	virtual void deleteScreen(Screen* theScreen);
	// slot 24, 0x4210f0
	virtual void removeScreen(Screen* theScreen);
	// slot 25, 0x421030: the status line's font
	virtual void setStatusFont(FontDesc* theDesc);
	// slot 26, 0x4011c0: shows theText on the status line until it changes
	virtual void showStatus(const Str& theText);
	// slot 27, 0x4211b0: the status line's text, cleared after theTime ms (never when 0)
	virtual void setStatus(const Str& theText, int theTime);
	// slot 28, 0x421230: whether a modal screen is up
	virtual bool inModal();

	// 0x420fa0: the default sound of a button press (0), release (1) or check box (2)
	void setDefaultSound(int theIndex, const Str& theName);
	// 0x421260: pushes theScreen on the modal stack; when theBlocking, runs the loop until it ends and answers its result
	int runModal(int theBlocking, Screen* theScreen);
	// 0x421340: ends the top modal screen with theResult (a screen that does not block, only from the next update
	// unless theNow)
	void endModal(int theResult, int theNow);
	// 0x4213d0: the widget with the focus, and the hand cursor over a button, check box or slider
	void setFocus(Widget* theWidget);

	int mResults[21];           // +0x3c: each modal level's result (+0x40 the first level's)
	int mDone[21];              // +0x90
	Screen* mModal[21];         // +0xe4
	int mBlocking[21];          // +0x138
	Screen* mTail;              // +0x18c
	Screen* mHead;              // +0x190
	Widget* mFocus;             // +0x194
	int mDepth;                 // +0x198
	char mDebugRects[7];        // +0x19c: widgets whose rectangles are drawn, by type
	Str mStatus;                // +0x1a4
	int mStatusUntil;           // +0x1ac
	Font* mStatusFont;          // +0x1b0
	uint32_t mStatusColor;      // +0x1b4
	uint32_t mStatusFlags;      // +0x1b8
	float mStatusX;             // +0x1bc: the line's centre
	Sample* mSounds[3];         // +0x1c0
	int mLastResult;            // +0x1cc
	int mEndPending;            // +0x1d0
	int mPendingResult;         // +0x1d4
};

extern App* gApp;  // 0x480b0c

} // namespace bonnies

#endif
