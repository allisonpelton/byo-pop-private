#ifndef __PIZZAFRENZY_EVENTPOPUPS_H__
#define __PIZZAFRENZY_EVENTPOPUPS_H__

// What a special event shows when its customer's pizza is sent (the factory 0x44a490 makes one of the classes below
// from the event's type, GameMode::startSpecialEvent 0x455b20 starts it): a shaded bar at the bottom of the screen with
// the event's portrait and a line of text that types itself out, a sound, and the event's own deed: the Thief takes
// the player's money and the Crank Caller only mocks (both are the events that want no pizza, which the police take
// in the normal course), the Graffiti Artist takes the happiness, the Movie Star tips, the Gossip makes every customer
// ask for one topping, the Clown makes every customer change toppings, the Monk slows the time, and the Banker picks
// up the tips that wait.
//
// Each popup is a machine of two states (below): after the portrait has flown to its place, the event speaks (its text
// and its first effects) and waits; when the wait is over it acts, which sets the next wait or ends the event. An event
// ends by fading its bar out and letting the mode go of the popup.
//
// Left out, with one comment where each would be called: the splats the events make (the portrait that flies from the
// customer's house to the bar, the money and the happiness that fly between the displays and the portrait): the splat
// factory is not translated, so SplatFlight stands in for the splat's flight, where something waits for the splat to
// arrive (the portrait's, and each of the Movie Star's tips), and the draws a splat makes are made.

#include "FadeContainer.h"
#include "GameMode.h"
#include "ImageNode.h"
#include "State.h"
#include "TextTyper.h"
#include "Tip.h"

#include <functional>
#include <string>
#include <vector>

namespace pizzafrenzy
{

class SpecialEventPopup;

// 0x44a490: the popup of the event's type ("Thief", "Gossip", "MovieStar", "CrankCaller", "Monk", "Clown",
// "TreasureHunter" or "GraffitiArtist"); none for another type
SpecialEventPopup* makeEventPopup(const std::string& theType);

// What a splat is for the events that wait for one (port): the splat factory makes a splat from a definition of
// res\manifests\splats.xml and the splat flies to its end over the distance divided by its speed, then tells the node
// that connected to it and is let go. This node is that flight: after the seconds it tells the popup, a frame after
// they are up (the factory looks at a splat's time before it moves it)
class SplatFlight : public engine::Container
{
public:
	SplatFlight(std::function<void()> theArrival, float theSeconds);

	// slot 0x94: 0x473b90's loop over the splats (0x4812f0 is the splat's own frame)
	void update(engine::UpdateInfo& theInfo) override;

private:
	std::function<void()> mArrival;
	float mSecondsLeft;
};

// The popup waits for the seconds in its timer (0x1c bytes, constructor 0x44aa00, vtable 0x4ff8a4): when they are up it
// acts, and acts again at each frame until the act makes another wait or ends the event
class EventWaitState : public engine::State
{
public:
	explicit EventWaitState(SpecialEventPopup* thePopup) : mPopup(thePopup) {}  // 0x44aa00
	// 0x0c: 0x44a890
	void update(engine::UpdateInfo& theInfo) override;

private:
	SpecialEventPopup* mPopup;  // +0x0c
};

// The event ends (0x18 bytes, constructor 0x448a40, vtable 0x4febbc): the bar fades out, and when it has
// the mode lets go of the popup
class EventFinishState : public engine::State
{
public:
	explicit EventFinishState(SpecialEventPopup* thePopup) : mPopup(thePopup) {}  // 0x448a40
	// 0x04: 0x44a970
	void enter() override;
	// 0x0c: 0x44a990
	void update(engine::UpdateInfo& theInfo) override;

private:
	SpecialEventPopup* mPopup;  // +0x0c
};

// The base of the events' popups (0x194 bytes, constructor 0x44b100, vtable 0x4ff8e8)
class SpecialEventPopup : public EventPopup
{
public:
	SpecialEventPopup();           // 0x44b100
	~SpecialEventPopup() override;  // 0x44aea0

	// slot 0x12c: 0x44ab20: the sound and the portrait of the event, the kitchen that sent the pizza and the customer's
	// house, and the bar with its shade, its text and the portrait (hidden until it has flown there)
	void begin(BuildingTile* theKitchen, CustomerTile* theTile) override;
	// slot 0x130: 0x44b230: the bar fades in, the portrait flies from the house, the sound plays, and the pizza is
	// delivered to the customer
	void show() override;
	// slot 0x138: 0x44a880: a frame of the state
	void updatePopup(engine::UpdateInfo& theInfo) override;

	// slot 0x134: 0x44aa90: the portrait is there: it shows, the event speaks, and waits for what it says
	virtual void onPortraitArrived();
	// slot 0x13c: the wait is over: the event's deed, and the next wait or the end
	virtual void act() = 0;
	// slot 0x140: the event says its line, and sets the wait before it acts
	virtual void speak() = 0;
	// slot 0x144: 0x44a930: the line shows on the bar, typed out
	void setCaption(const std::string& theText);

	// 0x44a970: the bar fades out in the seconds it takes
	void fadeBarOut();
	// 0x44a990: whether the bar has faded out
	bool isBarGone() { return mBar->isFadeDone(); }
	// 0x448a40's state: the event ends, the bar fades out, and the mode lets go of the popup
	void finish();
	// the seconds before the state acts (the original's +0x16c, which the wait counts down)
	float mWaitLeft;

	// port: what the splat factory draws from the generator for each splat it makes (0x474a20), the only part of a
	// splat that is made when the splat is not drawn
	static void drawSplatRandoms();

protected:
	// 0x474a20, 0x481200: the seconds a splat of the definition takes to fly from the point to the other, at the
	// definition's speed; the splat's picture is kept within the screen at its start, by the width of the
	// definition's picture
	static float getFlightSeconds(const char* theImage, float theSpeed, engine::Point theFrom,
	                              const engine::Point& theTo);
	// the splat of the definition is made, and the popup waits for its flight: drawSplatRandoms, and the node that
	// calls back at the splat's end
	void watchSplat(const char* theImage, float theSpeed, const engine::Point& theFrom, const engine::Point& theTo,
	                std::function<void()> theArrival);

	engine::StateMgr mStates;                      // +0x138
	std::string mSound;                            // +0x150: the sound the event plays
	engine::Ref<engine::FadeContainer> mBar;       // +0x170: the shaded bar, which holds the rest
	engine::Ref<engine::TextTyper> mCaption;       // +0x174
	engine::Ref<engine::ImageNode> mPortraitNode;  // +0x178
	engine::Ref<engine::Image> mPortrait;          // +0x17c
	engine::Ref<BuildingTile> mKitchen;            // +0x180
	engine::Ref<CustomerTile> mTile;               // +0x184
};

// The Thief (0x194 bytes, constructor 0x44b100 with vtable 0x4fee90): takes a hundred of the player's money ten times,
// a tenth of a second apart, while the player has any, and breaks the combo
class ThiefPopup : public SpecialEventPopup
{
public:
	// slot 0x12c: 0x449a70: the base's, and ten hundreds to take
	void begin(BuildingTile* theKitchen, CustomerTile* theTile) override;
	// slot 0x13c: 0x449b50
	void act() override;
	// slot 0x140: 0x449aa0
	void speak() override;
	// slot 0x60: 0x44a760
	std::string getName() override { return "ThiefEventPopup"; }

private:
	int mBillsLeft = 0;  // +0x188
};

// The Gossip (0x194 bytes, vtable 0x4fefd8): every customer waiting asks for the topping of the Gossip's own order
class GossipPopup : public SpecialEventPopup
{
public:
	// slot 0x130: 0x448bf0: with no other customer waiting for another topping, the pizza is delivered and the event
	// is over at once; else the base's
	void show() override;
	// slot 0x13c: 0x448cd0
	void act() override;
	// slot 0x140: 0x448e00
	void speak() override;
	// slot 0x60: 0x44a760
	std::string getName() override { return "ThiefGossipPopup"; }

private:
	Topping* mTopping = nullptr;  // +0x188: the topping of the Gossip's order, which every customer comes to ask for
};

// The Movie Star (0x194 bytes, vtable 0x4ff120): tips a hundred five times, each a splat of money that flies to the
// tip jar
class MovieStarPopup : public SpecialEventPopup
{
public:
	// slot 0x12c: 0x449440: the base's, and five tips to give
	void begin(BuildingTile* theKitchen, CustomerTile* theTile) override;
	// slot 0x13c: 0x449650
	void act() override;
	// slot 0x140: 0x449530
	void speak() override;
	// slot 0x60: 0x44a790
	std::string getName() override { return "MovieStarEventPopup"; }
	// slot 0x148: 0x449470: the tip has arrived at the tip jar
	void addTip();

private:
	int mTipsLeft = 0;  // +0x188
};

// The Crank Caller (0x194 bytes, vtable 0x4ff270): only mocks, and breaks the combo
class CrankCallerPopup : public SpecialEventPopup
{
public:
	// slot 0x13c: 0x448ad0
	void act() override;
	// slot 0x140: 0x448b40
	void speak() override;
	// slot 0x60: 0x44a7c0
	std::string getName() override { return "CrankCallerEventPopup"; }
};

// The Monk (0x194 bytes, vtable 0x4ff3b8): slows the time for eight seconds
class MonkPopup : public SpecialEventPopup
{
public:
	// slot 0x13c: 0x449310
	void act() override;
	// slot 0x140: 0x449380
	void speak() override;
	// slot 0x60: 0x44a7f0
	std::string getName() override { return "MonkEventPopup"; }
};

// The Clown (0x194 bytes, vtable 0x4ff510, named "ScramblerPopup"): every customer waiting changes to the next topping
// of the kitchens
class ClownPopup : public SpecialEventPopup
{
public:
	// slot 0x130: 0x4497d0: with no other customer waiting, the pizza is delivered and the event is over at once; else
	// the base's
	void show() override;
	// slot 0x13c: 0x449850
	void act() override;
	// slot 0x140: 0x4499e0
	void speak() override;
	// slot 0x60: 0x44a820
	std::string getName() override { return "ScramblerPopup"; }

private:
	bool mSwitched = false;  // +0x188: the customers have changed
};

// The Banker (0x19c bytes, vtable 0x4fed08, the event type "TreasureHunter"): takes up the tips that wait, one at a
// time, ten times in all
class BankerPopup : public SpecialEventPopup
{
public:
	~BankerPopup() override;  // 0x449d90 (0x449cd0 lets go of the helper)
	// slot 0x13c: 0x449db0
	void act() override;
	// slot 0x140: 0x449ed0
	void speak() override;
	// slot 0x60: 0x44a020
	std::string getName() override { return "TreasureHunterEventPopup"; }

private:
	std::vector<engine::Ref<TipPopup>>* mTips = nullptr;  // +0x188: the map's tips
	int mTipsLeft = 0;                                   // +0x18c
	engine::Ref<engine::ImageNode> mHelper;              // +0x190: the Banker's picture, on the tip it takes
};

// The Graffiti Artist (0x19c bytes, vtable 0x4ff668): paints a picture across the screen for two seconds while a
// sound loops, then takes ten of the happiness, and breaks the combo
class GraffitiArtistPopup : public SpecialEventPopup
{
public:
	~GraffitiArtistPopup() override;  // 0x44a410 (0x44a430 lets go of the picture)
	// slot 0xf4: 0x448eb0: the sound stops when the popup has left its parent
	void clearParent(engine::Node* theParent) override;
	// slot 0x13c: 0x4490a0
	void act() override;
	// slot 0x140: 0x448ee0
	void speak() override;
	// slot 0x60: 0x44a850
	std::string getName() override { return "GraffitiArtistEventPopup"; }

private:
	int mStage = 0;                                  // +0x188: 0 sound starts, 1 picture, 2 happiness, 3 over
	engine::Ref<engine::ImageNode> mPicture;         // +0x18c: the painting, shown from its left edge on
	engine::Sound* mPaintSound = nullptr;            // +0x190: the voice "special_graffiti"
};

} // namespace pizzafrenzy

#endif
