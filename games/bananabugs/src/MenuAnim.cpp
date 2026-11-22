#include "MenuAnim.h"

#include "DDImage.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

using namespace Sexy;

Anim* MenuAnim::sTilesAnim = nullptr;

// A tile of the sheet, and where the sheet's tiles are
const int MENU_TILE_SIZE = 100;
const int MENU_TILES_ACROSS = 3;
enum MenuTile
{
	TILE_TOP_LEFT = 0,
	TILE_TOP = 1,
	TILE_TOP_RIGHT = 2,
	TILE_LEFT = 3,
	TILE_MIDDLE = 4,
	TILE_RIGHT = 5,
	TILE_BOTTOM_LEFT = 6,
	TILE_BOTTOM = 7,
	TILE_BOTTOM_RIGHT = 8,
};

// 0x446490 (the original takes the rows in ECX, the columns in EDX): the panel is built at once and joins the registry
MenuAnim::MenuAnim(int theRows, int theColumns, bool openRight)
{
	mImage = nullptr;
	mColumns = theColumns;
	mRows = theRows;
	mWidth = theColumns * MENU_TILE_SIZE;
	mHeight = theRows * MENU_TILE_SIZE;
	mOpenRight = openRight;
	Build();
	sAnims.push_back(this);
}

// 0x446530: the panel again, at the window's new scale
void MenuAnim::Reload()
{
	Build();
}

// 0x446540 (EDI the column, EBX the row): which tile of the sheet a place of the grid takes. With mOpenRight the top
// and bottom rows end in an edge and the right column is middle.
int MenuAnim::GetTile(int theColumn, int theRow)
{
	int aTile = TILE_MIDDLE;
	if (theColumn == 0)
		aTile = TILE_LEFT;
	if (theRow == 0)
		aTile = TILE_TOP;
	if (!mOpenRight && theColumn == mColumns - 1)
		aTile = TILE_RIGHT;
	int aLastRow = mRows - 1;
	if (theRow == aLastRow)
		aTile = TILE_BOTTOM;
	if (theColumn == 0 && theRow == 0)
		aTile = TILE_TOP_LEFT;
	int aLastColumn = mColumns - 1;
	if (theColumn == aLastColumn) {
		if (theRow == 0)
			aTile = TILE_TOP_RIGHT - (mOpenRight ? 1 : 0);
		if (theColumn == aLastColumn && theRow == aLastRow)
			aTile = TILE_BOTTOM_RIGHT - (mOpenRight ? 1 : 0);
	}
	if (theColumn == 0 && theRow == aLastRow)
		aTile = TILE_BOTTOM_LEFT;
	return aTile;
}

// 0x4465d0: draws every tile into a new image of the panel's size and scales that into this Anim (one cel, smoothly,
// by the window's scale). The scaling takes the image over.
void MenuAnim::Build()
{
	MemoryImage* aPanel = new MemoryImage(sApp);
	aPanel->Create(mWidth, mHeight);
	{
		Graphics aPanelG(aPanel);
		int aY = 0;
		for (int aRow = 0; aRow < mRows; aRow++) {
			int aX = 0;
			for (int aColumn = 0; aColumn < mColumns; aColumn++) {
				int aTile = GetTile(aColumn, aRow);
				Rect aSrcRect((aTile % MENU_TILES_ACROSS) * MENU_TILE_SIZE, (aTile / MENU_TILES_ACROSS) * MENU_TILE_SIZE,
					MENU_TILE_SIZE, MENU_TILE_SIZE);
				Rect aDestRect(aX, aY, MENU_TILE_SIZE, MENU_TILE_SIZE);
				aPanelG.DrawImage(sTilesAnim->mImage, aDestRect, aSrcRect);
				aX += MENU_TILE_SIZE;
			}
			aY += MENU_TILE_SIZE;
		}
		// the original makes a DDImage of the panel's bits, commits them and makes its DirectDraw surface (0x47ffd0);
		// the framework's DDImage makes its texture when first drawn
		DDImage* aScaled = new DDImage(sApp->mDDInterface);
		aScaled->SetBits(aPanel->GetBits(), aPanel->GetWidth(), aPanel->GetHeight(), true);
		aScaled->CommitBits();
		delete aPanel;
		Scale(aScaled, ANIM_SCALE_SMOOTH, gImageScaleX, gImageScaleY, 0, 0, 1);
	}
}

// 0x446830
void Sexy::PreloadMenuTiles()
{
	MenuAnim::sTilesAnim = Anim::Get("images/menu/menutiles1", ANIM_SCALE_SMOOTH, true, 0, 0, 1, false);
}
