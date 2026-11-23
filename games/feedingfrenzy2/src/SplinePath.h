#ifndef __FEEDINGFRENZY2_SPLINEPATH_H__
#define __FEEDINGFRENZY2_SPLINEPATH_H__

// A smooth path through a list of nodes (SplinePath, 0x38 bytes, vtable 0x552bf4 (feedingfrenzy2); its constructor
// 0x493310 (feedingfrenzy2), over PathImpl<PathNodeSpline>, 0x493350 (feedingfrenzy2)), read from a <splinePath> file
// of <splineNode pos="x,y" sv="..."/> elements by a PathDocHandler. The game map spaces each level set's pins along
// one. The curve at a parameter blends the four nodes around it, each node's sv shaping it: a positive sv moves its
// knots apart, a negative one pulls the curve towards it. A parameter's whole part is the node a stretch starts from;
// the path also keeps, for each node, how far along the whole path every tenth of its stretch is, so a place can be
// asked for by the fraction of the path's length.
//
// The original keeps its x87 sums at the processor's 80 bits between the stores to floats; the port keeps them in
// doubles, which can differ by a float's last bit in a rare case.

#include "Geometry.h"
#include "MasterDocHandler.h"
#include "Object.h"

#include <vector>

// The tenths of a node's stretch the path measures at (0x434e10 and 0x434ed0 (feedingfrenzy2))
const int SPLINE_STRETCH_STEPS = 10;

// A <splineNode> (PathNodeSpline, 0x58 bytes; its constructor 0x415550 (feedingfrenzy2) zeroes +0x1c to +0x47)
struct SplineNode
{
	engine::Vector2f mPos;     // +0x0c
	float mShape;              // +0x14: the "sv" attribute, -1 when it has none
	float mStretchLength;      // +0x18: the length of the curve from this node's parameter to the next one's, 0 for the
	                           // last
	// +0x1c: the fraction of the whole path's length reached at this node's parameter and each tenth past it, up to
	// the next node's (1 at the last node's first)
	float mLengthFractions[SPLINE_STRETCH_STEPS + 1];

	SplineNode() : mShape(0), mStretchLength(0)
	{
		for (float& aFraction : mLengthFractions)
			aFraction = 0;
	}
};

class SplinePath : public engine::Object
{
public:
	SplinePath() : mLength(0) {}

	// 0x04 (0x47c0c0 (feedingfrenzy2)): the place at the fraction of the path's length, between the two tenths of a
	// stretch around it, the first node's at 0 or less (and for a path of no length), the last node's at 1 or more;
	// (0, 0) for no nodes
	engine::Vector2f getPositionAt(float theFraction) const;
	// 0x08 (0x489950 (feedingfrenzy2)): the node's place, (0, 0) past the last
	engine::Vector2f getNodePosition(int theIndex) const;
	// 0x10 (0x4246f0 (feedingfrenzy2))
	int getNodeCount() const { return (int)mNodes.size(); }
	// 0x14 (0x484820 (feedingfrenzy2)): the path measured again. (Its listeners at +0x0c, told after (0x41fc60
	// (feedingfrenzy2)), are not translated: the map's paths have none.)
	void finish() { mLength = measure(); }
	// 0x30 (0x4847d0 (feedingfrenzy2)): the node added at the end, then the path finished
	void addNode(const SplineNode& theNode);

private:
	// 0x38 (0x434ed0 (feedingfrenzy2)): each node's stretch measured and the fractions of the length filled in; the
	// whole length, 0 for no nodes
	float measure();
	// 0x40 (0x436540 (feedingfrenzy2)): the curve at the parameter, the first node's below 0 and the last node's
	// from the last node's parameter on
	engine::Vector2f evaluate(float theParam) const;
	// 0x44 (0x434e10 (feedingfrenzy2)): the length of the node's stretch, ten straight steps along it
	float measureStretch(int theIndex) const;

	std::vector<SplineNode> mNodes;  // +0x1c: Refs in the original
	float mLength;                   // +0x2c
};

// PathDocHandler::SplinePathDocHandler (0x5c bytes; its constructor 0x48b850 (feedingfrenzy2)): the nodes under a
// <splinePath> read into its path.
//
// The original's PathDocHandler (0x58 bytes; its constructor 0x48b9b0 (feedingfrenzy2)) is a MasterDocHandler that
// makes the path when a <splinePath> opens (0x47d360 (feedingfrenzy2)), puts this handler on its stack and finishes the
// path when it closes (0x480180 (feedingfrenzy2)); the port reads the file with a plain MasterDocHandler under this one,
// which finishes the path at the same close. (Its <linearPath>, a LinearPath read by a LinearPathDocHandler, is not
// translated: no file of the game's has one.)
class SplinePathDocHandler : public engine::SubDocHandler
{
public:
	explicit SplinePathDocHandler(SplinePath* thePath) : mPath(thePath) {}

	void begin(engine::Params& theAttributes) override {}
	// 0x00 (0x477240 (feedingfrenzy2)): a <splineNode> added
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// The PathDocHandler's 0x04 (0x480180 (feedingfrenzy2)): the path finished when the <splinePath> closes
	void endElement(const std::string& theName) override;

private:
	SplinePath* mPath;
};

// The level set's part of 0x461fd0 (feedingfrenzy2): the path read from the file, null when it cannot be
engine::Ref<SplinePath> loadSplinePath(const std::string& thePath);

#endif
