#ifndef __SPROUT_LISTBOX_H__
#define __SPROUT_LISTBOX_H__

// The signal build's list of rows (Pizza Frenzy's; Iggle Pop's Mac build names it engine::ListBox: a component with
// slots; 0x1b0 bytes, constructor 0x491ea0 (pizzafrenzy), vtable 0x504738 (pizzafrenzy)): rows of text in a size, in
// colours, with an indent, that the mouse picks from (the sounds are for the mouse entering a row and pressing one).
// Its drawing (0x6c: 0x4919d0 (pizzafrenzy)), layout (0x74: 0x491720 (pizzafrenzy)) and mouse events (0xa0:
// 0x491c00 (pizzafrenzy), 0xd0: 0x491930 (pizzafrenzy), 0xe4: 0x491750 (pizzafrenzy)) are here, with its model and its
// signal for the row picked.

#include "Color4.h"
#include "ListModel.h"
#include "Node.h"
#include "Signal.h"

namespace engine
{

class Sound;

class ListBox : public Node, public HasSlots
{
public:
	ListBox();  // 0x491ea0 (pizzafrenzy)

	// 0x491590 (pizzafrenzy)
	void setSize(float theWidth, float theHeight) { mSize = Point(theWidth, theHeight); }
	// Each colour with its getter (the layout reader reads its attribute with the colour it has as the default)
	void setBackgroundColor(const Color4& theColor) { mBackgroundColor = theColor; }   // 0x4915e0 (pizzafrenzy)
	const Color4& getBackgroundColor() const { return mBackgroundColor; }              // 0x491610 (pizzafrenzy)
	void setTextColor(const Color4& theColor) { mTextColor = theColor; }               // 0x4915a0 (pizzafrenzy)
	const Color4& getTextColor() const { return mTextColor; }                          // 0x4915d0 (pizzafrenzy)
	void setHighlightColor(const Color4& theColor) { mHighlightColor = theColor; }     // 0x491620 (pizzafrenzy)
	const Color4& getHighlightColor() const { return mHighlightColor; }                // 0x491650 (pizzafrenzy)
	// 0x491660 (pizzafrenzy)
	void setSelectionBackgroundColor(const Color4& theColor) { mSelectionBackgroundColor = theColor; }
	// 0x491690 (pizzafrenzy)
	const Color4& getSelectionBackgroundColor() const { return mSelectionBackgroundColor; }
	// 0x4916a0 (pizzafrenzy)
	void setSelectionTextColor(const Color4& theColor) { mSelectionTextColor = theColor; }
	// 0x4916d0 (pizzafrenzy)
	void setSelectionHighlightColor(const Color4& theColor) { mSelectionHighlightColor = theColor; }
	// 0x491700 (pizzafrenzy)
	void setIndent(int theIndent) { mIndent = theIndent; }
	// 0x491760 (pizzafrenzy): the sounds for the mouse entering a row and pressing one, loaded by their names (an empty
	// name is none)
	void setSounds(const std::string& theEnterSound, const std::string& thePressSound);

	// 0x492070 (pizzafrenzy): the list shows the model, and needs a layout when the model changes
	void setModel(ListModel* theModel);
	int getRowCount() { return mModel->rowCount(); }  // 0x491820 (pizzafrenzy)
	void modelChanged();                              // 0x491580 (pizzafrenzy)
	// 0x491830 (pizzafrenzy): how many rows the height holds
	int getVisibleRows();
	int getSelectedIndex() const { return mSelectedIndex; }  // 0x491710 (pizzafrenzy)
	void setSelectedIndex(int theIndex) { mSelectedIndex = theIndex; }
	// 0x491880 (pizzafrenzy): the row at the point (in the list's parent's coordinates), -1 when there is none
	int rowAt(const Point& thePoint);

	// 0x6c: 0x4919d0 (pizzafrenzy)
	void draw(Graphics* g) override;
	// 0x74: 0x491720 (pizzafrenzy)
	void layout() override;
	// 0xa0: 0x491c00 (pizzafrenzy): the row pressed is picked, which the signal tells
	void mouseDownEvent(MouseEvent& e) override;
	// 0xd0: 0x491930 (pizzafrenzy): the row the mouse is over; its sound plays when it is a new one
	void mouseMoveEvent(MouseEvent& e) override;
	// 0xe4: 0x491750 (pizzafrenzy): no row is under the mouse
	void mouseExitEvent(MouseEvent& e) override;

	Signal<int> mSelectedSignal;       // +0x118: the row picked

	Point mSize;                     // +0x128
	Color4 mBackgroundColor;           // +0x130
	Color4 mTextColor;                 // +0x140
	Color4 mHighlightColor;            // +0x150
	Color4 mSelectionBackgroundColor;  // +0x160
	Color4 mSelectionTextColor;        // +0x170
	Color4 mSelectionHighlightColor;   // +0x180
	int mIndent;                       // +0x190
	Ref<ListModel> mModel;             // +0x194: null until the game sets one
	int mHoverIndex;                   // +0x198: the row under the mouse
	int mSelectedIndex;                // +0x19c: the row picked
	Sound* mEnterSound;                // +0x1a0
	Sound* mPressSound;                // +0x1a4
};

} // namespace engine

#endif
