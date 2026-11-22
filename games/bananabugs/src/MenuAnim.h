// MenuAnim: the picture of a menu panel (0x446490..0x446850 in the original). A grid of 100 by 100 pixel tiles cut
// from "images/menu/menutiles1" (a 3 by 3 sheet of corners, edges and middle) is drawn into one picture, scaled to the
// window like any Anim, and rebuilt the same way when the window changes size.
#pragma once

#include "Anim.h"

namespace Sexy
{

class MenuAnim : public Anim
{
public:
	int mWidth;              // +0x18bc: the panel in design pixels, 100 a column
	int mHeight;             // +0x18c0: 100 a row
	int mColumns;            // +0x18c4
	int mRows;               // +0x18c8
	bool mOpenRight;         // +0x18cc: the right column is drawn as a middle one, with no right edge

	static Anim* sTilesAnim; // 0x60e694: the tile sheet, kept for every panel

	MenuAnim(int theRows, int theColumns, bool openRight);  // 0x446490
	void Reload() override;                                  // 0x446530
	int GetTile(int theColumn, int theRow);                  // 0x446540
	void Build();                                            // 0x4465d0
};

void PreloadMenuTiles(); // 0x446830

} // namespace Sexy
