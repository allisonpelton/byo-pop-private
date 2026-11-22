// ThunderCommon.h: the functions that make the game's buttons and lists in the way every screen and dialog wants them
// (the original's ThunderCommon.cpp). The ones that take an id and a listener give the widget that id, and tell the listener
// when it is pressed.

#ifndef __THUNDERCOMMON_H__
#define __THUNDERCOMMON_H__

#include <string>

namespace Sexy
{

class ButtonListener;
class ButtonWidget;
class Dialog;
class EditListener;
class EditWidget;
class Graphics;
class Image;
class ListListener;
class ListWidget;
class SimpleButton;
class ThunderButton;

// Sets a button's picture rects for a picture of three states one above the other (normal, mouse over, pressed), or side by
// side when theSideBySide is true; also its font and the white colours of its label.
void SetThreeStateButton(ThunderButton* theButton, bool theSideBySide); // 0x407c40

// A button with a label in the picture of the dialogs' buttons (or the picture it is given).
ThunderButton* MakeButton(int theId, ButtonListener* theListener, const std::string& theLabel, Image* theImage = nullptr); // 0x41bc70
ThunderButton* MakePurpleButton(int theId, ButtonListener* theListener, const std::string& theLabel);						// 0x41bd60
ThunderButton* MakeBottomButton(int theId, ButtonListener* theListener, const std::string& theLabel);						// 0x41bdc0

// A button that is only a picture drawn over the screen's artwork when the mouse is over it or it is pressed. The picture holds
// the over and the pressed states (side by side or one above the other); theSinglePicture makes one picture do for both.
ThunderButton* MakeEmbeddedButton(int theId, ButtonListener* theListener, Image* theImage, bool theSinglePicture); // 0x407d90

// A button that is a symbol, as large as the picture plus the extra width and height.
SimpleButton* MakeSimpleButton(int theId, ButtonListener* theListener, Image* theImage, int theExtraWidth, int theExtraHeight); // 0x405270

ListWidget* MakeListWidget(int theId, ListListener* theListener); // 0x4050a0

// The edit boxes of the game: plain text in black over the dialog's own drawing (DrawEditBox draws the frame), 24 high.
EditWidget* CreateEditWidget(int theId, EditListener* theListener);	// 0x405420 (SetupEditWidget is 0x405330)
void DoNameWidthEnforce(EditWidget* theEdit);						// 0x4052f0: a name is at most 12 letters and 190 pixels wide
void DrawEditBox(Graphics* g, EditWidget* theEdit);					// 0x408120: the frame of IMAGE_DLG_TEXTBOX around it

// Puts a dialog in its place and gives it its size: at least theWidth wide (and as wide as its header needs), as tall as it asks
// for. With another dialog up it goes at that one's middle; with none, in the middle of the screen, or to the right of the
// main menu's mascot. theY is the top edge, or -1 for the usual place; theCentered keeps it off the menu's side.
void PositionDialog(Dialog* theDialog, int theWidth, bool theCentered, int theY); // 0x40bc20

} // namespace Sexy

#endif
