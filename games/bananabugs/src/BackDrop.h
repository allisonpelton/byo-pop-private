// BackDrop: the picture behind a level (0x403f70..0x4048b0 in the original), the level's parent. Each map level has a
// background of its own and a strip of ground along the bottom that sways and stretches a little; the whole can be
// darkened, as it is behind the level's messages, and brought back.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

const int BACKDROP_PATH_SIZE = 0x2c;

class BackDrop : public ObjBase
{
public:
	ObjBase* mGround;        // +0x27c
	char mPath[BACKDROP_PATH_SIZE]; // +0x280: the background picture's file name
	int mBackground;         // +0x2ac: the background's number, which the ground's shares
	int mDarkenTrack;        // +0x2b0
	int mUndarkenTrack;      // +0x2b4
	bool mDarkened;          // +0x2b8

	static BackDrop* sCurrent; // 0x60e19c

	BackDrop();              // 0x403f70
	~BackDrop();             // 0x404120

	void Darken();           // 0x4041b0
	void Undarken();         // 0x4041d0
	void BuildGround();      // 0x4045b0

	static BackDrop* Create(ObjBase* theParent); // 0x4047e0

private:
	void BuildTracks();      // 0x4041f0
};

} // namespace Sexy
