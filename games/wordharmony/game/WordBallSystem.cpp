#include "WordBallSystem.h"
#include "App.h"
#include "FadeContainer.h"
#include "Game.h"
#include "Graphics.h"
#include "Hud.h"
#include "ImageNode.h"
#include "Level.h"
#include "Obstacles.h"
#include "Particles.h"
#include "Random.h"
#include "Selector.h"
#include "TextEffects.h"
#include <cctype>
#include <cmath>
#include <cstdio>

using namespace engine;

// 0x416fd0. The original leaves +0x108, +0x124-+0x12c, +0x1a8, +0x1d8, +0x1f4 and +0x200 unset; the port starts them
// at zero.
WordBallSystem::WordBallSystem(int theMode)
{
	mMode = theMode;
	mObstacles = nullptr;
	mFC = 0;
	mSpawnTime = 0;
	mStarted = false;
	mLongestPiece = 0;
	mNextSlot = 0;
	mBallsToCome = 0;
	mWaterLevel = 0;
	mFill = 0;
	mShortestWord = 0;
	m194 = false;
	mNoPicking = false;
	mWaveTime = 0;
	mHintIndex = 0;
	mEditor = false;
	m1FC = 0;
	mLevelBackup = nullptr;
	setName("WordBallSystem");
	mBounds = new Bounds(this, 441.0f);
	mRope = nullptr;
	mFollow = nullptr;
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
	if ((aFlags & BALL_FLAG_1) == 0)
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
		if ((aFlags & BALL_FLAG_20) != 0)
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
		if ((aFlags & BALL_STONE) != 0)
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
	mHud->grabChanged(false, false, false);
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
	mHud->grabChanged(true, false, false);
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
int WordBallSystem::weightedRandom(float theValue)
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
			aGame->finishPuzzles(false, false);
			mNoPicking = true;
			return;
		}
	}
	aGame->setState(7);
	mNoPicking = true;
}

// 0x412080
Obstacle* WordBallSystem::findObstacle(const Point& thePoint)
{
	std::vector<Obstacle*>::iterator anIt = mObstacles->begin();
	Obstacle* aBest = nullptr;
	float aBestDistance = 529.0f;
	if (anIt == mObstacles->end())
		return nullptr;
	do
	{
		Obstacle* anObstacle = *anIt;
		Point p = *anObstacle->getPosition();
		float aDistance = (thePoint.y - p.y) * (thePoint.y - p.y) + (thePoint.x - p.x) * (thePoint.x - p.x);
		if (aDistance < aBestDistance)
		{
			aBestDistance = aDistance;
			aBest = anObstacle;
		}
		anIt++;
	} while (anIt != mObstacles->end());
	return aBest;
}

// 0x412120: a free ball near the mouse gets the rope (or in the editor the drag), and lights up
void WordBallSystem::onMouseDown(Point& p)
{
	if (mNoPicking)
		return;
	unsigned int anIndex = findBall(mMouse);
	if (anIndex == (unsigned int)-1)
	{
		Obstacle* anObstacle;
		if (mEditor && (anObstacle = findObstacle(mMouse)) != nullptr && mFollow == nullptr)
		{
			mFollow = new Follow(anObstacle, 21.99f, 0.3f, 0.8f, 44.0f);
			mFollow->mSnap = Game::instance()->mGridSnap;
			mFollow->mTarget = mMouse;
		}
		return;
	}
	Ball* aBall = getBall(anIndex);
	if (!aBall->mFixed || mEditor)
	{
		if (mRope == nullptr && !mEditor)
			grab(aBall);
		else if (mFollow == nullptr && mEditor)
		{
			mFollow = new Follow(aBall, 21.99f, 0.3f, 0.8f, 44.0f);
			mFollow->mSnap = Game::instance()->mGridSnap;
			mFollow->mTarget = mMouse;
		}
		aBall->setOn(true);
	}
}

// 0x411d80
void WordBallSystem::onMouseUp(Point& p)
{
	if (mRope != nullptr)
		letGo();
	if (mFollow != nullptr)
	{
		delete mFollow;
		mFollow = nullptr;
	}
}

// 0x411260 (the ball under the mouse is looked up and not used)
void WordBallSystem::mouseMove(Point& p)
{
	mMouse = p;
	toLocal(mMouse);
	if (mRope != nullptr)
		mRope->mTarget = mMouse;
	if (mFollow != nullptr)
		mFollow->mTarget = mMouse;
	unsigned int anIndex = findBall(mMouse);
	if (anIndex != (unsigned int)-1)
		getBall(anIndex);
}

// 0x412310: in freematch the water (three layers of waves and the deep water under them) at the water level, then
// the children, then the links of each chain (the first link on its first ball)
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

// 0x412ca0
void WordBallSystem::pickAll()
{
	unsigned int i = 0;
	if (mCount != 0)
	{
		do
		{
			if (getBall(i)->mState == BALL_FREE)
				pickBall(i, 0.0f);
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
	if (theLetter == '\0')
	{
		std::string aLetters = Game::instance()->randomLetters(1);
		aBall->mLetter = (char)toupper(aLetters.at(0));
	}
	else
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
		Game::instance()->playSound("BALL_RUSTLE2", false);
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
		Game::instance()->playSound("BALL_RUSTLE2", false);
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

// 0x414b90: the bounds, the rope and the drag, the chains, the obstacles and the balls against each other; a
// breakable ball is a block of its radius to the others (an invisible box made for the moment)
void WordBallSystem::step()
{
	mBounds->apply();
	unsigned int i = 0;
	if (mRope != nullptr)
		mRope->apply();
	if (mFollow != nullptr)
		mFollow->apply();
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
		TextEffect* anEffect = aGame->mTextEffects->create("hint", 108.0f, 573.0f, 0);
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
		aGame->mHintsUsedTotal++;
		aGame->mHintsUsed++;
		aGame->mHints--;
	}
}

// 0x414fb0
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
	mHud->clearScore();
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
	if (mMode == MODE_FREEMATCH && (aLevel = Game::instance()->mCurrentLevel) != nullptr && !mEditor)
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

// 0x416400: in the editor, keys change the dragged ball (or delete it or the dragged obstacle); else Escape and F
// toggle the full screen, H asks the controller for help and P pauses
bool WordBallSystem::keyDown(int theKey)
{
	Game* aGame = Game::instance();
	int aKey = toupper(theKey);
	if (mEditor && mFollow != nullptr)
	{
		Node* aNode = mFollow->mNode;
		Ball* aBall = static_cast<Ball*>(aNode);
		std::string aName = aNode->getName();
		if (aKey > '@' && aKey < '[')
		{
			if (aName == "node")
			{
				aBall->setLetter((char)aKey);
				if ((aBall->mSpecial & BALL_BREAKABLE) == 0)
					aBall->mSpecial = (int)(char)aBall->mSpecial & BALL_STONE;
				else
					aBall->mSpecial = BALL_BREAKABLE;
				setupBall(aBall);
			}
			return true;
		}
		if (aKey == ' ')
		{
			if (aName == "node")
			{
				Ball* aTarget = aBall;
				if (aBall->mChain == nullptr)
				{
					if (aBall->mLetter == ' ')
					{
						unsigned int i = 0;
						if (mCount != 0)
						{
							do
							{
								if (aBall == getBall(i))
									break;
								i++;
							} while (i < mCount);
						}
						if (i < mCount)
						{
							Chain* aChain = addChain(i);
							aChain->mWord = " ";
							aBall->mChain = aChain;
						}
					}
					else
						aBall->setLetter(' ');
				}
				else
				{
					unsigned int i = 0;
					if (mCount != 0)
					{
						do
						{
							aTarget = getBall(i);
							if (aTarget->mChain == nullptr)
								break;
							i++;
						} while (i < mCount);
					}
					if (i < mCount)
					{
						aBall->mChain->mIndices.push_back(i);
						aTarget->mChain = aBall->mChain;
					}
				}
				setupBall(aTarget);
				return true;
			}
		}
		else if (aKey == '7')
		{
			if (aName == "node")
			{
				aBall->mSpecial = BALL_WILDCARD;
				aBall->setLetter(' ');
				setupBall(aBall);
				return true;
			}
		}
		else if (aKey == '8')
		{
			if (aName == "node")
			{
				aBall->mSpecial = 0;
				setupBall(aBall);
				return true;
			}
		}
		else if (aKey == '9')
		{
			if (aName == "node")
			{
				aBall->mSpecial = BALL_STONE;
				setupBall(aBall);
				aBall->setLetter(' ');
				return true;
			}
		}
		else if (aKey == '0')
		{
			if (aName == "node")
			{
				aBall->mSpecial = BALL_BREAKABLE;
				setupBall(aBall);
				return true;
			}
		}
		else
		{
			if (aKey > '1' && aKey < '5')
			{
				if (aName == "node")
				{
					unsigned int aFlags;
					if (aKey == '2')
						aFlags = aBall->mSpecial | BALL_X2;
					else if (aKey == '3')
						aFlags = aBall->mSpecial | BALL_X3;
					else
					{
						if (aKey != '4')
							return true;
						aFlags = aBall->mSpecial | BALL_X4;
					}
					aBall->mSpecial = aFlags;
					aBall->setLetter(' ');
					setupBall(aBall);
				}
				return true;
			}
			if (aKey == '.')
			{
				if (aName == "node")
				{
					if (aBall->mChain == nullptr)
					{
						aBall->mState = BALL_GONE;
						aBall->mSpecial = 0;
					}
					else
						removeChain(aBall->mChain);
				}
				else if (aName == "obstacle")
				{
					mLayer->removeChild(aNode);
					std::vector<Obstacle*>::iterator anIt = mObstacles->begin();
					if (anIt != mObstacles->end())
					{
						do
						{
							if (*anIt == aNode)
							{
								mObstacles->erase(anIt);
								break;
							}
							anIt++;
						} while (anIt != mObstacles->end());
					}
				}
				delete mFollow;
				mFollow = nullptr;
				return true;
			}
		}
	}
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
	Game::instance()->playSound("BALL_RUSTLE2", false);
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
	Game::instance()->playSound("BALL_RUSTLE2", false);
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
	unsigned int aBalls;
	if (!mEditor)
		aBalls = aLevel->mBalls;
	else
		aBalls = 0;
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
		mHud->clearScore();
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
		mHud->grabChanged(true, false, false);
	}
	if (mFollow != nullptr)
	{
		delete mFollow;
		mFollow = nullptr;
	}
	mStarted = true;
	aGame->playSound("LEVEL_BEGIN", false);
	mLayer->fade(true, 0.5f, 1.0f);
	mWord.clear();
	mHud->setPlaying(true);
	aGame->m18C = 0;
	aGame->m38 = 0;
	mBallsLeft = mCount;
}
