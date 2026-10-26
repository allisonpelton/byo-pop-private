// Sexy::Tile (0x88 bytes, constructor 0x4988f0, vtable 0x7b1048): a letter tile of the battle's grid. The C++ part
// keeps where the tile is (pixels and grid cell), whether it is selected, hilited, falling or sliding, and its
// alpha; the letter and what the tile looks like are in its Lua object (scripts/tiles), which the C++ part asks
// (GetLetter, IsBonus, CanSelect) and tells (Hilited, Selected, SetAlpha, Update, and TileXChanged and the
// other tile events through the tile engine). The tile engine makes them (CreateTile, CloneTile) and the board
// keeps them in its lists.
#pragma once

#include <string>

#include "CurvedValue.h"
#include "LuaLinkedClass.h"

namespace Sexy {

class Tile : public LuaLib::LuaLinkedClass {
public:
	int mX = 0;                           // +0x28: pixel position
	int mY = 0;                           // +0x2c
	int mCol = 0;                         // +0x30: grid cell
	int mRow = 0;                         // +0x34
	int mAlphaState = 1;                  // +0x38: 0 fading in (then 1), 1 as it is, 2 fading out (then 3)
	int mFallDistance = 0;                // +0x3c: how far it still has to fall, in pixels
	int mUpdateCnt = 0;                   // +0x40
	int mFallDelay = 0;                   // +0x44: updates to wait before it falls
	bool mSelected = false;               // +0x48
	bool mHilited = false;                // +0x49
	bool mFalling = true;                 // +0x4a
	CurvedValue* mAlphaCurve = nullptr;   // +0x4c: the alpha (mode 0, curve 3, 255 to 255, over t 0 to 1)
	bool mMoving = false;                 // +0x50: sliding (TileEngine::SlideTile)
	CurvedValue* mSlideCurve = nullptr;   // +0x54: how far along the slide (0 to 1, mode 0, curve 3)
	double mSlideFromX = 0.0;             // +0x58
	double mSlideFromY = 0.0;             // +0x60
	double mSlideToX = 0.0;               // +0x68
	double mSlideToY = 0.0;               // +0x70
	float mSlideSpeed = 0.0f;             // +0x78: mSlideCurve->mT grows by this each update; 0 when not sliding
	float mScale = 1.0f;                  // +0x7c
	int m80 = 0;                          // +0x80: set to 0 by the constructor; no use found

	Tile();   // 0x4988f0
	~Tile() override;  // 0x498c10: the Lua object's Destructor first

	const char* LuaGetRegisteredName() override { return "Tile"; }
	// The original registered the class in the constructor (0x4988f0), every time
	static void RegisterLua();

	// Pushes a tile as a Tile userdata (0x427470; nil for NULL)
	static void PushTile(Tile* theTile);

	// vf09 (0x498e30): LuaLinkedClass::CreateLinkedObject with one more argument for the script's CreateLinkedObject
	// (which ignores it)
	using LuaLib::LuaLinkedClass::CreateLinkedObject;
	void CreateLinkedObject(void* theObject, bool theUnused);
	// 0x498cb0: takes over another tile's state (position, cell, fall, slide, alpha), not its scale
	void CopyState(const Tile* theTile);

	// The scripts' methods
	void Update();                       // 0x499650
	char GetLetter();                    // 0x499020: the first character of the Lua object's GetLetter
	bool IsVowel();                      // 0x499160
	bool IsHilited() { return mHilited; }   // 0x416410
	bool IsSelected() { return mSelected; } // 0x416420
	bool IsBonus();                      // 0x499210: the Lua object's IsBonus
	void Hilite(bool theHilite);         // 0x499240
	void Select(bool theSelect);         // 0x499290
	bool CanSelect(int theX, int theY);  // 0x499380: not while it slides; the tile engine's CanSelect
	bool IsMoving() { return mMoving; }  // 0x416450
	bool IsFalling() { return mFalling; }  // 0x416460
	int GetX() { return mX; }            // 0x416470
	int GetY() { return mY; }            // 0x416480
	void SetXY(int theX, int theY, bool theNotify);  // 0x4164b0: the tile engine hears of it when theNotify
	void LuaSetScale(float theScale) { mScale = theScale; }  // 0x498840
	float GetTargetX() { return (float)mSlideToX; }  // 0x498850
	float GetTargetY() { return (float)mSlideToY; }  // 0x498860
	int GetCol() { return mCol; }        // 0x416490
	int GetRow() { return mRow; }        // 0x4164a0
	void StopFalling() { mFalling = false; }  // 0x498870
	int GetFallTarget();                 // 0x499750: where its y ends up
	void MaxAlpha();                     // 0x4990c0: the alpha curve to its end

	// Not bound
	bool Contains(int theX, int theY);   // 0x4992e0: the point is in the tile (scaled about its centre); never while it slides
	bool SlideStep();                    // 0x4995a0: one step of a slide; false when it is not sliding or has arrived
	void Fall(int theDistance, int theDelay);  // 0x416430
	bool IsInRow(int theRow) const { return theRow == mRow; }  // 0x416440
	void TileXChanged(int theX);         // 0x4993e0: the tile engine's TileXChanged(tile, x)
	void TileYChanged(int theY);         // 0x499440
	// 0x498d60: the tile engine's script gives it another letter (ChangeLetter(tile, letter))
	void SetLetter(const std::string& theLetter);
};

}  // namespace Sexy
