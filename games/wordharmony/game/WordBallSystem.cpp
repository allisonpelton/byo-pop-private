#include "WordBallSystem.h"
#include <cstdint>
#include <cstring>
#include "App.h"
#include "FadeContainer.h"
#include "Game.h"
#include "Graphics.h"
#include "Host.h"
#include "Hud.h"
#include "ImageNode.h"
#include "Level.h"
#include "Music.h"
#include "Obstacles.h"
#include "Particles.h"
#include "Random.h"
#include "SavedGame.h"
#include "Selector.h"
#include "TextEffects.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <set>

using namespace engine;

// 0x416fd0. The original leaves +0x108, +0x124-+0x12c, +0x1a8, +0x1d8, +0x1f4 and +0x200 unset; the port starts them
// at zero.
WordBallSystem::WordBallSystem(int theMode)
{
	mMode = theMode;
	mObstacles = nullptr;
	mStepTime = 0;
	mLastStep = 0;
	mSpawnTime = 0;
	mStarted = false;
	mLongestPiece = 0;
	mNextSlot = 0;
	mBallsToCome = 0;
	mWaterLevel = 0;
	mFill = 0;
	mShortestWord = 0;
	m194 = false;
	// The original leaves it unset. The heap block it gets held the text "background", whose first eight bytes make a
	// huge positive double, so the program's first word always counts as quick (twice the points): the same bytes here
	uint64_t aBits = 0x756f72676b636162ULL;
	memcpy(&mLastMatch, &aBits, 8);
	// fixes mod, "fairFirstWord": no word came before the first, so it counts as quick only when it is quick
	if (engine::getHost()->fixOn("fairFirstWord"))
		mLastMatch = 0;
	mNoPicking = false;
	mWaveTime = 0;
	m1F0 = false;
	mHintIndex = 0;
	m1FC = 0;
	setName("WordBallSystem");
	mBounds = new Bounds(this, 441.0f);
	mRope = nullptr;
	makeBalls();
	addOwnFlags(2);
	addOwnFlags(4);
	App::instance()->getRootWindow()->addKeyHandler(this);
	setRect(0, 0, 590.0f, 441.0f);
	mBlossoms = new ParticlePool("config\\exploding-blossoms.xml", 3);
	mBlossoms->addRef();
	mSparkles = new ParticlePool("config\\sparkles.xml", 2);
	mSparkles->addRef();
	Image* anImage = App::instance()->getImage("res\\target_hit.png");
	anImage->setPivot(1);
	mTarget = new ImageNode(anImage);
	mTarget->setVisible(false);
	mTarget->addRef();
	mChainLink = App::instance()->getImage("res\\chainLink.png");
	mChainLink->setPivot(1);
	mChainLink->addRef();
	int i = 0;
	Image** aWater = mWater;
	do
	{
		i++;
		char aName[260];
		sprintf(aName, "res\\effects\\water%d.png", i);
		*aWater = App::instance()->getImage(aName);
		(*aWater)->setPivot(0);
		(*aWater)->setAlphaMode(2, 0x80);
		(*aWater)->addRef();
		aWater++;
	} while (i < 4);
	mLayer = new FadeContainer();
	mLayer->addRef();
	makeHuds();
}

// 0x415920 (the pools, the images, the layer and the HUDs keep their references, as in the original)
WordBallSystem::~WordBallSystem()
{
	unsigned int i = 0;
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = getBall(i);
			if (aBall->mChain != nullptr)
			{
				removeChain(aBall->mChain);
				aBall->mChain = nullptr;
			}
			aBall->mState = BALL_GONE;
			aBall->mSpecial = 0;
			aBall->mLinked = false;
			aBall->mSelector = nullptr;
			aBall->removeAllChildren();
			i++;
		} while (i < mCount);
	}
	mCount = 0;
	delete mBounds;
}

// 0x4135c0: the array holds a reference to each ball, so the layer's release never deletes one
void WordBallSystem::makeBalls()
{
	mBalls = new Ball[105];
	unsigned int i = 0;
	if (mCount != 0)
	{
		do
		{
			mBalls[i].mState = BALL_FREE;
			mBalls[i].setName("node");
			mBalls[i].addRef();
			i++;
		} while (i < mCount);
	}
	i = mCount;
	if (i < 105)
	{
		unsigned int aLeft = 105 - i;
		do
		{
			mBalls[i].mState = BALL_GONE;
			mBalls[i].addRef();
			mBalls[i].setName("node");
			i++;
			aLeft--;
		} while (aLeft != 0);
	}
}

// Adds an image node of theFile to theParent (0x435d50 after the image's pivot and alpha mode)
static ImageNode* addImage(Container* theParent, Image* theImage)
{
	ImageNode* aNode = new ImageNode(theImage);
	theParent->addChild(aNode);
	return aNode;
}

// 0x4113f0: a selector of the off and on images (one image twice for the blocks and stones), the badges, and for a
// wildcard a green letter, a sparkle and the glow
void WordBallSystem::setupBall(Ball* theBall)
{
	theBall->removeAllChildren();
	theBall->setOpacity(1.0f);
	theBall->setScale(1.0f, 1.0f);
	if ((theBall->mSpecial & BALL_WILDCARD) == 0)
		theBall->setFont(App::instance()->getFont("res\\fonts\\bembo-bold-18-red.xml"));
	else
	{
		theBall->setFont(App::instance()->getFont("res\\fonts\\bembo-bold-18-green.xml"));
		theBall->mLetter = (char)toupper(' ');
		ParticleSystem* aSystem = mSparkles->getSystem();
		theBall->addChild(aSystem);
		aSystem->startEmitting();
		Image* aGlow = App::instance()->getImage("res\\effects\\wildcardGlow.png");
		aGlow->setPivot(1);
		addImage(theBall, aGlow);
	}
	Selector* aSelector = new Selector();
	theBall->mSelector = aSelector;
	theBall->addChild(aSelector);
	unsigned int aFlags = theBall->mSpecial;
	ImageNode* aNode;
	if ((aFlags & BALL_BOMB) == 0)
	{
		if ((aFlags & BALL_BREAKABLE) != 0)
		{
			Image* anImage = App::instance()->getImage("res\\cube02.png");
			anImage->setAlphaMode(1, 0x80);
			anImage->setPivot(1);
			aNode = new ImageNode(anImage);
			aSelector->addChild(aNode);
			aSelector->addChild(aNode);
			theBall->mFixed = true;
			goto badges;
		}
		if ((aFlags & BALL_BLOCK) != 0)
		{
			Image* anImage = App::instance()->getImage("res\\cube02.png");
			anImage->setAlphaMode(1, 0x80);
			anImage->setPivot(1);
			aNode = new ImageNode(anImage);
			aSelector->addChild(aNode);
			aSelector->addChild(aNode);
			theBall->mFixed = false;
			theBall->mInverseMass = 0.01f;
			goto badges;
		}
		if ((aFlags & BALL_HEAVY) != 0)
		{
			Image* anImage = App::instance()->getImage("res\\rock.png");
			anImage->setPivot(1);
			aNode = new ImageNode(anImage);
			aSelector->addChild(aNode);
			aSelector->addChild(aNode);
			theBall->mInverseMass = 0.25f;
			goto badges;
		}
		Image* anOn;
		if (theBall->mChain == nullptr)
		{
			Image* anOff = App::instance()->getImage("res\\ball.png");
			anOff->setAlphaMode(2, 0x80);
			anOff->setPivot(1);
			aSelector->addChild(new ImageNode(anOff));
			anOn = App::instance()->getImage("res\\ball_on.png");
			anOn->setPivot(1);
			anOn->setAlphaMode(2, 0);
		}
		else
		{
			Image* anOff = App::instance()->getImage("res\\ball-chain.png");
			anOff->setAlphaMode(2, 0);
			anOff->setPivot(1);
			aSelector->addChild(new ImageNode(anOff));
			anOn = App::instance()->getImage("res\\ball_on-chain.png");
			anOn->setPivot(1);
			anOn->setAlphaMode(2, 0);
		}
		aNode = new ImageNode(anOn);
	}
	else
	{
		theBall->mSpecial = aFlags | BALL_WILDCARD;
		Image* anImage = App::instance()->getImage("res\\ball.png");
		anImage->setAlphaMode(1, 0x80);
		anImage->setPivot(1);
		aNode = new ImageNode(anImage);
		aSelector->addChild(aNode);
	}
	aSelector->addChild(aNode);
badges:
	if ((theBall->mSpecial & BALL_X2) != 0)
	{
		Image* anImage = App::instance()->getImage("res\\x2.png");
		anImage->setPivot(1);
		addImage(theBall, anImage);
	}
	if ((theBall->mSpecial & BALL_X3) != 0)
	{
		Image* anImage = App::instance()->getImage("res\\x3.png");
		anImage->setPivot(1);
		addImage(theBall, anImage);
	}
	if ((theBall->mSpecial & BALL_X4) != 0)
	{
		Image* anImage = App::instance()->getImage("res\\x4.png");
		anImage->setPivot(1);
		addImage(theBall, anImage);
	}
	static_cast<Selector*>(theBall->mSelector)->select(0, 0.0f, true);
}

// 0x411a60
bool WordBallSystem::isBusy()
{
	unsigned int i = 0;
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = getBall(i);
			if (aBall->mState == BALL_PICKED_2 || aBall->mState == BALL_PICKED)
				return true;
			i++;
		} while (i < mCount);
	}
	return !mBlossoms->isDone();
}

// 0x411ac0
void WordBallSystem::grab(Ball* theBall)
{
	Point anOffset = mMouse;
	Point* aPosition = theBall->getPosition();
	anOffset.x -= aPosition->x;
	anOffset.y -= aPosition->y;
	mRope = new Rope(theBall, anOffset, 21.99f, 0.3f, 0.8f);
	mRope->mTarget = mMouse;
	mTarget->setVisible(true);
	mHud->setControlsEnabled(false, false, false);
}

// 0x411ba0
void WordBallSystem::letGo()
{
	if (mRope != nullptr)
	{
		mRope->mBall->setOn(false);
		delete mRope;
	}
	mRope = nullptr;
	mTarget->setVisible(false);
	mHud->setControlsEnabled(true, false, false);
}

// 0x411c00
void WordBallSystem::setMode(int theMode)
{
	mMode = theMode;
	if (theMode == MODE_SOLITAIRE)
		mHud = mSolitaireHud;
	else if (theMode == MODE_FREEMATCH)
		mHud = mFreematchHud;
	else
		mHud = mPuzzleHud;
	mHud->setMode(theMode);
	mBallsLeft = 0;
}

// 0x411c50: the original terminates the hint finder's thread while it still runs; the port's finder runs at once
void WordBallSystem::stopHintThread()
{
}

// 0x411c90
int WordBallSystem::waterStage(float theValue)
{
	int aValue = (int)(theValue * 100.0f);
	if (aValue < 26)
		return 0;
	if (aValue < 51)
		return 1;
	if (aValue < 61)
		return 2;
	if (aValue < 71)
		return 3;
	if (aValue < 81)
		return 4;
	if (aValue < 91)
		return 5;
	return (95 < aValue) + 6;
}

// 0x411dc0
void WordBallSystem::levelDone()
{
	Game* aGame = Game::instance();
	if (mMode == MODE_PUZZLE)
	{
		int aCount = (int)aGame->mLevels.size();
		if (aGame->mLevel == aCount - 1)
		{
			aGame->gameOver(false, false);
			mNoPicking = true;
			return;
		}
	}
	aGame->setState(7);
	mNoPicking = true;
}

// 0x412120: a free ball near the mouse gets the rope, and lights up
void WordBallSystem::onMouseDown(Point& p)
{
	if (mNoPicking)
		return;
	unsigned int anIndex = findBall(mMouse);
	if (anIndex == (unsigned int)-1)
		return;
	Ball* aBall = getBall(anIndex);
	if (!aBall->mFixed)
	{
		if (mRope == nullptr)
			grab(aBall);
		aBall->setOn(true);
	}
}

// 0x411d80
void WordBallSystem::onMouseUp(Point& p)
{
	if (mRope != nullptr)
		letGo();
}

// 0x411260 (the ball under the mouse is looked up and not used)
void WordBallSystem::mouseMove(Point& p)
{
	mMouse = p;
	toLocal(mMouse);
	if (mRope != nullptr)
		mRope->mTarget = mMouse;
	unsigned int anIndex = findBall(mMouse);
	if (anIndex != (unsigned int)-1)
		getBall(anIndex);
}

// 0x412310: in freematch the water (three layers of waves and the deep water under them) at the water level, then
// the children, then the links of each chain, one between each two of its balls
void WordBallSystem::draw(Graphics* g)
{
	if (mMode == MODE_FREEMATCH)
	{
		g->pushState();
		g->setOpacity(mLayer->getOpacity());
		float y = (1.0f - mWaterLevel) * 456.0f - 15.0f;
		float aTop = 0.0f <= -y ? -y : 0.0f;
		float aLimit = 442.0f - y;
		float aDrawY;

		float aHeight = (float)mWater[2]->getHeight();
		float aWave = sinf(mWaveTime * 0.33333334f) * 50.0f + 50.0f;
		if (!(aHeight < aLimit))
			aHeight = aLimit;
		IntRect aRect((int)aWave, (int)aTop, (int)(aWave + 441.0f), (int)aHeight);
		aDrawY = 0.0f <= y ? y : 0.0f;
		g->drawImage(mWater[2], aRect, -1.0f, aDrawY);

		aHeight = (float)mWater[1]->getHeight();
		aWave = sinf(mWaveTime * 0.5f) * 50.0f + 50.0f;
		if (!(aHeight < aLimit))
			aHeight = aLimit;
		aRect = IntRect((int)aWave, (int)aTop, (int)(aWave + 441.0f), (int)aHeight);
		aDrawY = 0.0f <= y ? y : 0.0f;
		g->drawImage(mWater[1], aRect, -1.0f, aDrawY);

		int aFirstHeight = mWater[0]->getHeight();
		aHeight = (float)aFirstHeight;
		aWave = sinf(mWaveTime) * 50.0f + 50.0f;
		if (!(aHeight < aLimit))
			aHeight = aLimit;
		aRect = IntRect((int)aWave, (int)aTop, (int)(aWave + 441.0f), (int)aHeight);
		aDrawY = 0.0f <= y ? y : 0.0f;
		g->drawImage(mWater[0], aRect, -1.0f, aDrawY);

		y = (float)aFirstHeight + y;
		float aBottom = (float)mWater[3]->getHeight() - y + 1.0f;
		if (aBottom < 0.0f)
			aBottom = 0.0f;
		aRect = IntRect(0, 0, 441, (int)aBottom);
		g->drawImage(mWater[3], aRect, -1.0f, y);
		g->popState();
	}
	Container::draw(g);
	std::vector<Chain*>::iterator anIt = mChains.begin();
	if (anIt != mChains.end())
	{
		do
		{
			Chain* aChain = *anIt;
			if (aChain->mIndices.size() != 0)
			{
				std::list<int>::iterator aLink = aChain->mIndices.begin();
				Ball* aPrevious = getBall(*aLink);
				aLink++;
				if (aLink != aChain->mIndices.end())
				{
					do
					{
						std::list<int>::iterator aNext = aLink;
						aNext++;
						Ball* aBall = getBall(*aLink);
						Point aMiddle;
						aMiddle.x = (aBall->getPosition()->x - aPrevious->getPosition()->x) * 0.5f + aPrevious->getPosition()->x;
						aMiddle.y = (aBall->getPosition()->y - aPrevious->getPosition()->y) * 0.5f + aPrevious->getPosition()->y;
						g->pushState();
						g->setOpacity(mLayer->getOpacity());
						g->translate(aMiddle.x, aMiddle.y);
						g->setSnap(3);
						g->drawImage(mChainLink);
						g->popState();
						aLink = aNext;
						aPrevious = aBall;
					} while (aLink != aChain->mIndices.end());
				}
			}
			anIt++;
		} while (anIt != mChains.end());
	}
}

// 0x412920
void WordBallSystem::removeChain(Chain* theChain)
{
	std::list<int>::iterator aLink = theChain->mIndices.begin();
	if (aLink != theChain->mIndices.end())
	{
		do
		{
			getBall(*aLink)->mChain = nullptr;
			aLink++;
		} while (aLink != theChain->mIndices.end());
	}
	std::vector<Chain*>::iterator anIt = mChains.begin();
	if (anIt != mChains.end())
	{
		while (*anIt != theChain)
		{
			anIt++;
			if (anIt == mChains.end())
				return;
		}
		mChains.erase(anIt);
		delete theChain;
	}
}

// 0x412b30
void WordBallSystem::pickBall(unsigned int theIndex, float theB)
{
	Ball* aBall = getBall(theIndex);
	mBallsLeft--;
	if (mMode == MODE_FREEMATCH)
	{
		float aFill = (float)(unsigned int)mBallsLeft / (float)Game::instance()->mCurrentLevel->mMaxBalls;
		if (1.0f <= aFill)
			aFill = 1.0f;
		mFill = aFill;
	}
	aBall->mB0 = theB;
	aBall->mState = BALL_PICKED;
	if (aBall->mChain != nullptr)
	{
		removeChain(aBall->mChain);
		aBall->mChain = nullptr;
	}
	aBall->setOn(true);
	if (mRope != nullptr && mRope->mBall == aBall)
		letGo();
}

// 0x412bf0
void WordBallSystem::removeBall(unsigned int theIndex)
{
	Ball* aBall = getBall(theIndex);
	if (aBall->mChain != nullptr)
	{
		removeChain(aBall->mChain);
		aBall->mChain = nullptr;
	}
	aBall->mState = BALL_GONE;
	aBall->mSpecial = 0;
	aBall->mLinked = false;
	aBall->mSelector = nullptr;
	aBall->removeAllChildren();
}

// 0x412ca0: every free ball picked, each 0.1 s after the one before it
void WordBallSystem::pickAll()
{
	unsigned int i = 0;
	float aDelay = 0.0f;
	if (mCount != 0)
	{
		do
		{
			if (getBall(i)->mState == BALL_FREE)
			{
				pickBall(i, aDelay);
				aDelay += 0.1f;
			}
			i++;
		} while (i < mCount);
	}
	if (mMode == MODE_FREEMATCH)
		mFill = 0;
}

// 0x4137b0
void WordBallSystem::addHud()
{
	Level* aLevel = Game::instance()->mCurrentLevel;
	addChild(mHud);
	mHud->reset();
	if (mMode == MODE_PUZZLE)
	{
		std::string* aWord = aLevel->mWords;
		int i = 14;
		do
		{
			mHud->addWord(*aWord);
			aWord++;
			i--;
		} while (i != 0);
	}
	mHud->setDefinition("");
}

// 0x413860: the slot is the one found free by the last call (or a new one); the search for the next one starts
// after it
int WordBallSystem::spawn(float x, float y, char theLetter, unsigned int theFlags, bool theQuiet)
{
	int aSlot = mNextSlot;
	unsigned int aCount = mCount;
	if (aSlot < (int)aCount)
	{
		unsigned int i = aSlot + 1;
		mNextSlot = aCount;
		if (i < aCount)
		{
			do
			{
				if (getBall(i)->mState == BALL_GONE)
				{
					mNextSlot = i;
					break;
				}
				i++;
			} while (i < mCount);
		}
	}
	else
	{
		if (105 < aCount + 1)
			return -1;
		mCount = aCount + 1;
		mNextSlot = aSlot + 1;
	}
	mBallsLeft++;
	Ball* aBall = getBall(aSlot);
	aBall->mState = BALL_STATE_4;
	aBall->mB0 = 1.0f;
	aBall->setPosition(x, y);
	aBall->mOld = Point(x, y);
	aBall->mAcceleration = Point(0, 0);
	aBall->mLetter = (char)toupper(theLetter);
	aBall->mGravity = new Point(-200.0f, 100.0f);
	aBall->mFixed = false;
	aBall->mSpecial = theFlags;
	if (aBall->mLetter == '?')
		aBall->mSpecial = theFlags | BALL_WILDCARD;
	setupBall(aBall);
	aBall->setScale(0.2f, 0.2f);
	mLayer->addChild(aBall);
	ParticleSystem* aSystem = mBlossoms->getSystem();
	aSystem->setPosition(*aBall->getPosition());
	aSystem->startEmitting();
	if (!theQuiet)
		Game::instance()->playSound("BALL_RUSTLE2");
	return aSlot;
}

// 0x4140d0
int WordBallSystem::spawnLetter(char theLetter, bool theQuiet)
{
	int aSlot = mNextSlot;
	unsigned int aCount = mCount;
	if (aSlot < (int)aCount)
	{
		unsigned int i = aSlot + 1;
		mNextSlot = aCount;
		if (i < aCount)
		{
			do
			{
				if (getBall(i)->mState == BALL_GONE)
				{
					mNextSlot = i;
					break;
				}
				i++;
			} while (i < mCount);
		}
	}
	else
	{
		if (105 < aCount + 1)
			return -1;
		mCount = aCount + 1;
		mNextSlot = aSlot + 1;
	}
	Ball* aBall = getBall(aSlot);
	aBall->mState = BALL_STATE_4;
	aBall->mB0 = 1.0f;
	aBall->setPosition(470.0f, -26.0f);
	aBall->mOld = Point(470.0f, -26.0f);
	aBall->mAcceleration = Point(0, 0);
	aBall->mGravity = new Point(-200.0f, 100.0f);
	aBall->mFixed = false;
	aBall->mSpecial = 0;
	aBall->mLetter = theLetter;
	if (theLetter == '?')
		aBall->mSpecial = BALL_WILDCARD;
	setupBall(aBall);
	aBall->setScale(1.0f, 1.0f);
	mLayer->addChild(aBall);
	ParticleSystem* aSystem = mBlossoms->getSystem();
	aSystem->setPosition(*aBall->getPosition());
	aSystem->startEmitting();
	if (!theQuiet)
		Game::instance()->playSound("BALL_RUSTLE2");
	mBallsLeft++;
	return aSlot;
}

// 0x413da0
void WordBallSystem::showDefinition(int theIndex)
{
	Level* aLevel = Game::instance()->mCurrentLevel;
	std::string aDefinition = aLevel->mDefinitions[theIndex];
	if (aDefinition.size() == 0)
		mHud->setDefinition("");
	else
	{
		std::string aText = aLevel->mWords[theIndex];
		aText.append(": ");
		aText.append(aDefinition);
		mHud->setDefinition(aText);
	}
}

// 0x413f00
void WordBallSystem::makeHuds()
{
	mPuzzleHud = new Hud();
	mPuzzleHud->addRef();
	mPuzzleHud->setPosition(-212.0f, -110.0f);
	mPuzzleHud->load("config\\HUD.xml");
	mSolitaireHud = new Hud();
	mSolitaireHud->addRef();
	mSolitaireHud->setPosition(-212.0f, -110.0f);
	mSolitaireHud->load("config\\HUD-solitaire.xml");
	mFreematchHud = new Hud();
	mFreematchHud->addRef();
	mFreematchHud->setPosition(-212.0f, -110.0f);
	mFreematchHud->load("config\\HUD-freematch.xml");
}

// 0x414b90: the bounds, the rope, the chains, the obstacles and the balls against each other; a
// breakable ball is a block of its radius to the others (an invisible box made for the moment)
void WordBallSystem::step()
{
	mBounds->apply();
	unsigned int i = 0;
	if (mRope != nullptr)
		mRope->apply();
	for (unsigned int j = 0; j < mChains.size(); j++)
		mChains[j]->apply();
	if (mObstacles != nullptr)
	{
		unsigned int j = 0;
		while (true)
		{
			if (j >= mObstacles->size())
				break;
			(*mObstacles)[j]->collide(this);
			j++;
		}
	}
	Collisions aCollisions(this);
	aCollisions.apply();
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = getBall(i);
			if ((aBall->mSpecial & BALL_BREAKABLE) != 0 && aBall->mState != BALL_GONE)
			{
				Box aBox(*aBall->getPosition(), 3);
				aBox.mExtents = Point(aBall->mRadius, aBall->mRadius);
				aBox.collide(this);
			}
			i++;
		} while (i < mCount);
	}
}

// 0x414d60: solitaire shows its hints in turn while some are left; the other modes one at random
void WordBallSystem::showHint()
{
	Game* aGame = Game::instance();
	if (mHints.size() != 0 && (0 < aGame->mHints || mMode != MODE_SOLITAIRE))
	{
		TextEffect* anEffect = aGame->mTextEffects->create("hint", 108.0f, 573.0f, nullptr, 0);
		std::string aHint;
		if (mMode == MODE_SOLITAIRE)
		{
			unsigned int anIndex = mHintIndex;
			mHintIndex = anIndex + 1;
			aHint = mHints.at(anIndex);
			if (mHints.size() <= mHintIndex)
				mHintIndex = 0;
		}
		else
			aHint = mHints.at(randomIndex(0, (int)mHints.size() - 1));
		anEffect->setText(aHint.c_str());
		aGame->mLevelHintsUsed++;
		aGame->mHintsUsed++;
		aGame->mHints--;
	}
}

// 0x414fb0 (a method of the queue, its pop_back)
void WordBallSystem::popQueue()
{
	if (mQueue.size() != 0)
		mQueue.pop_back();
}

// 0x415300
void WordBallSystem::reset()
{
	stopHintThread();
	mStarted = false;
	mSpawnTime = 0;
	mHud->restartNextBall();
	mQueue.clear();
	mHud->setPlaying(false);
	mWaterLevel = 0;
	letGo();
}

// 0x4153a0: in freematch the level is deleted (the controller keeps its pointer until it makes the next)
void WordBallSystem::clear()
{
	unsigned int i = 0;
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = getBall(i);
			if (aBall->mChain != nullptr)
			{
				removeChain(aBall->mChain);
				aBall->mChain = nullptr;
			}
			aBall->mState = BALL_GONE;
			aBall->mSpecial = 0;
			aBall->mLinked = false;
			aBall->mSelector = nullptr;
			aBall->removeAllChildren();
			i++;
		} while (i < mCount);
	}
	removeAllChildren();
	mLayer->removeAllChildren();
	reset();
	Level* aLevel;
	if (mMode == MODE_FREEMATCH && (aLevel = Game::instance()->mCurrentLevel) != nullptr)
		delete aLevel;
}

// A row or column group: a free one again, or a new one
static BallGroup* takeGroup(std::vector<BallGroup*>& theFree)
{
	BallGroup* aGroup;
	if (theFree.size() == 0)
		aGroup = new BallGroup;
	else
	{
		aGroup = theFree.back();
		theFree.pop_back();
	}
	for (int i = 0; i < 20; i++)
		aGroup->mIndices[i] = -1;
	aGroup->mCount = 0;
	return aGroup;
}

// 0x415c20: a ball joins the first row whose first ball is within 15 pixels of its height (and the first column
// within 15 pixels across) that has room for it, else starts one; then the rows are sorted from left to right and
// the columns from top to bottom (bubble sorts)
void WordBallSystem::groupBalls()
{
	unsigned int i = 0;
	while (i < mRows.size())
	{
		mFreeGroups.push_back(mRows[i]);
		i++;
	}
	std::vector<BallGroup*>().swap(mRows);
	i = 0;
	while (i < mColumns.size())
	{
		mFreeGroups.push_back(mColumns[i]);
		i++;
	}
	std::vector<BallGroup*>().swap(mColumns);
	unsigned int aBallIndex = 0;
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = getBall(aBallIndex);
			if (aBall->mState != BALL_GONE)
			{
				aBall->mColumn = -1;
				aBall->mRow = -1;
				BallGroup* aGroup;
				i = 0;
				while (true)
				{
					if (i >= mRows.size())
						goto columns;
					aGroup = mRows[i];
					float aY = getBall(aGroup->mIndices[0])->getPosition()->y;
					if (fabsf(aBall->getPosition()->y - aY) < 15.0f)
						break;
					i++;
				}
				if (aGroup->mCount < 20)
				{
					aGroup->mIndices[aGroup->mCount] = aBallIndex;
					aGroup->mCount++;
					aBall->mRow = i;
				}
			columns:
				i = 0;
				while (true)
				{
					if (i >= mColumns.size())
						goto add;
					aGroup = mColumns[i];
					float aX = getBall(aGroup->mIndices[0])->getPosition()->x;
					if (fabsf(aBall->getPosition()->x - aX) < 15.0f)
						break;
					i++;
				}
				if (aGroup->mCount < 20)
				{
					aGroup->mIndices[aGroup->mCount] = aBallIndex;
					aGroup->mCount++;
					aBall->mColumn = i;
				}
			add:
				if (aBall->mRow == -1)
				{
					aGroup = takeGroup(mFreeGroups);
					aGroup->mIndices[0] = aBallIndex;
					aGroup->mCount++;
					aBall->mRow = (int)mRows.size();
					mRows.push_back(aGroup);
				}
				if (aBall->mColumn == -1)
				{
					aGroup = takeGroup(mFreeGroups);
					aGroup->mIndices[0] = aBallIndex;
					aGroup->mCount++;
					aBall->mColumn = (int)mColumns.size();
					mColumns.push_back(aGroup);
				}
			}
			aBallIndex++;
		} while (aBallIndex < mCount);
	}
	i = 0;
	while (i < mRows.size())
	{
		BallGroup* aGroup = mRows[i];
		bool aSorted;
		do
		{
			unsigned int j = 0;
			aSorted = true;
			if (aGroup->mCount == 1)
				break;
			do
			{
				float aX = getBall(aGroup->mIndices[j])->getPosition()->x;
				if (getBall(aGroup->mIndices[j + 1])->getPosition()->x < aX)
				{
					int aSwap = aGroup->mIndices[j + 1];
					aGroup->mIndices[j + 1] = aGroup->mIndices[j];
					aGroup->mIndices[j] = aSwap;
					aSorted = false;
				}
				j++;
			} while (j < aGroup->mCount - 1);
		} while (!aSorted);
		i++;
	}
	i = 0;
	while (i < mColumns.size())
	{
		BallGroup* aGroup = mColumns[i];
		bool aSorted;
		do
		{
			unsigned int j = 0;
			aSorted = true;
			if (aGroup->mCount == 1)
				break;
			do
			{
				float aY = getBall(aGroup->mIndices[j])->getPosition()->y;
				if (getBall(aGroup->mIndices[j + 1])->getPosition()->y < aY)
				{
					int aSwap = aGroup->mIndices[j + 1];
					aGroup->mIndices[j + 1] = aGroup->mIndices[j];
					aGroup->mIndices[j] = aSwap;
					aSorted = false;
				}
				j++;
			} while (j < aGroup->mCount - 1);
		} while (!aSorted);
		i++;
	}
}

// 0x416300
Chain* WordBallSystem::addChain(int theIndex)
{
	Chain* aChain = new Chain(this, 2);
	aChain->mAlong = false;
	aChain->mIndices.push_back(theIndex);
	mChains.push_back(aChain);
	return aChain;
}

// 0x416400: Escape and F toggle the full screen, H asks the controller for help and P pauses
bool WordBallSystem::keyDown(int theKey)
{
	Game* aGame = Game::instance();
	int aKey = toupper(theKey);
	switch (aKey)
	{
	case 0x1b:
	case 'F':
		aGame->screenAction("optionToggleFullScreen");
		return true;
	default:
		return false;
	case 'H':
		aGame->helpKey(1, mMode);
		return true;
	case 'P':
		if (aGame->isPaused())
			aGame->resume();
		else
			aGame->pause(true);
		return true;
	}
}

// 0x416c60: BALL_RUSTLE2 once, the balls side by side ending at the spawn point, then the share of the level's
// maxBalls in play
void WordBallSystem::spawnWord(const std::string& theWord)
{
	Game::instance()->playSound("BALL_RUSTLE2");
	unsigned int i = 0;
	Chain* aChain = nullptr;
	if (theWord.size() != 0)
	{
		do
		{
			int aSlot = spawnLetter(theWord[i], true);
			Ball* aBall = getBall(aSlot);
			Point p = *aBall->getPosition();
			p.x = p.x - (float)(unsigned int)(theWord.size() - i - 1) * 44.0f;
			aBall->setPosition(p);
			aBall->mOld = Point(aBall->getPosition()->x, aBall->getPosition()->y);
			if (i == 0)
			{
				aChain = addChain(aSlot);
				aChain->mWord = theWord;
			}
			else
				aChain->mIndices.push_back(aSlot);
			aBall->mChain = aChain;
			setupBall(aBall);
			i++;
		} while (i < theWord.size());
	}
	float aFill = (float)(unsigned int)mBallsLeft / (float)Game::instance()->mCurrentLevel->mMaxBalls;
	if (aFill < 1.0f)
	{
		mFill = aFill;
		return;
	}
	mFill = 1.0f;
}

// 0x4174f0
void WordBallSystem::spawnWordAt(const std::string& theWord, float x, float y)
{
	Game::instance()->playSound("BALL_RUSTLE2");
	unsigned int i = 0;
	Chain* aChain = nullptr;
	if (theWord.size() != 0)
	{
		do
		{
			int aSlot = spawn(x, y, theWord[i], 0, true);
			Ball* aBall = getBall(aSlot);
			Point p = *aBall->getPosition();
			p.x = p.x - (float)(unsigned int)(theWord.size() - i - 1) * 44.0f;
			aBall->setPosition(p);
			aBall->mOld = Point(aBall->getPosition()->x, aBall->getPosition()->y);
			if (i == 0)
			{
				aChain = addChain(aSlot);
				aChain->mWord = theWord;
			}
			else
				aChain->mIndices.push_back(aSlot);
			aBall->mChain = aChain;
			setupBall(aBall);
			i++;
		} while (i < theWord.size());
	}
}

// Puts a starting ball of a level at theOrigin (part of 0x417b20)
static void placeBall(Ball* theBall, const Point& theOrigin, Level* theLevel, unsigned int theFlags, char theLetter)
{
	theBall->setPosition(theOrigin);
	theBall->mState = BALL_FREE;
	theBall->mOld = Point(theBall->getPosition()->x, theBall->getPosition()->y);
	theBall->mFixed = false;
	theBall->mGravity = new Point(theLevel->mGravity);
	theBall->mSpecial = theFlags;
	theBall->mLetter = (char)toupper(theLetter);
}

// 0x417b20: the puzzle levels put the starting balls in and queue the letters of the words without a bonus, less
// those already in play (the original's erase cuts the pool from the first such letter to its end); solitaire
// deals pieces of the controller's words and queues the letters left, shuffled; freematch queues nothing and
// spawns balls by time. Wildcards from the last level go first.
void WordBallSystem::startLevel()
{
	Game* aGame = Game::instance();
	Level* aLevel = aGame->mCurrentLevel;
	std::string aLetters;
	std::vector<std::string> aPieces;
	mLongestPiece = aLevel->mLongestPiece;
	unsigned int aBalls = aLevel->mBalls;
	mCount = 0;
	unsigned int i = 0;
	do
	{
		mBalls[i].mState = BALL_GONE;
		i++;
	} while (i < 105);
	mNextSlot = 0;
	unsigned int aSlot = 0;
	if (mMode == MODE_PUZZLE)
	{
		std::string aPool;
		mWordIndex.clear();
		i = 0;
		do
		{
			if (aLevel->mWords[i].size() != 0)
			{
				if (aLevel->mBonuses[i] == 0)
					aPool.append(aLevel->mWords[i]);
				mWordIndex[aLevel->mWords[i]] = i;
			}
			i++;
		} while (i < 14);
		for (unsigned int b = 0; b < aLevel->mBubbles.size(); b++)
		{
			Bubble* aBubble = aLevel->mBubbles[b];
			unsigned int c = 0;
			if (aBubble->mLetters.size() != 0)
			{
				do
				{
					size_t anAt = aPool.find(aBubble->mLetters[c]);
					if (anAt != std::string::npos)
						aPool.erase(anAt);
					mCount++;
					c++;
				} while (c < aBubble->mLetters.size());
			}
		}
		aLetters = aPool;
		for (unsigned int b = 0; b < aLevel->mBubbles.size(); b++)
		{
			Bubble* aBubble = aLevel->mBubbles[b];
			if (aBubble->mLetters.size() < 2)
			{
				Ball* aBall = getBall(aSlot);
				placeBall(aBall, aBubble->mOrigin, aLevel, aBubble->mFlags, aBubble->mLetters[0]);
				setupBall(getBall(aSlot));
				aSlot++;
			}
			else
			{
				std::string aWord = aBubble->mLetters;
				unsigned int c = 0;
				Chain* aChain = nullptr;
				if (aBubble->mLetters.size() != 0)
				{
					do
					{
						Ball* aBall = getBall(aSlot);
						Point anOrigin = aBubble->mOrigin;
						anOrigin.x = (float)c * 43.0f + anOrigin.x;
						placeBall(aBall, anOrigin, aLevel, aBubble->mFlags, aWord[c]);
						if (c == 0)
						{
							aChain = addChain(aSlot);
							aChain->mWord = aWord;
						}
						else
							aChain->mIndices.push_back(aSlot);
						aBall->mChain = aChain;
						setupBall(aBall);
						aSlot++;
						c++;
					} while (c < aBubble->mLetters.size());
				}
			}
		}
	}
	else if (mMode == MODE_SOLITAIRE)
	{
		std::vector<std::string> aWords;
		aGame->makeWords(aBalls, aWords);
		unsigned int aPiece = 0;
		if (aLevel->mPieces != 0)
		{
			do
			{
				int aLongest = 0;
				for (unsigned int w = 0; w < aWords.size(); w++)
				{
					int aRun = 0;
					std::string aWord = aWords[w];
					unsigned int c = 0;
					if (aWord.size() != 0)
					{
						do
						{
							if (aWord[c] == '\0')
							{
								if (aLongest <= aRun)
									aLongest = aRun;
								aRun = 0;
							}
							else
								aRun++;
							c++;
						} while (c < aWord.size());
					}
					if (aLongest <= aRun)
						aLongest = aRun;
				}
				if (aLongest < 2)
					break;
				std::string aPieceLetters;
				do
				{
					int w = randomIndex(0, (int)aWords.size() - 1);
					std::string aWord = aWords[w];
					int aLength = randomIndex(2, mLongestPiece);
					int aStart = randomIndex(0, (int)aWord.size() - aLength);
					aPieceLetters = aWord.substr(aStart, aLength);
					if (aPieceLetters.find('0') == std::string::npos)
					{
						aWord.replace(aStart, aLength, aLength, '0');
						aWords[w] = aWord;
					}
					else
						aPieceLetters.clear();
				} while (aPieceLetters.size() == 0);
				aPieces.push_back(aPieceLetters);
				aPiece++;
			} while (aPiece < (unsigned int)aLevel->mPieces);
		}
		std::string anAll;
		for (unsigned int w = 0; w < aWords.size(); w++)
		{
			std::string aWord = aWords[w];
			unsigned int c = 0;
			if (aWord.size() != 0)
			{
				do
				{
					if (aWord[c] != '0')
						anAll.append(1, aWord[c]);
					c++;
				} while (c < aWord.size());
			}
		}
		aLetters = Game::shuffle(anAll);
		mSpawnTime = aLevel->mSpawnDelay;
		mBallsToCome = aBalls;
	}
	else if (mMode == MODE_FREEMATCH)
	{
		mHud->restartNextBall();
		mFill = 0;
		mWaterLevel = 0;
		mWaveTime = 0;
		mBallsToCome = aBalls;
		if ((int)aBalls < 1)
			mSpawnTime = aLevel->mSpawnDelay;
		else
			mSpawnTime = 0.2f;
	}
	mShortestWord = aLevel->mShortestWord;
	for (i = 0; i < aPieces.size(); i++)
		mQueue.push_back(aPieces[i]);
	i = 0;
	if (aGame->mWildcards != 0)
	{
		do
		{
			mQueue.push_back("?");
			i++;
		} while (i < aGame->mWildcards);
	}
	i = 0;
	if (aLetters.size() != aGame->mWildcards)
	{
		do
		{
			mQueue.push_back(std::string(1, aLetters[i]));
			i++;
		} while (i < aLetters.size() - aGame->mWildcards);
	}
	aGame->mWildcards = 0;
	mObstacles = &aLevel->mObstacles;
	i = 0;
	while (i < mObstacles->size())
	{
		(*mObstacles)[i]->setActive(true);
		i++;
	}
	addHud();
	addChild(mTarget);
	addChild(mLayer);
	addChild(mBlossoms);
	i = 0;
	while (i < mObstacles->size())
	{
		mLayer->addChild((*mObstacles)[i]);
		i++;
	}
	i = 0;
	if (mCount != 0)
	{
		do
		{
			mLayer->addChild(getBall(i));
			i++;
		} while (i < mCount);
	}
	mNoPicking = false;
	if (mRope != nullptr)
	{
		mRope->mBall->setOn(false);
		delete mRope;
		mRope = nullptr;
		mTarget->setVisible(false);
		mHud->setControlsEnabled(true, false, false);
	}
	mStarted = true;
	aGame->playSound("LEVEL_BEGIN");
	mLayer->fade(true, 0.5f, 1.0f);
	mWord.clear();
	mHud->setPlaying(true);
	aGame->mLevelTime = 0;
	aGame->mLevelPoints = 0;
	mBallsLeft = mCount;
}

// 0x418f30
void WordBallSystem::queueLetters(std::string theLetters, const std::vector<std::string>& thePieces)
{
	unsigned int aPiece;
	for (aPiece = 0; aPiece < thePieces.size(); aPiece++)
	{
		std::string aLetters = thePieces[aPiece];
		unsigned int c = 0;
		if (aLetters.size() != 0)
		{
			do
			{
				size_t anAt = theLetters.find(aLetters[c]);
				if (anAt != std::string::npos)
					theLetters.erase(anAt, 1);
				c++;
			} while (c < aLetters.size());
		}
		mQueue.push_back(aLetters);
	}
	unsigned int i = 0;
	if (theLetters.size() != 0)
	{
		do
		{
			mQueue.push_back(std::string(1, theLetters[i]));
			i++;
		} while (i < theLetters.size());
	}
}

// 0x4191e0: the chains' letters still in play as words of their own, the loose letters together; the puzzle
// levels' finder fills the hints directly (and the button needs hints left), the others' only when it finds some
void WordBallSystem::findHints()
{
	Game* aGame = Game::instance();
	std::string aLetters;
	std::vector<std::string> aChains;
	WordBallSystem* aSystem = aGame->mBallSystem;
	for (unsigned int i = 0; i < aSystem->mChains.size(); i++)
	{
		Chain* aChain = aSystem->mChains[i];
		std::list<int>::iterator aLink = aChain->mIndices.begin();
		if (aLink != aChain->mIndices.end())
		{
			do
			{
				Ball* aBall = aSystem->getBall(*aLink);
				if (aBall->mState == BALL_FREE || aBall->mState == BALL_STATE_4)
					aLetters.append(1, aBall->mLetter);
				aLink++;
			} while (aLink != aChain->mIndices.end());
		}
		aChains.push_back(aLetters);
		aLetters.clear();
	}
	unsigned int i = 0;
	if (aSystem->mCount != 0)
	{
		do
		{
			Ball* aBall = aSystem->getBall(i);
			if ((aBall->mState == BALL_FREE || aBall->mState == BALL_STATE_4) && aBall->mChain == nullptr)
				aLetters.append(1, aBall->mLetter);
			i++;
		} while (i < aSystem->mCount);
	}
	if (aGame->mMode == MODE_SOLITAIRE)
	{
		bool aFound = aGame->mDictionary.findWords(aLetters, aChains, &aSystem->mHints);
		if (!aFound || aGame->mHints == 0)
			aSystem->mHud->setHintReady(false);
		else
			aSystem->mHud->setHintReady(true);
	}
	else
	{
		std::vector<std::string> aWords;
		bool aFound = aGame->mDictionary.findAnyWords(aLetters, aChains, aWords);
		if (aFound)
			aSystem->mHints = aWords;
		aSystem->mHud->setHintReady(aFound);
	}
}

// 0x419540: the original starts findHints on a thread (or logs "still finding hints..." while one runs); the port
// runs it at once
void WordBallSystem::startHints()
{
	mHintIndex = 0;
	m1F0 = false;
	if (mMode != MODE_FREEMATCH)
	{
		mHud->setHintReady(false);
		mHints.clear();
	}
	findHints();
}

// 0x4195d0: an empty queue gets a piece cut from a run of random letters, then the rest of them and another run
// one by one, in a random order
void WordBallSystem::nextPiece(std::string& theResult, bool theKeep)
{
	Game* aGame = Game::instance();
	if (mQueue.size() == 0)
	{
		std::vector<std::string> aPieces;
		std::string aLetters = aGame->makeLetters(4, 10);
		if (1 < mLongestPiece)
		{
			int aLength = randomIndex(2, mLongestPiece);
			int aStart = randomIndex(0, (int)aLetters.size() - aLength);
			std::string aPiece = aLetters.substr(aStart, aLength);
			aLetters.erase(aStart, aLength);
			aPieces.push_back(aPiece);
		}
		aLetters.append(aGame->makeLetters(4, 10));
		unsigned int i = 0;
		if (aLetters.size() != 0)
		{
			do
			{
				aPieces.push_back(std::string(1, aLetters[i]));
				i++;
			} while (i < aLetters.size());
		}
		while (aPieces.size() != 0)
		{
			int anIndex = randomIndex(0, (int)aPieces.size() - 1);
			mQueue.push_back(aPieces[anIndex]);
			aPieces.erase(aPieces.begin() + anIndex);
		}
	}
	std::string aNext = mQueue.back();
	if (!theKeep && mQueue.size() != 0)
		mQueue.pop_back();
	theResult = aNext;
}

// 0x419f40
void WordBallSystem::spawnNext()
{
	if (mMode == MODE_FREEMATCH)
	{
		float aFill = (float)(unsigned int)mBallsLeft / (float)Game::instance()->mCurrentLevel->mMaxBalls;
		if (1.0f <= aFill)
			aFill = 1.0f;
		mFill = aFill;
		std::string aNext;
		nextPiece(aNext, false);
		if (aNext.size() < 2)
			spawnLetter(aNext[0], false);
		else
			spawnWord(aNext);
		startHints();
		mSpawnTime = Game::instance()->mCurrentLevel->mSpawnDelay;
	}
}

// 0x41a050
void WordBallSystem::addWildcard()
{
	mQueue.push_back("?");
	if (mMode == MODE_SOLITAIRE)
	{
		Game::instance()->mLettersUsed++;
		return;
	}
	spawnNext();
	mHud->restartNextBall();
}

// Whether theLetters (with '?' for a wildcard) make the bonus word theBonus, letter by letter (inline in 0x41a120 and
// twice in 0x41a8b0)
static bool matchesBonus(const std::string& theBonus, const std::string& theLetters)
{
	const char* aLetter = theLetters.c_str();
	const char* aBonusLetter = theBonus.c_str();
	char c = *aBonusLetter;
	for (; c != '\0' && (c == *aLetter || *aLetter == '?'); aLetter++)
	{
		c = aBonusLetter[1];
		aBonusLetter++;
	}
	return *aBonusLetter == '\0' && *aLetter == '\0';
}

// 0x41a120: theCount balls from theIndices[theStart] make theWord (theLetters as on the balls, '?' for a wildcard): the
// balls picked, the specials' effects and sounds, and the points. Without +0x194 the balls only light up and the HUD
// shows its submit button. (The original also copies theLetters and calls the first ball's getPosition, using neither.)
void WordBallSystem::scoreWord(const int* theIndices, int theStart, const std::string& theLetters,
	const std::string& theWord, int theCount)
{
	Game* aGame = Game::instance();
	if (theLetters.size() == 0)
		return;
	if (!m194)
	{
		unsigned int i = 0;
		if (theLetters.size() != 0)
		{
			const int* anIndex = theIndices + theStart;
			do
			{
				Ball* aBall = getBall(*anIndex);
				aBall->mLinked = true;
				aBall->setOn(true);
				i++;
				anIndex++;
			} while (i < theLetters.size());
		}
		mHud->showSubmit(true);
		return;
	}

	Ball* aBall = getBall(theIndices[theStart]);
	int aScore = (int)theLetters.size() * 100;
	int aMultiplier = 1;
	float aDelay = 0.0f;
	std::vector<std::string> aChainWords;
	if (theCount != 0)
	{
		int aLeft = theCount;
		const int* anIndex = theIndices + theStart;
		do
		{
			int aBallIndex = *anIndex;
			aBall = getBall(aBallIndex);
			if (theLetters.compare(0, theLetters.size(), "BOMB", 4) == 0)
				aBall->mSpecial |= BALL_BOMB;
			unsigned int aSpecial = aBall->mSpecial;
			if ((aSpecial & BALL_X2) == 0)
			{
				if ((aSpecial & BALL_X3) == 0)
				{
					if ((aSpecial & BALL_X4) != 0)
						aMultiplier *= 4;
				}
				else
				{
					aMultiplier *= 3;
				}
			}
			else
			{
				aMultiplier *= 2;
			}
			Chain* aChain = aBall->mChain;
			if (aChain != NULL && aChain->mWord.size() != 0)
			{
				aChainWords.push_back(aChain->mWord);
				aChain->mWord.erase(aChain->mWord.begin(), aChain->mWord.end());
			}
			pickBall(aBallIndex, aDelay);
			aDelay += 0.1f;
			anIndex++;
			aLeft--;
		} while (aLeft != 0);
	}

	// The effects start at the last ball
	Point aPosition = *aBall->getPosition();
	aPosition.x += getPosition()->x;
	aPosition.y += getPosition()->y;

	if (0.0 < mLastMatch)
	{
		App* anApp = App::instance();
		double aSince = (double)(int)anApp->getTime() - mLastMatch;
		if (aSince / (double)anApp->getTicksPerSecond() < 2.0)
		{
			aGame->mTextEffects->create("speedy", aPosition.x, aPosition.y, nullptr, 0);
			aGame->playSound("SPEEDY_MATCH");
			aMultiplier *= 2;
		}
	}

	if (5 < theLetters.size())
	{
		aGame->mTextEffects->create("longword", aPosition.x, aPosition.y, nullptr, 0);
		aGame->playSound("SPECIAL_LONGWORD");
		aMultiplier *= 2;
		// A long word not made before earns a wildcard
		if (mMode != MODE_PUZZLE)
		{
			std::vector<std::string>::iterator anIt = m204.begin();
			if (anIt != m204.end())
			{
				while (anIt->compare(0, anIt->size(), theWord.c_str(), theWord.size()) != 0 && ++anIt != m204.end())
					;
				if (anIt != m204.end())
					goto aBonus;
			}
			m204.push_back(theWord);
			if (mMode == MODE_FREEMATCH)
				addWildcard();
			else
				aGame->mWildcards++;
		}
	}

aBonus:
	if (mWord.size() != 0)
	{
		if (matchesBonus(mWord, theLetters))
		{
			aGame->mTextEffects->create("bonusWord", aPosition.x, aPosition.y, nullptr, 0);
			aGame->playSound("SPECIAL_BONUS");
			aMultiplier *= 10;
			mWord.erase(mWord.begin(), mWord.end());
			pickAll();
		}
	}

	TextEffect* anEffect = aGame->mTextEffects->create("foundWord", aPosition.x, aPosition.y, &Game::scoreShown, (int)theLetters.size());
	anEffect->setText(theWord);
	aGame->playSound("BALL_POP_WIND");
	int aPoints = aScore * aMultiplier;
	aGame->mLettersUsed += (int)theLetters.size();
	aGame->addScore(aPoints);
	if (mMode == MODE_SOLITAIRE || mMode == MODE_FREEMATCH)
		mHud->addFound(theLetters, theWord, aChainWords, aPoints, aMultiplier);
	aGame->noteWord(theWord);
}

// 0x41a8b0: the words in a row or a column of balls, theMinLength letters or more. From each start the longest run
// that makes a word is kept: a run breaks at a ball not free or too far from the one before, and every chain in it must
// lie wholly in it. A word overlapping the kept one replaces it only when longer; the kept word is scored once a later
// start passes its end, and at the end. A wildcard ball takes its letter from the word, and a multiplier ball has no
// letter (a quirk kept: the wildcards' balls are then found by the letters' positions, not the balls'). Returns how
// many words it scored, counting the empty one "scored" when a start passes the end before any was kept.
int WordBallSystem::scoreGroup(BallGroup* theGroup, unsigned int theMinLength)
{
	Game* aGame = Game::instance();
	int aScored = 0;
	unsigned int aKeptCount = 0;
	unsigned int aKeptEnd = 0;
	unsigned int aKeptStart = 0;
	std::string aKeptLetters;
	std::string aKeptWord;
	std::string aLetters;
	std::string aWord;
	std::vector<std::string> aWords;
	std::set<int> aBallIndices;
	std::set<Chain*> aChains;
	unsigned int aCount = theGroup->mCount;
	if (aCount < theMinLength)
		return 0;
	unsigned int aStart = 0;
	do
	{
		unsigned int aLength = aCount - aStart;
	aTry:
		if (aLength < theMinLength)
			goto aNextStart;
		aLetters.erase();
		aBallIndices.clear();
		aChains.clear();
		{
			bool aBroken = false;
			unsigned int i = 0;
			if (aLength != 0)
			{
				const int* anIndex = theGroup->mIndices + aStart;
				do
				{
					int aBallIndex = *anIndex;
					Ball* aBall = getBall(aBallIndex);
					if (aBall->mState != BALL_FREE || (i != 0 && getGap(aBallIndex, anIndex[-1]) > 3.0f))
					{
						aBroken = true;
						break;
					}
					if ((aBall->mSpecial & BALL_WILDCARD) != 0)
						aLetters.append(1, '?');
					else if ((aBall->mSpecial & (BALL_X2 | BALL_X3 | BALL_X4)) == 0)
						aLetters.append(1, aBall->mLetter);
					if (aBall->mChain != NULL)
					{
						aBallIndices.insert(aBallIndex);
						aChains.insert(aBall->mChain);
					}
					i++;
					anIndex++;
				} while (i < aLength);
			}
			if (aLetters.size() < theMinLength)
				goto aNextStart;
			if (aBroken)
				goto aShorter;
		}
		if (aChains.size() != 0)
		{
			std::set<Chain*>::iterator aChain = aChains.begin();
			if (aChain != aChains.end())
			{
				do
				{
					std::list<int>& anIndices = (*aChain)->mIndices;
					std::list<int>::iterator anIt = anIndices.begin();
					if (anIt != anIndices.end())
					{
						do
						{
							if (aBallIndices.find(*anIt) == aBallIndices.end())
								goto aShorter;
							++anIt;
						} while (anIt != anIndices.end());
					}
					++aChain;
				} while (aChain != aChains.end());
			}
		}
		aWords.clear();
		aWord.erase();
		if (aLetters.find('?', 0) == std::string::npos)
		{
			if (mWord.size() != 0 && matchesBonus(mWord, aLetters))
			{
				aWord = mWord;
			}
			else
			{
				bool isWord = aGame->mWords.checkWord(aLetters.c_str(), false) == 0;
				aWord = aLetters;
				if (!isWord)
					goto aShorter;
			}
		}
		else if (mWord.size() != 0 && matchesBonus(mWord, aLetters))
		{
			aWord = mWord;
		}
		else
		{
			if (aGame->mWords.findMatches(aLetters.c_str(), &aWords) == 0)
				goto aShorter;
			aWord = aWords[randomIndex(0, (int)aWords.size() - 1)];
		}

		if (aStart > aKeptEnd)
		{
			aScored++;
			scoreWord(theGroup->mIndices, aKeptStart, aKeptLetters, aKeptWord, aKeptCount);
		}
		else if (aLength <= aKeptCount)
		{
			goto aNextStart;
		}
		aKeptStart = aStart;
		aKeptLetters = aLetters;
		aKeptWord = aWord;
		aKeptEnd = aLength + aStart - 1;
		aKeptCount = aLength;
		for (std::string::size_type aPos = aLetters.find('?', 0); aPos != std::string::npos;
			aPos = aLetters.find('?', aPos + 1))
		{
			Ball* aBall = getBall(theGroup->mIndices[aPos + aStart]);
			aBall->mLetter = (char)toupper(aWord[aPos]);
		}
		goto aNextStart;

	aShorter:
		aLength--;
		goto aTry;

	aNextStart:
		aCount = theGroup->mCount;
		aStart++;
	} while (aStart <= aCount - theMinLength);
	if (aKeptCount != 0)
	{
		aScored++;
		scoreWord(theGroup->mIndices, aKeptStart, aKeptLetters, aKeptWord, aKeptCount);
	}
	return aScored;
}

// 0x41b2d0: the puzzle's words in a row or a column: from each start the longest run (a run breaks at a ball not free
// or more than 1.5 from the one before; multiplier balls add no letter) that matches a word still to find, which is
// scored, marked in the HUD's list and its definition shown; the next start is the one after it. Returns how many.
int WordBallSystem::scorePuzzleGroup(BallGroup* theGroup)
{
	int aScored = 0;
	unsigned int aStart = 0;
	do
	{
		for (unsigned int aLength = theGroup->mCount - aStart; aLength != 0; aLength--)
		{
			unsigned int i = 0;
			std::string aLetters;
			if (aLength != 0)
			{
				const int* anIndex = theGroup->mIndices + aStart;
				do
				{
					int aBallIndex = *anIndex;
					Ball* aBall = getBall(aBallIndex);
					if (aBall->mState != BALL_FREE || (i != 0 && getGap(aBallIndex, anIndex[-1]) > 1.5f))
						goto aShorter;
					if ((aBall->mSpecial & (BALL_X2 | BALL_X3 | BALL_X4)) == 0)
						aLetters.append(1, aBall->mLetter);
					i++;
					anIndex++;
				} while (i < aLength);
			}
			{
				std::map<std::string, int>::iterator anIt = mWordIndex.begin();
				if (anIt != mWordIndex.end())
				{
					do
					{
						if (matchesBonus(anIt->first, aLetters))
							break;
						++anIt;
					} while (anIt != mWordIndex.end());
					if (anIt != mWordIndex.end())
					{
						aScored++;
						m194 = true;
						scoreWord(theGroup->mIndices, aStart, aLetters, aLetters, aLength);
						mHud->markFound(anIt->second);
						showDefinition(anIt->second);
						mWordIndex.erase(anIt);
						aStart = aStart - 1 + aLength;
						m194 = false;
						break;
					}
				}
			}
		aShorter:;
		}
		aStart++;
	} while (aStart <= theGroup->mCount);
	return aScored;
}

// 0x41b5c0: the puzzle's words in every row and column; how many
int WordBallSystem::scorePuzzle()
{
	unsigned int aScored = 0;
	groupBalls();
	for (unsigned int i = 0; i < mRows.size(); i++)
		aScored += scorePuzzleGroup(mRows[i]);
	for (unsigned int i = 0; i < mColumns.size(); i++)
		aScored += scorePuzzleGroup(mColumns[i]);
	if (aScored > 0)
		mLastMatch = (double)(int)App::instance()->getTime();
	return aScored;
}

// 0x41b650: the words of theMinLength letters or more in every row and column; how many
int WordBallSystem::scoreGroups(unsigned int theMinLength)
{
	unsigned int aScored = 0;
	groupBalls();
	for (unsigned int i = 0; i < mRows.size(); i++)
		aScored += scoreGroup(mRows[i], theMinLength);
	for (unsigned int i = 0; i < mColumns.size(); i++)
		aScored += scoreGroup(mColumns[i], theMinLength);
	if (aScored > 0 && m194)
		mLastMatch = (double)(int)App::instance()->getTime();
	return aScored;
}

// 0x41b6f0: the submit button: the hint finder stopped (inline in the original, as 0x411c50 does) and the words scored
void WordBallSystem::submit()
{
	stopHintThread();
	m194 = true;
	scoreGroups(mShortestWord);
	m194 = false;
	m1F0 = true;
	mHud->setHintReady(false);
}

// 0x41b760: while playing (or in state 7) the physics in steps of 0.03 seconds and each ball by its state: a free
// ball unlit (in freematch dimmed and slowed under the water), a picked one's multiplier and blossom when its time is
// up, a fading one gone, a new one grown and then free. Freematch's water rises with the balls in play, the words are
// looked for, the submit button shown when there are some, and freematch and solitaire spawn their next balls.
void WordBallSystem::update(UpdateInfo& theInfo)
{
	Game* aGame = Game::instance();
	if (aGame->mState == 5 || aGame->mState == 7)
	{
		mWaveTime = theInfo.mSeconds + mWaveTime;
		float aTime = theInfo.mSeconds + mStepTime;
		mStepTime = aTime;
		// 0.03f (0x482784): a float, so the step's time is taken off as a float
		if (0.03f < aTime)
		{
			do
			{
				mStepTime = mStepTime - 0.03f;
				accumulate(0.03f);
				integrate(0.03f, mLastStep);
				int i = 3;
				do
				{
					step();
					i--;
				} while (i != 0);
				mLastStep = 0.03f;
			} while (0.03f < mStepTime);
		}

		unsigned int anIndex = 0;
		if (mCount != 0)
		{
			do
			{
				Ball* aBall = getBall(anIndex);
				if (aBall->mState == BALL_GONE)
				{
					if (anIndex < (unsigned int)mNextSlot)
						mNextSlot = anIndex;
				}
				else
				{
					switch (aBall->mState)
					{
					case BALL_FREE:
						aBall->setOn(false);
						aBall->mLinked = false;
						if ((aBall->mSpecial & BALL_WILDCARD) != 0)
							aBall->mLetter = (char)toupper(' ');
						if (mMode != MODE_FREEMATCH || aBall->getPosition()->y <= (1.0 - mWaterLevel) * 440.0)
						{
							aBall->setOpacity(1.0f);
						}
						else
						{
							aBall->mDamping = (float)(aBall->mDamping - 0.4);
							aBall->setOpacity(0.9f);
						}
						break;
					case BALL_PICKED:
					{
						float aLeft = aBall->mB0 - theInfo.mSeconds;
						aBall->mB0 = aLeft;
						if (aLeft <= 0.0f)
						{
							unsigned int aSpecial = aBall->mSpecial;
							aBall->mState = BALL_PICKED_2;
							aBall->mB0 = (float)(aLeft + 0.2);
							const char* aText = NULL;
							if ((aSpecial & BALL_X2) == 0)
							{
								if ((aSpecial & BALL_X3) != 0)
									aText = "3x";
								else if ((aSpecial & BALL_X4) != 0)
									aText = "4x";
							}
							else
							{
								aText = "2x";
							}
							if (aText != NULL)
							{
								Point aPosition = *aBall->getPosition();
								aPosition.x += getPosition()->x;
								aPosition.y += getPosition()->y;
								aGame->mTextEffects->create(aText, aPosition.x, aPosition.y, nullptr, 0);
								aGame->playSound("SPECIAL_MULTIPLIER");
							}
							ParticleSystem* aSystem = mBlossoms->getSystem();
							aSystem->setPosition(*aBall->getPosition());
							aSystem->startEmitting();
							aGame->playSound("BALL_POP");
						}
						break;
					}
					case BALL_PICKED_2:
					{
						float aLeft = aBall->mB0 - theInfo.mSeconds;
						aBall->mB0 = aLeft;
						float anOpacity = aLeft * 5.0f;
						if (1.0f <= anOpacity)
							anOpacity = 1.0f;
						else if (anOpacity <= 0.0f)
							anOpacity = 0.0f;
						aBall->setOpacity(anOpacity);
						if (aBall->mB0 <= 0.0f)
							removeBall(anIndex);
						break;
					}
					case BALL_STATE_4:
					{
						float aLeft = aBall->mB0 - theInfo.mSeconds;
						aBall->mB0 = aLeft;
						if (mMode != MODE_FREEMATCH)
						{
							float aScale = (1.0f - aLeft) * 4.0f;
							if (aScale < 1.0f)
							{
								if (aScale <= 0.0f)
									aScale = 0.0f;
							}
							else
							{
								aScale = 1.0f;
							}
							aScale = aScale * 0.8f + 0.2f;
							aBall->setScale(aScale, aScale);
						}
						if (aBall->mB0 <= 0.0f)
						{
							// The old gravity leaks, as in the original
							aBall->mB0 = 0;
							aBall->mState = BALL_FREE;
							aBall->mGravity = new Point(aGame->mCurrentLevel->mGravity);
						}
						break;
					}
					}
				}
				anIndex++;
			} while (anIndex < mCount);
		}
		if (mRope != NULL)
			mTarget->setPosition(*mRope->mBall->getPosition());
	}

	if (mMode == MODE_FREEMATCH && aGame->mState == 5)
	{
		Level* aLevel = aGame->mCurrentLevel;
		// the x87 sums in double: each is stored as a float, and compared as it was before the store
		double aFill = 1.0 / ((double)aLevel->mMaxBalls * aLevel->mSpawnDelay) * theInfo.mSeconds + mFill;
		mFill = (float)aFill;
		if (aFill <= mWaterLevel)
		{
			if (aFill < mWaterLevel)
			{
				double aLevelNow = mWaterLevel - (double)theInfo.mSeconds * 0.1f;
				mWaterLevel = (float)aLevelNow;
				if (aLevelNow < mFill)
					mWaterLevel = mFill;
			}
		}
		else
		{
			double aLevelNow = (double)theInfo.mSeconds * 0.1f + mWaterLevel;
			mWaterLevel = (float)aLevelNow;
			if (mFill < aLevelNow)
				mWaterLevel = mFill;
		}
		if (1.0f <= mWaterLevel)
			aGame->gameOver(false, false);
	}
	if (aGame->mState != 5)
		goto aHints;

	{
		int aFound = scorePuzzle();
		if (mMode == MODE_FREEMATCH || mMode == MODE_SOLITAIRE)
			aFound += scoreGroups(mShortestWord);
		mHud->showSubmit(aFound != 0);
	}
	if (mStarted)
		mSpawnTime -= theInfo.mSeconds;
	if (mMode == MODE_FREEMATCH)
	{
		aGame->mMusic->select(waterStage(mWaterLevel), true);
		if (mWord.size() == 0)
		{
			mWord = aGame->makeLetters(5, 5);
			mHud->showBonusWord();
		}
	}
	{
		std::string aPiece;
		if (mMode != MODE_FREEMATCH || 0.0f <= mSpawnTime)
		{
			if (mMode != MODE_SOLITAIRE)
				goto aHints;
			if (mQueue.size() == 0 || mSpawnTime > 0.0f)
				goto aHints;
			aPiece = mQueue[mQueue.size() - 1];
			popQueue();
			mSpawnTime = 0.2f;
			if (aPiece.size() == 1)
				spawn(470.0f, -26.0f, aPiece[0], 0, false);
			else
				spawnWordAt(aPiece, 470.0f, -26.0f);
			if (aGame->mLettersLeft < 0)
				aGame->mLettersLeft = 0;
			if (mQueue.size() == 0)
			{
				stopHintThread();
				startHints();
			}
		}
		else
		{
			nextPiece(aPiece, false);
			if (aPiece.size() < 2)
				spawnLetter(aPiece[0], false);
			else
				spawnWord(aPiece);
			mHud->restartNextBall();
			if (mBallsToCome < 1)
			{
				mSpawnTime = aGame->mCurrentLevel->mSpawnDelay;
				startHints();
			}
			else
			{
				mSpawnTime = 0.1f;
				mBallsToCome--;
				float aFill = (float)(unsigned int)mBallsLeft / (float)aGame->mCurrentLevel->mMaxBalls;
				if (1.0f <= aFill)
					aFill = 1.0f;
				mFill = aFill;
			}
		}
	}

aHints:
	// After a submit the hints are looked for again once the balls are done
	if (m1F0)
	{
		if (!isBusy() && mMode != MODE_PUZZLE)
		{
			stopHintThread();
			startHints();
		}
	}
	mHud->setWildcardReady(0 < aGame->mWildcards);
	Container::update(theInfo);
}

// 0x41c2d0: the right button submits (but in the puzzles), then works as the left one's release
void WordBallSystem::onRightUp(Point& p)
{
	if (mMode != MODE_PUZZLE)
		submit();
	onMouseUp(p);
}

// 0x4142c0
void WordBallSystem::saveState(SavedGame* theGame)
{
	Game* aGame = Game::instance();
	theGame->setHeader(aGame->mLevel, aGame->mScore, mSpawnTime);
	theGame->mBonusWord = mWord;
	for (std::vector<Chain*>::iterator it = mChains.begin(); it != mChains.end(); ++it)
		theGame->addChain(*it);
	unsigned int i = 0;
	if (mCount != 0)
	{
		do
		{
			Ball* aBall = getBall(i);
			if (aBall->mState == BALL_FREE || aBall->mState == BALL_STATE_4)
				theGame->addBall(aBall);
			else
				theGame->addEmpty();
			i++;
		} while (i < mCount);
	}
	mHud->saveWords(theGame);
	for (unsigned int j = 0; j < mQueue.size(); j++)
	{
		std::string aLetters = mQueue[j];
		theGame->addLetters(aLetters);
	}
}

// 0x4199d0: after startLevel, the balls, chains, found words and queue of a saved game
void WordBallSystem::loadState(SavedGame* theGame)
{
	Game* aGame = Game::instance();
	mCount = 0;
	mBallsLeft = 0;
	unsigned int i = 0;
	do
	{
		mBalls[i].mState = BALL_GONE;
		mBalls[i].mChain = nullptr;
		i++;
	} while (i < 105);
	for (std::vector<Chain*>::iterator it = mChains.begin(); it != mChains.end(); ++it)
		delete *it;
	mChains.clear();
	unsigned int aCount = (unsigned int)theGame->mBubbles.size();
	mCount = aCount;
	mNextSlot = (int)aCount;
	i = 0;
	if (aCount != 0)
	{
		do
		{
			Bubble* aBubble = theGame->mBubbles[i];
			if (aBubble != nullptr)
			{
				Ball* aBall = getBall(i);
				mBallsLeft++;
				aBall->setPosition(Point(aBubble->mOrigin));
				aBall->mOld = Point(aBall->getPosition()->x, aBall->getPosition()->y);
				aBall->mFixed = false;
				aBall->mChain = nullptr;
				aBall->mState = BALL_FREE;
				// (the original replaces the ball's gravity without freeing the one it had)
				delete aBall->mGravity;
				aBall->mGravity = new Point(aGame->mCurrentLevel->mGravity);
				aBall->mSpecial = aBubble->mFlags;
				aBall->mLetter = (char)toupper(aBubble->mLetters.c_str()[0]);
				setupBall(aBall);
				mLayer->addChild(aBall);
			}
			i++;
		} while (i < mCount);
	}
	for (i = 0; i < theGame->mChains.size(); i++)
	{
		Chain* aSaved = theGame->mChains[i];
		Chain* aChain = new Chain(this, 2);
		aChain->mWord = aSaved->mWord;
		aChain->mIndices = aSaved->mIndices;
		aChain->mAlong = false;
		mChains.push_back(aChain);
		for (std::list<int>::iterator it = aChain->mIndices.begin(); it != aChain->mIndices.end(); ++it)
		{
			Ball* aBall = getBall(*it);
			aBall->mChain = aChain;
			setupBall(aBall);
		}
	}
	mHud->setWords(theGame->mWords);
	mQueue.clear();
	for (unsigned int j = 0; j < theGame->mQueue.size(); j++)
	{
		std::string aLetters = theGame->mQueue[j];
		mQueue.push_back(aLetters);
	}
	if (mMode == MODE_FREEMATCH)
	{
		mSpawnTime = theGame->mRespawnTimer;
		mHud->restartNextBall();
		float aFill = (float)mBallsLeft / (float)aGame->mCurrentLevel->mMaxBalls;
		if (1.0f <= aFill)
			aFill = 1.0f;
		mFill = aFill;
		mWaterLevel = aFill;
	}
	aGame->playSound("LEVEL_BEGIN");
	if (mMode != MODE_PUZZLE)
	{
		stopHintThread();
		startHints();
	}
	mWord = theGame->mBonusWord;
}
