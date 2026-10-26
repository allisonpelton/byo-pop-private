// JeffLib::SexyScrollbarWidget (0xf0 bytes, constructor 0x6c8a30, vtable 0x7c5f44; its ButtonListener part at +0x88)
// and JeffLib::SexyScrollbuttonWidget (0x128 bytes, vtable 0x7c60bc, code 0x6c8eb0-0x6c9190): the scrollbar of
// JeffLib's list boxes. The framework's scrollbar with scroll buttons and thumb drawn in the tan, black and gold
// of the game's dialogs, and the track in a colour of its own. The scrollbar replaces the buttons its base makes
// with its own kind.
#pragma once

#include "Color.h"
#include "ScrollbarWidget.h"
#include "ScrollbuttonWidget.h"

namespace JeffLib {

// An arrow button: the tan block of the framework's, its arrow in gold (grey when disabled)
class SexyScrollbuttonWidget : public Sexy::ScrollbuttonWidget {
public:
	using Sexy::ScrollbuttonWidget::ScrollbuttonWidget;

	void Draw(Sexy::Graphics* g) override;  // 0x6c8f00
};

class SexyScrollbarWidget : public Sexy::ScrollbarWidget {
public:
	Sexy::Color mTrackColor;  // +0xe0: the track, above and below the thumb (black)

	// 0x6c8a30: id 1
	explicit SexyScrollbarWidget(Sexy::ScrollListener* theListener);

	void Draw(Sexy::Graphics* g) override;  // 0x6c8d10
	// 0x6c8be0: the same as the framework's
	void DrawThumb(Sexy::Graphics* g, int theX, int theY, int theWidth, int theHeight) override;
};

}  // namespace JeffLib
