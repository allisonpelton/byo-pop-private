#include "Enemy.h"
#include "WinWSApp.h"
#include "ImageFont.h"

using namespace Sexy;

// 0x40a3f0: GameObject (0x4611b0) first, then the Enemy part
Enemy::Enemy(WinWSApp* theApp) :
	mFlag08(false),
	mApp(theApp),
	mTarget(NULL),
	mAttacker(NULL),
	mFlag18(false),
	mFlag19(false),
	mFlag1a(false),
	mFlag1b(true),
	mFlag1c(true),
	mFlag1d(true),
	mTextOffsetY(0),
	mTextOffsetX(0),
	mField28(0),
	mField2c(0),
	mFlag30(false),
	mFlag50(false),
	mField54(0),
	mNumTyped(0),
	mImages(6, (Image*)NULL),
	mImage(NULL),
	mFlagD0(false)
{
	mFont = mApp->mFontMyriadCondensed15;
	mTypedFont = mApp->mFontMyriadCondensed15b;
}

Enemy::~Enemy()
{
}

// 0x403130: each letter is worth 1 to 4 (the common letters little, the rare ones more), the word is worth the sum times 15
int Enemy::GetPoints()
{
	int aSum = 0;
	unsigned int i = 0;
	if (mWord.size() != 0)
	{
		do
		{
			switch (mWord[i])
			{
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case 'C': case 'O': case 'P': case 'Q': case 'W': case 'X': case 'Z':
				aSum += 3;
				break;
			default:
				aSum += 4;
				break;
			case 'A': case 'D': case 'E': case 'F': case 'G': case 'H': case 'J': case 'K': case 'L': case 'S':
				aSum += 1;
				break;
			case 'B': case 'I': case 'M': case 'N': case 'R': case 'T': case 'U': case 'V': case 'Y':
				aSum += 2;
			}
			i++;
		} while (i < mWord.size());
	}
	return aSum * 15;
}

// 0x4011f0
double Enemy::GetSpeed()
{
	return mVelX;
}

// 0x40a5b0: the last letter is never counted: when the word is complete the creature reports event 1 and lets go of its
// target, and mNumTyped stays at the last position
void Enemy::TypeLetter()
{
	int aOld = mNumTyped;
	mNumTyped = aOld + 1;
	if (mNumTyped == (int)mWord.size())
	{
		mNumTyped = aOld;
		mEvents.push_back(1);
		ReleaseTarget();
		if (mAttacker != NULL)
			mAttacker->SetTarget(NULL);
	}
	mTyped = mWord.substr(0, mNumTyped);
	mRemaining = mWord.substr(mNumTyped, mWord.size());
}

// 0x4022d0: let go of the creature this one is tied to. A target that is waiting to the right of the screen, still at
// state 0 and hidden, is put at the right edge and shown
void Enemy::ReleaseTarget()
{
	if (mTarget != NULL)
	{
		mTarget->SetAttacker(NULL);
		double aWidth = (double)mApp->mWidth;
		if (mTarget->mState == 0 && aWidth < mTarget->mX && !mTarget->mVisible)
		{
			mTarget->SetX(aWidth);
			mTarget->SetVisible(true);
		}
		mTarget = NULL;
	}
}

// 0x407070: the first event of the list, removed from it; 0 when there is none
int Enemy::PopEvent()
{
	int aEvent = 0;
	if (mEvents.size() > 0)
	{
		aEvent = mEvents[0];
		mEvents.erase(mEvents.begin());
	}
	return aEvent;
}

// 0x403790: whether the list holds the event (all of the list is read)
bool Enemy::HasEvent(int theEvent)
{
	bool aFound = false;
	for (size_t i = 0; i < mEvents.size(); i++)
	{
		if (mEvents[i] == theEvent)
			aFound = true;
	}
	return aFound;
}

// 0x407a30: the word to type, and what it is worth (GetPoints, virtual)
void Enemy::SetWord(const std::string& theWord)
{
	mWord = theWord;
	mField54 = GetPoints();
}

// 0x401230: the death animation of a dying creature (state 3) has played to its end (mFlag18 is set when it does)
bool Enemy::IsDeathFinished()
{
	if (mState == 3 && mFlag18)
		return true;
	return false;
}

// 0x407100: removes every entry equal to the event from the list
void Enemy::RemoveEvent(int theEvent)
{
	for (unsigned int i = 0; !mEvents.empty() && i < mEvents.size(); i++)
	{
		if (mEvents[i] == theEvent)
		{
			mEvents.erase(mEvents.begin() + i);
			i--;
		}
	}
}

// Not translated, the rest of the entity range 0x401000-0x412330 (E3):
// - Enemy and GameObject in their standalone layouts: 0x401160 (GameObject's deleting destructor), 0x401190 (the
//   Enemy-level SetState, writes the state), 0x4079d0, 0x4079e0, 0x4079f0, 0x407a00, 0x407a10, 0x407a20 (vtordisp thunks
//   of Enemy's GameObject part, slots 0, 1, 2, 3, 4, 7), 0x409cf0 (Enemy's deleting destructor), 0x40a413 (not a function:
//   Ghidra's second entry into the middle of the Enemy constructor 0x40a3f0)
// - deleting destructors and part destructors of the creature classes (they call the string destructor and the base
//   destructors): 0x409ae0 (StealthPiranha), 0x409d30 (Shark), 0x409e10 (ProjectileBoss), 0x409e50 (TorpedoBoss),
//   0x410180 (RedPiranha), 0x410f00 (WhitePiranha), 0x412230 (StealthWhitePiranha); in the same files 0x405050 is the
//   inlined destructor of the std::string a WhitePiranha holds
// - WSTimer (WSTimer.cpp): 0x4012c0 (the constructor the bosses use, same as the inlined one) and 0x401330 (deleting
//   destructor)
// - framework: 0x402000 (Color(r, g, b, a)), 0x402020 (Rect::Intersects)
// - the standard library, instantiated for the game's types. std::string: 0x403010 (operator== with a C string),
//   0x403200 (_Tidy), 0x4033b0 and 0x4033f0 (comparisons), 0x406450 (append of a substring), 0x407210 (substr).
//   std::vector of 4-byte elements (Image* in Enemy::mImages, int in the events): 0x409c00 (resize), 0x406f60 (erase).
//   std::vector of the 24-byte explosion records of TorpedoBoss: 0x402070 (size), 0x403050, 0x403090, 0x403420, 0x403450
//   (copy, copy_backward, fill), 0x4030d0, 0x403490 (uninitialized copy and fill), 0x406420, 0x406d70, 0x409510
//   (_Insert_n), 0x409c80, 0x409e90 (insert, push_back). std::map of the sharks' and piranhas' pictures (map<int,
//   map<int, Image*>>, WinWSApp's ImageVariantTable): 0x4020d0, 0x402e80, 0x402ee0, 0x402f40, 0x402f50, 0x403350,
//   0x403370, 0x406370, 0x406e50 (_Min, rotations, iterators, _Erase, _Copy), 0x4091b0 and 0x40fb00 (find or insert in
//   the inner and the outer map), 0x40fbc0 (the outer map's operator[] that the creature constructors call), 0x409810
//   (erase range), 0x409f20, 0x40f8a0 (copy of the maps), 0x40df40, 0x40df80 (clear). Their exception funclets:
//   0x403285, 0x40331d, 0x406405, 0x409403, 0x40940f, 0x4096e8, 0x4096f4, 0x409f95, 0x40f926 (Catch@...), 0x4020b0 (a thunk
//   to operator delete)
