#ifndef __MAHJONG_MAPLAYER_H__
#define __MAHJONG_MAPLAYER_H__

// The escape's map (0x40f610..0x410e80 (mahjongchina), 0x40f670..0x4113b0 (mahjongjapan)): the stages' pictures shown
// as they are reached, the path of the levels with the marker of the chosen one (0x40f610 (mahjongchina),
// 0x40f670 (mahjongjapan)), the curtain that slides over the play layer (0x410c50 (mahjongchina),
// 0x411140 (mahjongjapan)), the question whether to start the escape again once it is won (Japan,
// 0x410170 (mahjongjapan)), and the layer (0x4100e0 (mahjongchina), 0x410470 (mahjongjapan)) that starts a level with
// its title, stage, background and seed from TXT resource 0x17f.

#include "Dialog.h"
#include "Edition.h"
#include "Layer.h"
#include "Object.h"

#include <cstdint>
#include <string>

namespace jb
{
class Font;
}

namespace mahjong
{

class PlayLayer;
class MapLayer;

// 0x410a90 (mahjongchina), 0x410ef0 (mahjongjapan): a level's stage, its place in the stage and the stage's levels
void GetStage(int theLevel, int* theStage, int* theIndex, int* theCount);
// 0x410b90 (mahjongchina), 0x411060 (mahjongjapan): the level is the last of its stage
bool IsStageEnd(int theLevel);
// 0x410bc0 (mahjongchina), 0x4110b0 (mahjongjapan): the stage of the escape's progress
int GetProgressStage();

// 0x40f610 (mahjongchina), 0x40f670 (mahjongjapan) (vtable 0x51c1cc (mahjongchina), 0x51d28c (mahjongjapan), 0xd8
// bytes): the map
class MapView : public jb::Object
{
public:
	std::string mStartText;      // +0x90 (STRING_FIRST_YEAR)
	std::string mEndText;        // +0x98 (STRING_LAST_YEAR)
	std::string mStageText;      // +0xa0 (STRING_STAGE_COMPLETED) the stage reached
	jb::Font* mFont = nullptr;   // +0xa8
	jb::Font* mIntroFont = nullptr;  // +0xac
	jb::Font* mStageFont = nullptr;  // +0xb0
	std::string mIntroText;      // +0xb4 (STRING_MAP_INTRO)
	int mLevel = 0;              // +0xbc the chosen level
	uint32_t mTick = 0;          // +0xc0 the marker's blink, by updates
	bool mBack = false;          // +0xc4 the last level won is drawn as not yet won (before the move)
	bool mNewStage = false;      // +0xc5 the move reaches a new stage
	bool mMoving = false;        // +0xc6 the marker moves to the next level
	int mMoveAlpha = 0;          // +0xc8 the move's progress, 0..0xff
	bool mFadeIn = false;        // +0xcc the text fades in
	int mTextAlpha = 0;          // +0xd0
	int mTextHold = 0;           // +0xd4 the stage's text stays this long

	// 0x40f610 (mahjongchina), 0x40f670 (mahjongjapan)
	MapView() {}
	// 0x40f6c0 (mahjongchina), 0x40f720 (mahjongjapan), 0x40f6e0 (mahjongchina), 0x40f740 (mahjongjapan)
	~MapView() override {}

	void Draw() override;                                           // 0x40f770 (mahjongchina), 0x40f7d0 (mahjongjapan)
	void Init() override;                                           // 0x40fc00 (mahjongchina), 0x40fc70 (mahjongjapan)
	bool OnLButtonDown(int theFlags, int theX, int theY) override;  // 0x40fed0 (mahjongchina), 0x40ff40 (mahjongjapan)
	// 0x40ff20 (mahjongchina), 0x40ffa0 (mahjongjapan)
	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;
	void OnMouseMove(int theFlags, int theX, int theY) override;    // 0x410010 (mahjongchina), 0x4100a0 (mahjongjapan)
	bool Update() override;                                         // 0x40fcf0 (mahjongchina), 0x40fd60 (mahjongjapan)
	bool OnTimer(int theId, int theIndex) override;                 // 0x40fe50 (mahjongchina), 0x40fec0 (mahjongjapan)

	// 0x40ff70 (mahjongchina), 0x410000 (mahjongjapan): the level or stage picture under the point, or -1
	static int HitTest(int theX, int theY);
	// 0x410050 (mahjongchina), 0x4100e0 (mahjongjapan)
	void StepBack();
	// 0x410070 (mahjongchina), 0x410100 (mahjongjapan)
	void StartMove(bool theNewStage, const char* theStageName);
};

// 0x410c50 (mahjongchina), 0x411140 (mahjongjapan) (vtable 0x51c380 (mahjongchina), 0x51d490 (mahjongjapan), 0xc4
// bytes): the curtain between the map and the play layer under it
class MapCurtain : public jb::Object
{
public:
	bool mClosing = false;       // +0x90 it moves right, over the play layer
	int mX = 0xffff;             // +0x94 0xffff when still
	jb::RegionRect mClip;        // +0x98 the map's
	jb::RegionRect mPlayClip;    // +0xac the play layer's
	PlayLayer* mPlay;            // +0xc0

	// 0x410c50 (mahjongchina), 0x411140 (mahjongjapan)
	explicit MapCurtain(PlayLayer* thePlay) : mPlay(thePlay) {}
	// 0x410c90 (mahjongchina), 0x4111b0 (mahjongjapan), 0x410cb0 (mahjongchina), 0x4111d0 (mahjongjapan)
	~MapCurtain() override {}

	void Draw() override;    // 0x410cc0 (mahjongchina), 0x4111e0 (mahjongjapan)
	void Init() override;    // 0x410cf0 (mahjongchina), 0x411210 (mahjongjapan)
	bool Update() override;  // 0x410d10 (mahjongchina), 0x411230 (mahjongjapan)

	// 0x410e00 (mahjongchina), 0x411320 (mahjongjapan): the map uncovers the play layer
	void Open();
	// 0x410e30 (mahjongchina), 0x411350 (mahjongjapan): the map covers the play layer
	void Close();
};

#if EDITION_RESTART_BOX
// 0x410170 (mahjongjapan) (vtable 0x51d2e4 (mahjongjapan), 0x58 bytes): the escape is won; start again?
class RestartBox : public Dialog
{
public:
	MapLayer* mMap;  // +0x54

	// 0x410170 (mahjongjapan)
	explicit RestartBox(MapLayer* theMap) : Dialog(0x94, 0x87), mMap(theMap) {}
	// 0x4101a0 (mahjongjapan), 0x4101c0 (mahjongjapan)
	~RestartBox() override {}

	void Create() override;               // 0x4101d0 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x410330 (mahjongjapan)
};
#endif

// 0x4100e0 (mahjongchina), 0x410470 (mahjongjapan) (vtable 0x51c358 (mahjongchina), 0x51d468 (mahjongjapan), 0x430
// bytes)
class MapLayer : public jb::Layer, public DialogListener
{
public:
	// 0x10 bytes: a line of TXT 0x17f
	struct Level
	{
		int mQuadPercent = 0;  // +0x00
		uint32_t mSeed = 0;    // +0x04
		std::string mTitle;    // +0x08
	};

	bool mShowing = false;     // +0x54 the curtain closes over the play layer
	bool mProgressed = false;  // +0x55 a level was won
	bool mFromPlay = false;    // +0x56
	PlayLayer* mPlay;          // +0x58
	MapView* mView = nullptr;  // +0x5c
	MapCurtain* mCurtain = nullptr;  // +0x60
	jb::Button* mMenu = nullptr;     // +0x64 0xe4
	jb::Button* mContinue = nullptr; // +0x68 0xe6
	jb::Button* mEnd = nullptr;      // +0x6c 0x16c the ending again
	Level mLevels[edition::NUM_LEVELS];         // +0x70

	// 0x4100e0 (mahjongchina), 0x410470 (mahjongjapan)
	explicit MapLayer(PlayLayer* thePlay) : Layer(0, 0), mPlay(thePlay) {}
	// 0x410170 (mahjongchina), 0x410500 (mahjongjapan), 0x4101a0 (mahjongchina), 0x410530 (mahjongjapan)
	~MapLayer() override;

	void Load() override;                 // 0x410280 (mahjongchina), 0x410610 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x4104a0 (mahjongchina), 0x410840 (mahjongjapan)
	void OnShown() override;              // 0x4105e0 (mahjongchina), 0x410970 (mahjongjapan)
	bool OnYes(int theId) override;       // 0x4107c0 (mahjongchina), 0x410bc0 (mahjongjapan)
	bool OnNo(int theId) override;        // 0x4107d0 (mahjongchina), 0x410c10 (mahjongjapan)

	// 0x410220 (mahjongchina), 0x4105b0 (mahjongjapan): shown over the play layer after a level, won or not
	void SetProgressed(bool theProgressed);
	// 0x410600 (mahjongchina), 0x410a00 (mahjongjapan): the curtain stopped
	void CurtainDone();
	// 0x4106a0 (mahjongchina), 0x410aa0 (mahjongjapan): the chosen level, or the question whether to resume its saved
	// game
	void Continue();
	// 0x4107e0 (mahjongchina), 0x410c30 (mahjongjapan)
	void StartLevel(bool theResume);
	// 0x410900 (mahjongchina), 0x410d60 (mahjongjapan)
	bool LoadLevels();
	// 0x410b10 (mahjongchina), 0x410fe0 (mahjongjapan): the marker's move after a level won
	void StartProgress();
	// 0x410bf0 (mahjongchina), 0x4110e0 (mahjongjapan)
	void EnableAll(bool theEnabled);
	// 0x410c30 (mahjongchina), 0x411120 (mahjongjapan)
	void ShowEnd();
};

} // namespace mahjong

#endif
