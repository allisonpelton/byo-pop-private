// ThunderDialog.h: the game's dialog (the original's ThunderDialog.cpp). Every dialog of the game is one: a framework Dialog
// with the game's pictures, fonts, colours and buttons, that slides in from above or below and slides out again, and can hang
// from a chain.

#ifndef __THUNDERDIALOG_H__
#define __THUNDERDIALOG_H__

#include "Dialog.h"

#include <string>

namespace Sexy
{

class Graphics;

class ThunderDialog : public Dialog
{
public:
	// mScrollTargetY while the dialog is not sliding
	static const int SCROLL_NONE = -100000;

	// theId is the dialog's id (what KillDialog and GetDialog take), theButtonMode one of Dialog's BUTTONS_
	ThunderDialog(int theId, bool isModal, const std::string& theHeader, const std::string& theLines, const std::string& theFooter,
				  int theButtonMode); // 0x497280
	~ThunderDialog() override;		  // 0x497550

	bool IsScrolling() const;			   // 0x497570
	void DoScroll(int theDirection);	   // 0x497580: slides in; from above for a positive direction, from below otherwise
	void DoScrollOff(bool theKillWhenDone); // 0x497690: slides out of the screen, takes the dialog off the app's list and, when
											// it is out, off the manager (and deletes it if told to)
	void FinishScroll();					// 0x497700: puts it where the slide was going at once
	void CancelScroll();					// 0x497720: stops the slide where it is

	// where the dialog's contents are: the edges of the area inside its frame, the top below the header
	int GetLeft() const;  // 0x497750
	int GetRight() const; // 0x497780
	int GetTop() const;	  // 0x4977a0
	int GetWidth() const; // 0x4977e0

	void Draw(Graphics* g) override;					// 0x497820
	void Update() override;								// 0x49bf20
	void MouseDown(int x, int y, int theClickCount) override; // 0x497910
	void KeyDown(KeyCode theKey) override;				// 0x497930

	using Dialog::MouseDown;

	int mScrollTargetY;			// +0x150: where the slide is going, SCROLL_NONE when it is not sliding
	int mScrollSpeedX;			// +0x158: pixels the dialog also moves sideways in every update of a slide (the main menu's switch from
								// another screen sets it to 40 on the dialog it replaces, so that one leaves to the side)
	int mScrollDirection;		// +0x15c: positive for a slide down, negative for one up
	bool mScrollingOff;			// +0x160: the slide is the one out of the screen
	bool mKillWhenScrolledOff;	// +0x161: deletes the dialog when it is out
	bool mFastScrollOff;		// +0x162: slides out at the top speed
	bool mHangsOnChain;			// +0x163: draws the chain it hangs from above it
	int mChainSpacing;			// +0x168: the distance between two links of the chain
	bool mAlwaysDirty;			// +0x170: redraws every update
	void (*mDrawHook)(ThunderDialog* theDialog, Graphics* g); // +0x174: drawn after the dialog (null for none)
};

} // namespace Sexy

#endif
