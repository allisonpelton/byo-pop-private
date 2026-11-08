#ifndef __TRIANGLEWRAP_H__
#define __TRIANGLEWRAP_H__

// port: texture addressing for DrawTrianglesTex. Direct3D's stage state (0xd and 0xe, "address u" and "address v") lets a
// game repeat a picture where a triangle's u or v leaves 0..1 (wrap) instead of taking the picture's edge (clamp); a
// game's triangle drawing that needs it (Bejeweled 2's Hyperspace tunnel) sets it with DDInterface::SetTextureAddress.
//
// SDL's renderer has no address mode and the GPU path draws images from a texture that may hold a padded or cut image, so
// neither can be told to wrap. Instead the triangles are cut at whole-number u and v: every piece lies in one cell of the
// repeating picture, and its coordinates are moved into 0..1, where clamping does what wrapping would. The pieces are
// drawn as they are by the display's own clamped drawing. Pure (no SDL, no GL), so that tools/wraptest drives it.

#include "Common.h"
#include "TriVertex.h"

#include <vector>

namespace Sexy
{

enum TextureAddress
{
	TEXTURE_ADDRESS_CLAMP = 0,  // the picture's edge, the default of every display
	TEXTURE_ADDRESS_WRAP = 1    // the picture repeats
};

// Appends the triangles of theVertices (theNumTriangles of three) to theOut, three vertices for each triangle, with
// those that cross a whole number of u (when wrapU) or of v (when wrapV) cut into pieces by that line, and every piece's
// u and v moved by a whole number so that it lies in 0..1. A vertex colour is interpolated along the cut edges; x, y, u
// and v the same. A triangle that spans more than MAX_WRAP_CELLS cells in u or v is appended as it is (clamped).
void SplitWrappedTriangles(const TriVertex theVertices[][3], int theNumTriangles, bool wrapU, bool wrapV,
	std::vector<TriVertex>& theOut);

const int MAX_WRAP_CELLS = 32;

} // namespace Sexy

#endif
