// Small 3D helpers the challenge menu and the visualizers share: unit vectors, looking a transform from a place, and the
// listener that puts a WorldGraphicsListener around a DrawPrimitiveEx.
#pragma once

#include "Math3D.h"
#include "TwistD3D.h"

namespace Sexy {

// DrawPrimitiveEx's listener slots 0 and 1 are a WorldGraphicsListener's Apply and Remove (as in HyperspaceD3D.cpp)
class PrimitiveListener : public D3DDrawPrimitiveListener {
public:
	explicit PrimitiveListener(WorldGraphicsListener* theListener) : mListener(theListener) {}
	void Before() override { mListener->Apply(); }
	void After() override { mListener->Remove(); }
	WorldGraphicsListener* mListener;
};

Vec3 NormalizedVec(const Vec3& v);                  // 0x4c0840: a vector made unit (left alone when it has no length)
Vec3 CrossVec(const Vec3& a, const Vec3& b);        // 0x4c06d0: a crossed with b
float DotVec(const Vec3& a, const Vec3& b);         // 0x4c06a0
bool LookFrom(Transform3D& thePlace, const Vec3& theTarget, const Vec3& theUp);  // 0x674035 (see Camera3D.cpp)

}  // namespace Sexy
