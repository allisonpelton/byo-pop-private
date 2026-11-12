// The board's panel (0x417d40..0x4183b0), the power-ups (0x4183b0..0x419780) and the weapons some of them give
// (0x419790..0x41aec0). A power-up falls from where an enemy died; when the ship touches it, it shows its name and
// changes the ship or the game, or gives the ship a weapon to orbit it. A click launches the first orbiting weapon.

#include "CBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstring>
#include <new>

namespace cb
{

namespace
{

int gPanelPercent = -1;            // 0x460a38  what the panel last drew
int gPanelLives = -1;              // 0x460a34
int gPanelScore = -1;              // 0x460a30
int gPowerupKinds[0x78];           // 0x6e5c00  a power-up's kind by a random index
int gRebuildKinds = 1;             // 0x460a48  (never cleared: each power-up builds the table again)

// 0x417e60: a new item at the tail, made current
void Panel_AppendItem(List<Obj>* theList)
{
	Obj* anItem = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(anItem);
	List_Append(theList, anItem);
}

// 0x4183b0: ten entries for each kind from 0 to 9, 0 for the indexes past 99 but six more of 10 at 110..115
void Powerup_BuildKinds()
{
	for (uint32_t i = 0; (int)i < 0x78; i++)
	{
		gPowerupKinds[i] = i / 10;
		if (99 < (int)i)
			gPowerupKinds[i] = 0;
	}
	gPowerupKinds[110] = 10;
	gPowerupKinds[111] = 10;
	gPowerupKinds[112] = 10;
	gPowerupKinds[113] = 10;
	gPowerupKinds[114] = 10;
	gPowerupKinds[115] = 10;
}

// The effects. 0x418440: a faster beam (by the beams' starting speed)
void Powerup_FastShot()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->beamSpeed = (float)(((double)gBeamSpeed - (double)gDifficulty * (double)0.05f) + (double)aShip->beamSpeed);
}

// 0x418470: half the speed, but not under half the starting speed
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

// 0x4184c0
void Powerup_ExtraLife()
{
	gLives = gLives + 1;
}

// 0x4184d0: a cell scores one more
void Powerup_Multiplier()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->cellPoints = aShip->cellPoints + 1;
}

// 0x4184f0
void Powerup_SlowMotion()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 1;
	gLevels.current->shooters.current->unk490 = 5000;
}

// 0x418520
void Powerup_HyperMotion()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 2;
	gLevels.current->shooters.current->unk494 = 5000;
}

// 0x418550
void Powerup_Sweepers()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 4;
}

// 0x418570
void Powerup_FlipFlopper()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 8;
}

// 0x418610
void Powerup_Invincibubble()
{
	Shooter* aShip = gLevels.current->shooters.current;
	aShip->flags = aShip->flags | 0x10;
}

// 0x418630: a new weapon at the tail, made current; 0x40abe0 (a weapon's constructor, which only calls Obj_Init) is
// inlined here
void Weapon_Append(List<Weapon>* theList)
{
	Weapon* aWeapon = (Weapon*)::operator new(sizeof(Weapon));
	Obj_Init(aWeapon); // 0x40abe0
	List_Append(theList, aWeapon);
}

void Weapon_Setup(Weapon* theWeapon, float x, float y, float theRadius, int theKind);

// 0x418720, 0x4187c0, 0x418810, 0x418860: a weapon where the power-up is
void Powerup_GiveWeapon(Powerup* thePowerup, float theRadius, int theKind)
{
	Weapon_Append(&gLevels.current->weapons);
	Weapon_Setup(gLevels.current->weapons.current, thePowerup->x, thePowerup->y, theRadius, theKind);
}

// 0x419420: shows its name (removing any long or main_font text still up) and takes effect
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
		Powerup_SlowMotion();
		break;
	case 2:
		Powerup_Multiplier();
		break;
	case 3:
		Powerup_SlowShot();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 4:
		Powerup_GiveWeapon(thePowerup, 50.0f, 4);
		break;
	case 5:
		Powerup_HyperMotion();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 6:
		Powerup_GiveWeapon(thePowerup, 40.0f, 1);
		break;
	case 7:
		Powerup_GiveWeapon(thePowerup, 40.0f, 3);
		break;
	case 8:
		Powerup_GiveWeapon(thePowerup, 50.0f, 0);
		break;
	case 9:
		Powerup_Sweepers();
		break;
	case 10:
		Powerup_ExtraLife();
		break;
	default:
		Powerup_FastShot();
		break;
	case 0xc:
		Powerup_FlipFlopper();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 0xd:
		Powerup_Invincibubble();
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
	int y = RandRange(-8, 8) + thePowerup->image->mH / 2;
	int x = RandRange(-8, 8) + thePowerup->image->mW / 2;
	Emitter_Setup(static_cast<Emitter*>(thePowerup->emitters.current), thePowerup, x, y, 30.0f, 30.0f, 0x5a, 0,
		theAnim, 1, 255.0f, 255.0f, 1200.0f);
}

// 0x419660: falls; collected when it touches the ship, gone below the screen
int Powerup_Fall(Powerup* thePowerup)
{
	thePowerup->vy = thePowerup->gravity + thePowerup->vy;
	Obj_Update(thePowerup);
	Shooter* aShip = gLevels.current->shooters.current;
	if (aShip != nullptr && aShip->state == 3 &&
		CircleOverlap(thePowerup->x, thePowerup->y, (float)(thePowerup->image->mH / 2), aShip->x, aShip->y,
			(float)(aShip->image->mH / 4)) != 0.0)
	{
		Powerup_Collect(thePowerup);
		return 0;
	}
	float y = thePowerup->y;
	int h = thePowerup->image->mH;
	Obj_UpdateEmitters(thePowerup);
	return y <= (float)(h + 600);
}

// The weapons. 0x4199c0: a missile's animation by the way it points
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

// 0x419e70 (a mine or a missile): blows up the enemies it touches; 0 when it did
int Weapon_Blast(Weapon* theWeapon)
{
	int aResult = 1;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (anEnemy->state == 3 &&
			CircleOverlap(theWeapon->x, theWeapon->y, (float)(theWeapon->image->mW / 3), anEnemy->x, anEnemy->y,
				(float)(anEnemy->image->mW / 3)) != 0.0)
		{
			anEnemy->dieAnim = Animation_Find("second_explo", &anEnemy->dieFrameCount, &anEnemy->dieFrameTime);
			Enemy_Kill(anEnemy, 500);
			aResult = 0;
		}
	});
	return aResult;
}

// 0x419fb0 (an ice shot): freezes the enemies it touches (0 when it did), shedding the odd blue sparkle
int Ice_Freeze(Weapon* theWeapon)
{
	int aResult = 1;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (anEnemy->state == 3 &&
			CircleOverlap(theWeapon->x, theWeapon->y, (float)(theWeapon->image->mW / 3), anEnemy->x, anEnemy->y,
				(float)(anEnemy->image->mW / 3)) != 0.0)
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

// 0x41a1d0 (a time bomb): sparks fly off the top of the fuse, which burns down, hissing every 230 updates; at the
// end it blows up the enemies within 100 pixels and then itself
int TimeBomb_Update(Weapon* theWeapon)
{
	double anEnd = (double)theWeapon->lifetime - 1.0;
	if ((double)theWeapon->age < anEnd)
	{
		if (RandRange(1, 10) == 1)
		{
			double aBase = (double)theWeapon->y - (double)(theWeapon->image->mH / 2);
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
					(float)(anEnemy->image->mW / 3)) != 0.0)
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

// 0x41a490: a click sends it straight out at its speed, the way it is from the ship (unless an enemy is too close)
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
			else if (aKind == 0)
				aSound = "audio\\mine.wav";
			else
			{
				if (aKind == 4)
				{
					Sound_Play(Sound_Find("audio\\mine.wav"));
					theWeapon->lifetime = 2000.0f;
				}
				return;
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

// 0x41a7a0 (launched): slows by its drag and does its work; 0 when it is done or off the screen
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
	}
	Obj_UpdateEmitters(theWeapon);
	int w = theWeapon->image->mW;
	if ((float)(kScreenClip[0] - w) <= theWeapon->x && theWeapon->x <= (float)(w + kScreenClip[2]))
	{
		int h = theWeapon->image->mH;
		if ((float)(kScreenClip[1] - h) <= theWeapon->y && theWeapon->y <= (float)(h + kScreenClip[3]))
			return aResult;
	}
	return 0;
}

// 0x41a8b0: a new weapon orbits opposite the first one already there; any second one goes. It ends current.
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

// 0x41a990
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
	}
	theWeapon->dirty = 1;
	theWeapon->drawKind = 4;
	theWeapon->midHandle = 1;
	Weapon_TakeSlot(theWeapon);
}

// 0x41ac00 (orbiting): at its angle and radius about the ship, a quarter turn more each time the ship turns;
// faint while an enemy touches it or when it is not the first, which a click launches
void Weapon_Orbit(Weapon* theWeapon)
{
	theWeapon->tooClose = 0;
	theWeapon->alpha = 0xff;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (CircleOverlap(theWeapon->x, theWeapon->y, (float)(theWeapon->image->mW / 2), anEnemy->x, anEnemy->y,
				(float)(anEnemy->image->mW / 2)) != 0.0 &&
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

// The panel a level without a panel tag gets: the panel image along the bottom, the logo at its right
void Panel_Setup(Panel* thePanel)
{
	thePanel->percentX = 0x44;
	thePanel->percentY = 0x220;
	thePanel->livesX = 0x19b;
	thePanel->livesY = 0x220;
	thePanel->scoreX = 0xa8;
	thePanel->scoreY = 0x220;
	thePanel->levelX = 0x2c4;
	thePanel->levelY = 0x230;
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
	anItems->current->anim = Animation_Find("logo_o", &anItems->current->frameCount, &anItems->current->frameTime);
	anItems->current->x = 540.0f;
	anItems->current->y = 542.0f;
	anItems->current->midHandle = 1;
	anItems->current->drawKind = 1;
	thePanel->drawKind = 0;
}

// (it returns 1)
void Panel_Update(Panel* thePanel)
{
	List_Each(&thePanel->items, [](Obj* anItem) { Obj_Update(anItem); });
}

// Into the back buffer when theAll is set or a number changed (the lives shown stop at 9, and so do the lives);
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
		if (99 < aPercent)
			aPercent = 99;
	}
	Obj_Update(thePanel);
	if (theAll != 0 || aPercent != gPanelPercent || gLives != gPanelLives || gScore != gPanelScore)
	{
		Image* anImage = thePanel->image;
		Image_BlitRect(anImage, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, (int)thePanel->x,
			(int)thePanel->y, 0, 0, anImage->mW, anImage->mH);
		std::sprintf(aText, "%02d", aPercent);
		Font_SetTarget(thePanel->percentFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->percentFont, aRect, thePanel->percentX, thePanel->percentY, aText);
		std::sprintf(aText, "%03d", gLevels.current->number);
		Font_SetTarget(thePanel->levelFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->levelFont, aRect, thePanel->levelX, thePanel->levelY, aText);
		int aScore = gScore;
		if (999999 < gScore)
			aScore = 999999;
		std::sprintf(aText, "%06d", aScore);
		Font_SetTarget(thePanel->scoreFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->scoreFont, aRect, thePanel->scoreX, thePanel->scoreY, aText);
		if (8 < gLives)
			gLives = 9;
		std::sprintf(aText, "%01d", gLives);
		Font_SetTarget(thePanel->livesFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->livesFont, aRect, thePanel->livesX, thePanel->livesY, aText);
		Dirty_AddClippedShown(&gDirty, 0, (int)thePanel->y, thePanel->image->mW, thePanel->image->mH, kScreenClip);
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
}

void List_DeleteNode(Powerup* thePowerup)
{
	// 0x4186c0
	Obj_Destroy(thePowerup);
	::operator delete(thePowerup);
}

// A kind by the level (the later the level, the more kinds), and its text, animation and three emitters of sparkles.
// The keys 0..9 and A..D, held while it is made, choose kinds 0..13 (the developers' test). The table gives kinds
// 0..10 (0 a faster beam), so only those keys make 12 and 13. The original's kinds 14..17 (an anchor, two
// explode-alls and a magnet) nothing makes, and the port leaves them out (docs/README.md, "The unneeded code").
Powerup* Powerup_Init(Powerup* thePowerup)
{
	Obj_Init(thePowerup);
	if (gRebuildKinds != 0)
		Powerup_BuildKinds();
	int aRange = gLevels.current->number / 2;
	if (0xb < aRange)
		aRange = 0xb;
	aRange = aRange * 10 + 9;
	thePowerup->kind = gPowerupKinds[RandRange(0, aRange)];
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
		aText = GTA(0x4598d0);
		aName = "powerup_slowmo";
		break;
	case 2:
		aText = GTA(0x459854);
		aName = "powerup_2x";
		break;
	case 3:
		aText = GTA(0x459894);
		aName = "powerup_slow";
		break;
	case 4:
		aText = GTA(0x4597b8);
		aName = "powerup_timebomb";
		break;
	case 5:
		aText = GTA(0x4598b0);
		aName = "powerup_hypermo";
		break;
	case 6:
		aText = GTA(0x459808);
		aName = "powerup_missile";
		break;
	case 7:
		aText = GTA(0x4597d4);
		aName = "powerup_freeze";
		break;
	case 8:
		aText = GTA(0x45983c);
		aName = "powerup_mine";
		break;
	case 9:
		aText = GTA(0x4597ec);
		aName = "powerup_sweeper";
		break;
	case 10:
		aText = GTA(0x459878);
		aName = "powerup_plusone";
		break;
	default:
		aText = GTA(0x4596d0);
		aName = "powerup_speed";
		break;
	case 0xc:
		aText = GTA(0x459794);
		aName = "powerup_flip_flopper";
		break;
	case 0xd:
		aText = GTA(0x45970c);
		aName = "powerup_invincibubble";
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
	// (the original then calls 0x40fe10, which is empty, while the flag at 0x460a40 is set)
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
		double aBase = (double)theWeapon->y - (double)(theWeapon->image->mH / 2);
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
	std::strcpy(aText, GT(0x459918));
	if (theWeapon->tooClose != 0)
	{
		Font* aFont = Font_Find("micro_font");
		Font_SetEscapes(aFont, 1);
		Font_SetTarget(aFont, gScreenBits, gScreenPitch, &gBackBuffer.mFmt, gBackBuffer.mW, gBackBuffer.mH);
		Font_SetPalette(aFont, nullptr);
		aFont->mAlpha = 0xff;
		aFont->mAlign = 0;
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

} // namespace cb
