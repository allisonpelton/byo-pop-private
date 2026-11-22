#include "Intro.h"

#include "Anim.h"
#include "BugImgSkeeter.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "MenuButton.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

Anim* Intro::sCartAnim = nullptr;
Anim* Intro::sWalkAnim = nullptr;
Anim* Intro::sLookUpAnim = nullptr;
Anim* Intro::sTurnAnim = nullptr;
Anim* Intro::sDespairAnim = nullptr;
Anim* Intro::sIdolAnim = nullptr;
Anim* Intro::sWheelAnim = nullptr;
Anim* Intro::sBunchAnim = nullptr;
Anim* Intro::sFlyAnim = nullptr;
Anim* Intro::sFlareAnim = nullptr;
Anim* Intro::sTalk1Anim = nullptr;
Anim* Intro::sTalk2Anim = nullptr;
bool Intro::sReady = true;
bool Intro::sFinished = true;

// What a fly is doing (IntroBug::mPhase). A fly's flight ends with event 3, and what happens then depends on the phase.
enum IntroBugPhase
{
	BUG_WANDER = 0,        // flies about at random, waiting to be let go
	BUG_RELEASED = 1,      // let go: its next flight goes to its banana (the wheel, the cart)
	BUG_TO_LOOT = 2,       // on its way to its banana or the wheel; on arrival it takes it and flies off
	BUG_TO_CART = 3,       // on its way to hover over the cart; on arrival it rides on it
	BUG_ON_WHEEL = 4,      // on arrival it rides on the wheel (nothing sets this phase)
	BUG_LEAVING = 5,       // flying off the screen; nothing more happens
};

// The key events the intro's objects raise (AnimKey::mEventId)
enum IntroEvent
{
	INTRO_EVENT_CART_TILTED = 2,    // the cart's tilting track has ended: it drives off
	INTRO_EVENT_BUG_ARRIVED = 3,    // a fly's flight has ended
	INTRO_EVENT_MAN_WALKED = 4,     // the walk has ended (nothing reacts)
	INTRO_EVENT_MAN_LOOKED_UP = 5,  // the monkey has looked up: he turns
	INTRO_EVENT_MAN_TURNED = 6,     // he has turned: despair, and the first speech bubble
	INTRO_EVENT_MAN_DESPAIRED = 7,  // despair is over: he looks at the idol, the second bubble; the level may go on
	INTRO_EVENT_MAN_LOOKED = 8,     // he has looked: the flare
	INTRO_EVENT_FLARE_GROWN = 10,   // the flare has grown: the monkey goes
};

// The fly who takes the wheel is the sixteenth; the later ones go to the cart
const int INTRO_WHEEL_BUG = 15;
// Once this many flies are let go, the last five (the wheel's and the cart's) go too
const int INTRO_LAST_BUGS_AT = 13;

// The release: 670 frames before the first fly, then one in 1 to 4 frames
const int INTRO_FIRST_RELEASE = 0x29e;
const int INTRO_RELEASE_MASK = 3;     // the frames to the next are 1 plus a draw masked to this
// The frames in which a mouse press does not end the intro
const int INTRO_SKIP_DELAY = 100;

// Each fly starts its first flight 390 frames in, and the next one 19 frames after the one before
const int INTRO_BUG_DELAY_FIRST = 0x186;
const int INTRO_BUG_DELAY_STEP = 0x13;

// The frames a fly's key lasts when no flight shortens it
const int INTRO_FOR_EVER = 9999;

// Where the wheel and the cart are made, and the cart's keys: they roll in from the left
const int INTRO_WHEEL_Y = 0x20d;
const int INTRO_CART_Y = 0x218;
const int INTRO_CART_OFFSET_Y = -76;
const int INTRO_ROLL_FRAMES = 900;
const float INTRO_WHEEL_ROLL_FROM = -1161.0f;
const float INTRO_CART_ROLL_FROM = -1160.0f;
// The cart tips back by this much (15 degrees), the leaving flight goes this far left and up
const float INTRO_CART_TILT = 0.2617994f;
const double INTRO_CART_LEAVE_X = 1100.0;
const double INTRO_CART_LEAVE_Y = 300.0;

// The depth the intro itself is at, and its pieces
const int INTRO_Z = 0xdc;
const int INTRO_WHEEL_Z = 3;
const int INTRO_CART_Z = 2;
const int INTRO_MAN_Z = 0xe1;
const int INTRO_TALK_Z = 0x32;

// A banana: 5 to a row, 50 apart from -150, rows 30 apart from -126, each a thousandth nearer than the one before
const int INTRO_BANANA_COLUMNS = 5;
const int INTRO_BANANA_STEP_X = 50;
const int INTRO_BANANA_STEP_Y = 30;
const double INTRO_BANANA_FIRST_X = 150.0;
const double INTRO_BANANA_FIRST_Y = 126.0;
// The banana held by a fly hangs 32 below it
const int INTRO_BANANA_HANG = 0x20;

// The place a fly picks to wander to (design pixels from the middle)
const int INTRO_WANDER_WIDTH = 0x529;
const int INTRO_WANDER_LEFT = 0x294;
const int INTRO_WANDER_HEIGHT = 0x1f6;
const int INTRO_WANDER_TOP = 500;
// The flies' starting places: right or left of the screen, above it
const int INTRO_START_SPREAD = 0xfb;
const int INTRO_START_RIGHT = 1000;
const int INTRO_START_LEFT = 0x4e2;
const int INTRO_START_HEIGHT = 1000;
const int INTRO_START_TOP = 999;
// The place a fly picks to leave to: beyond the left or right edge, above the screen
const int INTRO_EXIT_SPREAD = 0xc9;
const int INTRO_EXIT_RIGHT = 1000;
const int INTRO_EXIT_LEFT = 0x4b0;
const int INTRO_EXIT_HEIGHT = 0x4b1;
const int INTRO_EXIT_TOP = 0x4b0;
// The wheel thief leaves for here
const float INTRO_WHEEL_EXIT = -700.0f;
// Around the wheel and the cart the thieves go to: a random place in this much around their middle
const int INTRO_WHEEL_AREA_WIDTH = 0xa9;
const int INTRO_WHEEL_AREA_LEFT = 0x54;
const int INTRO_WHEEL_AREA_HEIGHT = 0x5b;
const int INTRO_WHEEL_AREA_TOP = 0x4a;
const int INTRO_CART_AREA_WIDTH = 0xb9;
const int INTRO_CART_AREA_LEFT = 100;
const int INTRO_CART_AREA_HEIGHT = 0x6f;
const int INTRO_CART_AREA_TOP = 0x5e;
// The speed of a flight: frames a pixel of distance takes (the divisor, or the factor), for each kind of flight
const double INTRO_SPEED_WANDER = 9.0;       // divisor: a pixel is a ninth of a frame
const double INTRO_SPEED_TO_BANANA = 0.0625; // factor
const double INTRO_SPEED_TO_WHEEL = 0.25;
const double INTRO_SPEED_TO_CART = 14.0;     // divisor
const double INTRO_SPEED_LEAVE_WHEEL = 9.0;  // divisor
const double INTRO_SPEED_LEAVE = 0.0625;     // factor
// A fly's flight is lengthened by up to this many frames at random (the skeeter's kind is not)
const int INTRO_FLIGHT_SPREAD = 0x1a;
// The sound of a fly picking its next place: one time in 13 each, for two of the draws
const int INTRO_SOUND_CHANCE = 0xd;

// The man's key counts
const float INTRO_WALK_RATE = 30.0f;
const float INTRO_WALK_CELS = 40.0f;
const float INTRO_FLY_RATE = 60.0f;          // a fly's two wing cels
const float INTRO_FLY_CELS = 2.0f;

// The speech bubbles' places
const int INTRO_TALK1_X = -10;
const int INTRO_TALK1_Y = 0x136;
const int INTRO_TALK2_X = 0x14c;
const int INTRO_TALK2_Y = -400;
const int INTRO_TALK2_DELAY = 100;

// A key of the intro's tracks that shows no picture of its own
static AnimKey* MakeKey(int theFrames, int theEventId)
{
	AnimKey* aKey = new AnimKey();
	aKey->mEventId = theEventId;
	aKey->SetDuration(theFrames);
	return aKey;
}

// A key that shows one of the intro's pictures
static AnimKey* MakePictureKey(Anim* thePicture, int theFrames, int theEventId)
{
	AnimKey* aKey = new AnimKey();
	thePicture->AddRef();
	aKey->mAnim = thePicture;
	aKey->mEventId = theEventId;
	aKey->SetDuration(theFrames);
	return aKey;
}

// A key of a speech bubble: the three values (width, height and see-through) held, or going to the next key's
static AnimKey* MakeBubbleKey(int theFrames, float theValue, int theKind)
{
	AnimKey* aKey = MakeKey(theFrames, 0);
	aKey->AddValue(STATE_SCALE_X, theValue, theKind);
	aKey->AddValue(STATE_SCALE_Y, theValue, theKind);
	aKey->AddValue(STATE_ALPHA, theValue, theKind);
	return aKey;
}

// The distance between two places (the original adds the squares on the x87 and rounds once)
static float Distance(float theFromX, float theFromY, float theToX, float theToY)
{
	double aDeltaX = (double)theToX - (double)theFromX;
	double aDeltaY = (double)theToY - (double)theFromY;
	return FloatSqrt((float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY));
}

// 0x436460: the pictures, then the pieces (wheel, cart, bananas, monkey, flies, flare and the two bubbles), the wheel and
// the cart start to roll in and the hum starts; the mouse stays on the menu button's pointer until it ends
Intro::Intro()
{
	sReady = false;
	sFinished = false;
	LoadPictures();
	SetStateInt(STATE_Z, INTRO_Z);
	mSkipDelay = INTRO_SKIP_DELAY;
	mStolen = 0;
	mReleased = 0;
	mReleaseTimer = INTRO_FIRST_RELEASE;
	BuildWheel();
	BuildCart();
	BuildBananas();
	BuildMan();
	BuildBugs();
	BuildFlare();
	BuildTalk1();
	BuildTalk2();
	mWheel->StartTrack(mWheelRollTrack);
	mCart->StartTrack(mCartRollTrack);
	sApp->mSoundQueue->Request(SOUND_INTROHUM, 0, false);
	Activate(this, false);
	LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
	if (aLevel->mMenuButton != nullptr)
		aLevel->mMenuButton->SetKeepCursor(true);
}

// 0x436580 (the body 0x4365a0 is the base's)
Intro::~Intro()
{
}

// 0x439550
void Intro::LoadPictures()
{
	sCartAnim = Anim::Get("images/intro/intro_cart", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	sWalkAnim = Anim::Get("images/intro/cartoon_walk", ANIM_SCALE_SMOOTH, false, 0x61, 0x61, 0x28, false);
	sLookUpAnim = Anim::Get("images/intro/cartoon_lookup", ANIM_SCALE_SMOOTH, false, 0x61, 0x61, 8, false);
	sTurnAnim = Anim::Get("images/intro/cartoon_turn", ANIM_SCALE_SMOOTH, false, 0x61, 0x61, 0xc, false);
	sDespairAnim = Anim::Get("images/intro/cartoon_desperate", ANIM_SCALE_SMOOTH, false, 0x61, 0x61, 0xc, false);
	sIdolAnim = Anim::Get("images/intro/cartoon_lookidol", ANIM_SCALE_SMOOTH, false, 0x61, 0x61, 0xc, false);
	sWheelAnim = Anim::Get("images/intro/intro_wheel", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	sBunchAnim = Anim::Get("images/intro/intro_bunch", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	sFlyAnim = Anim::Get("images/intro/intro_fly", ANIM_SCALE_SMOOTH, false, 0x60, 0x5f, 2, false);
	sTalk1Anim = Anim::Get("images/intro/talk_01", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	sTalk2Anim = Anim::Get("images/intro/talk_02", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	sFlareAnim = Anim::Get("images/flare", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
}

// 0x436ad0: the last five flies (the wheel's thief and the four over the cart) are let go
void Intro::ReleaseLastBugs()
{
	for (int i = INTRO_WHEEL_BUG; i < INTRO_BUGS; i++)
		mBugs[i]->mPhase = BUG_RELEASED;
}

// 0x436b20: the wheel, a child of the intro: it rolls in from the left in 900 frames turning by its speed
void Intro::BuildWheel()
{
	mWheel = new ObjBase();
	sWheelAnim->AddRef();
	mWheel->mAnim = sWheelAnim;
	mWheel->SetParent(this, nullptr);
	mWheel->SetStateInt(STATE_X, 0);
	mWheel->SetStateInt(STATE_Y, INTRO_WHEEL_Y);
	mWheel->SetStateInt(STATE_Z, INTRO_WHEEL_Z);

	AnimKey* aRoll = MakeKey(INTRO_ROLL_FRAMES, 0);
	aRoll->AddModifier(STATE_ROTATION, false, 0.0f, MOD_SPIN_BY_SPEED, 1.0f, 1.0f, 0.0f, nullptr);
	aRoll->AddValue(STATE_X, INTRO_WHEEL_ROLL_FROM, MOD_TOWARD_NEXT);
	AnimKey* aStop = MakeKey(10, 0);
	aStop->AddValue(STATE_X, -1.0f, MOD_HOLD);
	AnimTrack* aRollTrack = new AnimTrack();
	aRollTrack->AddKey(aRoll, nullptr);
	aRollTrack->AddKey(aStop, aStop);
	mWheelRollTrack = mWheel->AddTrack(aRollTrack, nullptr);

	// a second track the original never starts: 400 frames from where the wheel is to the place its second key names
	mWheelLeaveFrom = MakeKey(400, 0);
	mWheelLeaveFrom->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWheelLeaveFrom->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mWheelLeaveTo = MakeKey(1, 0);
	mWheelLeaveTo->AddValue(STATE_X, 1.0f, MOD_HOLD);
	mWheelLeaveTo->AddValue(STATE_Y, 1.0f, MOD_HOLD);
	AnimTrack* aLeaveTrack = new AnimTrack();
	aLeaveTrack->AddKey(mWheelLeaveFrom, nullptr);
	aLeaveTrack->AddKey(mWheelLeaveTo, nullptr);
	mWheelLeaveTrack = mWheel->AddTrack(aLeaveTrack, nullptr);
}

// 0x436dd0: the cart, a child of the intro, listening for its own events. Three tracks: it rolls in from the left; it
// tips back 15 degrees (the cubic easing in 40 frames), stays 150 frames and raises event 2; it drives off (the two
// keys' places are set when event 2 comes). The wheel and the cart are activated at the end.
void Intro::BuildCart()
{
	mCart = new ObjBase();
	sCartAnim->AddRef();
	mCart->mAnim = sCartAnim;
	mCart->SetParent(this, nullptr);
	mCart->SetEventTarget(this);
	mCart->SetStateInt(STATE_X, 0);
	mCart->SetStateInt(STATE_Y, INTRO_CART_Y);
	mCart->SetStateInt(STATE_Z, INTRO_CART_Z);
	mCart->SetStateInt(STATE_OFFSET_Y, INTRO_CART_OFFSET_Y);

	AnimKey* aRoll = MakeKey(INTRO_ROLL_FRAMES, 0);
	aRoll->AddValue(STATE_X, INTRO_CART_ROLL_FROM, MOD_TOWARD_NEXT);
	AnimKey* aStop = MakeKey(10, 0);
	aStop->AddValue(STATE_X, 0.0f, MOD_HOLD);
	AnimTrack* aRollTrack = new AnimTrack();
	aRollTrack->AddKey(aRoll, nullptr);
	aRollTrack->AddKey(aStop, aStop);
	mCartRollTrack = mCart->AddTrack(aRollTrack, nullptr);

	AnimKey* aTip = MakeKey(0x28, 0);
	aTip->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aTip->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aTip->mEasing = EASE_IN_CUBE;
	AnimKey* aTipped = MakeKey(0x96, 0);
	aTipped->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aTipped->AddValue(STATE_ROTATION, INTRO_CART_TILT, MOD_HOLD);
	AnimKey* aTippedEnd = MakeKey(10, INTRO_EVENT_CART_TILTED);
	aTippedEnd->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aTippedEnd->AddValue(STATE_ROTATION, INTRO_CART_TILT, MOD_HOLD);
	AnimTrack* aTipTrack = new AnimTrack();
	aTipTrack->AddKey(aTip, nullptr);
	aTipTrack->AddKey(aTipped, nullptr);
	aTipTrack->AddKey(aTippedEnd, nullptr);
	mCartTiltTrack = mCart->AddTrack(aTipTrack, nullptr);

	mCartLeaveFrom = MakeKey(400, 0);
	mCartLeaveFrom->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mCartLeaveFrom->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mCartLeaveTo = MakeKey(1, 0);
	mCartLeaveTo->AddValue(STATE_X, 1.0f, MOD_HOLD);
	mCartLeaveTo->AddValue(STATE_Y, 1.0f, MOD_HOLD);
	AnimTrack* aLeaveTrack = new AnimTrack();
	aLeaveTrack->AddKey(mCartLeaveFrom, nullptr);
	aLeaveTrack->AddKey(mCartLeaveTo, nullptr);
	mCartLeaveTrack = mCart->AddTrack(aLeaveTrack, nullptr);

	Activate(mCart, false);
	Activate(mWheel, false);
}

// 0x437250: the bunch of bananas on the cart, each a picture of the bunch turned at random
void Intro::BuildBananas()
{
	for (int i = 0; i < INTRO_BANANAS; i++) {
		ObjBase* aBanana = new ObjBase();
		mBananas[i] = aBanana;
		sBunchAnim->AddRef();
		aBanana->mAnim = sBunchAnim;
		aBanana->SetParent(mCart, nullptr);
		aBanana->SetState(STATE_X, (float)((double)((i % INTRO_BANANA_COLUMNS) * INTRO_BANANA_STEP_X) - INTRO_BANANA_FIRST_X));
		aBanana->SetState(STATE_Y, (float)((double)((i / INTRO_BANANA_COLUMNS) * INTRO_BANANA_STEP_Y) - INTRO_BANANA_FIRST_Y));
		aBanana->SetState(STATE_Z, (float)(-0.5 - (double)i / 1000.0));
		aBanana->SetState(STATE_ROTATION, (float)((double)(int)(GameRandomNext() % 360u) * PI_F / 180.0));
		aBanana->SetStateInt(STATE_RED, 1);
		aBanana->SetStateInt(STATE_GREEN, 1);
		aBanana->SetStateInt(STATE_BLUE, 1);
		aBanana->SetStateInt(STATE_ALPHA, 1);
		aBanana->SetStateInt(STATE_SCALE_X, 1);
		aBanana->SetStateInt(STATE_SCALE_Y, 1);
		Activate(aBanana, false);
	}
}

// 0x4373e0 (the original's name is unknown): a fly's next flight, by what it is doing. A wandering fly picks a place
// anywhere on the screen; a released one picks its banana, the wheel or a place over the cart; one that has arrived at
// its banana takes it (the banana becomes its child and hangs below it) and picks a place off the screen above, and the
// one at the wheel takes that. When fifteen bananas are gone the monkey looks up and the cart tips. The flight is
// two keys: it starts where the fly is and ends at the place picked, its length from the distance (plus up to 25 frames
// at random, but not for the skeeter's kind), its picture turned to the side it goes. theMakeSound is false for the
// first flight of each fly, made while the intro is built.
void Intro::SteerBug(int theNumber, bool theMakeSound)
{
	IntroBug* aBug = mBugs[theNumber];
	float aTargetX = 0.0f;
	float aTargetY = 0.0f;
	float aFrames = 0.0f;
	// a fly that is on its way to the cart or leaving picks nothing: its key stays as it is
	if (aBug->mPhase != BUG_WANDER && aBug->mPhase != BUG_RELEASED && aBug->mPhase != BUG_TO_LOOT)
		return;
	if (aBug->mPhase == BUG_WANDER) {
		aTargetX = (float)((int)(GameRandomNext() % (unsigned)INTRO_WANDER_WIDTH) - INTRO_WANDER_LEFT);
		aTargetY = (float)((int)(GameRandomNext() % (unsigned)INTRO_WANDER_HEIGHT) - INTRO_WANDER_TOP);
		aFrames = (float)((double)Distance(aBug->mState[STATE_X], aBug->mState[STATE_Y], aTargetX, aTargetY) /
			INTRO_SPEED_WANDER);
		if (theMakeSound) {
			unsigned aRoll = GameRandomNext() % (unsigned)INTRO_SOUND_CHANCE;
			if (aRoll == 0)
				sApp->mSoundQueue->Request(SOUND_BUGFLY1, 0, false);
			else if (aRoll == 2)
				sApp->mSoundQueue->Request(SOUND_FLYFLY, 0, false);
		}
	} else if (aBug->mPhase == BUG_RELEASED) {
		if (aBug->mNumber < INTRO_BANANAS) {
			// its banana: the place it is drawn at
			ObjBase* aBanana = mBananas[aBug->mNumber];
			aTargetX = aBanana->mScreenX;
			aTargetY = aBanana->mScreenY;
			aBug->mPhase = BUG_TO_LOOT;
			aFrames = (float)((double)Distance(aBug->mState[STATE_X], aBug->mState[STATE_Y], aTargetX, aTargetY) *
				INTRO_SPEED_TO_BANANA);
		} else if (aBug->mNumber == INTRO_WHEEL_BUG) {
			// the wheel: a place around its middle
			aTargetX = (float)((int)(GameRandomNext() % (unsigned)INTRO_WHEEL_AREA_WIDTH) - INTRO_WHEEL_AREA_LEFT) +
				mWheel->mScreenX;
			aTargetY = (float)((int)(GameRandomNext() % (unsigned)INTRO_WHEEL_AREA_HEIGHT) - INTRO_WHEEL_AREA_TOP) +
				mWheel->mScreenY;
			aBug->mPhase = BUG_TO_LOOT;
			aFrames = (float)((double)Distance(aBug->mState[STATE_X], aBug->mState[STATE_Y], aTargetX, aTargetY) *
				INTRO_SPEED_TO_WHEEL);
		} else {
			// over the cart
			aTargetX = (float)((int)(GameRandomNext() % (unsigned)INTRO_CART_AREA_WIDTH) - INTRO_CART_AREA_LEFT) +
				mCart->mScreenX;
			aTargetY = (float)((int)(GameRandomNext() % (unsigned)INTRO_CART_AREA_HEIGHT) - INTRO_CART_AREA_TOP) +
				mCart->mScreenY;
			aBug->mPhase = BUG_TO_CART;
			aFrames = (float)((double)Distance(aBug->mState[STATE_X], aBug->mState[STATE_Y], aTargetX, aTargetY) /
				INTRO_SPEED_TO_CART);
		}
	} else if (aBug->mPhase == BUG_TO_LOOT) {
		if (aBug->mNumber < INTRO_BANANAS) {
			// the banana is taken: it hangs under the fly (a bit nearer than the one before), and the flight is off
			// the screen, left or right at random
			mStolen = mStolen + 1;
			if (mStolen % 2 == 0)
				sApp->mSoundQueue->Request(SOUND_DART, 0, false);
			ObjBase* aBanana = mBananas[aBug->mNumber];
			aBanana->SetParent(aBug, nullptr);
			aBanana->SetStateInt(STATE_X, 0);
			aBanana->SetStateInt(STATE_Y, INTRO_BANANA_HANG);
			aBanana->SetState(STATE_Z, (float)((double)aBug->mNumber / 1000.0 - 1.0));
			aBanana->Update();
			if ((GameRandomNext() & 1) == 0)
				aTargetX = (float)(int)(GameRandomNext() % (unsigned)INTRO_EXIT_SPREAD + INTRO_EXIT_RIGHT);
			else
				aTargetX = (float)((int)(GameRandomNext() % (unsigned)INTRO_EXIT_SPREAD) - INTRO_EXIT_LEFT);
			aTargetY = (float)((int)(GameRandomNext() % (unsigned)INTRO_EXIT_HEIGHT) - INTRO_EXIT_TOP);
			aFrames = (float)((double)Distance(aBug->mState[STATE_X], aBug->mState[STATE_Y], aTargetX, aTargetY) *
				INTRO_SPEED_LEAVE);
			aBug->mPhase = BUG_LEAVING;
		}
		if (aBug->mNumber == INTRO_WHEEL_BUG) {
			// the wheel is taken: it hangs under the fly, and the flight is off the screen to the top left
			sApp->mSoundQueue->Request(SOUND_DART, 0, false);
			mWheel->SetParent(aBug, nullptr);
			mWheel->SetStateInt(STATE_X, 0);
			mWheel->SetStateInt(STATE_Y, INTRO_BANANA_HANG);
			mWheel->SetStateInt(STATE_Z, 1);
			mWheel->Update();
			aTargetX = INTRO_WHEEL_EXIT;
			aTargetY = INTRO_WHEEL_EXIT;
			aFrames = (float)((double)Distance(aBug->mState[STATE_X], aBug->mState[STATE_Y], aTargetX, aTargetY) /
				INTRO_SPEED_LEAVE_WHEEL);
			aBug->mPhase = BUG_LEAVING;
		}
		if (mStolen >= INTRO_BANANAS && mMan != nullptr) {
			// all the bananas are gone: the monkey looks up (and is let go of the cart), the cart tips back
			mMan->SetStateInt(STATE_FRAME, 0);
			mMan->StartTrack(mManLookUpTrack);
			mStolen = 0;
			mCart->StartTrack(mCartTiltTrack);
			mMan->SetState(STATE_X, mMan->mScreenX);
			mMan->SetState(STATE_Y, mMan->mScreenY);
			mMan->SetParent(nullptr, nullptr);
			mMan->Update();
		}
	}

	int aLength;
	if (aBug->mKind == 1) {
		aLength = (int)aFrames;
	} else {
		int aMore = (int)(GameRandomNext() % (unsigned)INTRO_FLIGHT_SPREAD);
		aLength = (int)aFrames + aMore;
	}
	AnimKey* aFrom = mFlyFrom[theNumber];
	AnimKey* aTo = mFlyTo[theNumber];
	aFrom->SetDuration(aLength);
	aFrom->mValue[STATE_X] = aBug->mState[STATE_X];
	aFrom->mValue[STATE_Y] = aBug->mState[STATE_Y];
	aTo->mValue[STATE_X] = aTargetX;
	aTo->mValue[STATE_Y] = aTargetY;
	aBug->SetStateInt(STATE_SCALE_X, aTargetX > aBug->mState[STATE_X] ? 1 : -1);
	aBug->SetStateInt(STATE_X, 0);
	aBug->SetStateInt(STATE_Y, 0);
	aBug->StartTrack(0);
	aBug->Update();
}

// 0x437a30: the twenty flies, each a child of the intro that listens for its own events. A fly is the intro's own
// picture or the skeeter's (at random), starts above the screen to the right (odd numbers) or left (even), and has two
// tracks: the flight (its first flight is picked at once; it waits 390 frames, and 19 more for each fly after the first,
// before it starts) and a hover that goes on flapping for ever. The flight's second key raises event 3 as the fly
// arrives.
void Intro::BuildBugs()
{
	for (int i = 0; i < INTRO_BUGS; i++) {
		IntroBug* aBug = new IntroBug();
		mBugs[i] = aBug;
		aBug->SetParent(this, nullptr);
		aBug->mNumber = i;
		aBug->mPhase = BUG_WANDER;
		aBug->mKind = (int)(GameRandomNext() & 1);
		Anim* aPicture = aBug->mKind == 0 ? sFlyAnim : BugImgSkeeter::sFlyAnim;
		aPicture->AddRef();
		aBug->mAnim = aPicture;
		// the side it starts on is by its number (the original tests the low bit of the register that holds the
		// number, after the draw)
		unsigned aStartRoll = GameRandomNext();
		if ((i & 1) == 0)
			aBug->SetStateInt(STATE_X, (int)(aStartRoll % (unsigned)INTRO_START_SPREAD) - INTRO_START_LEFT);
		else
			aBug->SetStateInt(STATE_X, (int)(aStartRoll % (unsigned)INTRO_START_SPREAD) + INTRO_START_RIGHT);
		aBug->SetStateInt(STATE_Y, (int)(GameRandomNext() % (unsigned)INTRO_START_HEIGHT) - INTRO_START_TOP);
		aBug->SetStateInt(STATE_Z, 0);
		aBug->SetStateInt(STATE_SCALE_X, 0);
		aBug->SetStateInt(STATE_SCALE_Y, 1);
		aBug->SetEventTarget(this);

		AnimKey* aFrom = MakeKey(INTRO_FOR_EVER, 0);
		aFrom->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aFrom->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aFrom->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, INTRO_FLY_RATE, INTRO_FLY_CELS, 0.0f, nullptr);
		aFrom->mEasing = EASE_LINEAR;
		mFlyFrom[i] = aFrom;
		AnimKey* aTo = MakeKey(1, INTRO_EVENT_BUG_ARRIVED);
		aTo->AddValue(STATE_X, 0.0f, MOD_HOLD);
		aTo->AddValue(STATE_Y, 0.0f, MOD_HOLD);
		mFlyTo[i] = aTo;
		AnimTrack* aFlight = new AnimTrack();
		aFlight->AddKey(aFrom, nullptr);
		aFlight->AddKey(aTo, nullptr);
		aBug->AddTrack(aFlight, nullptr);
		SteerBug(i, false);
		aFlight->SetDelay(INTRO_BUG_DELAY_FIRST + INTRO_BUG_DELAY_STEP * i, true);

		AnimKey* aHover = MakeKey(INTRO_FOR_EVER, 0);
		aHover->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, INTRO_FLY_RATE, INTRO_FLY_CELS, 0.0f, nullptr);
		AnimTrack* aHoverTrack = new AnimTrack();
		aHoverTrack->AddKey(aHover, aHover);
		aBug->AddTrack(aHoverTrack, nullptr);
		Activate(aBug, false);
	}
}

// 0x437d80: the monkey, a child of the cart (at 156, -16 from its middle) that listens for its own events. Five tracks,
// each of keys that show one stretch of one picture: he walks beside the cart (cels counted up 30 a second over 40;
// a second key after 900 frames raises event 4), looks up (event 5), turns (6), is in despair (7; the original
// activates him between the third and fourth track) and looks at the idol (8).
void Intro::BuildMan()
{
	mMan = new ObjBase();
	mMan->SetParent(mCart, nullptr);
	mMan->SetStateInt(STATE_X, 0x9c);
	mMan->SetStateInt(STATE_Y, -16);
	mMan->SetStateInt(STATE_Z, INTRO_MAN_Z);
	mMan->SetStateInt(STATE_SCALE_X, 0);
	mMan->SetEventTarget(this);

	// walking: 900 frames, then 100 more standing in the first cel
	AnimKey* aWalk = MakePictureKey(sWalkAnim, INTRO_ROLL_FRAMES, 0);
	aWalk->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, INTRO_WALK_RATE, INTRO_WALK_CELS, 0.0f, nullptr);
	aWalk->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aWalked = MakePictureKey(sWalkAnim, 100, INTRO_EVENT_MAN_WALKED);
	aWalked->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aWalked->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimTrack* aWalkTrack = new AnimTrack();
	aWalkTrack->AddKey(aWalk, nullptr);
	aWalkTrack->AddKey(aWalked, aWalked);
	mManWalkTrack = mMan->AddTrack(aWalkTrack, nullptr);

	// looking up: 20 frames, then 25 moving 32 right, 64 up and 4 cels on, then 10 holding 64 right and 7 cels on
	AnimKey* aLookStart = MakePictureKey(sLookUpAnim, 0x14, 0);
	aLookStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLookStart->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aLookStart->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	aLookStart->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aLookMove = MakePictureKey(sLookUpAnim, 0x19, 0);
	aLookMove->AddValue(STATE_X, 32.0f, MOD_TOWARD_NEXT);
	aLookMove->AddValue(STATE_Y, -64.0f, MOD_TOWARD_NEXT);
	aLookMove->AddValue(STATE_FRAME, 4.0f, MOD_TOWARD_NEXT);
	aLookMove->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aLookEnd = MakePictureKey(sLookUpAnim, 10, INTRO_EVENT_MAN_LOOKED_UP);
	aLookEnd->AddValue(STATE_X, 64.0f, MOD_HOLD);
	aLookEnd->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aLookEnd->AddValue(STATE_FRAME, 7.0f, MOD_HOLD);
	aLookEnd->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimTrack* aLookTrack = new AnimTrack();
	aLookTrack->AddKey(aLookStart, nullptr);
	aLookTrack->AddKey(aLookMove, nullptr);
	aLookTrack->AddKey(aLookEnd, nullptr);
	mManLookUpTrack = mMan->AddTrack(aLookTrack, nullptr);

	// turning: the cels run on to 11 over four keys, the last of 1 frame raising event 6
	AnimKey* aTurn1 = MakePictureKey(sTurnAnim, 0x3e, 0);
	aTurn1->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	aTurn1->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aTurn2 = MakePictureKey(sTurnAnim, 0x32, 0);
	aTurn2->AddValue(STATE_FRAME, 11.0f, MOD_TOWARD_NEXT);
	aTurn2->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aTurn3 = MakePictureKey(sTurnAnim, 0x32, 0);
	aTurn3->AddValue(STATE_FRAME, 9.0f, MOD_TOWARD_NEXT);
	aTurn3->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aTurn4 = MakePictureKey(sTurnAnim, 1, INTRO_EVENT_MAN_TURNED);
	aTurn4->AddValue(STATE_FRAME, 11.0f, MOD_TOWARD_NEXT);
	aTurn4->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimTrack* aTurnTrack = new AnimTrack();
	aTurnTrack->AddKey(aTurn1, nullptr);
	aTurnTrack->AddKey(aTurn2, nullptr);
	aTurnTrack->AddKey(aTurn3, nullptr);
	aTurnTrack->AddKey(aTurn4, nullptr);
	mManTurnTrack = mMan->AddTrack(aTurnTrack, nullptr);
	Activate(mMan, false);

	// despair: the same shape, the last key (50 frames) raising event 7
	AnimKey* aDespair1 = MakePictureKey(sDespairAnim, 100, 0);
	aDespair1->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	aDespair1->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aDespair2 = MakePictureKey(sDespairAnim, 0x32, 0);
	aDespair2->AddValue(STATE_FRAME, 11.0f, MOD_TOWARD_NEXT);
	aDespair2->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aDespair3 = MakePictureKey(sDespairAnim, 0x32, 0);
	aDespair3->AddValue(STATE_FRAME, 4.0f, MOD_TOWARD_NEXT);
	aDespair3->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aDespair4 = MakePictureKey(sDespairAnim, 0x32, INTRO_EVENT_MAN_DESPAIRED);
	aDespair4->AddValue(STATE_FRAME, 11.0f, MOD_TOWARD_NEXT);
	aDespair4->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimTrack* aDespairTrack = new AnimTrack();
	aDespairTrack->AddKey(aDespair1, nullptr);
	aDespairTrack->AddKey(aDespair2, nullptr);
	aDespairTrack->AddKey(aDespair3, nullptr);
	aDespairTrack->AddKey(aDespair4, nullptr);
	mManDespairTrack = mMan->AddTrack(aDespairTrack, nullptr);

	// looking at the idol: 120 frames from cel 0, 60 to 11, 60 holding 11, then two single frames at 11 (the first
	// raises event 8); the last key holds
	AnimKey* aIdol1 = MakePictureKey(sIdolAnim, 0x78, 0);
	aIdol1->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	aIdol1->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aIdol2 = MakePictureKey(sIdolAnim, 0x3c, 0);
	aIdol2->AddValue(STATE_FRAME, 11.0f, MOD_TOWARD_NEXT);
	aIdol2->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aIdol3 = MakePictureKey(sIdolAnim, 0x3c, 0);
	aIdol3->AddValue(STATE_FRAME, 11.0f, MOD_HOLD);
	aIdol3->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aIdol4 = MakePictureKey(sIdolAnim, 1, INTRO_EVENT_MAN_LOOKED);
	aIdol4->AddValue(STATE_FRAME, 11.0f, MOD_HOLD);
	aIdol4->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimKey* aIdol5 = MakePictureKey(sIdolAnim, 1, 0);
	aIdol5->AddValue(STATE_FRAME, 11.0f, MOD_HOLD);
	aIdol5->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	AnimTrack* aIdolTrack = new AnimTrack();
	aIdolTrack->AddKey(aIdol1, nullptr);
	aIdolTrack->AddKey(aIdol2, nullptr);
	aIdolTrack->AddKey(aIdol3, nullptr);
	aIdolTrack->AddKey(aIdol4, nullptr);
	aIdolTrack->AddKey(aIdol5, aIdol5);
	mManIdolTrack = mMan->AddTrack(aIdolTrack, nullptr);
	Activate(mMan, false);
}

// 0x438a40: the flare, a child of the monkey (at his place, in front of him by one step of depth, and not shown until
// it is started): in 30 frames it grows from a tenth see-through to 0.8 (the first key eases in by a square), in 18
// more (event 10 when they end) to 2.3 times the size in full light, then in a frame it is back to 0.6; all drawn
// additively.
void Intro::BuildFlare()
{
	mFlare = new ObjBase();
	mFlare->SetParent(mMan, nullptr);
	mFlare->SetStateInt(STATE_X, 0);
	mFlare->SetStateInt(STATE_Y, 0);
	mFlare->SetStateInt(STATE_SCALE_X, 0);
	mFlare->SetStateInt(STATE_SCALE_Y, 0);
	mFlare->SetStateInt(STATE_ALPHA, 0);
	mFlare->SetStateInt(STATE_Z, 1);
	mFlare->SetEventTarget(this);

	AnimKey* aGrow = MakePictureKey(sFlareAnim, 0x1e, 0);
	aGrow->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_Y, 0.8f, MOD_TOWARD_NEXT);
	aGrow->mDrawMode = 1;
	aGrow->mEasing = EASE_IN_SQUARE;
	AnimKey* aFull = MakePictureKey(sFlareAnim, 0x12, INTRO_EVENT_FLARE_GROWN);
	aFull->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aFull->AddValue(STATE_SCALE_X, 2.3f, MOD_TOWARD_NEXT);
	aFull->AddValue(STATE_SCALE_Y, 2.3f, MOD_TOWARD_NEXT);
	aFull->mDrawMode = 1;
	AnimKey* aBack = MakePictureKey(sFlareAnim, 1, 0);
	aBack->AddValue(STATE_ALPHA, 0.1f, MOD_HOLD);
	aBack->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aBack->AddValue(STATE_SCALE_Y, 0.6f, MOD_HOLD);
	aBack->mDrawMode = 1;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aGrow, nullptr);
	aTrack->AddKey(aFull, nullptr);
	aTrack->AddKey(aBack, nullptr);
	mFlare->AddTrack(aTrack, nullptr);
}

// One of the speech bubbles' tracks: it grows from nothing with a small wobble in 8 frames, stays 200, shrinks away in
// 8 and is gone
static AnimTrack* MakeBubbleTrack()
{
	AnimKey* aGrow = MakeKey(8, 0);
	aGrow->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
	aGrow->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
	aGrow->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aStay = MakeBubbleKey(200, 1.0f, MOD_HOLD);
	AnimKey* aShrink = MakeBubbleKey(8, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aGone = MakeBubbleKey(1, 0.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aGrow, nullptr);
	aTrack->AddKey(aStay, nullptr);
	aTrack->AddKey(aShrink, nullptr);
	aTrack->AddKey(aGone, nullptr);
	return aTrack;
}

// 0x438d60: the first speech bubble, a child of the intro, in place until its event: not active
void Intro::BuildTalk1()
{
	mTalk1 = new ObjBase();
	sTalk1Anim->AddRef();
	mTalk1->mAnim = sTalk1Anim;
	mTalk1->SetParent(this, nullptr);
	mTalk1->SetStateInt(STATE_X, INTRO_TALK1_X);
	mTalk1->SetStateInt(STATE_Y, INTRO_TALK1_Y);
	mTalk1->SetStateInt(STATE_SCALE_X, 0);
	mTalk1->SetStateInt(STATE_SCALE_Y, 0);
	mTalk1->SetStateInt(STATE_ALPHA, 0);
	mTalk1->SetStateInt(STATE_Z, INTRO_TALK_Z);
	mTalk1->SetEventTarget(this);
	mTalk1->AddTrack(MakeBubbleTrack(), nullptr);
	Deactivate(mTalk1);
}

// 0x439150: the second, the same, 100 frames after it is started
void Intro::BuildTalk2()
{
	mTalk2 = new ObjBase();
	sTalk2Anim->AddRef();
	mTalk2->mAnim = sTalk2Anim;
	mTalk2->SetParent(this, nullptr);
	mTalk2->SetStateInt(STATE_X, INTRO_TALK2_X);
	mTalk2->SetStateInt(STATE_Y, INTRO_TALK2_Y);
	mTalk2->SetStateInt(STATE_SCALE_X, 0);
	mTalk2->SetStateInt(STATE_SCALE_Y, 0);
	mTalk2->SetStateInt(STATE_ALPHA, 0);
	mTalk2->SetStateInt(STATE_Z, INTRO_TALK_Z);
	mTalk2->SetEventTarget(this);
	AnimTrack* aTrack = MakeBubbleTrack();
	aTrack->SetDelay(INTRO_TALK2_DELAY, false);
	mTalk2->AddTrack(aTrack, nullptr);
	Deactivate(mTalk2);
}

// 0x4365b0: a key of the intro's pieces ended
void Intro::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case INTRO_EVENT_CART_TILTED:
		// the cart drives off: from where it is, 1100 left and 300 up, in 400 frames
		mCartLeaveFrom->mValue[STATE_X] = mCart->mState[STATE_X];
		mCartLeaveFrom->mValue[STATE_Y] = mCart->mState[STATE_Y];
		mCartLeaveTo->mValue[STATE_X] = (float)((double)mCart->mState[STATE_X] - INTRO_CART_LEAVE_X);
		mCartLeaveTo->mValue[STATE_Y] = (float)((double)mCart->mState[STATE_Y] - INTRO_CART_LEAVE_Y);
		mCart->SetStateInt(STATE_X, 0);
		mCart->SetStateInt(STATE_Y, 0);
		mCart->StartTrack(mCartLeaveTrack);
		mCart->Update();
		break;
	case INTRO_EVENT_BUG_ARRIVED: {
		IntroBug* aBug = static_cast<IntroBug*>(theObj);
		switch (aBug->mPhase) {
		case BUG_WANDER:
		case BUG_RELEASED:
		case BUG_TO_LOOT:
			SteerBug(aBug->mNumber, true);
			break;
		case BUG_TO_CART:
			// it hovers over the cart from here, where it is on the screen, and flaps
			aBug->SetParent(mCart, nullptr);
			aBug->SetState(STATE_X, aBug->mScreenX - mCart->mScreenX);
			aBug->SetState(STATE_Y, aBug->mScreenY - mCart->mScreenY);
			aBug->StartTrack(1);
			aBug->Update();
			break;
		case BUG_ON_WHEEL:
			// (nothing sets this phase) the same over the wheel, 32 higher
			aBug->SetParent(mWheel, nullptr);
			aBug->SetState(STATE_X, aBug->mScreenX - mWheel->mScreenX);
			aBug->SetState(STATE_Y, (float)((double)(aBug->mScreenY - mWheel->mScreenY) - INTRO_BANANA_HANG));
			aBug->StartTrack(1);
			aBug->Update();
			break;
		default:
			break;
		}
		break;
	}
	case INTRO_EVENT_MAN_LOOKED_UP:
		theObj->SetStateInt(STATE_FRAME, 0);
		mMan->StartTrack(mManTurnTrack);
		sApp->mSoundQueue->Request(SOUND_INTROYELP, 0, false);
		break;
	case INTRO_EVENT_MAN_TURNED:
		theObj->SetStateInt(STATE_FRAME, 0);
		mMan->StartTrack(mManDespairTrack);
		mTalk1->StartTrack(0);
		Activate(mTalk1, false);
		sApp->mSoundQueue->Request(SOUND_INTROMONKEY, 0, false);
		break;
	case INTRO_EVENT_MAN_DESPAIRED:
		// the level may go on from here
		theObj->SetStateInt(STATE_FRAME, 0);
		mMan->StartTrack(mManIdolTrack);
		mTalk2->StartTrack(0);
		Activate(mTalk2, false);
		sApp->mSoundQueue->Request(SOUND_INTROIDOL, 0, false);
		sReady = true;
		break;
	case INTRO_EVENT_MAN_LOOKED:
		mFlare->StartTrack(0);
		Activate(mFlare, false);
		sApp->mSoundQueue->Request(SOUND_DISAPPEAR, 0, false);
		break;
	case INTRO_EVENT_FLARE_GROWN: {
		mMan->SetHidden(true);
		LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
		if (aLevel->mMenuButton != nullptr) // port: the original does not look
			aLevel->mMenuButton->SetKeepCursor(false);
		break;
	}
	default:
		break;
	}
}

// 0x436950: the flies are let go one at a time (1 to 4 frames apart, from 670 frames in), the last five when the
// thirteenth is. After the first 100 frames a mouse press ends the intro, with a flash at the monkey's place;
// presses in those 100 frames are thrown away.
bool Intro::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mReleased < INTRO_BANANAS) {
		mReleaseTimer = mReleaseTimer - 1;
		if (mReleaseTimer < 1) {
			mBugs[mReleased]->mPhase = BUG_RELEASED;
			mReleaseTimer = (int)(GameRandomNext() & INTRO_RELEASE_MASK) + 1;
			mReleased = mReleased + 1;
		}
		if (mReleased == INTRO_LAST_BUGS_AT)
			ReleaseLastBugs();
	}
	if (mSkipDelay < 1) {
		if (sMouseHeld) {
			mFlare->StartTrack(0);
			Activate(mFlare, false);
			LevelObj* aLevel = static_cast<LevelObj*>(sLevel);
			if (aLevel->mMenuButton != nullptr)
				aLevel->mMenuButton->SetKeepCursor(false);
			Finish();
			return true;
		}
	} else {
		sMouseHeld = false;
		mSkipDelay = mSkipDelay - 1;
	}
	return true;
}

// 0x436a20: once. The intro lets the level go on; the monkey and the wheel (which may be carried off by a fly, or
// still on the cart) go for good, the song of the world the player is on starts, and the intro, with all that is left
// of it, goes.
void Intro::Finish()
{
	if (sFinished)
		return;
	sFinished = true;
	Deactivate(this);
	sReady = true;
	if (mMan != nullptr) {
		mMan->SetParent(nullptr, nullptr);
		mMan->Delete();
		mMan = nullptr;
	}
	if (mWheel != nullptr) {
		mWheel->SetParent(nullptr, nullptr);
		mWheel->Delete();
		mWheel = nullptr;
	}
	sApp->ChangeMusic(Player::sCurrent->mMapLevel % 4);
	Delete();
}
