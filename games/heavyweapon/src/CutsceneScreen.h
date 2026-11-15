#ifndef __HEAVYWEAPON_CUTSCENESCREEN_H__
#define __HEAVYWEAPON_CUTSCENESCREEN_H__

// CutsceneScreen: a Flash movie over the whole screen after a level: middle.swf after the 9th level, then a fade
// through green to the armory; final.swf after the last, on a dark red ground, which a click ends on the high scores
// (or the main menu when the campaign's score did not place). A click on the middle one starts its fade.
// Original: a Widget of 0x70 bytes with a FlashListener at +0x58, constructor 0x41f070, vtable 0x52cf78 (listener
// 0x52cf70), destructor body 0x41ed50 (deleting 0x41f1a0).

#include "FlashWidget.h"
#include "Widget.h"

namespace Sexy
{

class HeavyWeaponApp;

class CutsceneScreen : public Widget, public FlashListener
{
public:
	enum
	{
		CUTSCENE_MIDDLE = 0,
		CUTSCENE_FINAL = 1
	};

	int				mCutscene;			// +0x5c CUTSCENE_
	double			mFadeAngle;			// +0x60 the fade's progress: its alpha is sin of it, from 0.2 to pi
	HeavyWeaponApp*	mApp;				// +0x68
	FlashWidget*	mFlashWidget;		// +0x6c the movie (gone once the fade has covered the screen)

public:
	CutsceneScreen(HeavyWeaponApp* theApp, bool isFinal);					// 0x41f070
	virtual ~CutsceneScreen();												// 0x41f1a0 (body 0x41ed50)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;	// 0x41f1d0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x41edb0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x41ede0 slot 13
	virtual void	Draw(Graphics* g) override;								// 0x41ef50 slot 16
	virtual void	Update() override;										// 0x41efc0 slot 17
	virtual void	MouseDown(int x, int y, int theClickCount) override;	// 0x41ef10 slot 28
	virtual void	FlashAnimEnded(int theId) override;						// 0x41ef30 (listener slot 0)

	void			EndCutscene();											// 0x41ee20

	using Widget::Resize;
	using Widget::MouseDown;
};

}

#endif
