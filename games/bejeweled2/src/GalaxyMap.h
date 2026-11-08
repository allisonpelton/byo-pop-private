#ifndef __GALAXYMAP_H__
#define __GALAXYMAP_H__

// GalaxyMap.h: the map of the puzzle galaxies, where the player picks a planet and a moon (Sexy::GalaxyMap, 0x5ba6e1-0x5bd98a;
// the object is 0x214 bytes). Board::mGalaxyMap (+0x94) holds it in the puzzle modes. The map slides down over the board:
// the planets of the pack (the last one is the sun, with a swirl behind it) with five moons round each, a ring that turns
// round a planet with four puzzles solved (another one once all five are), a ring round the moon of the puzzle that is
// picked, and a panel with the names of the planet and the puzzle, the planet's text and the gems of the puzzle. A click on
// a moon picks its puzzle, a click on a planet picks the first puzzle of the planet, a click on the puzzle that is picked
// warps to it, and a planet is locked until four of the five puzzles of the planet before it are solved. The offsets in the
// comments are the original's, from the start of the object.
//
// The original builds the map (Setup: it loads the pictures of every planet and moon) on its worker thread; the port has no
// threads and StartSetup runs Setup in line.

#include "Widget.h"
#include "ButtonListener.h"
#include <string>
#include <vector>

namespace Sexy
{

class WinDMApp;
class Graphics;
class Image;
class MemoryImage;
class PodButton;
class WidgetManager;

// a moon on the map (0xc bytes): its picture and how brightly its ring glows
struct MapMoon
{
	Image*			mImage;             // +0x00 the moon's picture: the file that galaxy.xml names for a solved moon (simage), for a moon of
									// a locked planet (dimage) or for any other (uimage), with the alpha of the moon mask
	float			mGlow;              // +0x04 0 to 1: the ring round the moon of the puzzle that is picked fades in and the others fade out
	bool			mSolved;            // +0x08 the puzzle is solved

	MapMoon() : mImage(NULL), mGlow(0), mSolved(false) {}
};

// a planet on the map (0x24 bytes)
struct MapPlanet
{
	bool			mIsSun;             // +0x00 the last planet of the galaxy: it has the swirl behind it and its own moon places
	int				mX;                 // +0x04 the planet's place on the map (the planet's mapx)
	int				mY;                 // +0x08 (mapy)
	Image*			mImage;             // +0x0c the planet's picture
	bool			mOpen;              // +0x10 the planet before it has four puzzles solved (the first planet is always open): the moons that
									// are not solved show a question mark
	bool			mNearlyDone;        // +0x11 four or five of its puzzles are solved: the next planet opens
	bool			mDone;              // +0x12 all five are solved
	std::vector<MapMoon>	mMoons;     // +0x14 (an allocator dword and three pointers, 0x10 bytes)

	MapPlanet() : mIsSun(false), mX(0), mY(0), mImage(NULL), mOpen(false), mNearlyDone(false), mDone(false) {}
};

class GalaxyMap : public Widget, public ButtonListener
{
public:
	WinDMApp*		mApp;               // +0x8c (the ButtonListener subobject is at +0x88)
	PodButton*		mWarpButton;        // +0x90 id 0: warps to the puzzle that is picked
	PodButton*		mCloseButton;       // +0x94 id 1: back to the puzzle that was being played
	std::vector<MapPlanet>	mPlanets;   // +0x98 the planets, in the order of the galaxy (0x10 bytes)
	int				mPuzzleBoard[8][8]; // +0xa8 the gems of the picked puzzle (row by row; -1 for none), drawn small in the panel
	int				mSelectedPuzzle;    // +0x1a8 the puzzle (planet times 5 plus moon) that is picked
	bool			mSettingUp;         // +0x1ac Setup has not finished (the original's worker thread is still loading)
	std::string		mPlanetName;        // +0x1b0 the name of the picked puzzle's planet
	std::string		mPuzzleName;        // +0x1cc the name of the picked puzzle
	std::string		mPlanetDesc;        // +0x1e8 the text of the planet's panel
	Image*			mMapImage;          // +0x204 the picture behind the planets (the map of the pack)
	MemoryImage*	mTitleGlow;         // +0x208 the pack's name in the glow font
	MemoryImage*	mPlanetGlow;        // +0x20c the planet's name in the glow font
	MemoryImage*	mPuzzleGlow;        // +0x210 the puzzle's name in the glow font

public:
	GalaxyMap(WinDMApp* theApp);        // 0x5bcff3
	virtual ~GalaxyMap();               // 0x5bd3aa (0x5bd4eb deletes)

	void			Warp();                                     // 0x5ba6e1 leaves the map for the picked puzzle
	void			Cleanup();                                  // 0x5bd2c2 frees the pictures of the planets and moons
	int				GetPlanetAt(int theX, int theY);            // 0x5bac5c the planet under the point, -1
	int				GetMoonAt(int theX, int theY);              // 0x5bad42 the puzzle (planet times 5 plus moon) under the point, -1
	void			CreateFontGlowImages();                     // 0x5bb561
	void			LoadPuzzleData(int thePuzzle);              // 0x5bc1c9 the gems and the names of the puzzle that is picked
	void			Setup();                                    // 0x5bd50a
	static void		SetupStub(void* theMap);                    // 0x5bd97e
	void			StartSetup();                               // 0x5bd98a

	virtual void	ButtonPress(int theId) override;            // 0x5ba84d
	virtual void	ButtonDepress(int theId) override;          // 0x5ba861
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5ba708
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5ba738
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x5ba768
	virtual void	Update() override;                          // 0x5bae99
	virtual void	Draw(Graphics* g) override;                 // 0x5bb7f5
	virtual void	MouseMove(int theX, int theY) override;     // 0x5bae58
	virtual void	MouseDown(int theX, int theY, int theClickCount) override;  // 0x5bc2fe
	// the original's overrides that do nothing (or call the base): the key, the mouse leaving, the button coming up and a drag
	virtual void	KeyChar(SexyChar theChar) override;         // 0x5ba705
	virtual void	MouseLeave() override;                      // 0x5ba6f2
	virtual void	MouseUp(int theX, int theY, int theClickCount) override;    // 0x5ba6f3
	virtual void	MouseDrag(int theX, int theY) override;     // 0x5ba6fc
};

}

#endif
