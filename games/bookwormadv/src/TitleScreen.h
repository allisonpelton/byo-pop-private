// Sexy::TitleScreen (0x15c bytes, constructor 0x49e1f0, vtable 0x7b1224; its PopAnimListener part at +0x88 has
// the vtable 0x7b1340): the PopCap logo fading in and out, the curtain closing, then the loading screen: the
// game's logo slides down while Lex eats the tiles of "LOADING" as the resources load, and walks off to the
// right when they are done (then GameApp::TitleScreenDone).
#pragma once

#include <vector>

#include "Color.h"
#include "PopAnim.h"
#include "Widget.h"

namespace Sexy {

class GameApp;
class Graphics;
class Image;
class MemoryImage;

class TitleScreen : public Widget, public PopAnimListener {
public:
	enum State {
		STATE_POPCAP_LOGO = 0,
		STATE_PARTNER_LOGOS = 1,  // not ported: the game has no partner logos (its NumPartnerLogos property is 0)
		STATE_LOADING = 2,
		STATE_CURTAIN = 3,        // the curtain closes over the fader's colour
	};

	// A tile of "LOADING" (0x14 bytes)
	struct LoadingTile {
		MemoryImage* mImage;  // the tile with its letter
		int mX;
		int mY;
		int mDir;     // 1 or -1: the tile bobs up and down by 5 pixels
		int mOffset;  // from its resting y
	};

	GameApp* mApp;                        // +0x8c
	std::vector<LoadingTile*> mTiles;     // +0xac
	Color mFadeColor;                     // +0xdc (JeffLib's colour fader for the partner logos; its colour)
	int mEatWidth;                        // +0x124, the width of the seven tiles Lex eats
	int mTileWidth;                       // +0x128
	int mLexX;                            // +0x12c
	int mLexY;                            // +0x130
	int mLexStartX;                       // +0x134
	int mState;                           // +0x138
	int mTimer;                           // +0x13c, updates the PopCap logo stays up
	int mLogoAlpha;                       // +0x140, the game's logo (fades out when loaded, for a first game)
	int mLogoY;                           // +0x144
	int mPopCapLogoAlpha;                 // +0x148
	int mFadeSpeed;                       // +0x14c, alpha per update (negative while fading out)
	bool mLoaded;                         // +0x150, set by GameApp::LoadingThreadCompleted
	bool mFirstWalk;                      // +0x151, one more chomp before Lex walks off
	PopAnim* mLex;                        // +0x154

	explicit TitleScreen(GameApp* theApp);
	~TitleScreen() override;

	// 0x49e8c0: makes the tiles and loads Lex; false when his animation cannot be loaded
	bool InitLoadingScreen();

	void Update() override;           // 0x49ee70
	void Draw(Graphics* g) override;  // 0x49f670

	// PopAnimListener: Lex ate a tile (0x49f530)
	void PopAnimStopped(int theId) override;

private:
	void StartCurtain();    // 0x49f4c0
	void CurtainClosed();   // 0x49f500, called by the app when the curtain is closed
	static void CurtainClosedCallback(void* theTitleScreen);
};

}  // namespace Sexy
