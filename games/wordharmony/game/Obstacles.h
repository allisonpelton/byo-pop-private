#ifndef __WORDHARMONY_OBSTACLES_H__
#define __WORDHARMONY_OBSTACLES_H__

// The level's obstacles: square blocks and ramps (a block with one diagonal half), 0xb0 bytes each. In the original
// they derive virtually from a node and an obstacle interface (vtable 0x48210c); the port makes the interface's slots
// virtuals of an Obstacle node.

#include "Font.h"
#include "Graphics.h"
#include "Node.h"
#include <string>

class BallArray;

// 0x42b610: "x,y" as the level files and the saved game write a point
std::string pointText(const engine::Point& p);

class Obstacle : public engine::Node
{
public:
	virtual void collide(BallArray* theBalls) = 0;  // 0xc8
	virtual void setActive(bool theActive) = 0;     // 0xcc
	// (the editor's slots 0xd0 isActive, 0xd4 touches, 0xdc setOrigin, 0xe8 getLetter, 0xf0 toXml and 0xf4 getType are left
	// out; README, "The unneeded code")
	virtual engine::Point getOrigin() { return mPosition; }               // 0xd8: 0x40f9e0
	virtual engine::Point getExtents() { return mExtents; }               // 0xe0: 0x40fa40
	virtual void setExtents(const engine::Point& p) { mExtents = p; }     // 0xe4: 0x40fa70, 0x410980
	virtual void setLetter(char theLetter) = 0;     // 0xec
	virtual Obstacle* clone() = 0;                  // 0xf8

	// 0x40: 0x410800, the rectangle origin - extents to origin + extents, edges included
	bool contains(engine::Point& p) override;

	engine::Point mExtents;  // +0x04: half the size
};

// A block (0x40fb00, vtable 0x481ea8): type 0 is drawn with cube01.png, 3 is invisible
class Box : public Obstacle
{
public:
	Box(const engine::Point& theOrigin, int theType);  // 0x40fb00
	~Box() override;                                   // 0x40f360

	void draw(engine::Graphics* g) override;  // 0x6c: 0x40f3c0
	void layout() override;                   // 0x74: 0x40f880

	void collide(BallArray* theBalls) override;  // 0x40f440
	void setActive(bool theActive) override { mActive = theActive; }  // 0x40f990
	void setLetter(char theLetter) override { mLetter = theLetter; }   // 0x40f740
	Obstacle* clone() override;                                        // 0x40fc70

	int mType;              // +0x0c
	bool mActive;           // +0x10
	engine::Image* mImage;  // +0x14 (counted)
	char mLetter;           // +0x18
};

// A ramp (0x4109f0, vtable 0x482010): cube03-<rotation>.png, its slope across the corner the rotation names
class Ramp : public Obstacle
{
public:
	Ramp(const engine::Point& theOrigin, int theRotation);  // 0x4109f0
	~Ramp() override;                                       // 0x40ff60

	void draw(engine::Graphics* g) override;  // 0x6c: 0x40ffc0
	void layout() override;                   // 0x74: 0x410910

	void collide(BallArray* theBalls) override;  // 0x410040
	void setActive(bool theActive) override { mActive = theActive; }  // 0x410930
	void setLetter(char theLetter) override { mLetter = theLetter; }   // 0x4106c0
	Obstacle* clone() override;                                        // 0x410b90

	bool mActive;           // +0x0c
	engine::Image* mImage;  // +0x10 (counted)
	char mLetter;           // +0x14
	int mRotation;          // +0x18
};

#endif
