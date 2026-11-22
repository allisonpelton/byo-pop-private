// MenuObj: the base of the game's windows (0x446ac0..0x447060 in the original). It builds a panel (a MenuAnim, the
// object's own picture) and a column of text lines on it, each an ObjString of a placeholder text that is set later.
#pragma once

#include "BBFont.h"
#include "ObjBase.h"

namespace Sexy
{

class Anim;
class MenuAnim;
class ObjString;

const int MENU_MAX_LINES = 40;

class MenuObj : public ObjBase
{
public:
	float mLineTop;          // +0x280: the first line's place
	float mLineX;            // +0x284
	bool mHasLines;          // +0x288
	int mLineCount;          // +0x28c
	MenuAnim* mPanel;        // +0x290
	ObjString* mLines[MENU_MAX_LINES]; // +0x294

	MenuObj();               // 0x446ac0
	~MenuObj();              // 0x446af0

	void BuildPanel(int theColumns, int theRows, bool openRight);       // 0x446b90
	void BuildLines(BBFont* theFont, bool leftAligned);                   // 0x446c30
	void SetLineColor(int theLine, float theRed, float theGreen, float theBlue, float theAlpha); // 0x446e40
	void MoveLines(int theFirst, float theDeltaY, bool throughLast);    // 0x446eb0
	void SetLineScale(int theLine, float theScaleX, float theScaleY);   // 0x446f10
	void SetLineText(int theLine, const char* theText);                 // 0x446f50
	float GetPanelHalfWidth();                                          // 0x446f90
	float GetPanelHalfHeight();                                         // 0x446fb0
	static float RandomWhole(float theMin, float theMax);               // 0x446fd0
};

} // namespace Sexy
