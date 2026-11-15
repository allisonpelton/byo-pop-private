#ifndef __HEAVYWEAPON_ARMORYWEAPONPANEL_H__
#define __HEAVYWEAPON_ARMORYWEAPONPANEL_H__

// ArmoryWeaponPanel: one weapon's tile in the armory: its picture with a glow under the mouse, its level, and the plus and
// minus buttons that spend and take back upgrade points (the whole picture also acts as a plus button). Original: a Widget
// with a ButtonListener at +0x58, 0x7c bytes, constructor 0x42ff80, vtable 0x52fe90 (listener 0x52fe74), destructor body
// 0x42fa20.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class Armory;
class ArmoryArrowButton;
class ButtonWidget;
class HeavyWeaponApp;

class ArmoryWeaponPanel : public Widget, public ButtonListener
{
public:
	HeavyWeaponApp*		mApp;			// +0x5c
	Armory*				mArmory;		// +0x60
	ArmoryArrowButton*	mPlusButton;	// +0x64 id 0
	ArmoryArrowButton*	mMinusButton;	// +0x68 id 1
	ButtonWidget*		mPictureButton;	// +0x6c the invisible button over the picture, id 0
	int					mWeapon;		// +0x70 the weapon (HeavyWeaponApp WEAPON_)
	int					mGlow;			// +0x74 the picture's glow, 0..255
	int					mColumn;		// +0x78 0 or 1: which side of the armory the tile is on

public:
	ArmoryWeaponPanel(Armory* theArmory, int theWeapon, int theColumn);		// 0x42ff80
	virtual ~ArmoryWeaponPanel();											// 0x430090 (body 0x42fa20)

	virtual void	AddedToManager(WidgetManager* theManager) override;		// 0x42faa0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;	// 0x42fae0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x42fde0 slot 13
	virtual void	Draw(Graphics* g) override;								// 0x42fb20 slot 16
	virtual void	Update() override;										// 0x42fc30 slot 17
	virtual void	MouseEnter() override;									// 0x42fc90 slot 24
	virtual void	MouseLeave() override;									// 0x42fce0 slot 25
	virtual void	MouseMove(int x, int y) override;						// 0x42fd30 slot 26

	virtual void	ButtonMouseEnter(int theId) override;					// 0x42fd40 listener slot 0
	virtual void	ButtonMouseLeave(int theId) override;					// 0x42fd80 listener slot 1
	virtual void	ButtonMouseMove(int theId, int x, int y) override;		// 0x42fdd0 listener slot 2
	virtual void	ButtonPress(int theId) override;						// 0x42fe90 listener slot 4
	virtual void	ButtonDepress(int theId) override;						// 0x42feb0 listener slot 5

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
