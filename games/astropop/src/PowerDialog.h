#ifndef __ASTROPOP_POWERDIALOG_H__
#define __ASTROPOP_POWERDIALOG_H__

// PowerDialog: the box that explains the pilot's weapon before the first move of a game (id 0x7b; original class of 0x140
// bytes, constructor 0x48b7a0, destructor 0x48a770 (body 0x48a560), vftable 0x56b8b8; it stands on a base class of 0x120
// bytes, constructor 0x484880, destructor 0x484210 (body 0x4839a0), vftable 0x56acc8, that nothing else uses, so the port
// keeps the two as one). The box has the weapon's pictures in a row, each with two or three lines under it, a box to tick
// that says the player does not want to see it again (ClosePowerDialog keeps the answer in the profile), and the footer
// button OK that the Return, Space and Escape keys press too. It is twice as high as its pictures and 150 more, and as wide as its pictures need.
// Offsets in the comments are the original's.

#include "Text2004.h"
#include "CheckboxListener.h"
#include "Dialog.h"
#include <string>
#include <vector>

namespace Sexy
{

class AstroPopApp;
class Checkbox;
class Image;
class MemoryImage;

class PowerDialog : public Text2004<Dialog>, public CheckboxListener
{
public:
	AstroPopApp*				mApp;       // +0x114 (+0x118: set to 0, read by ButtonPress as a sample's number)
	Checkbox*					mCheckbox;  // +0x11c "don't show this again": one picture in two halves, the ticked one on the right
	std::vector<std::string>	mLines;     // +0x120 what is written under each picture
	std::vector<Image*>			mImages;    // +0x130 the pictures of the weapon (the game's own, not owned)

public:
	// The box owns the three pictures it is given (the inset, the strip of the buttons and the tick box's picture): the
	// original's ClosePowerDialog frees them before the box goes; here the box frees them with itself.
	PowerDialog(AstroPopApp* theApp, int thePilot, MemoryImage* theInsetImage, MemoryImage* theStripImage, Image* theCheckImage);
	virtual ~PowerDialog();

	virtual void	AddedToManager(WidgetManager* theManager) override;                 // 0x483a60
	virtual void	RemovedFromManager(WidgetManager* theManager) override;             // 0x483a90
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x483bb0 (base 0x483ac0)
	virtual void	Draw(Graphics* g) override;                                         // 0x489190 (base 0x489070)
	virtual void	KeyChar(SexyChar theChar) override;                                 // 0x483a20
	virtual int		GetPreferredHeight(int theWidth) override;                          // 0x483b90
	virtual void	ButtonPress(int theId) override;                                    // 0x483b50 (the box's listener at +0x58)
	virtual void	CheckboxChecked(int theId, bool theChecked) override;               // 0x483b20 (the listener at +0x110)
};

}

#endif
