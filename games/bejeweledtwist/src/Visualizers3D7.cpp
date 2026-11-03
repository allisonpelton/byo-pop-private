// VizJellyBlob (Visualizers3D.h). As in Visualizers3D3.cpp the original's float code runs on the x87 stack (rounded to
// float only where it stores, and to 53 bits, the CRT's default precision), so expressions are written in double where
// the original's are. A constant the export prints short in a float expression is a float the compiler widened (0.01 is
// 0.009999999776482582, 0.99 is 0.9900000095367432, read from the exe); 50.0, 255.0, 768.0 and 32767.0 are real doubles.
#include "Visualizers3D.h"

#include "Compat2004.h"
#include "DDImage.h"
#include "GameText.h"
#include "Graphics.h"
#include "Math3D.h"
#include "Res.h"
#include "TriVertex.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdint>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------------------
// Cell

// 0x684802: the heading's squared length (rounded to a float) is 0.0001 or less
bool VizJellyBlob::Cell::IsSlow() const
{
	float aSquare = (float)((double)mVelX * (double)mVelX + (double)mVelY * (double)mVelY);
	return (double)aSquare <= 9.999999747378752e-05;
}

// 0x6854e4: unless it is fixed, the point is carried by its pull and its heading (each step worked out as a float, both
// steps 0.01), the heading slowed to 0.99, and the pull is made again, 50 times the way to the mean of the neighbours (the
// sums are kept as floats, the neighbours as they stand: earlier points have moved)
void VizJellyBlob::Cell::Update()
{
	if (mFixed)
		return;
	double aStep = 0.009999999776482582;
	float aStepX = (float)((double)mAccX * aStep);
	float aStepY = (float)((double)mAccY * aStep);
	mVelX = (float)((double)mVelX + (double)aStepX);
	mVelY = (float)((double)aStepY + (double)mVelY);
	double aSlow = 0.9900000095367432;
	mVelX = (float)((double)mVelX * aSlow);
	mVelY = (float)(aSlow * (double)mVelY);
	aStepX = (float)((double)mVelX * aStep);
	aStepY = (float)(aStep * (double)mVelY);
	mX = (float)((double)mX + (double)aStepX);
	mY = (float)((double)mY + (double)aStepY);
	int aCount = (int)mNeighbors.size();
	if (aCount != 0) {
		int i = 0;
		float aSumX = 0.0f;
		float aSumY = 0.0f;
		if (0 < aCount) {
			do {
				const Cell* aNeighbor = mNeighbors[i];
				i++;
				aSumX = (float)((double)aNeighbor->mX + (double)aSumX);
				aSumY = (float)((double)aNeighbor->mY + (double)aSumY);
			} while (i < aCount);
		}
		float aCountFloat = (float)aCount;
		float aMeanX = (float)((double)aSumX / (double)aCountFloat);
		float aMeanY = (float)((double)aSumY / (double)aCountFloat);
		mAccX = (float)((double)aMeanX - (double)mX);
		mAccY = (float)((double)aMeanY - (double)mY);
		mAccX = (float)((double)mAccX * 50.0);
		mAccY = (float)(50.0 * (double)mAccY);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Droplet

// 0x686acb: the three curves are the exe's (the size, the turn's step and the alpha)
VizJellyBlob::Droplet::Droplet(Image* theImage, Cell* theCell) : mImage(theImage), mCell(theCell)
{
	mSizeCurve.SetCurve(GTS(0x7d81ac));
	mRotCurve.SetCurve(GTS(0x7d8184));
	mAlphaCurve.SetCurve(GTS(0x7d815c));
}

// 0x68484c: the life down by 0.005 (the float the exe holds); while there is some, the curves step and give the size, the
// turn (added to the turn so far) and the alpha
bool VizJellyBlob::Droplet::Update()
{
	float aLife = (float)((double)mLife - 0.004999999888241291);
	mLife = aLife;
	if (0.0 < (double)aLife) {
		mSizeCurve.IncInVal();
		mRotCurve.IncInVal();
		mAlphaCurve.IncInVal();
		mSize = (float)mSizeCurve.GetOutVal();
		mRot = (float)(mRotCurve.GetOutVal() + (double)mRot);
		mAlpha = (float)mAlphaCurve.GetOutVal();
	}
	return 0.0 < (double)aLife;
}

// 0x687e06: a square round the point, turned by the turn times pi/10, its half side the size times 50 and the scale; its
// colour white with the alpha (the top byte is 0xffffff less the int of alpha times -255, shifted up, as the exe works it
// out). Two triangles: corners 0, 1, 2 and 0, 2, 3.
void VizJellyBlob::Droplet::Build(std::vector<TriVertex>& theVertices, float theOffsetX, float theOffsetY, float theScale) const
{
	float aCenterX = (float)(((double)mCell->mX + (double)theOffsetX) * (double)theScale);
	float aCenterY = (float)(((double)mCell->mY + (double)theOffsetY) * (double)theScale);
	float aSize = (float)((double)mSize * (double)theScale * 50.0);
	int aAlpha = (int)((double)mAlpha * -255.0);  // 0x77e016, which truncates
	uint32_t aColor = 0xffffffu - ((uint32_t)aAlpha << 24);
	Mat3 aTurn;                                                                         // 0x4bd450
	aTurn.RotateZ((float)((double)mRot * 3.1415927410125732 * 0.10000000149011612));  // 0x673fd0
	Vec3 aRight = TimesMat3(Vec3{aSize, 0.0f, 0.0f}, aTurn);                            // 0x4c0c80
	Vec3 anUp = TimesMat3(Vec3{0.0f, aSize, 0.0f}, aTurn);
	float aBackX = (float)((double)aCenterX - (double)aRight.x);
	float aBackY = (float)((double)aCenterY - (double)aRight.y);
	float aFrontX = (float)((double)aRight.x + (double)aCenterX);
	float aFrontY = (float)((double)aRight.y + (double)aCenterY);
	TriVertex a0((float)((double)aBackX - (double)anUp.x), (float)((double)aBackY - (double)anUp.y), 0.0f, 0.0f, aColor);
	TriVertex a1((float)((double)aFrontX - (double)anUp.x), (float)((double)aFrontY - (double)anUp.y), 1.0f, 0.0f, aColor);
	TriVertex a2((float)((double)aFrontX + (double)anUp.x), (float)((double)aFrontY + (double)anUp.y), 1.0f, 1.0f, aColor);
	TriVertex a3((float)((double)aBackX + (double)anUp.x), (float)((double)aBackY + (double)anUp.y), 0.0f, 1.0f, aColor);
	theVertices.push_back(a0);
	theVertices.push_back(a1);
	theVertices.push_back(a2);
	theVertices.push_back(a0);
	theVertices.push_back(a2);
	theVertices.push_back(a3);
}

// ---------------------------------------------------------------------------------------------------------------------
// Blob

// 0x6855e7
VizJellyBlob::Cell* VizJellyBlob::Blob::GetCell(int theColumn, int theRow)
{
	if (-1 < theColumn && -1 < theRow && theRow < (int)mRows.size()) {
		if (theColumn < (int)mRows[theRow].size())
			return &mRows[theRow][theColumn];
	}
	return nullptr;
}

// 0x6880c6
void VizJellyBlob::Blob::AddTriangle(Cell* a, Cell* b, Cell* c)
{
	if (a != nullptr && b != nullptr && c != nullptr) {
		Triangle aTriangle;
		aTriangle.mCells[0] = a;
		aTriangle.mCells[1] = b;
		aTriangle.mCells[2] = c;
		mTriangles.push_back(aTriangle);
	}
}

// 0x6893ee: theRows rows, the even ones theColumns - 1 points long and the odd ones theColumns, the points theSpacing
// apart (the even rows shifted half a step) with their places in the picture. Each point is joined to the points beside
// it, above and below (the odd rows' neighbours slant the other way); a point with fewer than six is fixed. Then the
// triangles between the rows, in the order the exe adds them.
void VizJellyBlob::Blob::Make(int theColumns, int theRows, float theSpacing)
{
	mRows.clear();  // 0x6893c9
	mRows.resize(theRows);
	int i = 0;
	if (0 < theRows) {
		do {
			int aLength = theColumns;
			if ((i & 1) == 0)
				aLength = theColumns - 1;
			mRows[i].resize(aLength);  // 0x688f3e
			i++;
		} while (i < theRows);
	}
	i = 0;
	if (0 < theRows) {
		do {
			std::vector<Cell>& aRow = mRows[i];
			if (aRow.size() != 0) {
				int aPrevious = i - 1;
				int j = -1;  // the column before the current one
				unsigned int aNext = 0;
				do {
					int aColumn = j + 1;
					Cell* aCell = &aRow[aColumn];
					aCell->mX = (float)((double)(float)aColumn * (double)theSpacing);
					aCell->mY = (float)((double)(float)i * (double)theSpacing);
					Cell* aNeighbor = GetCell(j, i);
					if (aNeighbor != nullptr)
						aCell->mNeighbors.push_back(aNeighbor);
					aNeighbor = GetCell(j + 2, i);
					if (aNeighbor != nullptr)
						aCell->mNeighbors.push_back(aNeighbor);
					aNeighbor = GetCell(aColumn, aPrevious);
					if (aNeighbor != nullptr)
						aCell->mNeighbors.push_back(aNeighbor);
					aNeighbor = GetCell(aColumn, i + 1);
					if (aNeighbor != nullptr)
						aCell->mNeighbors.push_back(aNeighbor);
					if ((i & 1) == 0) {
						aNeighbor = GetCell(j + 2, aPrevious);
						if (aNeighbor != nullptr)
							aCell->mNeighbors.push_back(aNeighbor);
						aNeighbor = GetCell(j + 2, i + 1);
						if (aNeighbor != nullptr)
							aCell->mNeighbors.push_back(aNeighbor);
						aCell->mX = (float)((double)theSpacing * 0.5 + (double)aCell->mX);
					} else {
						aNeighbor = GetCell(j, aPrevious);
						if (aNeighbor != nullptr)
							aCell->mNeighbors.push_back(aNeighbor);
						aNeighbor = GetCell(j, i + 1);
						if (aNeighbor != nullptr)
							aCell->mNeighbors.push_back(aNeighbor);
					}
					if (aCell->mNeighbors.size() < 6)
						aCell->mFixed = true;
					aCell->mU = (float)((double)aCell->mX / (double)(float)((double)(float)(theColumns - 1) * (double)theSpacing));
					aCell->mV = (float)((double)aCell->mY / (double)(float)((double)(float)(theRows - 1) * (double)theSpacing));
					aNext = (unsigned int)(j + 2);
					j = aColumn;
				} while (aNext < aRow.size());
			}
			i++;
		} while (i < theRows);
	}
	i = 0;
	if (0 < theRows) {
		do {
			std::vector<Cell>& aRow = mRows[i];
			if (aRow.size() != 0) {
				int aBelow = i + 1;
				unsigned int j = 1;
				bool aMore;
				do {
					Cell* aLeft = &aRow[j - 1];
					Cell* aCorner;
					Cell* aSecond;
					Cell* aThird;
					if ((i & 1) == 0) {
						aThird = GetCell((int)j, aBelow);
						aSecond = GetCell((int)j, i);
						AddTriangle(aLeft, aSecond, aThird);
						aThird = GetCell((int)j - 1, aBelow);
						aSecond = GetCell((int)j, aBelow);
						aCorner = aLeft;
					} else {
						aThird = GetCell((int)j - 1, aBelow);
						aSecond = GetCell((int)j, i);
						AddTriangle(aLeft, aSecond, aThird);
						aThird = GetCell((int)j - 1, aBelow);
						aSecond = GetCell((int)j, aBelow);
						aCorner = GetCell((int)j, i);
					}
					AddTriangle(aCorner, aSecond, aThird);
					aMore = j < aRow.size();
					j++;
				} while (aMore);
			}
			i++;
		} while (i < theRows);
	}
}

// 0x68818d: a point that has all but stopped is found (up to 9 tries: a row, then a point in it) and given a push of
// thePower in a random direction; a droplet is made at it, with one of the pictures
void VizJellyBlob::Blob::Pulse(float thePower)
{
	Cell* aCell = nullptr;
	int aTries = 10;
	for (;;) {
		if (aCell != nullptr) {
			if (aCell->IsSlow())
				break;
		}
		aTries--;
		if (aTries < 1)
			break;
		unsigned int aRowCount = (unsigned int)mRows.size();
		std::vector<Cell>& aRow = mRows[(unsigned int)CrtRand() % aRowCount];
		unsigned int aCellCount = (unsigned int)aRow.size();
		aCell = &aRow[(unsigned int)CrtRand() % aCellCount];
	}
	int aBits = CrtRand() & 0x7fff;
	float aX = (float)(((double)aBits / 32767.0 - 0.5) * 2.0);
	aBits = CrtRand() & 0x7fff;
	float aY = (float)(((double)aBits / 32767.0 - 0.5) * 2.0);
	// 0x673b6f: made unit (the length is rounded to a float, as 0x618620 does), left as it is when it has none
	float aSquare = (float)((double)aX * (double)aX + (double)aY * (double)aY);
	float aLength = (float)std::sqrt((double)aSquare);
	if (aLength != 0.0f) {
		float aUnitX = (float)((double)aX / (double)aLength);
		float aUnitY = (float)((double)aY / (double)aLength);
		aX = aUnitX;
		aY = aUnitY;
	}
	aX = (float)((double)aX * (double)thePower);
	aY = (float)((double)aY * (double)thePower);
	aCell->mVelX = (float)((double)aX + (double)aCell->mVelX);
	aCell->mVelY = (float)((double)aCell->mVelY + (double)aY);
	unsigned int anImageCount = (unsigned int)mOwner->mImages.size();
	Image* anImage = mOwner->mImages[(unsigned int)CrtRand() % anImageCount];
	mDroplets.push_back(Droplet(anImage, aCell));  // 0x686acb, 0x687bc9
}

// 0x686831: every point in turn, then the droplets, those whose life has run out taken away
void VizJellyBlob::Blob::Update()
{
	int aRowCount = (int)mRows.size();
	int i = 0;
	if (0 < aRowCount) {
		do {
			std::vector<Cell>& aRow = mRows[i];
			int aCellCount = (int)aRow.size();
			int j = 0;
			if (0 < aCellCount) {
				do {
					aRow[j].Update();
					j++;
				} while (j < aCellCount);
			}
			i++;
		} while (i < aRowCount);
	}
	unsigned int aDroplet = 0;
	if (mDroplets.size() != 0) {
		do {
			if (!mDroplets[aDroplet].Update()) {
				mDroplets.erase(mDroplets.begin() + aDroplet);
				aDroplet--;
			}
			aDroplet++;
		} while (aDroplet < mDroplets.size());
	}
}

// 0x6880fc: every triangle's three corners as vertices at the point's place (moved by the offset, times the scale) with
// the point's place in the picture
void VizJellyBlob::Blob::BuildMesh(std::vector<TriVertex>& theVertices, float theOffsetX, float theOffsetY, float theScale)
{
	int aCount = (int)mTriangles.size();
	int t = 0;
	if (0 < aCount) {
		do {
			const Triangle& aTriangle = mTriangles[t];
			int k = 0;
			do {
				const Cell* aCell = aTriangle.mCells[k];
				float aX = (float)(((double)theOffsetX + (double)aCell->mX) * (double)theScale);
				float aY = (float)(((double)aCell->mY + (double)theOffsetY) * (double)theScale);
				theVertices.push_back(TriVertex(aX, aY, aCell->mU, aCell->mV));  // 0x67fa2f
				k++;
			} while (k < 3);
			t++;
		} while (t < aCount);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// VizJellyBlob

// 0x6896d9: IMAGE_JELLYBLOB_BG, then IMAGE_YINYANG2Z to 7Z, and the net (15 by 9, 100 apart)
VizJellyBlob::VizJellyBlob(WinTwistApp* theApp) : Visualizer(theApp)
{
	mCount = 0;
	mBackground = LoadVizImage(IMAGE_JELLYBLOB_BG_ID);
	mImageIds.push_back(IMAGE_YINYANG2Z_ID);
	mImageIds.push_back(IMAGE_YINYANG3Z_ID);
	mImageIds.push_back(IMAGE_YINYANG4Z_ID);
	mImageIds.push_back(IMAGE_YINYANG5Z_ID);
	mImageIds.push_back(IMAGE_YINYANG6Z_ID);
	mImageIds.push_back(IMAGE_YINYANG7Z_ID);
	unsigned int i = 0;
	if (mImageIds.size() != 0) {
		do {
			mImages.push_back(LoadVizImage(mImageIds[i]));
			i++;
		} while (i < mImageIds.size());
	}
	mBlob = new Blob(this);
	mBlob->Make(0xf, 9, 100.0f);
}

// 0x689907 (0x689830): the net first (0x6896c2), then IMAGE_JELLYBLOB_BG and the droplets' pictures
VizJellyBlob::~VizJellyBlob()
{
	if (mBlob != nullptr) {
		delete mBlob;
		mBlob = nullptr;
	}
	DeleteVizImage(IMAGE_JELLYBLOB_BG_ID);
	unsigned int i = 0;
	if (mImageIds.size() != 0) {
		do {
			DeleteVizImage(mImageIds[i]);
			i++;
		} while (i < mImageIds.size());
	}
}

// 0x6882b4 (Init, 0x6848c5, does nothing): the count; every 15th update the net is pulsed (power 100), then it moves
void VizJellyBlob::Update()
{
	mCount++;
	if (mBlob != nullptr) {
		if (mCount % 0xf == 0)
			mBlob->Pulse(100.0f);
		mBlob->Update();
	}
}

// 0x6882ea: the screen cleared to black, then, additive: the net's lines (half-white, from each point to each of its
// neighbours), then the net's triangles over IMAGE_JELLYBLOB_BG (normal blend, white), then each droplet's square
// (additive) with its picture. The places are the net's, moved 50 left and scaled to the screen.
void VizJellyBlob::Draw(Graphics* g)
{
	g->PushState();  // 0x41fd70
	gD3DInterface->Clear(Color::Black.mRed, Color::Black.mGreen, Color::Black.mBlue, Color::Black.mAlpha);  // slot 0x9c
	g->SetDrawMode(1);                           // 0x41ff20
	g->SetColor(Color(0xff, 0xff, 0xff, 0x80));  // 0x41fef0
	float aScale = (float)((double)g->mDestImage->mWidth / (double)mApp->mWidth);
	float aSize = (float)(((double)mApp->mArtRes / 768.0) * (double)aScale);
	float aOffsetX = -50.0f;
	float aOffsetY = 0.0f;
	int aRowCount = (int)mBlob->mRows.size();
	int i = 0;
	if (0 < aRowCount) {
		do {
			std::vector<Cell>& aRow = mBlob->mRows[i];
			int aCellCount = (int)aRow.size();
			int j = 0;
			if (0 < aCellCount) {
				do {
					Cell* aCell = &aRow[j];
					int aNeighborCount = (int)aCell->mNeighbors.size();
					int k = 0;
					if (0 < aNeighborCount) {
						do {
							Cell* aNeighbor = aCell->mNeighbors[k];
							int aEndY = (int)((double)aSize * ((double)aNeighbor->mY + 0.0));
							int aEndX = (int)((double)aSize * ((double)aNeighbor->mX - 50.0));
							int aStartY = (int)((double)aSize * ((double)aCell->mY + 0.0));
							int aStartX = (int)(((double)aCell->mX - 50.0) * (double)aSize);
							g->DrawLine(aStartX, aStartY, aEndX, aEndY);  // 0x421040
							k++;
						} while (k < aNeighborCount);
					}
					j++;
				} while (j < aCellCount);
			}
			i++;
		} while (i < aRowCount);
	}
	std::vector<TriVertex> aVertices;
	mBlob->BuildMesh(aVertices, aOffsetX, aOffsetY, aSize);
	unsigned int aVertexCount = (unsigned int)aVertices.size();
	gD3DInterface->DrawTrianglesTex(mBackground, reinterpret_cast<const TriVertex(*)[3]>(aVertices.data()), (int)(aVertexCount / 3),
		Color(0xff, 0xff, 0xff, 0xff), 0, 0.0f, 0.0f, true);  // slot 0x74
	int aDropletCount = (int)mBlob->mDroplets.size();
	i = 0;
	if (0 < aDropletCount) {
		do {
			const Droplet& aDroplet = mBlob->mDroplets[i];
			aVertices.clear();  // 0x6868f7
			aDroplet.Build(aVertices, aOffsetX, aOffsetY, aSize);
			aVertexCount = (unsigned int)aVertices.size();
			gD3DInterface->DrawTrianglesTex(aDroplet.mImage, reinterpret_cast<const TriVertex(*)[3]>(aVertices.data()),
				(int)(aVertexCount / 3), Color(0xff, 0xff, 0xff, 0xff), 1, 0.0f, 0.0f, true);
			i++;
		} while (i < aDropletCount);
	}
	g->PopState();  // 0x41fe00
}
