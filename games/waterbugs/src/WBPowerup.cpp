// The board's panel (0x418050..cb:0x4183b0), the power-ups (cb:0x4183b0..0x419c00) and the weapons some of them give
// (0x419c10..0x41b340). A power-up falls from where an enemy died; when the ship touches it, it shows its name and
// changes the ship or the game, or gives the ship a weapon to orbit it. A click launches the first orbiting weapon.

#include "WBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstring>
#include <new>

namespace wb
{

namespace
{

int gPanelPercent = -1;            // cb:0x460a38  what the panel last drew
int gPanelLives = -1;              // cb:0x460a34
int gPanelScore = -1;              // cb:0x460a30
int gPowerupKinds[0x96];           // 0x6e5c08  a power-up's kind by a random index
int gRebuildKinds = 1;             // cb:0x460a48  (never cleared: each power-up builds the table again)

// cb:0x4183b0: ten entries for each kind from 0 to 9, 0 for the indexes past 99 but six more of 10 at 110..115 (the
// test for kinds 15 and 16 never holds)
// 0x418810
void Powerup_BuildKinds()
{
	for (uint32_t i = 0; (int)i < 0x96; i++)
	{
		uint32_t aKind = i / 10;
		gPowerupKinds[i] = aKind;
		if (0x81 < (int)i)
			gPowerupKinds[i] = 0;
		if ((aKind == 9 || aKind == 8) && (i & 1) == 0)
			gPowerupKinds[i] = 0;
	}
	gPowerupKinds[140] = 0xd;
	gPowerupKinds[141] = 0xd;
	gPowerupKinds[142] = 0xd;
	gPowerupKinds[143] = 0xd;
	gPowerupKinds[144] = 0xd;
	gPowerupKinds[145] = 0xd;
}

// The effects. 0x4188a0: a faster beam (by the beams' starting speed)
void Powerup_FastShot()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->beamSpeed = (float)(((double)gBeamSpeed - (double)gDifficulty * (double)0.05f) + (double)aShip->beamSpeed);
}

// 0x4188d0: half the speed, but not under half the starting speed
void Powerup_SlowShot()
{
	Shooter* aShip = gLevels.current->shooters.current;
	float aMin = (float)(((double)gBeamSpeed - (double)gDifficulty * (double)0.05f) * 0.5);
	float aHalf = (float)((double)aShip->beamSpeed * 0.5);
	if (aHalf < aMin)
	{
		aShip->beamSpeed = aMin;
		return;
	}
	aShip->beamSpeed = aHalf;
}

// 0x418920
void Powerup_ExtraLife()
{
	gLives = gLives + 1;
}

// 0x418930: a cell scores one more
void Powerup_Multiplier()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->cellPoints = aShip->cellPoints + 1;
}

// 0x418950
void Powerup_SlowMotion()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 1;
	gLevels.current->shooters.current->unk490 = 5000;
}

// 0x418980
void Powerup_HyperMotion()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 2;
	gLevels.current->shooters.current->unk494 = 5000;
}

// 0x4189b0
void Powerup_Sweepers()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 4;
}

// 0x4189d0
void Powerup_FlipFlopper()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 8;
}

// 0x4189f0: explode all floaters: the countdowns of the enemies of kinds 0..6 are set to 300, 600 and so on
void Powerup_ExplodeFloaters()
{
	int aDelay = 300;
	for (Enemy* anEnemy = gLevels.current->enemies.head; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
	{
		if (-1 < anEnemy->kind && anEnemy->kind < 7)
		{
			anEnemy->countdown = aDelay;
			aDelay = aDelay + 300;
		}
	}
}

// 0x418a30: explode all walkers (kinds 7..0xc)
void Powerup_ExplodeWalkers()
{
	int aDelay = 300;
	for (Enemy* anEnemy = gLevels.current->enemies.head; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
	{
		if (6 < anEnemy->kind && anEnemy->kind < 0xd)
		{
			anEnemy->countdown = aDelay;
			aDelay = aDelay + 300;
		}
	}
}

// 0x418a70
void Powerup_Invincibubble()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 0x10;
}

// 0x418a90: a new weapon at the tail, made current
void Weapon_Append(List<Weapon>* theList)
{
	Weapon* aWeapon = (Weapon*)::operator new(sizeof(Weapon));
	Obj_Init(aWeapon); // 0x40ad50
	List_Append(theList, aWeapon);
}

void Weapon_Setup(Weapon* theWeapon, float x, float y, float theRadius, int theKind);

// 0x418b80, 0x418bd0, 0x418c20, 0x418c70, 0x418cc0, 0x418d10: a weapon where the power-up is
void Powerup_GiveWeapon(Powerup* thePowerup, float theRadius, int theKind)
{
	Weapon_Append(&gLevels.current->weapons);
	Weapon_Setup(gLevels.current->weapons.current, thePowerup->x, thePowerup->y, theRadius, theKind);
}

// 0x4198a0: shows its name (removing any long or main_font text still up) and takes effect
int Powerup_Collect(Powerup* thePowerup)
{
	gProfiles[gProfile].powerupsCaught = gProfiles[gProfile].powerupsCaught + 1;
	List<Text>* aTexts = &gLevels.current->texts;
	List_Each(aTexts, [&](Text* aText) {
		if (0xd < std::strlen(aText->text) || aText->font == Font_Find("main_font"))
			List_RemoveCurrent(aTexts);
	});
	Text_Append(aTexts);
	Text_Make(aTexts->current, thePowerup->text, Font_Find("main_font"), 400, 0xe6, 0, 200, 2000, 200, 0, 0);
	const char* aSound;
	switch (thePowerup->kind)
	{
	case 1:
		Powerup_FlipFlopper();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 2:
		Powerup_SlowMotion();
		break;
	case 3:
		Powerup_GiveWeapon(thePowerup, 50.0f, 5);
		break;
	case 4:
		Powerup_GiveWeapon(thePowerup, 50.0f, 4);
		break;
	case 5:
		Powerup_SlowShot();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 6:
		Powerup_Invincibubble();
		break;
	case 7:
		Powerup_GiveWeapon(thePowerup, 50.0f, 6);
		break;
	case 8:
		Powerup_ExplodeWalkers();
		break;
	case 9:
		Powerup_ExplodeFloaters();
		break;
	case 10:
		Powerup_Multiplier();
		break;
	case 0xb:
		Powerup_HyperMotion();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 0xc:
		Powerup_GiveWeapon(thePowerup, 40.0f, 1);
		break;
	case 0xd:
		Powerup_ExtraLife();
		break;
	default:
		Powerup_FastShot();
		break;
	case 0xf:
		Powerup_Sweepers();
		break;
	case 0x10:
		Powerup_GiveWeapon(thePowerup, 50.0f, 0);
		break;
	case 0x11:
		Powerup_GiveWeapon(thePowerup, 40.0f, 3);
		break;
	}
	aSound = "audio\\powerup.wav";
play:
	Sound_Play(Sound_Find(aSound));
	return 1;
}

// Powerup_Init's emitters, about its middle
void Powerup_AddSparkles(Powerup* thePowerup, const char* theAnim)
{
	Emitter_Append(&thePowerup->emitters);
	int y = RandRange(-8, 8) + thePowerup->image->h / 2;
	int x = RandRange(-8, 8) + thePowerup->image->w / 2;
	Emitter_Setup(static_cast<Emitter*>(thePowerup->emitters.current), thePowerup, x, y, 30.0f, 30.0f, 0x5a, 0,
		theAnim, 1, 255.0f, 255.0f, 1200.0f);
}

// 0x419ae0: falls; collected when it touches the ship, gone below the screen
int Powerup_Fall(Powerup* thePowerup)
{
	thePowerup->vy = thePowerup->gravity + thePowerup->vy;
	Obj_Update(thePowerup);
	if (thePowerup->attached != nullptr)
	{
		Obj_Update(thePowerup->attached);
		Obj* anAttached = thePowerup->attached;
		double aShift = (double)anAttached->image->w * (double)anAttached->vx;
		anAttached->x = (float)((aShift + aShift) / 3.0 + (double)thePowerup->x);
		thePowerup->attached->y = thePowerup->y;
	}
	Shooter* aShip = gLevels.current->shooters.current;
	if (aShip != nullptr && aShip->state == 3 &&
		CircleOverlap(thePowerup->x, thePowerup->y, (float)(thePowerup->image->h / 2), aShip->x, aShip->y,
			(float)(aShip->image->h / 4)) != 0.0)
	{
		Powerup_Collect(thePowerup);
		return 0;
	}
	float y = thePowerup->y;
	int h = thePowerup->image->h;
	Obj_UpdateEmitters(thePowerup);
	return y <= (float)(h + 600);
}

// The weapons. 0x419e40: a missile's animation by the way it points
void Missile_Face(Weapon* theWeapon)
{
	if (theWeapon->kind == 1)
	{
		if (theWeapon->across == 0)
		{
			if (0.0f < theWeapon->vy)
			{
				theWeapon->anim = Animation_Find("missile_down", &theWeapon->frameCount, &theWeapon->frameTime);
				return;
			}
			theWeapon->anim = Animation_Find("missile_up", &theWeapon->frameCount, &theWeapon->frameTime);
			return;
		}
		if (0.0f < theWeapon->vx)
		{
			theWeapon->anim = Animation_Find("missile_left", &theWeapon->frameCount, &theWeapon->frameTime);
			return;
		}
		theWeapon->anim = Animation_Find("missile_right", &theWeapon->frameCount, &theWeapon->frameTime);
	}
}

int Sign(float x)
{
	if (x == 0.0f)
		return 0;
	if (x <= 0.0f)
		return -1;
	return 1;
}

// 0x419ee0 (the magnet): off cells that are not open, a third of its size ahead; it slows only over open cells
int Weapon_Bounce(Weapon* theWeapon)
{
	int aCol = (int)theWeapon->x / 16;
	int aRow = (int)theWeapon->y / 16;
	Level* aLevel = gLevels.current;
	if (aLevel->columns[aCol][aRow].kind == 0)
	{
		Image* anImage = theWeapon->image;
		int x = (int)((double)(anImage->w * Sign(theWeapon->vx) / 3) + (double)theWeapon->x);
		if (aLevel->columns[x / 16][aRow].kind != 0)
		{
			theWeapon->vx = -theWeapon->vx;
			aLevel = gLevels.current;
		}
		int y = (int)((double)(anImage->h * Sign(theWeapon->vy) / 3) + (double)theWeapon->y);
		if (aLevel->columns[aCol][y / 16].kind != 0)
			theWeapon->vy = -theWeapon->vy;
		theWeapon->drag = 0.996f;
		// (its own cell was open above, so this never holds)
		if (gLevels.current->columns[aCol][aRow].kind != 0)
		{
			theWeapon->vx = -theWeapon->vx;
			theWeapon->vy = -theWeapon->vy;
			return 0;
		}
	}
	else
		theWeapon->drag = 1.0f;
	return 0;
}

// 0x41a060 (the anchor): settles in the middle of its cell and makes that cell and the four beside it edges
int Anchor_Drop(Weapon* theWeapon)
{
	int aCol = (int)theWeapon->x / 16;
	int aRow = (int)theWeapon->y / 16;
	theWeapon->x = (float)(aCol * 0x10 + 8);
	theWeapon->y = (float)(aRow * 0x10 + 8);
	for (int dc = -1; dc < 2; dc++)
	{
		for (int dr = -1; dr < 2; dr++)
		{
			if ((dr == 0 || dc == 0) && 0 < dc + aCol && dc + aCol < 0x31 && 0 < dr + aRow && dr + aRow < 0x24)
			{
				Cell* aCell = &gLevels.current->columns[aCol + dc][aRow + dr];
				if (aCell->kind == 0)
					gLevels.current->totalCells = gLevels.current->totalCells - 1;
				aCell->kind = 3;
				aCell->image = Animation_Frame(gLevels.current->cornerTiles, 0);
				Cell_Redraw(aCell);
			}
		}
	}
	return 0;
}

// 0x41a1a0 (the magnet): fades out over its last 1000 updates, pulling the moving enemies it touches a 300th of
// the way to it each update
int Magnet_Pull(Weapon* theWeapon)
{
	if (9000.0f < theWeapon->age)
	{
		if (theWeapon->age >= 10000.0f)
			return 0;
		theWeapon->alpha = (int)((10000.0 - (double)theWeapon->age) * 255.0 / 1000.0);
	}
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (anEnemy->state == 3 &&
			CircleOverlap(theWeapon->x, theWeapon->y, (float)theWeapon->image->w, anEnemy->x, anEnemy->y,
				(float)anEnemy->image->w) != 0.0)
		{
			if (anEnemy->vx != 0.0f)
				anEnemy->x = (float)(((double)theWeapon->x - (double)anEnemy->x) / 300.0 + (double)anEnemy->x);
			if (anEnemy->vy != 0.0f)
				anEnemy->y = (float)(((double)theWeapon->y - (double)anEnemy->y) / 300.0 + (double)anEnemy->y);
		}
	});
	return 1;
}

// 0x41a2f0 (a mine or a missile): blows up the enemies it touches; 0 when it did
int Weapon_Blast(Weapon* theWeapon)
{
	int aResult = 1;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (anEnemy->state == 3 &&
			CircleOverlap(theWeapon->x, theWeapon->y, (float)(theWeapon->image->w / 3), anEnemy->x, anEnemy->y,
				(float)(anEnemy->image->w / 3)) != 0.0)
		{
			anEnemy->dieAnim = Animation_Find("second_explo", &anEnemy->dieFrameCount, &anEnemy->dieFrameTime);
			Enemy_Kill(anEnemy, 500);
			aResult = 0;
		}
	});
	return aResult;
}

// 0x41a430 (an ice shot): freezes the enemies it touches (0 when it did), shedding the odd blue sparkle
int Ice_Freeze(Weapon* theWeapon)
{
	int aResult = 1;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (anEnemy->state == 3 &&
			CircleOverlap(theWeapon->x, theWeapon->y, (float)(theWeapon->image->w / 3), anEnemy->x, anEnemy->y,
				(float)(anEnemy->image->w / 3)) != 0.0)
		{
			aResult = 0;
			Enemy_Freeze(anEnemy);
		}
	});
	if (theWeapon->kind == 3 && RandRange(1, 0x23) == 1)
	{
		ParticleSystem_New();
		int aParticle = theWeapon->particle;
		int aLifetime = RandRange(600, 800);
		float aVy = (float)((double)RandRange(-0x32, 0x32) / 400.0);
		float aVx = (float)((double)RandRange(-0x32, 0x32) / 400.0);
		float y = (float)RandRange(-5, 5) + theWeapon->y;
		float x = (float)RandRange(-5, 5) + theWeapon->x;
		ParticleSystem_Add(gLevels.current->systems.current, x, y, aVx, aVy, aParticle, aParticle, 0, 200, 200, 0xff,
			0x58, 0x58, 0xff, 400, 10, aLifetime, 0.999f, 0.0002f, 5);
	}
	return aResult;
}

// 0x41a650 (a time bomb): sparks fly off the top of the fuse, which burns down, hissing every 230 updates; at the
// end it blows up the enemies within 100 pixels and then itself
int TimeBomb_Update(Weapon* theWeapon)
{
	double anEnd = (double)theWeapon->lifetime - 1.0;
	if ((double)theWeapon->age < anEnd)
	{
		if (RandRange(1, 10) == 1)
		{
			double aBase = (double)theWeapon->y - (double)(theWeapon->image->h / 2);
			int aFuse = (int)(9.0 - (double)theWeapon->age * 8.0 / (double)theWeapon->lifetime);
			int aTop = (int)(aBase - (double)aFuse);
			ParticleSystem_New();
			int aParticle = theWeapon->particle;
			int aLifetime = RandRange(400, 500);
			float aVy = (float)((double)RandRange(-0x32, 0x32) / 1200.0);
			float aVx = (float)((double)RandRange(-0x32, 0x32) / 1200.0);
			ParticleSystem_Add(gLevels.current->systems.current, (float)((double)theWeapon->x - 0.0), (float)aTop, aVx,
				aVy, aParticle, aParticle, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 400, 10, aLifetime, 0.999f, 0.0003f, 5);
		}
		if ((int)theWeapon->age % 0xe6 == 0)
		{
			Sound_Play(theWeapon->fuseSound);
			return 1;
		}
	}
	else if ((double)theWeapon->age == anEnd)
	{
		List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
			if (anEnemy->state == 3 &&
				CircleOverlap(theWeapon->x, theWeapon->y, 100.0f, anEnemy->x, anEnemy->y,
					(float)(anEnemy->image->w / 3)) != 0.0)
			{
				anEnemy->dieAnim = Animation_Find("second_explo", &anEnemy->dieFrameCount, &anEnemy->dieFrameTime);
				Enemy_Kill(anEnemy, 500);
			}
		});
		Obj_Explode(theWeapon, 0.0f, 0.0f);
		theWeapon->lifetime = 0.0f;
		theWeapon->age = 0.0f;
	}
	return 1;
}

// 0x41a910: a click sends it straight out at its speed, the way it is from the ship (unless an enemy is too close)
void Weapon_Launch(Weapon* theWeapon)
{
	if (theWeapon->tooClose == 0)
	{
		theWeapon->vx = (float)Sign(theWeapon->vx) * theWeapon->launchSpeed;
		theWeapon->state = 4;
		theWeapon->vy = (float)Sign(theWeapon->vy) * theWeapon->launchSpeed;
		theWeapon->age = 0.0f;
		int aKind = theWeapon->kind;
		// (each sound is panned by x: the port's Sound_Play takes no pan)
		if (aKind != 1)
		{
			const char* aSound;
			if (aKind == 3)
				aSound = "audio\\ice_launch.wav";
			else if (aKind == 0 || aKind == 5)
				aSound = "audio\\mine.wav";
			else
			{
				if (aKind == 4)
				{
					Sound_Play(Sound_Find("audio\\mine.wav"));
					theWeapon->lifetime = 2000.0f;
					return;
				}
				if (aKind != 6)
					return;
				aSound = "audio\\anchor.wav";
			}
			Sound_Play(Sound_Find(aSound));
			return;
		}
		Sound_Play(Sound_Find("audio\\missile.wav"));
		List<Obj>* anEmitters = &theWeapon->emitters;
		Emitter_Append(anEmitters);
		Emitter* anEmitter = static_cast<Emitter*>(anEmitters->current);
		if (0.0f < theWeapon->vx)
		{
			Emitter_Setup(anEmitter, theWeapon, -6, 0x15, 0.0f, 11.0f, 0x14, 0, "smoke", 1, 100.0f, 10.0f, 400.0f);
			return;
		}
		if (theWeapon->vx < 0.0f)
		{
			Emitter_Setup(anEmitter, theWeapon, 0x2a, 0x15, 0.0f, 11.0f, 0x14, 0, "smoke", 1, 100.0f, 10.0f, 400.0f);
			return;
		}
		if (0.0f < theWeapon->vy)
		{
			Emitter_Setup(anEmitter, theWeapon, 0x15, -6, 11.0f, 0.0f, 0x14, 0, "smoke", 1, 100.0f, 10.0f, 400.0f);
			return;
		}
		if (theWeapon->vy < 0.0f)
		{
			Emitter_Setup(anEmitter, theWeapon, 0x15, 0x2a, 11.0f, 0.0f, 0x14, 0, "smoke", 1, 100.0f, 10.0f, 400.0f);
			return;
		}
	}
}

// 0x41ac20 (launched): slows by its drag and does its work; 0 when it is done or off the screen
int Weapon_Move(Weapon* theWeapon)
{
	int aResult = 1;
	theWeapon->vx = theWeapon->drag * theWeapon->vx;
	theWeapon->vy = theWeapon->vy * theWeapon->drag;
	switch (theWeapon->kind)
	{
	case 0:
		aResult = Weapon_Blast(theWeapon);
		break;
	case 1:
		aResult = Weapon_Blast(theWeapon);
		break;
	case 3:
		aResult = Ice_Freeze(theWeapon);
		break;
	case 4:
		aResult = TimeBomb_Update(theWeapon);
		break;
	case 5:
		Weapon_Bounce(theWeapon);
		aResult = Magnet_Pull(theWeapon);
		break;
	case 6:
		aResult = Anchor_Drop(theWeapon);
		break;
	}
	Obj_UpdateEmitters(theWeapon);
	int w = theWeapon->image->w;
	if ((float)(kScreenClip[0] - w) <= theWeapon->x && theWeapon->x <= (float)(w + kScreenClip[2]))
	{
		int h = theWeapon->image->h;
		if ((float)(kScreenClip[1] - h) <= theWeapon->y && theWeapon->y <= (float)(h + kScreenClip[3]))
			return aResult;
	}
	return 0;
}

// 0x41ad30: a new weapon orbits opposite the first one already there; any second one goes. It ends current.
void Weapon_TakeSlot(Weapon* theWeapon)
{
	int aCount = 0;
	if (gLevels.current->shooters.current->beamAcross == 0)
		theWeapon->angle = 0x5a;
	else
		theWeapon->angle = 0;
	List<Weapon>* aWeapons = &gLevels.current->weapons;
	if (aWeapons->current == nullptr)
	{
		aWeapons->current = theWeapon;
		return;
	}
	List_Each(aWeapons, [&](Weapon* aWeapon) {
		if (aWeapon->state == 3 && aWeapon != theWeapon)
		{
			if (aCount == 0)
				theWeapon->angle = (aWeapon->angle + 0xb4) % 0x168;
			else
				List_RemoveCurrent(aWeapons);
			aCount = aCount + 1;
		}
	});
	aWeapons->current = theWeapon;
}

// 0x41ae10
void Weapon_Setup(Weapon* theWeapon, float x, float y, float theRadius, int theKind)
{
	theWeapon->x = x;
	theWeapon->tooClose = 0;
	theWeapon->y = y;
	theWeapon->kind = theKind;
	theWeapon->state = 3;
	theWeapon->radius = theRadius;
	theWeapon->depth = 3;
	theWeapon->shadow = 1;
	theWeapon->alpha = 0xff;
	theWeapon->across = gLevels.current->shooters.current->beamAcross;
	switch (theKind)
	{
	case 0:
		theWeapon->anim = Animation_Find("shooter_mine", &theWeapon->frameCount, &theWeapon->frameTime);
		theWeapon->launchSpeed = 0.15f;
		theWeapon->drag = 0.996f;
		break;
	case 1:
		Missile_Face(theWeapon);
		theWeapon->launchSpeed = 0.15f;
		theWeapon->drag = 1.002f;
		break;
	case 3:
		theWeapon->anim = Animation_Find("ice_shot", &theWeapon->frameCount, &theWeapon->frameTime);
		theWeapon->launchSpeed = 0.2f;
		theWeapon->drag = 1.001f;
		theWeapon->particle = Bank_IndexOf(&gArtBank, "particle_0");
		break;
	case 4:
	{
		Shooter* aShip = gLevels.current->shooters.current;
		theWeapon->anim = Animation_Find("shooter_timebomb", &theWeapon->frameCount, &theWeapon->frameTime);
		theWeapon->launchSpeed = 0.15f;
		theWeapon->drag = 0.996f;
		theWeapon->dieAnim = aShip->dieAnim;
		theWeapon->dieFrameCount = aShip->dieFrameCount;
		theWeapon->dieFrameTime = aShip->dieFrameTime;
		theWeapon->particle = Bank_IndexOf(&gArtBank, "particle_0");
		theWeapon->exploSound = Sound_Find("audio\\explosion1.wav");
		theWeapon->fuseSound = Sound_Find("audio\\fuse.wav");
		break;
	}
	case 5:
		theWeapon->anim = Animation_Find("shooter_magnet", &theWeapon->frameCount, &theWeapon->frameTime);
		theWeapon->launchSpeed = 0.35f;
		theWeapon->drag = 0.996f;
		break;
	case 6:
		theWeapon->anim = Animation_Find("anchor", &theWeapon->frameCount, &theWeapon->frameTime);
		theWeapon->launchSpeed = 0.0f;
		theWeapon->drag = 1.001f;
		theWeapon->particle = Bank_IndexOf(&gArtBank, "particle_0");
		theWeapon->shadow = 0;
		break;
	}
	theWeapon->dirty = 1;
	theWeapon->drawKind = 4;
	theWeapon->midHandle = 1;
	Weapon_TakeSlot(theWeapon);
}

// 0x41b080 (orbiting): at its angle and radius about the ship, a quarter turn more each time the ship turns;
// faint while an enemy touches it or when it is not the first, which a click launches
void Weapon_Orbit(Weapon* theWeapon)
{
	theWeapon->tooClose = 0;
	theWeapon->alpha = 0xff;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (CircleOverlap(theWeapon->x, theWeapon->y, (float)(theWeapon->image->w / 2), anEnemy->x, anEnemy->y,
				(float)(anEnemy->image->w / 2)) != 0.0 &&
			theWeapon->kind != 4)
			theWeapon->tooClose = 1;
	});
	Shooter* aShip = gLevels.current->shooters.current;
	if (theWeapon->tooClose == 1 || gFirstWeapon == 0)
		theWeapon->alpha = 0x4b;
	else
		theWeapon->alpha = aShip->alpha;
	if (theWeapon->across != aShip->beamAcross)
		theWeapon->angle = (theWeapon->angle + 0x5a) % 0x168;
	theWeapon->vx = CosDeg(theWeapon->angle) * theWeapon->radius;
	float aVy = SinDeg(theWeapon->angle) * theWeapon->radius;
	theWeapon->vy = aVy;
	theWeapon->y = aVy + gLevels.current->shooters.current->y;
	theWeapon->x = gLevels.current->shooters.current->x + theWeapon->vx;
	Missile_Face(theWeapon);
	if (gLevels.current->clicked != 0 && gFirstWeapon != 0)
	{
		Weapon_Launch(theWeapon);
		gLevels.current->clicked = 0;
	}
	gFirstWeapon = 0;
	theWeapon->across = gLevels.current->shooters.current->beamAcross;
}

} // namespace

// 0x418170: a new item at the tail, made current
void Panel_AppendItem(List<Obj>* theList)
{
	Obj* anItem = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(anItem);
	List_Append(theList, anItem);
}

void Panel_FromXml(Panel* thePanel, const char* p, const XmlTag* theTag)
{
	thePanel->percentX = Xml_GetInt(theTag, "percent_x");
	thePanel->percentY = Xml_GetInt(theTag, "percent_y");
	thePanel->livesX = Xml_GetInt(theTag, "lives_x");
	thePanel->livesY = Xml_GetInt(theTag, "lives_y");
	thePanel->scoreX = Xml_GetInt(theTag, "score_x");
	thePanel->scoreY = Xml_GetInt(theTag, "score_y");
	thePanel->levelX = Xml_GetInt(theTag, "level_x");
	thePanel->levelY = Xml_GetInt(theTag, "level_y");
	thePanel->percentFont = Font_Find(Xml_GetString(theTag, "percent_font"));
	thePanel->livesFont = Font_Find(Xml_GetString(theTag, "lives_font"));
	thePanel->scoreFont = Font_Find(Xml_GetString(theTag, "score_font"));
	thePanel->levelFont = Font_Find(Xml_GetString(theTag, "level_font"));
	Obj_FromXml(thePanel, p, theTag);
	thePanel->dirty = 0;
}

// 0x4182a0: the panel a level without a panel tag gets: the panel image along the bottom, the logo at its right and,
// in the level's menus, a quit button
void Panel_Setup(Panel* thePanel)
{
	thePanel->percentX = 0x50;
	thePanel->percentY = 0x224;
	thePanel->livesX = 0x1de;
	thePanel->livesY = 0x224;
	thePanel->scoreX = 0xbd;
	thePanel->scoreY = 0x224;
	thePanel->levelX = 0x163;
	thePanel->levelY = 0x224;
	thePanel->percentFont = Font_Find("percent_font");
	thePanel->livesFont = Font_Find("lives_font");
	thePanel->scoreFont = Font_Find("score_font");
	thePanel->levelFont = Font_Find("level_font");
	thePanel->x = 0.0f;
	thePanel->y = 520.0f;
	thePanel->dirty = 0;
	thePanel->anim = Animation_Find("panel", &thePanel->frameCount, &thePanel->frameTime);
	List<Obj>* anItems = &thePanel->items;
	Panel_AppendItem(anItems);
	anItems->current->anim = Animation_Find("water_panel", &anItems->current->frameCount, &anItems->current->frameTime);
	anItems->current->x = 656.0f;
	anItems->current->y = 531.0f;
	anItems->current->midHandle = 0;
	anItems->current->drawKind = 1;
	thePanel->drawKind = 0;
	Menu_Insert(&gLevels.current->menus);
	MenuItem_Insert(&gLevels.current->menus.current->items);
	MenuItem* aQuit = gLevels.current->menus.current->items.current;
	aQuit->x = 549.0f;
	std::strcpy(aQuit->name, "aaa");
	aQuit->y = 567.0f;
	aQuit->font = Font_Find("main_font");
	aQuit->anim = Animation_Find("quit", &aQuit->frameCount, &aQuit->frameTime);
	aQuit->midHandle = 1;
	aQuit->drawKind = 1;
	aQuit->dirty = 1;
	aQuit->kind = 0x21;
	aQuit->firstParticle = Bank_IndexOf(&gArtBank, "bubble01");
	aQuit->lastParticle = Bank_IndexOf(&gArtBank, "bubble02");
}

// (it returns 1)
void Panel_Update(Panel* thePanel)
{
	List_Each(&thePanel->items, [](Obj* anItem) { Obj_Update(anItem); });
}

// 0x4184e0: into the back buffer when theAll is set or a number changed (the lives shown stop at 99, and so do the lives);
// its items onto the screen
void Panel_Draw(Panel* thePanel, int theAll)
{
	char aText[32];
	int aRect[4];
	int aPercent;
	Level* aLevel = gLevels.current;
	if (aLevel->totalCells == 0)
		aPercent = 0;
	else
	{
		aPercent = (aLevel->filledCells * 100) / aLevel->totalCells;
		if (100 < aPercent)
			aPercent = 100;
	}
	Obj_Update(thePanel);
	if (theAll != 0 || aPercent != gPanelPercent || gLives != gPanelLives || gScore != gPanelScore)
	{
		Image* anImage = thePanel->image;
		Image_BlitRect(anImage, gBackBuffer.bits, gBackBuffer.pitch, &gBackBuffer.fmt, (int)thePanel->x,
			(int)thePanel->y, 0, 0, anImage->w, anImage->h);
		std::sprintf(aText, "%03d", aPercent);
		Font_SetTarget(thePanel->percentFont, gBackBuffer.bits, gBackBuffer.pitch, &gBackBuffer.fmt, gBackBuffer.w,
			gBackBuffer.h);
		Font_Printf(thePanel->percentFont, aRect, thePanel->percentX, thePanel->percentY, aText);
		std::sprintf(aText, "%03d", gLevels.current->number);
		Font_SetTarget(thePanel->levelFont, gBackBuffer.bits, gBackBuffer.pitch, &gBackBuffer.fmt, gBackBuffer.w,
			gBackBuffer.h);
		Font_Printf(thePanel->levelFont, aRect, thePanel->levelX, thePanel->levelY, aText);
		int aScore = gScore;
		if (9999999 < gScore)
			aScore = 9999999;
		std::sprintf(aText, "%07d", aScore);
		Font_SetTarget(thePanel->scoreFont, gBackBuffer.bits, gBackBuffer.pitch, &gBackBuffer.fmt, gBackBuffer.w,
			gBackBuffer.h);
		Font_Printf(thePanel->scoreFont, aRect, thePanel->scoreX, thePanel->scoreY, aText);
		if (0x62 < gLives)
			gLives = 99;
		std::sprintf(aText, "%02d", gLives);
		Font_SetTarget(thePanel->livesFont, gBackBuffer.bits, gBackBuffer.pitch, &gBackBuffer.fmt, gBackBuffer.w,
			gBackBuffer.h);
		Font_Printf(thePanel->livesFont, aRect, thePanel->livesX, thePanel->livesY, aText);
		Dirty_AddClippedShown(&gDirty, 0, (int)thePanel->y, thePanel->image->w, thePanel->image->h, kScreenClip);
		gPanelLives = gLives;
		gPanelScore = gScore;
		gPanelPercent = aPercent;
	}
	List_Each(&thePanel->items, [](Obj* anItem) {
		if (anItem->image != nullptr)
		{
			Obj_Draw(anItem);
			Dirty_AddClippedImage(&gDirty, (int)anItem->x, (int)anItem->y, anItem->image, kScreenClip);
		}
	});
}

void Powerup_Draw(Powerup* thePowerup)
{
	Obj_Draw(thePowerup);
	if (thePowerup->attached != nullptr && thePowerup->state == 4)
		Obj_Draw(thePowerup->attached);
}

void List_DeleteNode(Powerup* thePowerup)
{
	// 0x418b20
	if (thePowerup->attached != nullptr)
	{
		Obj_Destroy(thePowerup->attached);
		::operator delete(thePowerup->attached);
	}
	Obj_Destroy(thePowerup);
	::operator delete(thePowerup);
}

// 0x418d60: a kind by the level (the later the level, the more kinds; no water mines or sweepers in an unregistered
// copy, only sweepers in the demo), and its text, animation and three emitters of sparkles. The keys 0..9 and A..D,
// held while it is made, choose kinds 0..13 (the developers' test). The table gives kinds 0..13 (0 a faster beam; the
// extra life is in its last six places), so nothing but the keys makes the sweepers, the water mine or the freeze (15..17).
Powerup* Powerup_Init(Powerup* thePowerup)
{
	Obj_Init(thePowerup);
	if (gRebuildKinds != 0)
		Powerup_BuildKinds();
	int aRange = gLevels.current->number / 4;
	if (0xe < aRange)
		aRange = 0xe;
	aRange = aRange * 10 + 9;
	thePowerup->kind = gPowerupKinds[RandRange(0, aRange)];
	while (gRegistered == 0 && (thePowerup->kind == 0xf || thePowerup->kind == 0x10))
		thePowerup->kind = gPowerupKinds[RandRange(0, aRange)];
	if (gDemo != 0)
	{
		thePowerup->kind = 0xf;
		gPumpMessages = 1;
	}
	std::sprintf(thePowerup->text, "^%d", RandRange(1, 8));
	if (Key_Down('0') != 0)
		thePowerup->kind = 0;
	if (Key_Down('1') != 0)
		thePowerup->kind = 1;
	if (Key_Down('2') != 0)
		thePowerup->kind = 2;
	if (Key_Down('3') != 0)
		thePowerup->kind = 3;
	if (Key_Down('4') != 0)
		thePowerup->kind = 4;
	if (Key_Down('5') != 0)
		thePowerup->kind = 5;
	if (Key_Down('6') != 0)
		thePowerup->kind = 6;
	if (Key_Down('7') != 0)
		thePowerup->kind = 7;
	if (Key_Down('8') != 0)
		thePowerup->kind = 8;
	if (Key_Down('9') != 0)
		thePowerup->kind = 9;
	if (Key_Down('A') != 0)
		thePowerup->kind = 10;
	if (Key_Down('B') != 0)
		thePowerup->kind = 11;
	if (Key_Down('C') != 0)
		thePowerup->kind = 12;
	if (Key_Down('D') != 0)
		thePowerup->kind = 13;
	// The texts and names are the exe's (each strcat'd from its .rdata)
	uint32_t aText;
	const char* aName;
	switch (thePowerup->kind)
	{
	case 1:
		aText = GTA(0x45992c);
		aName = "powerup_flip_flopper";
		break;
	case 2:
		aText = GTA(0x459a68);
		aName = "powerup_slowmo";
		break;
	case 3:
		aText = GTA(0x4599bc);
		aName = "powerup_magnet";
		break;
	case 4:
		aText = GTA(0x459950);
		aName = "powerup_timebomb";
		break;
	case 5:
		aText = GTA(0x459a2c);
		aName = "powerup_slow";
		break;
	case 6:
		aText = GTA(0x4598a4);
		aName = "powerup_invincibubble";
		break;
	case 7:
		aText = GTA(0x459884);
		aName = "powerup_anchor";
		break;
	case 8:
		aText = GTA(0x4598cc);
		aName = "powerup_destroywalkers";
		break;
	case 9:
		aText = GTA(0x4598fc);
		aName = "powerup_destroyrollers";
		break;
	case 10:
		aText = GTA(0x4599ec);
		aName = "powerup_2x";
		break;
	case 0xb:
		aText = GTA(0x459a48);
		aName = "powerup_hypermo";
		break;
	case 0xc:
		aText = GTA(0x4599a0);
		aName = "powerup_missile";
		break;
	case 0xd:
		aText = GTA(0x459a10);
		aName = "powerup_plusone";
		break;
	default:
		aText = GTA(0x459868);
		aName = "powerup_speed";
		break;
	case 0xf:
		aText = GTA(0x459984);
		aName = "powerup_sweeper";
		break;
	case 0x10:
		aText = GTA(0x4599d4);
		aName = "powerup_mine";
		break;
	case 0x11:
		aText = GTA(0x45996c);
		aName = "powerup_freeze";
		break;
	}

	std::strcat(thePowerup->text, GT(aText));
	thePowerup->state = 4;
	thePowerup->depth = 1;
	thePowerup->x = gLevels.current->enemies.current->x;
	thePowerup->y = gLevels.current->enemies.current->y;
	Animation* anAnim = Animation_Find(aName, &thePowerup->frameCount, &thePowerup->frameTime);
	thePowerup->anim = anAnim;
	thePowerup->image = anAnim->frames[0];
	thePowerup->drawKind = 1;
	thePowerup->dirty = 1;
	thePowerup->midHandle = 1;
	int aSide = Sign(thePowerup->x - 400.0f);
	thePowerup->shadow = 1;
	thePowerup->alpha = 0xff;
	thePowerup->vx = (float)aSide * -0.1f;
	thePowerup->vy = -0.2f;
	thePowerup->gravity = 0.0002f;
	List<Obj>* anEmitters = &thePowerup->emitters;
	Powerup_AddSparkles(thePowerup, "sparkles1");
	anEmitters->current->drawKind = 4;
	Powerup_AddSparkles(thePowerup, "sparkles3");
	Powerup_AddSparkles(thePowerup, "sparkles2");
	Emitter* aLast = static_cast<Emitter*>(anEmitters->current);
	aLast->drawKind = 5;
	aLast->startR = 200;
	aLast->startG = 200;
	aLast->startB = 0xff;
	aLast->endR = 200;
	aLast->endG = 200;
	aLast->endB = 0xff;
	aLast->variance = 0;
	thePowerup->attached = nullptr;
	// (the original then calls 0x401410, which is empty, while the flag at cb:0x460a40 is set)
	return thePowerup;
}

int Powerup_Update(Powerup* thePowerup)
{
	if (thePowerup->state == 4)
		return Powerup_Fall(thePowerup);
	return 0;
}

void Weapon_Draw(Weapon* theWeapon)
{
	Obj_Draw(theWeapon);
	if (theWeapon->kind == 4 && theWeapon->state == 4)
	{
		int aFuse = (int)(9.0 - (double)theWeapon->age * 8.0 / (double)theWeapon->lifetime);
		double aBase = (double)theWeapon->y - (double)(theWeapon->image->h / 2);
		int aTop = (int)(aBase - (double)aFuse);
		int aBottom = (int)aBase;
		int x = (int)theWeapon->x;
		Draw_Line(x, aTop, x, aBottom, 0x96, 0x96, 0x96, 0xff);
		x = (int)((double)theWeapon->x - 1.0);
		Draw_Line(x, aTop, x, aBottom, 0xdc, 0xdc, 0xdc, 0xff);
		Dirty_AddClippedShown(&gDirty, (int)((double)theWeapon->x - 1.0), aTop, 2, aFuse, kScreenClip);
	}
}

// To the weapon's right, or above it while the beams go across
void Weapon_DrawTooClose(Weapon* theWeapon)
{
	char aText[32];
	int aRect[4];
	// (the original sprintfs the text, which has no %)
	std::strcpy(aText, GT(0x459ab0));
	if (theWeapon->tooClose != 0)
	{
		Font* aFont = Font_Find("micro_font");
		Font_SetEscapes(aFont, 1);
		Font_SetTarget(aFont, gScreenBits, gScreenPitch, &gBackBuffer.fmt, gBackBuffer.w, gBackBuffer.h);
		Font_SetPalette(aFont, nullptr);
		aFont->alpha = 0xff;
		aFont->align = 0;
		int x;
		int y;
		if (gLevels.current->shooters.current->beamAcross == 0)
		{
			x = (int)((double)theWeapon->x + 20.0);
			y = (int)theWeapon->y;
		}
		else
		{
			x = (int)((double)theWeapon->x - (double)(Font_StringWidth(aFont, aText) / 2));
			y = (int)((double)theWeapon->y - 20.0);
		}
		Font_DrawOutlined(aFont, aRect, x, y, aText);
		int aHeight = Font_StringHeight(aFont, aText) + 2;
		Dirty_AddClippedShown(&gDirty, x - 1, y - 1, Font_StringWidth(aFont, aText) + 2, aHeight, kScreenClip);
	}
}

// 0 when it is done, and whenever the ship is dying
int Weapon_Update(Weapon* theWeapon)
{
	if (gLevels.current->shooters.current->state == 6)
		return 0;
	float x = theWeapon->x;
	float y = theWeapon->y;
	int aResult = Obj_Update(theWeapon);
	if (theWeapon->state == 3)
	{
		theWeapon->x = x;
		theWeapon->y = y;
		Weapon_Orbit(theWeapon);
	}
	else if (theWeapon->state == 4)
	{
		int aCol = (int)theWeapon->x / 16;
		if (-1 < aCol && aCol < 0x32)
		{
			int aRow = (int)theWeapon->y / 16;
			if (-1 < aRow && aRow < 0x25)
				return aResult & Weapon_Move(theWeapon);
		}
		return 0;
	}
	return aResult;
}

} // namespace wb
