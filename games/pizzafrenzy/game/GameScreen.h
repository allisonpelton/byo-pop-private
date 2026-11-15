#ifndef __PIZZAFRENZY_GAMESCREEN_H__
#define __PIZZAFRENZY_GAMESCREEN_H__

// The screen the game is played on (0x220 bytes, constructor 0x434be0, vtable 0x4fd430; the application keeps it at
// +0x114). It holds the city's ground picture and the groups of nodes the map's tiles are put in, with the hud and the
// layout of the screen over them. The city select draws the cities' postcards from it (CitySelectScreen.h).
//
// What the game's play does with it is translated with the game: the mouse (slot 0xa8, 0x435320, and the right
// button's 0xc0, 0x435340), the key (slot 0x158, 0x434bd0) and its destructor (0x435360); the speed bonus's banner
// (0x44fb70, kept at +0x204, made and added by the link) and the message banner (0x42bb30, +0x208, which sets its
// text with 0x42bb00 and plays with 0x42bb10) with the slots 0x4352e0 and 0x44fcd0 that use them; and the vehicles
// of the map (+0xc0), which the city's load puts in the groups.

#include "CityMap.h"
#include "GenericScreen.h"
#include "Hud.h"

namespace pizzafrenzy
{

// A container that keeps its children in the order they are drawn: by the bottom of their bounds, which is how far
// down the screen they stand (0x40df00 with the vtable 0x4fd5c0; the ground group's)
class SortedContainer : public engine::Container
{
public:
	// 0x41ada0: a child that is there already stays; the others are put before the first child that is below them
	// (or at the same bottom and not as far to the right). Its slot 0x12c (0x41ae70, a child that moved is put in
	// its place again) and slot 0x130 (0x436430, the children) are used by the game's play.
	void addChild(engine::Node* theChild) override;
};

class GameScreen : public engine::GenericScreen
{
public:
	GameScreen();  // 0x434be0

	// 0x436440: the map and the hud are the screen's, and the nodes the screen is made of are made
	void link(Hud* theHud, CityMap* theMap);
	// 0x4350c0: the groups have no children
	void clearGroups();
	// 0x435c60: the city's picture and nodes, from the map (the postcards and the game take them from here)
	void drawCity();
	// 0x4352a0: the node is the only one in the order cursor, at its corner (none empties it): the coupon held
	void setCouponCursor(engine::Node* theNode);
	// 0x435180: the order cursor is emptied
	void hideOrderCursor();
	// 0x435ba0: a frame of the screen (the speed bonus banner's, the vehicles', and the nodes of the sorted group);
	// translated next
	void updateScreen(engine::UpdateInfo& theInfo);

	engine::Ref<CityMap> mMap;                         // +0x1d4
	engine::Ref<Hud> mHud;                             // +0x1d8
	engine::Ref<engine::ImageNode> mGroundNode;        // +0x1dc: draws the ground's picture
	engine::Ref<engine::Container> mRoadGroup;         // +0x1e0: "RoadGroup"
	engine::Ref<SortedContainer> mGroundGroup;         // +0x1e4: "GroundGroup"
	engine::Ref<engine::Container> mAirGroup;          // +0x1e8: "AirGroup"
	engine::Ref<engine::Container> mSfxGroup;          // +0x1ec: "SfxGroup"
	engine::Ref<engine::Container> mTipsGroup;         // +0x1f0: "TipsGroup"
	engine::Ref<engine::Container> mOrderCursor;       // +0x1f4: "OrderCursor"
	engine::Ref<engine::Image> mGroundImage;           // +0x200: the city's ground, made once
	// +0x20c: the layout of the screen (res/screenLayouts/gameScreen.xml), which the city select hides while it draws
	// the postcards
	engine::Ref<engine::ScreenLayout> mGameLayout;
	engine::Ref<engine::Node> mTitles;                 // +0x210: its node "titles"
};

} // namespace pizzafrenzy

#endif
