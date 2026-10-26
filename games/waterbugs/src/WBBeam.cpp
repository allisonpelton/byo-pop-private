// The ship's beams and the beamriders that run back along them (cb:0x402850..0x4035d0)

#include "WBLevel.h"

#include <cmath>

namespace wb
{

void Beam_Setup(Beam* theBeam, Shooter* theOwner, float theX, float theY, float theVx, float theVy)
{
	theBeam->startX = theX;
	theBeam->owner = theOwner;
	theBeam->depth = 6;
	theBeam->startY = theY;
	theBeam->drawKind = 1;
	theBeam->hit = 0;
	theBeam->vx = theVx;
	theBeam->vy = theVy;
	const char* aName = nullptr;
	if (0.0f < theVx)
		aName = "shooter_beam_right";
	else if (theVx < 0.0f)
		aName = "shooter_beam_left";
	else if (0.0f < theVy)
		aName = "shooter_beam_down";
	else if (theVy < 0.0f)
		aName = "shooter_beam_up";
	if (aName != nullptr)
		theBeam->anim = Animation_Find(aName, &theBeam->frameCount, &theBeam->frameTime);
	// Water Bugs' beam has a harpoon at its head (unkcc), going the same way
	const char* aTip = nullptr;
	if (0.0f < theVx)
		aTip = "harpoon_right";
	else if (theVx < 0.0f)
		aTip = "harpoon_left";
	else if (0.0f < theVy)
		aTip = "harpoon_down";
	else if (theVy < 0.0f)
		aTip = "harpoon_up";
	if (aTip != nullptr)
		theBeam->unkcc.anim = Animation_Find(aTip, &theBeam->unkcc.frameCount, &theBeam->unkcc.frameTime);
	theBeam->unkcc.drawKind = 1;
	theBeam->unkcc.x = theBeam->x;
	theBeam->unkcc.midHandle = 1;
	theBeam->unkcc.dirty = 1;
	theBeam->unkcc.y = theBeam->y;
	theBeam->unkcc.depth = 7;
	theBeam->unkcc.vx = theBeam->vx;
	theBeam->unkcc.vy = theBeam->vy;
}

// It starts on the beam level with the enemy and runs the other way at twice the beam's speed
void Rider_Setup(Rider* theRider, Beam* theBeam, float theX, float theY, Enemy* theEnemy)
{
	if (theBeam->vx == 0.0f)
	{
		theRider->x = theBeam->x;
		theRider->y = theY;
	}
	else
	{
		theRider->y = theBeam->y;
		theRider->x = theX;
	}
	theRider->firstImage = Bank_IndexOf(&gArtBank, "particle_0");
	theRider->lastImage = Bank_IndexOf(&gArtBank, "particle_0");
	theRider->vx = -theBeam->vx * 2.0f;
	theRider->drawKind = 1;
	theRider->beam = theBeam;
	theRider->enemy = theEnemy;
	theRider->vy = -theBeam->vy * 2.0f;
	theRider->image = Bank_ImageAt(&gArtBank, Bank_IndexOf(&gArtBank, "beamrider"));
	Image_SetMidHandle(theRider->image);
	theRider->dirty = 1;
}

int Rider_Update(Rider* theRider)
{
	int aResult = 1;
	Obj_Update(theRider);
	if (0.0f <= theRider->x && theRider->x < 800.0f && 0.0f <= theRider->y && theRider->y <= 592.0f)
	{
		int aCol = (int)theRider->x;
		int aRow = (int)theRider->y;
		if (gLevels.current->columns[aCol / 16][aRow / 16].kind != 5)
		{
			if (RandRange(1, 10) == 1)
			{
				ParticleSystem_New();
				int aLifetime = RandRange(400, 500);
				int aLast = theRider->lastImage;
				int aFirst = theRider->firstImage;
				float aVy = (float)((double)RandRange(-50, 50) / 500.0);
				float aVx = (float)((double)RandRange(-50, 50) / 500.0);
				float aY = (float)RandRange(-5, 5) + theRider->y;
				float aX = (float)RandRange(-5, 5) + theRider->x;
				ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
					0xff, 0xff, 0xff, 0, 400, 10, aLifetime, 0.999f, 0.0002f, 5);
			}
			Shooter* aShooter = theRider->beam->owner;
			double aDy = (double)theRider->y - (double)aShooter->y;
			int aRadius = aShooter->image->w / 2;
			double aDx = (double)theRider->x - (double)aShooter->x;
			double aDx2 = aDx * aDx;
			double aDy2 = aDy * aDy;
			if (aDx2 + aDy2 < (double)(aRadius * aRadius))
			{
				aShooter = theRider->beam->owner;
				aResult = 0;
				if ((aShooter->flags & 0x10) != 0)
				{
					// (the original works out a pan from x for a play call that takes none)
					Sound_Play(Sound_Find("audio\\invincibubble.wav"));
					return 0;
				}
				float aHitY = (float)((double)theRider->vy / 12.0);
				float aHitX = (float)((double)theRider->vx / 6.0);
				Shooter_Hit(aShooter, aHitX, aHitY);
				if (gLives != 0 && gDemo == 0)
					gLives = gLives - 1;
				gProfiles[gProfile].riderHits = gProfiles[gProfile].riderHits + 1;
			}
			return aResult;
		}
	}
	return 0;
}

// From the start to the head, a frame of the beam's image every image length, clipped to the beam's rectangle
void Beam_DrawTo(Beam* theBeam, uint8_t* theDst, int theDPitch)
{
	int aRect[4];
	float aPos;
	Image* anImage;
	if (theBeam->x <= theBeam->startX)
	{
		if (theBeam->startX <= theBeam->x)
		{
			if (theBeam->startY < theBeam->y)
			{
				// down
				theBeam->image->handleY = 0;
				theBeam->image->handleX = theBeam->image->origW / 2;
				anImage = theBeam->image;
				aRect[0] = (int)((double)theBeam->startX - (double)(anImage->origW / 2));
				aRect[1] = (int)theBeam->startY;
				aRect[2] = (int)((double)(anImage->origW / 2) + (double)theBeam->startX);
				aRect[3] = (int)theBeam->y;
				for (aPos = theBeam->startY; aPos <= theBeam->y; aPos = (float)anImage->h + aPos)
				{
					Image_BlitBlend(anImage, theDst, theDPitch, &gBackBuffer.fmt, aRect, (int)theBeam->x, (int)aPos);
					anImage = theBeam->image;
				}
			}
			else
			{
				if (theBeam->startY <= theBeam->y)
					goto riders;
				// up
				theBeam->image->handleY = theBeam->image->origH + -1;
				theBeam->image->handleX = theBeam->image->origW / 2;
				anImage = theBeam->image;
				aRect[0] = (int)((double)theBeam->startX - (double)(anImage->origW / 2));
				aRect[1] = (int)theBeam->y;
				aRect[2] = (int)((double)(anImage->origW / 2) + (double)theBeam->startX);
				aRect[3] = (int)theBeam->startY;
				for (aPos = theBeam->startY; theBeam->y <= aPos; aPos = aPos - (float)anImage->h)
				{
					Image_BlitBlend(anImage, theDst, theDPitch, &gBackBuffer.fmt, aRect, (int)theBeam->x, (int)aPos);
					anImage = theBeam->image;
				}
			}
		}
		else
		{
			// left
			theBeam->image->handleX = theBeam->image->origW + -1;
			theBeam->image->handleY = theBeam->image->origH / 2;
			aRect[0] = (int)theBeam->x;
			anImage = theBeam->image;
			aRect[1] = (int)((double)theBeam->startY - (double)(anImage->origH / 2));
			aRect[2] = (int)theBeam->startX;
			aRect[3] = (int)((double)(anImage->origH / 2) + (double)theBeam->startY);
			for (aPos = theBeam->startX; theBeam->x <= aPos; aPos = aPos - (float)anImage->w)
			{
				Image_BlitBlend(anImage, theDst, theDPitch, &gBackBuffer.fmt, aRect, (int)aPos, (int)theBeam->y);
				anImage = theBeam->image;
			}
		}
	}
	else
	{
		// right
		theBeam->image->handleX = 0;
		theBeam->image->handleY = theBeam->image->origH / 2;
		aRect[0] = (int)theBeam->startX;
		anImage = theBeam->image;
		aRect[1] = (int)((double)theBeam->startY - (double)(anImage->origH / 2));
		aRect[2] = (int)theBeam->x;
		aRect[3] = (int)((double)(anImage->origH / 2) + (double)theBeam->startY);
		for (aPos = theBeam->startX; aPos <= theBeam->x; aPos = (float)anImage->w + aPos)
		{
			Image_BlitBlend(anImage, theDst, theDPitch, &gBackBuffer.fmt, aRect, (int)aPos, (int)theBeam->y);
			anImage = theBeam->image;
		}
	}
	Dirty_AddClippedRect(&gDirty, aRect, kScreenClip);
riders:
	List_Each(&theBeam->riders, [](Rider* aRider) { Obj_Draw(aRider); });
	if (theBeam->hit == 0)
		Obj_Draw(&theBeam->unkcc);
}

void Beam_Draw(Beam* theBeam)
{
	Beam_DrawTo(theBeam, gScreenBits, gScreenPitch);
}

static int Sign(float theValue)
{
	if (theValue == 0.0f)
		return 0;
	if (theValue <= 0.0f)
		return -1;
	return 1;
}

// 0x4031f0: the head moves on while the cell ahead is open; the first time it is not, the beam pops in a ring of
// particles; the harpoon (unkcc) follows it, four pixels behind
int Beam_Update(Beam* theBeam)
{
	int aResult = 1;
	Obj_Update(theBeam);
	int aHit = theBeam->hit;
	if (aHit != 0)
		theBeam->frameTimer = 0;
	// back to where it was, to look at the cell it moved into
	float aX = theBeam->x - theBeam->vx;
	theBeam->x = aX;
	float aY = theBeam->y - theBeam->vy;
	theBeam->y = aY;
	int aCol = (int)((double)theBeam->vx + (double)aX);
	int aRow = (int)((double)aY + (double)theBeam->vy);
	if (gLevels.current->columns[aCol / 16][aRow / 16].kind == 0)
	{
		theBeam->x = aX + theBeam->vx;
		theBeam->y = aY + theBeam->vy;
		// (Cosmic Bugs' beam sometimes shed a sparkle here; this one only takes the number)
		if (RandMod(0x28) == 2)
			RandRange(-50, 50);
	}
	else
	{
		if (aHit == 0)
		{
			Sound_Play(theBeam->owner->hitSound);
			for (int anAngle = 0; anAngle < 0x168; anAngle = anAngle + 0x14)
			{
				float aVx = (float)((double)CosDeg(anAngle) * 0.11999999731779099);
				float aVy = (float)((double)SinDeg(anAngle) * 0.11999999731779099);
				ParticleSystem_New();
				Cell* aCell = gLevels.current->columns[0];
				ParticleSystem_Add(gLevels.current->systems.current, theBeam->x, theBeam->y, aVx, aVy, aCell->firstParticle,
					aCell->lastParticle, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x21c, 10, 400, 0.999f, 0.0f, 5);
			}
		}
		theBeam->hit = 1;
		aResult = 0;
	}
	List<Rider>* aRiders = &theBeam->riders;
	List_Each(aRiders, [&](Rider* aRider) {
		if (Rider_Update(aRider) == 0)
			List_RemoveCurrent(aRiders);
	});
	// the tip follows the head, four pixels behind
	if (theBeam->hit == 0)
		Obj_Update(&theBeam->unkcc);
	theBeam->unkcc.x = (float)((double)theBeam->x - (double)Sign(theBeam->vx) * 4.0);
	theBeam->unkcc.y = (float)((double)theBeam->y - (double)Sign(theBeam->vy) * 4.0);
	return aResult;
}

// The ship's last beam going gives the ship back its moves and its beams' speed (at least the start's for the
// difficulty)
void Beam_Destroy(Beam* theBeam)
{
	if (theBeam->next == nullptr && theBeam->prev == nullptr)
	{
		gLevels.current->shooters.current->flags = 0;
		Shooter* aShooter = gLevels.current->shooters.current;
		double aSpeed = (double)gBeamSpeed - (double)gDifficulty * 0.05;
		if (aSpeed <= (double)aShooter->beamSpeed)
			aSpeed = aShooter->beamSpeed;
		aShooter->beamSpeed = (float)aSpeed;
	}
	Obj_Destroy(&theBeam->unkcc);
	List_Clear(&theBeam->riders);
	Obj_Destroy(theBeam);
}

void List_DeleteNode(Beam* theBeam)
{
	Beam_Destroy(theBeam);
	::operator delete(theBeam);
}

Beam* Beam_Init(Beam* theBeam)
{
	Obj_Init(theBeam);
	List_Init(&theBeam->riders);
	Obj_Init(&theBeam->unkcc);
	return theBeam;
}

void Rider_Append(List<Rider>* theList)
{
	Rider* aRider = (Rider*)::operator new(sizeof(Rider));
	Obj_Init(aRider);
	List_Append(theList, aRider);
}

// The ship's beam an object touches: a box twice its image's size over the divisors overlaps the beam's, and the
// beam passes within the image's width over theWDiv of it (the walk stops on that beam)
Beam* Obj_TouchedBeam(Obj* theObj, int theWDiv, int theHDiv)
{
	Shooter* aShip = gLevels.current->shooters.current;
	if (aShip != nullptr && aShip->beams.current != nullptr)
	{
		aShip->beams.current = aShip->beams.head;
		while (true)
		{
			Beam* aBeam = gLevels.current->shooters.current->beams.current;
			if (BoxOverlap(theObj->x, theObj->y, (float)((theObj->image->w * 2) / theWDiv),
					(float)((theObj->image->h * 2) / theHDiv), (float)(((double)aBeam->startX + aBeam->x) * 0.5),
					(float)(((double)aBeam->startY + aBeam->y) * 0.5), std::fabs(aBeam->startX - aBeam->x),
					std::fabs(aBeam->startY - aBeam->y)) != 0 &&
				SegmentDist(theObj->x, theObj->y, aBeam->startX, aBeam->startY, aBeam->x, aBeam->y) <
					(float)(theObj->image->w / theWDiv))
				return aBeam;
			aShip = gLevels.current->shooters.current;
			Beam* aNode = aShip->beams.current;
			if (aNode == nullptr)
				return nullptr;
			if (aNode->next == nullptr)
				break;
			aShip->beams.current = Node_Next(aNode);
		}
		aShip->beams.current = aShip->beams.head;
	}
	return nullptr;
}

} // namespace wb
