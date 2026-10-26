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
	virtual bool isActive() = 0;                    // 0xd0
	// 0xd4: whether a ball is within theMargin of the obstacle (a rectangle test)
	virtual bool touches(BallArray* theBalls, int theIndex, float theMargin) = 0;
	virtual engine::Point getOrigin() { return mPosition; }               // 0xd8: 0x40f9e0
	virtual void setOrigin(const engine::Point& p) { mPosition = p; }     // 0xdc: 0x40fa10
	virtual engine::Point getExtents() { return mExtents; }               // 0xe0: 0x40fa40
	virtual void setExtents(const engine::Point& p) { mExtents = p; }     // 0xe4: 0x40fa70, 0x410980
	virtual char getLetter() = 0;                   // 0xe8
	virtual void setLetter(char theLetter) = 0;     // 0xec
	virtual std::string toXml() = 0;                // 0xf0: the level file's line
	virtual int getType() = 0;                      // 0xf4
	virtual Obstacle* clone() = 0;                  // 0xf8

	// 0x40: 0x410800, the rectangle origin - extents to origin + extents, edges included
	bool contains(engine::Point& p) override;

	engine::Point mExtents;  // +0x04: half the size
};

// A block (0x40fb00, vtable 0x481ea8): type 0 is drawn with cube01.png, 3 is invisible; 1 is written as a star
class Box : public Obstacle
{
public:
	Box(const engine::Point& theOrigin, int theType);  // 0x40fb00
	~Box() override;                                   // 0x40f360

	void draw(engine::Graphics* g) override;  // 0x6c: 0x40f3c0
	void layout() override;                   // 0x74: 0x40f880

	void collide(BallArray* theBalls) override;  // 0x40f440
	void setActive(bool theActive) override { mActive = theActive; }  // 0x40f990
	bool isActive() override { return mActive; }                      // 0x40f9b0
	bool touches(BallArray* theBalls, int theIndex, float theMargin) override;  // 0x40f750
	char getLetter() override { return mLetter; }                      // 0x40fa80
	void setLetter(char theLetter) override { mLetter = theLetter; }   // 0x40f740
	std::string toXml() override;                                      // 0x40fd70
	int getType() override { return mType; }                          // 0x40fab0
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
	bool isActive() override { return mActive; }                      // 0x410950
	bool touches(BallArray* theBalls, int theIndex, float theMargin) override;  // 0x4106d0
	char getLetter() override { return mLetter; }                      // 0x4109a0
	void setLetter(char theLetter) override { mLetter = theLetter; }   // 0x4106c0
	std::string toXml() override;                                      // 0x410c90
	int getType() override { return 0; }                              // 0x4109e0
	Obstacle* clone() override;                                        // 0x410b90

	bool mActive;           // +0x0c
	engine::Image* mImage;  // +0x10 (counted)
	char mLetter;           // +0x14
	int mRotation;          // +0x18
};

#endif
