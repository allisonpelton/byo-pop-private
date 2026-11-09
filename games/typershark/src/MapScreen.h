#ifndef __TYPERSHARK_MAPSCREEN_H__
#define __TYPERSHARK_MAPSCREEN_H__

// MapScreen.cpp (the node table, construction, the small Widget overrides and the level choice), MapScreenDraw.cpp
// (Draw) and MapScreenUpdate.cpp (Update): the map of the 36 dive sites between the adventure's games. The ship sails from
// the last visited site along the dotted route; at a fork the screen puts a treasure chest (an XWidget with a letter, A..C)
// on each site not yet visited and waits for a click or the letter key; when only one way is left the ship takes it by
// itself. The chosen site's number is mLevel, which the app reads when "Press space bar to continue" is pressed
// (ButtonDepress 0x1cf3 -> StartBoard). After the last site (the whirlpool, level 35) the map scrolls away and Gumbo's
// win dialog opens (SetFinalLevel, Update).
//
// Original: class Sexy::MapScreen : public Widget, vftable 0x530a7c, constructor 0x458d10, deleting destructor 0x45acf0
// (body 0x4530f0), sizeof 0x238. The offsets below are the original's. The app makes it in ShowMapScreen (0x426440), hidden
// and disabled, Resize()s it to the window and adds it; the constructor ends with OnEnter().
//
// Not translated: the STL instantiations of std::map<int, MapNode*> (insert/operator[] 0x41ded0, erase 0x44cfd0, tree walk
// 0x442330, destructor 0x4530b0), of the three vectors (push_back 0x4511c0, 0x44cd30, 0x421380, insert 0x41e410,
// copy 0x494750) and of std::string (assign 0x406da0, substr 0x406cb0).

#include "Widget.h"
#include "HyperlinkWidget.h"	// the callers (XWidget) reach mContinueLink
#include "UserProfile.h"	// ProfilePair
#include <string>
#include <vector>
#include <map>

namespace Sexy
{

class Graphics;
class WSTimer;
class WinWSApp;
class XWidget;

// One dive site of the map. Original: a 0x3c-byte object made by operator new(0x3c) and the constructor 0x450e80
// (the string and the link list start empty; the map's constructor sets every other field).
struct MapNode
{
	int						mX;				// +0x00 position of the site on the map
	int						mY;				// +0x04
	std::string				mText;			// +0x08 the tooltip text of the site (the name and what lives there)
	int						mId;			// +0x24 the level (site) number, 0..35; the callers' mField24
	std::vector<int>		mLinks;			// +0x28 the sites reachable from here (ids); -1 on the last site
	bool					mVisited;		// +0x38 the ship has been here (the callers' mFlag38)
	bool					mDeadEnd;		// +0x39 a spur: once visited, the ship sails back by itself and the way in is closed

	MapNode() : mX(0), mY(0), mId(0), mVisited(false), mDeadEnd(false) {}	// 0x450e80 (port: the numbers start at 0, the original sets all of them)
};

// Two helpers that the original has as plain functions (they do not touch the screen): 0x4537b0 is a function with three
// arguments (callee pops 0xc) that puts the dots of a line between two sites on a list; 0x4506a0 sets the tooltip text of a
// chest (an XWidget method of the original, written here because XWidget.h is another job's).
void MapAddRoutePoints(MapNode* theFrom, MapNode* theTo, std::vector<ProfilePair*>& theVector);	// 0x4537b0
void MapSetChestTip(XWidget* theChest, std::string theText);										// 0x4506a0

// The moving average that eases the map and the prompt text towards their targets (three copies in the original,
// inlined in Draw 0x447400). Layout 0x50 bytes: a ring of the last six steps of the target, the eased value, the ring
// position, a double that is zeroed and never used, and the previous target.
struct ScrollSmoother
{
	double					mRing[6];		// +0x00 the last six differences of the target
	double					mValue;			// +0x30 the eased value that Draw reads
	int						mIndex;			// +0x38 next ring slot
	double					mUnused;		// +0x40 zeroed by the constructor, never read
	double					mPrevious;		// +0x48 the target of the last Draw

	ScrollSmoother();						// the constructor's zeroing loops
	void					Step(int theTarget);	// the body that Draw 0x447400 repeats for the three of them
};

class MapScreen : public Widget
{
public:
	ScrollSmoother			mSmoothX;		// +0x58 follows mScrollX: where the map picture is drawn (x)
	ScrollSmoother			mSmoothY;		// +0xa8 follows mScrollY: (y)
	ScrollSmoother			mSmoothText;	// +0xf8 follows mPromptX: the x of "Type the letter of the dive site..."
	int						mGumboFrame;	// +0x148 0 or 6: the frame of the 60 by 60 Gumbo strip on the final panel
	std::string				mDifficultyName;	// +0x14c set by OnEnter from the difficulty (five texts); shown on the final panel
	double					mDifficultyPercent;	// +0x168 set by OnEnter: the level number scaled by the difficulty, times 100
	int						mGumboPeriod;	// +0x170 updates between two Gumbo frames (OnEnter, at least 5)
	bool					mHintPending;	// +0x174 the "open chest" hint is to be shown once (SetVisible sets, Update shows)
	HyperlinkWidget*		mContinueLink;	// +0x178 "Press space bar to continue" (id 0x1cf3), made by AddedToManager; owned
	WinWSApp*				mApp;			// +0x17c
	int						mPromptX;		// +0x180 target x of the prompt text (the window width: off screen); OnEnter
	int						mPromptEndX;	// +0x184 where the prompt stops (ctor: half its width, minus 120)
	int						mShipFrame;		// +0x188 0..5: the frame of the 50 by 45 ship strip, next every 11 updates
	int						mDotsDone;		// +0x18c how many dots of mVector1c4 lie behind the ship
	bool					mShipArrived;	// +0x190 set when the ship has reached the chosen site (never read)
	WSTimer*				mTimer;			// +0x194 the 500 ms step of the ship; owned, added to the manager
	std::vector<ProfilePair*>	mVector198;	// +0x198 dots (owned) of the routes to the sites offered at a fork
	std::map<int, MapNode*>	mMap1a8;		// +0x1a8 site number -> site; owned (the 36 sites)
	std::vector<XWidget*>	mVector1b4;		// +0x1b4 the chests on the screen; owned by the widget manager's safe-delete
	std::vector<ProfilePair*>	mVector1c4;	// +0x1c4 the dots (owned) of the route sailed so far and the chosen one; the app copies them to the profile
	std::vector<MapNode*>	mVector1d4;		// +0x1d4 the sites sailed through, in order (the ship is at the last)
	bool					mFlag1e4;		// +0x1e4 set by OnEnter (never read)
	int						mDotIndex;		// +0x1e8 the dot the ship has reached (counts with the timer)
	bool					mFlag1ec;		// +0x1ec the choice at the ship's site has not been set up yet
	int						mArrivals;		// +0x1f0 how often the arrival bell has been counted since OnEnter
	bool					mFinal;			// +0x1f4 the callers' SetFinalLevel flag: the map is on the whirlpool panel
	int						mField1f8;		// +0x1f8 the level number shown at the top ("Level %d"); the callers set it from the profile
	int						mScrollX;		// +0x1fc target x of the map picture (the final scroll counts it down)
	int						mScrollY;		// +0x200 target y (see ResetScroll: the original uses the width difference for both)
	bool					mScrolling;		// +0x204 the final scroll is under way
	bool					mHeadingOn;		// +0x205 the last site of mVector1d4 is the one being sailed to (the ship stamp is drawn one site back)
	std::string				mTipText;		// +0x208 an old tooltip text; nothing sets it (the original's Draw shows it when not empty)
	bool					mFlag224;		// +0x224 a level is chosen (the start button works); the callers' name
	int						mLevel;			// +0x228 the chosen level (site number), -1 while none; the callers' name
	int						mTipX;			// +0x22c position of mTipText (never set)
	int						mTipY;			// +0x230
	bool					mFlag234;		// +0x234 the final scroll has run: the prompts are gone; the callers' name
	char					mChosenLetter;	// +0x235 the letter of the chosen chest (' ' while none; 'A' for a single way)

public:
	MapScreen(WinWSApp* theApp);														// 0x458d10
	virtual ~MapScreen() override;														// 0x45acf0 (body 0x4530f0), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4480d0 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4410b0 slot 2
	virtual void			SetVisible(bool isVisible) override;							// 0x450ec0 slot 4
	virtual void			Draw(Graphics* g) override;										// 0x447400 slot 16
	virtual void			Update() override;												// 0x45ad20 slot 17
	virtual void			KeyChar(SexyChar theChar) override;								// 0x442880 slot 20

	void					OnEnter();					// 0x4533b0 (the app calls it before the screen shows again)
	void					OnLevelChosen();			// 0x4538e0 (XWidget::MouseDown calls it)
	void					ResetScroll();				// 0x441030
	void					SetFinalLevel(bool theFinal);	// 0x441080
	void					ClearNodeWidgets();			// 0x443e20
};

}

#endif
