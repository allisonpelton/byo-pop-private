// The player's rank: 35 ranks by score, the picture that shows one, and the points to the next.

#include "AstroPopApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

// The score below which each rank holds (the original keeps them at 0x55dcf8); a score past the last is the last rank.
static const int kRankLimit[35] =
{
	25000, 75000, 125000, 175000, 250000, 325000, 400000, 475000, 550000, 650000, 750000, 850000, 1000000, 1150000,
	1300000, 1500000, 1700000, 2000000, 2300000, 2600000, 3000000, 3400000, 3800000, 4200000, 4600000, 5100000,
	5600000, 6100000, 6800000, 7500000, 8500000, 9500000, 10500000, 12000000, 15000000
};

// The names of the ranks are texts of the game (read from the player's exe by their address, like the others).
static const unsigned int kRankNameVA[35] =
{
	0x55dcb8, 0x55dcac, 0x55dc9c, 0x55dc94, 0x55dc8c, 0x55dc84, 0x55dc78, 0x55dc6c, 0x55dc60, 0x55dc50, 0x55dc3c,
	0x55dc2c, 0x55dc1c, 0x55dc08, 0x55dbfc, 0x55dbec, 0x55dbe0, 0x55dbd4, 0x55dbc4, 0x55dbb0, 0x55dba0, 0x55db94,
	0x55db84, 0x55db74, 0x55db5c, 0x55db4c, 0x55db3c, 0x55db2c, 0x55db1c, 0x55db0c, 0x55daf8, 0x55dae8, 0x55dad4,
	0x55dac0, 0x55dab0
};

// 0x40f110: the name of the rank of a score; theIndex (if given) gets the number of the rank
std::string AstroPopApp::GetRankName(int theScore, int* theIndex)
{
	for (int i = 0; i < 0x23; i++)
	{
		if (theScore < kRankLimit[i])
		{
			if (theIndex != NULL)
				*theIndex = i;
			return GT(kRankNameVA[i]);
		}
	}
	if (theIndex != NULL)
		*theIndex = 0x22;
	return GT(kRankNameVA[34]);
}

// 0x409210: the points a score lacks for the next rank, -1 at the highest rank
int AstroPopApp::PointsToNextRank(int theScore)
{
	for (int i = 0; i < 0x22; i++)
	{
		if (theScore < kRankLimit[i])
			return kRankLimit[i] - theScore;
	}
	return -1;
}

// 0x40c000: the picture of a rank: a strap, a centre picture by the rank's group of seven (star, sun, planet, atom,
// ship), small stars by the place in the group, wings for the last. With thePercent above 0 the picture is scaled to
// that many percent. The caller owns the image.
MemoryImage* AstroPopApp::MakeRankImage(int theRank, int thePercent)
{
	if (theRank > 0x22)
		theRank = 0x22;

	Image* aCenters[5] = {IMAGE_RANK_CENTER_STAR, IMAGE_RANK_CENTER_SUN, IMAGE_RANK_CENTER_PLANET, IMAGE_RANK_CENTER_ATOM,
		IMAGE_RANK_CENTER_SHIP};

	MemoryImage* anImage = new MemoryImage(this);
	anImage->Create(0xbe, 0xab);
	Graphics g(anImage);

	g.DrawImage(IMAGE_RANK_STRAP, anImage->mWidth / 2 - IMAGE_RANK_STRAP->mWidth / 2, 0);

	int aStep = theRank % 7;
	Image* aCenter = aCenters[theRank / 7];
	int aCenterX = 0x5e - aCenter->mWidth / 2;
	g.DrawImage(aCenter, aCenterX, 0x42);

	Image* aStar = IMAGE_RANK_SMALL_STAR;
	if (aStep == 1 || aStep == 3 || aStep == 5)
		g.DrawImage(aStar, 0x5e - aStar->mWidth / 2, aCenter->mHeight + 0x42);
	if (aStep == 2 || aStep == 3 || aStep == 6)
	{
		g.DrawImage(aStar, aCenterX - aStar->mWidth, (aCenter->mHeight / 2 - aStar->mHeight / 2) + 0x42);
		g.DrawImage(aStar, aCenter->mWidth + aCenterX, (aCenter->mHeight / 2 - aStar->mHeight / 2) + 0x42);
	}
	if (aStep == 4 || aStep == 5)
	{
		g.DrawImage(aStar, aCenterX - aStar->mWidth, 0x42 - aStar->mHeight / 2);
		g.DrawImage(aStar, aCenter->mWidth + aCenterX, 0x42 - aStar->mHeight / 2);
		g.DrawImage(aStar, aCenterX - aStar->mWidth, (aCenter->mHeight - aStar->mHeight / 2) + 0x42);
		g.DrawImage(aStar, aCenter->mWidth + aCenterX, (aCenter->mHeight - aStar->mHeight / 2) + 0x42);
	}
	if (aStep == 6)
		g.DrawImage(IMAGE_RANK_WINGS, 0x17, anImage->mHeight - IMAGE_RANK_WINGS->mHeight);

	if (thePercent <= 0)
		return anImage;

	// x87 at 24 bits (0x40c2cb): the scale and each size are rounded to a float, then truncated
	float aScale = (float) thePercent * 0.01f;
	MemoryImage* aScaled = new MemoryImage(this);
	aScaled->Create((int) ((float) anImage->mWidth * aScale), (int) ((float) anImage->mHeight * aScale));
	{
		Graphics aScaledGraphics(aScaled);
		Rect aSrc(0, 0, anImage->mWidth, anImage->mHeight);
		Rect aDest(0, 0, aScaled->mWidth, aScaled->mHeight);
		aScaledGraphics.DrawImage(anImage, aDest, aSrc);
	}
	delete anImage;
	return aScaled;
}
