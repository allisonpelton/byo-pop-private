// The geometry the jewels' flight and the bumpers use: distances, the overlap of circles and boxes, and the reflection of a
// velocity off a wall or a ball. Its arithmetic is the original's, single precision except where it takes a square root,
// which the original does in double precision and rounds to a float where it stores the result.

#ifndef VENICE_GEOMETRY_H
#define VENICE_GEOMETRY_H

// 0x43a030: the distance between two points (the original's vectors have a third coordinate that is always zero here)
double PointDistance(float theX1, float theY1, float theX2, float theY2);

// 0x43a0b0 and its wrapper 0x43a240: the distance from a point to a line segment (to the nearer end when the point's
// projection falls beside the segment)
double SegmentDistance(float thePointX, float thePointY, float theStartX, float theStartY, float theEndX, float theEndY);

// 0x43a2d0: the unit vector at a right angle to a segment
void SegmentNormal(float theStartX, float theStartY, float theEndX, float theEndY, float* theNormalX, float* theNormalY);

// 0x43a360: a velocity that runs into a segment (the dot product with its normal is negative) is reflected off it
void ReflectOffSegment(float* theVelocityX, float* theVelocityY, float theStartX, float theStartY, float theEndX,
	float theEndY);

// 0x43a400 and 0x43a490: the unit vector from one point to another, and the reflection of a velocity that runs into a ball
void ReflectOffBall(float* theVelocityX, float* theVelocityY, float theX, float theY, float theBallX, float theBallY);

// 0x438e30: how much two circles overlap (the square of the sum of the radii less the square of the distance between the
// centres), zero when they are apart
float CircleOverlap(float theX1, float theY1, float theRadius1, float theX2, float theY2, float theRadius2);

// 0x438db0: whether two boxes given by their centres and sizes overlap
bool BoxesOverlap(float theX1, float theY1, float theWidth1, float theHeight1, float theX2, float theY2, float theWidth2,
	float theHeight2);

#endif
