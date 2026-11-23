#ifndef __FEEDINGFRENZY_FISHSCHOOL_H__
#define __FEEDINGFRENZY_FISHSCHOOL_H__

// A school (FishSchool, constructor 0x41d6f0): the fish of a kind that a spawner puts in the stage together. They swim
// in one group (AIFish's school state), steering by the others' headings and positions and towards a place the school
// picks, one after another, in the stage. The school ends when its last fish is gone.

#include "Geometry.h"
#include "Node.h"
#include <vector>

namespace feedingfrenzy
{

class FishBase;

class FishSchool
{
public:
	FishSchool();  // 0x41d6f0

	// 0x41d010: how strongly the fish steer: from the others' heading, to the school's centre (a divisor), to its
	// destination, towards a fish they can eat and away from one that can eat them (the kind's "schoolSepMult" to
	// "schoolRunMult"; the first is not used, the fish keeping to a fixed multiple)
	void setMultipliers(float theSeparation, float theAlignment, float theCohesionDivisor, float theTarget,
	                    float theChase, float theRun);
	// 0x41d060: how near a fish must be to another to count as its neighbour
	void setNeighbourDistance(float theDistance);
	// 0x41cff0: how near the nearest fish may come before a fish turns away (the least distance, and its square)
	void setSeparationDistance(float theDistance);
	float getSeparationDistance() const { return mSeparationDistance; }  // 0x41d050
	float getSeparationDistanceSquared() const { return mSeparationDistanceSquared; }  // 0x41d040
	float getAlignmentMultiplier() const { return mAlignmentMultiplier; }
	float getCohesionDivisor() const { return mCohesionDivisor; }
	float getTargetMultiplier() const { return mTargetMultiplier; }
	float getChaseMultiplier() const { return mChaseMultiplier; }
	float getRunMultiplier() const { return mRunMultiplier; }
	// 0x476e20: the centre of the school, as of the last count
	const engine::Point& getCentre() const { return mCentre; }
	// 0x41d080: where the school is swimming to
	engine::Point getDestination() const { return mDestination; }
	// 0x41d0a0: one of the school's fish has been eaten by the player; the school counts the others
	void onFishEaten() { mAliveCount--; }
	int getAliveCount() const { return mAliveCount; }
	// 0x41d7b0 and 0x41d5d0: a fish joins the school, and one leaves it (the school ends with its last)
	void addFish(FishBase* theFish);
	void removeFish(FishBase* theFish);
	// 0x41d0b0: the centre and the mean velocity of the school
	void recalculate();
	// 0x41d210: the school's fish, other than the fish, whose tail is nearest to its nose (null when it has no other)
	FishBase* findNearest(FishBase* theFish);
	// 0x41d350: the mean velocity (zero when it is slow) and the mean position of the others near a fish
	void averageNeighbours(FishBase* theFish, engine::Point& theVelocity, engine::Point& thePosition);
	// 0x41d540: the school swims off to a side of the stage, for a good while
	void resetDestination();
	// 0x41d650: a frame; the school picks a new destination now and then
	void update(engine::UpdateInfo& theInfo);

	std::vector<FishBase*> mFish;           // +0x4
	engine::Point mCentre;                  // +0x10
	engine::Point mMeanVelocity;            // +0x18
	float mSeparationDistance;              // +0x20: 20
	float mSeparationDistanceSquared;       // +0x24: 400
	float mNeighbourDistance;               // +0x28
	float mNeighbourDistanceSquared;        // +0x2c
	engine::Point mDestination;             // +0x30
	float mDestinationSeconds;              // +0x38: until it picks another
	engine::Point mDestinationSecondsRange; // +0x3c
	float mSeparationMultiplier;            // +0x44
	float mAlignmentMultiplier;             // +0x48
	float mCohesionDivisor;                 // +0x4c
	float mTargetMultiplier;                // +0x50
	float mChaseMultiplier;                 // +0x54
	float mRunMultiplier;                   // +0x58
	int mAliveCount;                        // +0x5c: its fish that the player has not eaten
};

} // namespace feedingfrenzy

#endif
