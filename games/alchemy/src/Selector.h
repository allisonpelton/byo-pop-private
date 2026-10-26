#ifndef __ALCHEMY_SELECTOR_H__
#define __ALCHEMY_SELECTOR_H__

// The game selector (class 0x4e43a8, ctor 0x40c817, size 0xa4): the intro screen over the board area with the wizard,
// three skill buttons, strategic or time trial, and the start and tutorial buttons. The ButtonListener part is at +0x58.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class AlchemyApp;
class ButtonWidget;
class MemoryImage;
class WizardBox;

class SelectorScreen : public Widget, public ButtonListener
{
public:
	AlchemyApp*			mApp;                   // +0x60
	int					mSkill;                 // +0x64 1..3
	bool				mTimeTrial;             // +0x68
	ButtonWidget*		mStartButton;           // +0x6c id 0: new game
	ButtonWidget*		mTutorialPicButton;     // +0x70 id 1: tutorial
	ButtonWidget*		mTutorialButton;        // +0x74 id 4: the suggestion's link (no picture), visible for the first five plays
	ButtonWidget*		mSkillButtons[3];       // +0x78 ids 0xb..0xd
	MemoryImage*		mSkillImages[3];        // +0x84
	ButtonWidget*		mStrategicButton;       // +0x90 id 2
	MemoryImage*		mStrategicImage;        // +0x94
	ButtonWidget*		mTimeTrialButton;       // +0x98 id 3
	MemoryImage*		mTimeTrialImage;        // +0x9c
	WizardBox*			mWizard;                // +0xa0

	SelectorScreen(AlchemyApp* theApp);                     // 0x40c817
	virtual ~SelectorScreen();                              // 0x40c7fb

	void				SetSkill(int theSkill);             // 0x40c488
	void				SetTimeTrial(bool isTimeTrial);     // 0x40c4c6

	virtual void		Update() override;                  // 0x40c526
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;       // 0x40c559
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x40c5c8
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x40c637
	virtual void		Draw(Graphics* g) override;         // 0x40cefe
	virtual void		ButtonPress(int theId) override;    // 0x40c760
	virtual void		ButtonDepress(int theId) override;  // 0x40c770

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
