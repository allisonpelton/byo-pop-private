// ThunderballApp.h: Peggle's application class (the original's Sexy::ThunderballApp, a global object whose
// constructor runs before WinMain; WinMain calls Init, Start and Shutdown on it).

#ifndef __THUNDERBALLAPP_H__
#define __THUNDERBALLAPP_H__

#include "CharacterMgr.h"
#include "HighScoreMgr.h"
#include "ImageMgr.h"
#include "ProfileMgr.h"
#include "SexyApp.h"
#include "StageMgr.h"
#include "StatsMgr.h"
#include "TrophyMgr.h"

namespace Sexy
{

class ThunderballApp : public SexyApp
{
public:
	ThunderballApp(); // 0x426f10
	~ThunderballApp() override;

	void Init() override; // 0x42ca30

	// the managers; the original makes them with new in its constructor and keeps them at these offsets of the app
	CharacterMgr mCharacterMgr;		 // +0x844
	HighScoreMgr mHighScoreMgr;		 // +0x848
	ImageMgr mImageMgr;				 // +0x84c (the level pictures)
	ProfileMgr mProfileMgr;			 // +0x850
	StageMgr mStageMgr;				 // +0x858
	StatsMgr mStatsMgr;				 // +0x85c
	StatsMgr mCheatStatsMgr;		 // +0x860 (what the level editor's test games record)
	TrophyMgr mTrophyMgr;			 // +0x864

	PlayerInfo* mCurrentPlayer = nullptr; // +0x878 (the profile in use; owned by mProfileMgr)
};

extern ThunderballApp* gThunderballApp;

} // namespace Sexy

#endif
