#ifndef __GALAXYPARSER_H__
#define __GALAXYPARSER_H__

// GalaxyParser.h: reads puzzles\<pack>\galaxy.xml, the map of a puzzle pack (Sexy::GalaxyParser, 0x5be56d-0x5c05b5), into
// the planets of the galaxy map. A planet has five moons: its five puzzles. The offsets in the comments are the original's
// (its std::string is 0x1c bytes, its std::vector 0x10: an allocator dword and three pointers).

#include <string>
#include <vector>

namespace Sexy
{

class Buffer;
class XMLParser;

// One puzzle of a planet (Sexy::Moon, 0x8c bytes): <Moon file= name= dimage= simage= uimage=/>
class Moon
{
public:
	std::string		mFile;              // +0x00 file: the puzzle's file, relative to the pack's folder
	std::string		mName;              // +0x1c name
	std::string		mDImage;            // +0x38 dimage
	std::string		mSImage;            // +0x54 simage
	std::string		mUImage;            // +0x70 uimage
};

// A step of a planet's tutorial (8 bytes): <Move col= row=/>, a cell of the board
class TutorialMove
{
public:
	int				mCol;               // +0x00 col
	int				mRow;               // +0x04 row

public:
	TutorialMove() : mCol(0), mRow(0) {}
	TutorialMove(int theCol, int theRow) : mCol(theCol), mRow(theRow) {}
};

// A planet of the galaxy map (Sexy::Planet, 0xe0 bytes): <Planet name= desc= mapx= mapy= bkg= aimage= dimage=>
class Planet
{
public:
	std::string		mName;              // +0x00 name
	std::string		mDesc;              // +0x1c desc: the text of the planet's tooltip
	int				mMapX;              // +0x38 mapx: the planet's place on the map picture
	int				mMapY;              // +0x3c mapy
	std::string		mBackdrop;          // +0x40 bkg: the backdrop's image path
	std::string		mAImage;            // +0x5c aimage
	std::string		mDImage;            // +0x78 dimage
	std::vector<Moon>	mMoons;         // +0x94 the planet's puzzles, in the order of the <Moon> sections
	std::string		mTutorialFile;      // +0xa4 <Tutorial file=>: the puzzle that the planet's tutorial plays
	std::vector<std::string>	mTutorialText;  // +0xc0 the <Text> lines of the tutorial
	std::vector<TutorialMove>	mTutorialMoves; // +0xd0 the <Move> steps of the tutorial

public:
	Planet() : mMapX(0), mMapY(0) {}

	int				GetPuzzleCount() const { return (int)mMoons.size(); }
	Moon&			GetPuzzle(int theIndex);                // asserts when out of range, as the original's vector access errors out
	const Moon&		GetPuzzle(int theIndex) const;
};

class GalaxyParser
{
public:
	XMLParser*		mParser;            // +0x00 only while ParseBuffer parses
	bool			mHasFailed;         // +0x04
	std::string		mErrorText;         // +0x08
	std::vector<Planet>	mPlanets;       // +0x24 the planets of the galaxy, in the order of the <Planet> sections
	std::string		mPuzzlePack;        // +0x34 the pack's folder name, set by WinDMApp::LoadGalaxyData (puzzles\<pack>)
	std::string		mMapImage;          // +0x50 <Galaxy mapimage=>

public:
	GalaxyParser();
	~GalaxyParser();

	void			Fail(const std::string& theError);                  // 0x5be56d
	bool			HasFailed() const { return mHasFailed; }
	const std::string& GetErrorText() const { return mErrorText; }      // 0x5be07b

	int				GetPlanetCount() const { return (int)mPlanets.size(); }
	Planet&			GetPlanet(int theIndex);                            // asserts when out of range (0x596d73)
	const Planet&	GetPlanet(int theIndex) const;

	bool			ParseBuffer(const Buffer& theBuffer);               // 0x5c05b5

protected:
	bool			ParseSingleElement(std::string* theString);         // 0x5bf057 the text of the section that has begun
	bool			ParseEnd();                                         // 0x5bf26a the end of a section that has no content
	bool			ParseTutorial(Planet* thePlanet);                   // 0x5bf3ab
	bool			ParsePlanet(Planet* thePlanet);                     // 0x5bf628
	bool			ParseGalaxy();                                      // 0x5c004b
	bool			DoParse();                                          // 0x5c03d4
};

}

// the file name of a puzzle: the pack's folder (puzzles\<pack>) and the name from galaxy.xml (0x412500)
std::string PuzzlePath(const std::string& theName, const std::string& theDir);

#endif
