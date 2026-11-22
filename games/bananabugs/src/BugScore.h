// BugScore: the score a caught bug shows where it was (0x426990..0x426e70 in the original), a line of text that pops
// up wobbling, circles a little while it fades, and goes. It draws and updates as a plain object, not as a text line:
// it never shows a text cursor.
#pragma once

#include "ObjString.h"

namespace Sexy
{

class BugScore : public ObjString
{
public:
	AnimKey* mPopKey;        // +0x2a8
	AnimKey* mCircleKey;     // +0x2ac
	AnimKey* mGoneKey;       // +0x2b0
	AnimTrack* mTrack;       // +0x2b4
	int mTrackIndex;         // +0x2b8

	BugScore();              // 0x426990
	void Notify(ObjBase* theObj) override {}        // 0x42c290, empty
	bool Update() override { return ObjBase::Update(); } // 0x4219c0
	void Draw(Graphics* g) override { ObjBase::Draw(g); } // 0x426a50

	void Show(const char* theText, float theX, float theY); // 0x426a60

private:
	void BuildTrack();       // 0x426b00
};

} // namespace Sexy
