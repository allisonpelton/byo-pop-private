#ifndef __HEAVYWEAPON_MAPSCREEN_H__
#define __HEAVYWEAPON_MAPSCREEN_H__

// MapScreen: the mission map before a level (and before a survival game). The map comes up with the region of the level
// framed in clouds and zoomed to its place; the mission's briefing (its number, the region, the intel text and the enemy
// craft in turn, each with its picture and figures) fades in beside it. Clicking the region or the link starts the level
// (a first game shows the help screen first), through a zoom into the region and a red flash. Original: a Widget with a
// ButtonListener at +0x58, 0xd8 bytes, constructor 0x425500, vtable 0x52dca8 (listener 0x52dc88), destructor body
// 0x424b30 (deleting 0x4254d0).

#include "Widget.h"
#include "ButtonListener.h"
#include <vector>

namespace Sexy
{

class HeavyWeaponApp;
class HelpScreen;
class HyperlinkWidget;

class MapScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		BUTTON_INVADE = 0,			// "Click here to invade!"
		BUTTON_INTEL = 1,			// the whole briefing panel: shows the next craft
		BUTTON_HELP_PLAY = 0x3f,	// the help screen's PLAY, which it reports to the map
		NUM_CRAFT_IDS = 22
	};

	HeavyWeaponApp*		mApp;				// +0x5c
	HyperlinkWidget*	mInvadeLink;		// +0x60 id 0
	HelpScreen*			mHelpScreen;		// +0x64 the help screen shown before the level (it reports to this screen)
	HyperlinkWidget*	mIntelButton;		// +0x68 id 1: invisible, over the briefing panel
	double				mLeft;				// +0x70 the frame's corners (the region's rectangle; the centre of the zoom)
	double				mTop;				// +0x78
	double				mRight;				// +0x80
	double				mBottom;			// +0x88
	int					mFade;				// +0x90 how dark the map is made, 0..255
	int					mTextAlpha;			// +0x94 how visible the briefing is, 0..255
	int					mStartDelay;		// +0x98 updates left before anything moves (50 at the start)
	bool				mFindingRegion;		// +0x9c the frame is still closing in on the region
	bool				mMovingRegion;		// +0x9d the frame is moving the region to the left of the screen
	bool				mOverRegion;		// +0x9e the mouse is over the region's picture
	int					mHoverGlow;			// +0xa0 the region's tint, 0..255, rises while it is pointed at
	double				mZoom;				// +0xa8 0 until the invasion starts, then 1 and growing
	int					mFlash;				// +0xb0 -1 until the zoom ends, then the red flash's alpha falling to 0
	int					mMouseX;			// +0xb4
	int					mMouseY;			// +0xb8
	int					mIntelTick;			// +0xbc updates since the briefing's craft changed
	int					mIntelIndex;		// +0xc0 the craft shown, an index in mCrafts
	bool				mAutoCycle;			// +0xc4 the craft change by themselves every 300 updates
	std::vector<int>	mCrafts;			// +0xc8 the craft (CraftInfo indexes) the level has, in order

public:
	MapScreen(HeavyWeaponApp* theApp);										// 0x425500
	virtual ~MapScreen();													// 0x4254d0 (body 0x424b30)

	virtual void	AddedToManager(WidgetManager* theManager) override;			// 0x423ee0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;		// 0x423f10 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x423f40 slot 13
	virtual void	Draw(Graphics* g) override;									// 0x424bf0 slot 16
	virtual void	Update() override;											// 0x424660 slot 17
	virtual void	MouseMove(int x, int y) override;							// 0x424410 slot 26
	virtual void	MouseUp(int x, int y) override;								// 0x424620 slot 31
	virtual void	MouseDrag(int x, int y) override;							// 0x4244e0 slot 32
	virtual void	ButtonDepress(int theId) override;							// 0x424580 listener slot 5

	using Widget::Resize;
	using Widget::MouseUp;

private:
	void			DrawCraft(Graphics* g, int theCraft);						// 0x423fc0
	void			StartInvasion();											// 0x424510
};

}

#endif
