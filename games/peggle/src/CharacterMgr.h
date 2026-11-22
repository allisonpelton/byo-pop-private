// CharacterMgr.h: the ten masters of the game (Bjorn, Jimmy Lightning, ...) with their pictures, eyes, powers and tips, read
// from characters/characters.cfg, and the one shown at the moment. The original's Sexy::CharacterMgr. The animation (the
// eyes following the ball, the turn of the head) is in CharacterMgrDraw.cpp.

#ifndef __CHARACTERMGR_H__
#define __CHARACTERMGR_H__

#include "Common.h"
#include "ImageMgr.h"
#include "Rect.h"

#include <string>
#include <vector>

namespace Sexy
{

class Board;

class ConfigParser;
class DDImage;
class Image;
class MemoryImage;
class StringParser;

// The powers a master can have, in the order of characters.cfg's names (the table at 0x6496a8)
enum PowerupType
{
	POWERUP_NONE = 0,
	POWERUP_GUIDE,
	POWERUP_FLIPPERS,
	POWERUP_MULTIBALL,
	POWERUP_PYRAMID,
	POWERUP_SPACE_BLAST,
	POWERUP_SPOOKY_BALL,
	POWERUP_ZEN_BALL,
	POWERUP_SHOT_EXTENDER,
	POWERUP_FLOWER_POWER,
	POWERUP_NUDGE,
	POWERUP_TIME_BOMB,
	POWERUP_FIREBALL,
	POWERUP_LUCKY_SPIN,
	NUM_POWERUPS
};

// One picture of a master (the head turned a step further). The loader cuts the master's picture into these and packs
// them, with their pixels trimmed to what is drawn, into one new picture (0x4657c0).
struct CharacterFrame
{
	Image* mSourceImage = nullptr; // +0: the master's picture as it is in the file
	Image* mImage = nullptr;	   // +4: the packed picture (mImageRect is this frame's place in it)
	Image* mMaskImage = nullptr;   // +8: the picture with the mask's transparency (mMaskRect is the place in it)
	Rect mSourceRect;			   // +0xc: this frame's place in mSourceImage
	Rect mImageRect;			   // +0x1c
	Rect mMaskRect;				   // +0x2c
	Rect mTrimmedRect;			   // +0x3c: the part that has pixels, in mSourceImage
	int mTrimmedOffsetX = 0;	   // +0x4c: where the part is in the frame
	int mTrimmedOffsetY = 0;	   // +0x50
	int mMaskOffsetX = 0;		   // +0x54
	int mMaskOffsetY = 0;		   // +0x58
	float mEyes[4] = {};		   // +0x5c: the two eyes' places in the frame (x1, y1, x2, y2), -1 for no second eye
	int mCharOffsetX = 0;		   // +0x6c: "CharOff"
	int mCharOffsetY = 0;		   // +0x70
};

struct Character
{
	std::vector<CharacterFrame> mFrames; // +0
	std::vector<std::string> mTips;		 // +0x10
	int mCurFrame = 0;					 // +0x20
	Image* mEyeImage = nullptr;			 // +0x24
	Image* mEyeMaskImage = nullptr;		 // +0x28
	std::string mName = "Name";			 // +0x2c
	std::string mDesc;					 // +0x48
	float mPosX = 0;					 // +0x64
	float mPosY = 0;					 // +0x68
	int mPowerup = POWERUP_GUIDE;		 // +0x6c
	int mTweenSpeed = 6;				 // +0x70
	int mTurnLine = 50;					 // +0x74
	bool mFramesForward = true;			 // +0x78
};

class CharacterMgr
{
public:
	CharacterMgr();	 // 0x473c30
	~CharacterMgr();

	void Load();							 // 0x474cb0: characters/characters.cfg
	void InitDefault();						 // 0x4747b0: one master without a file, for the level editor and tests
	void Clear(bool theClearCharacters);	 // 0x473d90
	int GetCharacterIndex(const std::string& theName); // 0x43e3b0: -1 for none
	int GetNumCharacters() const { return (int)mCharacters.size(); }
	int SetCurCharacter(int theCharacter);	 // 0x43e320
	void CalcLookPos(Board* theBoard, bool theForce); // 0x4410c0: where the master looks (the mouse, or the ball): the eyes' animation, with the drawing
	void Resync();							 // 0x43e460: the eyes are placed again (CalcEyePos 0x438a10)

	ImageMgr mImageMgr;						 // +4 (a pointer in the original)
	MemoryImage* mBlankImage;				 // +8: a small black picture, the eyes' picture until the file gives one
	std::vector<Character> mCharacters;		 // +0x10
	int mCurCharacter;						 // +0x20
	int mEyeLinesLeft;						 // +0x24: while a master's "Eyes" lines are read, how many frames are still to come
	int mNumFrames;							 // +0x28: the number of frames of the master being read
	int mSlowMoFrames;						 // +0x2c
	int mUnknown30;							 // +0x30
	int mUnknown34;							 // +0x34
	int mUnknown38;							 // +0x38
	int mUnknown3C;							 // +0x3c
	int mUnknown40;							 // +0x40
	bool mUnknown45;						 // +0x45
	float mUnknown48;						 // +0x48
	int mPosOffsetX;						 // +0x78: added to every master's "Pos"
	int mPosOffsetY;						 // +0x7c

private:
	void ReadConfig(const std::string& theFileName);			 // 0x474870
	void ReadCharacterConfig(ConfigParser& theParser, Character& theCharacter); // 0x4657c0
	int ReadPowerupType(ConfigParser& theParser, const std::string& theName);	 // 0x446490
	CharacterFrame* GetEyesFrame(Character& theCharacter);		 // 0x442bd0
};

} // namespace Sexy

#endif
