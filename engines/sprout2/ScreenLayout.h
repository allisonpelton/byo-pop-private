#ifndef __SPROUT2_SCREENLAYOUT_H__
#define __SPROUT2_SCREENLAYOUT_H__

// A screen read from an XML file (ScreenLayout, vtable 0x55c684 (feedingfrenzy2) with 86 slots, 0x174 bytes; its
// constructor 0x47d230 (feedingfrenzy2)): a fading container that keeps the screen's components, modifiers and
// effects by name, and turns its buttons and keys into actions, strings it passes to its listener. Enter fires the
// default action and Escape the cancel action, when the screen has them; other keys fire the actions added for them.

#include "BltFx.h"
#include "ButtonBase.h"
#include "FadeContainer.h"
#include "TextItem.h"
#include "sigslot.h"

#include <map>

namespace engine
{

class Theme;

// What a screen's actions go to (ILayoutActionListener (feedingfrenzy2))
class ILayoutActionListener : public virtual Interface
{
public:
	// 0x00
	virtual void layoutAction(std::string theAction) = 0;
};

class ScreenLayout : public FadeContainer, public IButtonListener, public sigslot::HasSlots
{
public:
	// 0x47d230 (feedingfrenzy2): animated
	ScreenLayout();

	// 0x68 (0x5230f8 (feedingfrenzy2))
	std::string getClassName() override { return "ScreenLayout"; }
	// 0x90 (0x40c760) and 0x94 (0x41cae0) (feedingfrenzy2): the keys listened to while enabled
	void enable() override;
	void disable() override;

	// 0x128 (0x406a70 (feedingfrenzy2))
	virtual bool load(const std::string& thePath) { return load(thePath, nullptr); }
	// 0x12c (0x47d790 (feedingfrenzy2)): the key actions cleared, the screen named after the file, and its elements
	// read at the engine's detail level, the theme's images found before the engine's
	virtual bool load(const std::string& thePath, Theme* theTheme);
	// 0x130 (0x46cf50 (feedingfrenzy2))
	virtual void registerComponent(Component* theComponent, const std::string& theName);
	// 0x134 (0x478cc0 (feedingfrenzy2)): a named text kept by its name too
	virtual void registerText(TextItem* theText, const std::string& theName) { mTexts[theName] = theText; }
	// 0x138 (0x4756b0 (feedingfrenzy2)): a button's clicks passed on as the action (the name is not used)
	virtual void addButton(ButtonBase* theButton, const std::string& theName, std::string theAction)
	{
		theButton->addListener(this, theAction);
	}
	// 0x13c (0x499a70 (feedingfrenzy2)): a signal of a component's (its +0xbc) connected to the layout (not yet
	// translated: its callers are not yet)
	// 0x140 (0x492ef0 (feedingfrenzy2))
	virtual void doAction(const std::string& theAction);
	// 0x144 (0x414b70) and 0x148 (0x414b50) (feedingfrenzy2)
	virtual void setDefaultAction(const std::string& theAction) { mDefaultAction = theAction; }
	virtual void setCancelAction(const std::string& theAction) { mCancelAction = theAction; }
	// 0x14c (0x493640 (feedingfrenzy2))
	virtual void keyDown(int theKey);
	// 0x150 (0x41a820 (feedingfrenzy2)): a key's first action stays; false for a key that has one
	virtual bool addKeyAction(int theKey, const std::string& theAction);
	// 0x154 (0x414da0 (feedingfrenzy2)): the key actions only
	virtual void clearKeyActions() { mKeyActions.clear(); }

	// IButtonListener's 0x00 to 0x0c (0x5014fa (feedingfrenzy2)): nothing
	void buttonEnter(const std::string& theAction) override {}
	void buttonLeave(const std::string& theAction) override {}
	void buttonDown(const std::string& theAction) override {}
	void buttonUp(const std::string& theAction) override {}
	// IButtonListener's 0x10 (0x40b470 (feedingfrenzy2)): a copy passed to the listener, if there is one
	void buttonAction(const std::string& theAction) override { doAction(theAction); }

	void setListener(ILayoutActionListener* theListener) { mListener = theListener; }
	Component* findComponent(const std::string& theName);
	// 0x48e570 and 0x402ae0 (feedingfrenzy2): a <modifier> and a <bltFx> kept by name
	void registerModifier(const std::string& theName, Modifier* theModifier);
	void registerBltFx(const std::string& theName, BltFx* theBltFx);
	// 0x40b9e0 and 0x490ec0 (feedingfrenzy2)
	Modifier* findModifier(const std::string& theName);
	BltFx* findBltFx(const std::string& theName);
	void setScreenSize(int theWidth, int theHeight)
	{
		mScreenWidth = theWidth;
		mScreenHeight = theHeight;
	}

private:
	// +0xe8: a map not yet translated
	std::map<std::string, Ref<TextItem>> mTexts;        // +0xf4
	std::map<std::string, Component*> mComponents;      // +0x100
	ILayoutActionListener* mListener;                   // +0x10c
	std::map<int, std::string> mKeyActions;             // +0x110
	std::string mDefaultAction;                         // +0x11c
	std::string mCancelAction;                          // +0x138
	int mScreenWidth;                                   // +0x154: the screen's screenSize, 0 when it has none
	int mScreenHeight;                                  // +0x158
	std::map<std::string, Ref<Modifier>> mModifiers;    // +0x15c
	std::map<std::string, Ref<BltFx>> mBltFxs;          // +0x168
};

} // namespace engine

#endif
