// Sexy::UnderDialogWidget (0x90 bytes, vtable 0x7f28ac; 0x75676f-0x756e90): the whole-screen layer the app puts
// behind its dialogs. It darkens what is under them by the app's mDialogDim, which UpdateFrames fades in while a
// dialog (or the options) is up and out after, and over a paused board it writes the pause text. With Direct3D it
// draws a blurred, rippling copy of the screen instead (two quarter-size textures, +0x88 and +0x8c); the game takes
// its 2D paths, so they are never made.
#pragma once

#include "Widget.h"

namespace Sexy {

class DDImage;

class UnderDialogWidget : public Widget {
public:
	UnderDialogWidget();  // 0x75676f
	void Draw(Graphics* g) override;  // 0x756a92
	void DrawPaused(Graphics* g);     // 0x756925
	DDImage* mQuarterA = nullptr;     // +0x88: the screen in gray at a quarter of its size (a render target)
	DDImage* mQuarterB = nullptr;     // +0x8c: the second picture the ripples pass between (a render target)
	void MakeQuarterImages();         // 0x7567e7
	void DrawBlurred(Graphics* g);    // 0x756ab0
};

}  // namespace Sexy
