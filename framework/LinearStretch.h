#ifndef __LINEARSTRETCH_H__
#define __LINEARSTRETCH_H__

// port: the renderer's present of a software-drawn frame, computed on the processor (docs/MEMORY.md, "A smooth stretch on the
// processor"). The renderer shows such a frame by putting it into the screen texture at the render scale R with the nearest
// filter (each pixel repeated R times each way) and copying that into the window's presentation rect with the linear filter
// (SurfaceDisplay's renderer mode, DDInterface::Present). This gives the same pixels straight from the frame: a table for the
// columns and one for the rows say, for each pixel of the presentation rect, which two pixels of the frame it falls between
// and the weight of the second, and the blend is done in the card's own arithmetic, so that the result is the card's to the
// byte. tools/stretchcal measured it on this machine's card (Direct3D 11, NVIDIA): the texture coordinate is (i + 0.5) times
// the setup's step (the rect's other side times 1/(width * height), in 32-bit floats), truncated to 21 fractional bits; the texel position is rounded half up
// to 8 fractional bits (the weight); where both axes blend, the product of the two weights is rounded half up to 8 bits and
// the other three weights follow from it; the weighted sum goes to 12 bits (times 257/4096) and then to 8 bits rounding half
// down. Where both texels are one pixel of the frame, as most are, the pixel is a copy; only the seams between the frame's
// pixels blend. Pure: no SDL, no window.

#include <stdint.h>
#include <vector>

namespace Sexy
{

class LinearStretch
{
public:
	LinearStretch();

	// a frame of theSrcWidth x theSrcHeight shown at render scale theRenderScale in a presentation rect of theOutWidth x
	// theOutHeight
	void Setup(int theSrcWidth, int theSrcHeight, int theRenderScale, int theOutWidth, int theOutHeight);
	bool IsSetFor(int theSrcWidth, int theSrcHeight, int theRenderScale, int theOutWidth, int theOutHeight) const;

	// the pixels of the presentation rect's columns theX0 to theX1 and rows theY0 to theY1 (ends excluded) from the frame
	// (thePixels, thePitch bytes a row, 0x??RRGGBB), into theOut (theOutPitch pixels a row; its first pixel is theX0, theY0)
	void Draw(const uint32_t* thePixels, int thePitch, int theX0, int theY0, int theX1, int theY1, uint32_t* theOut,
		int theOutPitch) const;

	// the columns (rows) of the presentation rect that read any of the frame's columns (rows) theSrc0 to theSrc1 (end
	// excluded): *theOut0 to *theOut1, end excluded, empty when none do
	void OutputColumns(int theSrc0, int theSrc1, int* theOut0, int* theOut1) const;
	void OutputRows(int theSrc0, int theSrc1, int* theOut0, int* theOut1) const;

	int OutWidth() const { return (int) mCols.size(); }
	int OutHeight() const { return (int) mRows.size(); }

private:
	struct Tap
	{
		int mA;       // the frame's pixel of the first texel
		int mB;       // of the second (mA when both are one pixel of the frame, or at the edge)
		int mWeight;  // the second's weight in 256ths, 0 to 255; 0 when mA == mB
	};
	// a run of output columns with the same taps (a frame pixel's copies, or one seam): mCount columns from mFirst
	struct Span
	{
		int mFirst;
		int mCount;
		Tap mTap;
	};
	static void MakeTaps(int theSrcSize, int theRenderScale, int theOutSize, int theOtherSize, std::vector<Tap>& theTaps);
	static void OutputRange(const std::vector<Tap>& theTaps, int theSrc0, int theSrc1, int* theOut0, int* theOut1);

	std::vector<Tap> mCols;
	std::vector<Tap> mRows;
	std::vector<Span> mColSpans;   // mCols in runs, left to right
	std::vector<int> mColSpanOf;   // the run each column is in
	int mSrcWidth;
	int mSrcHeight;
	int mRenderScale;
};

}

#endif
