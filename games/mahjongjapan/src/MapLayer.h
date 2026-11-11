#ifndef __MAHJONGJAPAN_MAPLAYER_H__
#define __MAHJONGJAPAN_MAPLAYER_H__

// The escape's map (0x40f670..0x4113b0): the twelve stages' pictures shown as they are reached, the path of the sixty
// levels with the marker of the chosen one (0x40f670), the curtain that slides over the play layer (0x411140), the
// question whether to start the escape again once it is won (0x410170), and the layer (0x410470) that starts a level
// with its title, stage, background and seed from TXT resource 0x17f.

#include "Dialog.h"
#include "Layer.h"
#include "Object.h"

#include <cstdint>
#include <string>

namespace jb
{
class Font;
}

namespace mahjongjapan
{

class PlayLayer;
class MapLayer;

// 0x410ef0: a level's stage, its place in the stage and the stage's levels
void GetStage(int theLevel, int* theStage, int* theIndex, int* theCount);
// 0x411060: the level is the last of its stage
bool IsStageEnd(int theLevel);
// 0x4110b0: the stage of the escape's progress
int GetProgressStage();

// 0x40f670 (vtable 0x51d28c, 0xd8 bytes): the map
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

	// 0x40f670
	MapView() {}
	// 0x40f720, 0x40f740
	~MapView() override {}

	void Draw() override;                                           // 0x40f7d0
	void Init() override;                                           // 0x40fc70
	bool OnLButtonDown(int theFlags, int theX, int theY) override;  // 0x40ff40
	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;  // 0x40ffa0
	void OnMouseMove(int theFlags, int theX, int theY) override;    // 0x4100a0
	bool Update() override;                                         // 0x40fd60
	bool OnTimer(int theId, int theIndex) override;                 // 0x40fec0

	// 0x410000: the level or stage picture under the point, or -1
	static int HitTest(int theX, int theY);
	// 0x4100e0
	void StepBack();
	// 0x410100
	void StartMove(bool theNewStage, const char* theStageName);
};

// 0x411140 (vtable 0x51d490, 0xc4 bytes): the curtain between the map and the play layer under it
class MapCurtain : public jb::Object
{
public:
	bool mClosing = false;       // +0x90 it moves right, over the play layer
	int mX = 0xffff;             // +0x94 0xffff when still
	jb::RegionRect mClip;        // +0x98 the map's
	jb::RegionRect mPlayClip;    // +0xac the play layer's
	PlayLayer* mPlay;            // +0xc0

	// 0x411140
	explicit MapCurtain(PlayLayer* thePlay) : mPlay(thePlay) {}
	// 0x4111b0, 0x4111d0
	~MapCurtain() override {}

	void Draw() override;    // 0x4111e0
	void Init() override;    // 0x411210
	bool Update() override;  // 0x411230

	// 0x411320: the map uncovers the play layer
	void Open();
	// 0x411350: the map covers the play layer
	void Close();
};

// 0x410170 (vtable 0x51d2e4, 0x58 bytes): the escape is won; start again?
class RestartBox : public Dialog
{
public:
	MapLayer* mMap;  // +0x54

	// 0x410170
	explicit RestartBox(MapLayer* theMap) : Dialog(0x94, 0x87), mMap(theMap) {}
	// 0x4101a0, 0x4101c0
	~RestartBox() override {}

	void Create() override;               // 0x4101d0
	bool OnCommand(int theCmd) override;  // 0x410330
};

// 0x410470 (vtable 0x51d468, 0x430 bytes)
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
	Level mLevels[60];         // +0x70

	// 0x410470
	explicit MapLayer(PlayLayer* thePlay) : Layer(0, 0), mPlay(thePlay) {}
	// 0x410500, 0x410530
	~MapLayer() override;

	void Load() override;                 // 0x410610
	bool OnCommand(int theCmd) override;  // 0x410840
	void OnShown() override;              // 0x410970
	bool OnYes(int theId) override;       // 0x410bc0
	bool OnNo(int theId) override;        // 0x410c10

	// 0x4105b0: shown over the play layer after a level, won or not
	void SetProgressed(bool theProgressed);
	// 0x410a00: the curtain stopped
	void CurtainDone();
	// 0x410aa0: the chosen level, or the question whether to resume its saved game
	void Continue();
	// 0x410c30
	void StartLevel(bool theResume);
	// 0x410d60
	bool LoadLevels();
	// 0x410fe0: the marker's move after a level won
	void StartProgress();
	// 0x4110e0
	void EnableAll(bool theEnabled);
	// 0x411120
	void ShowEnd();
};

} // namespace mahjongjapan

#endif
