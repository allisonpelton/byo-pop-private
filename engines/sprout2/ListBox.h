#ifndef __SPROUT2_LISTBOX_H__
#define __SPROUT2_LISTBOX_H__

// A list of lines to pick one from (engine::ListBox, 0x148 bytes, vtable 0x5605cc (feedingfrenzy2) with 60 slots; its
// constructor 0x404260 (feedingfrenzy2)): a model's rows drawn one font line each over a filled rectangle, from the top
// row down as many as fit. The row under the mouse is highlighted, and a click selects a row, emitting its index.

#include "Component.h"
#include "ListModel.h"
#include "sigslot.h"

#include <string>

namespace engine
{

class ListBox : public Component, public sigslot::HasSlots
{
public:
	// 0x404260 (feedingfrenzy2): no model, nothing highlighted or selected, black behind white text, visible and
	// taking the mouse
	ListBox();

	// (0x68 is Component's: the class name is "Component")
	// 0xa0 (0x42bce0 (feedingfrenzy2)): the background, then each row from the top one, the selected row on a filled
	// line of its own, in the state's font
	void paint(DrawContext* theContext) override;
	// 0xa8 (0x4917d0 (feedingfrenzy2)): the size from the position
	void computeBounds() override;
	// 0xb4 (0x495cc0 (feedingfrenzy2)): a row other than the selected one selected and its index emitted
	void mouseDown(MouseEvent& theEvent) override;
	// 0xc4 (0x485f20 (feedingfrenzy2)): the row under the mouse highlighted
	void mouseMove(MouseEvent& theEvent) override;
	// 0xd0 (0x49c4a0 (feedingfrenzy2)): nothing highlighted
	void mouseLeave(MouseEvent& theEvent) override;

	// 0x494d20 (feedingfrenzy2): the model kept, its changes followed, and the bounds recomputed
	void setModel(ListModel* theModel);
	// 0x4858b0 (feedingfrenzy2): the model's rows, 0 without one
	int getCount();
	// 0x485430 (feedingfrenzy2): the rows the height holds in the box's font (truncated)
	int getVisibleRows();
	int getSelected() const { return mSelected; }
	void setSelected(int theRow) { mSelected = theRow; }

	// Set by the screen's reader (inline there)
	void setSize(float theWidth, float theHeight)
	{
		mWidth = theWidth;
		mHeight = theHeight;
	}
	void setIndent(int theIndent) { mIndent = theIndent; }
	void setAlignH(int theAlignH) { mAlignH = theAlignH; }
	void setColors(const Color& theBackground, const Color& theText, const Color& theHighlight, const Color& theSelection,
	               const Color& theSelectionText, const Color& theSelectionHighlight)
	{
		mBackgroundColor = theBackground;
		mTextColor = theText;
		mHighlightColor = theHighlight;
		mSelectionColor = theSelection;
		mSelectionTextColor = theSelectionText;
		mSelectionHighlightColor = theSelectionHighlight;
	}
	const Color& getBackgroundColor() const { return mBackgroundColor; }
	const Color& getTextColor() const { return mTextColor; }
	const Color& getHighlightColor() const { return mHighlightColor; }
	const Color& getSelectionColor() const { return mSelectionColor; }

	sigslot::signal1<int> mSelectSignal;  // +0xa8: the row a click selected (nothing the export shows connects to it)

private:
	// 0x485820 (feedingfrenzy2): the row at a point of the event, -1 for none
	int rowAt(const Vector2f& thePoint);
	// 0x494d80 (feedingfrenzy2): the model's rows changed
	void modelChanged() { addState(STATE_BOUNDS_DIRTY); }

	float mWidth;                    // +0xb8
	float mHeight;                   // +0xbc
	Color mBackgroundColor;          // +0xc0
	Color mTextColor;                // +0xd0
	Color mHighlightColor;           // +0xe0: the text of the row under the mouse
	Color mSelectionColor;           // +0xf0: the selected row's line
	Color mSelectionTextColor;       // +0x100: the selected row's text
	Color mSelectionHighlightColor;  // +0x110: the selected row's text under the mouse
	int mIndent;                     // +0x120: the text's distance from the side it is aligned to
	Ref<ListModel> mModel;           // +0x124
	int mHighlighted;                // +0x128: -1 for none
	int mSelected;                   // +0x12c: -1 for none
	int mTopRow;                     // +0x130: the first row drawn (nothing the export shows moves it)
	// +0x134 and +0x138: the sound instances of the mouse coming over a row and of a click (0x496190 (feedingfrenzy2)
	// makes them from the reader's overSound and activeSound): not yet translated, as the sounds are not
	int mAlignH;                     // +0x13c: TextAlignH
};

} // namespace engine

#endif
