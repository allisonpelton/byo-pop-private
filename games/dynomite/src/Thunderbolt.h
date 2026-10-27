#ifndef __THUNDERBOLT_H__
#define __THUNDERBOLT_H__

// The board's +0x154 (vtable 0x4f75f8, 0x1c bytes; 0x407e00-0x407fa0), made by its slot 51: a thunderbolt strikes
// the loaded egg, which flickers between its colour (with the bolt shown) and a bomb every 15 frames for 175
// frames, and stays a bomb.

namespace Sexy
{

class BinFile;
class Dynomite;
class FossilizedApp;

class Thunderbolt
{
public:
	Dynomite*			mBoard;				// +4
	FossilizedApp*		mApp;				// +8
	int					mTime;				// +0xc frames left (175)
	bool				mBomb;				// +0x10 the egg shows as a bomb
	int					mFlip;				// +0x14 frames to the next flicker
	int					mColor;				// +0x18 the egg's own colour

	Thunderbolt(FossilizedApp* theApp, Dynomite* theBoard);	// 0x407e00
	virtual ~Thunderbolt();										// 0x407e60

	bool				Update();							// 0x407e80: false when it is over
	void				Draw();								// 0x407f70
	void				SaveState(BinFile* theFile);		// 0x407ed0
	void				LoadState(BinFile* theFile);		// 0x407f10
};

}

#endif
