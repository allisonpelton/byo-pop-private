// Cursor: the game's own pointer (0x426e70..0x4270e0 in the original), a 32-cel spinning picture that follows the
// mouse above everything else. The system cursor stays hidden while it shows.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class Cursor : public ObjBase
{
public:
	static Cursor* sCursor;  // 0x60e4d8

	Cursor();
	~Cursor();
	bool Update() override;

	static void Create();    // 0x427040
	static void Show();      // 0x4270b0
	static void Hide();      // 0x4270d0
};

} // namespace Sexy
