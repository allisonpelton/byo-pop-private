#ifndef __ASTROPOP_HELPDIALOG_H__
#define __ASTROPOP_HELPDIALOG_H__

// HelpDialog: the "how to play" box (id 0x79; original class of 0x140 bytes, constructor 0x48ab10, destructor body 0x48a6b0,
// vftable 0x56b9a0, with the button listener at +0x58, vftable 0x56b988). Three pages: the rules of the mode that is on (or,
// from the selector, of the game), and the bricks, ten of them with a picture and a line, five on each of the two other pages.
// The first button is CLOSE, the second MORE... goes on to the next page. Offsets in the comments are the original's.

#include "Text2004.h"
#include "Dialog.h"
#include <string>
#include <vector>

namespace Sexy
{

class AstroPopApp;

class HelpDialog : public Text2004<Dialog>
{
public:
	// a brick with its picture and the line that tells what it does (0x20 bytes in the original)
	struct Item
	{
		Image*			mImage;
		std::string		mText;
	};

	AstroPopApp*				mApp;       // +0x110
	std::vector<Item>			mItems;     // +0x114 the bricks of the two last pages
	std::vector<std::string>	mLines;     // +0x124 the rules of the first page
	int							m134;       // +0x134 (0: nothing in the original sets it)
	int							mTextIndent;    // +0x138 where the text of a brick starts: the width of the fuel cell and 10
	int							mPage;      // +0x13c 0: the rules, 1 and 2: the bricks

public:
	HelpDialog(int theColourSet, AstroPopApp* theApp);                      // 0x48ab10
	virtual ~HelpDialog();                                                  // 0x48aa20 (body 0x48a6b0)

	virtual void	Draw(Graphics* g) override;                             // 0x4882d0
	virtual int		GetPreferredHeight(int theWidth) override;              // 0x4837e0
	virtual void	ButtonDepress(int theId) override;                      // 0x4886a0 CLOSE goes to the application, MORE... turns the page

private:
	void			AddItem(Image* theImage, const std::string& theText);   // 0x48aa50
};

}

#endif
