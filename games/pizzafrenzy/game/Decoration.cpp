#include "Decoration.h"
#include "CityMap.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
const char* const DECORATION_CLASS = "decoration";
}

// 0x4114b0
Decoration::Decoration(const std::string& theName, int theLayer) : mMap(nullptr), mLayer(theLayer), mName(theName)
{
	mPosition.mFirst = 0;
	mPosition.mSecond = 0;
}

// 0x411570
bool Decoration::load()
{
	TileManifest* aManifest = PizzaFrenzy::instance()->mTileManifest;
	TileType* anEntry = aManifest->getTile(mName, DECORATION_CLASS);
	if (anEntry != nullptr)
	{
		mNode = new engine::ImageNode(anEntry->mImage);
		mImage = anEntry->mImage;
		mLayer = anEntry->mLayer;
		return true;
	}
	ParticleEffect* anEffect = aManifest->getParticleEffect(mName);
	if (anEffect != nullptr)
	{
		engine::SignalParticleSystem* aSystem = new engine::SignalParticleSystem();
		mParticleNode = aSystem;
		aSystem->setUp(anEffect->mDef);
		mLayer = anEffect->mLayer;
		aSystem->start();
		return true;
	}
	return false;
}

// 0x411260
void Decoration::addTo(CityMap* theMap)
{
	mMap = theMap;
	theMap->addDecoration(this);
}

// 0x411750
engine::Node* Decoration::getNode(int theLayer)
{
	if (theLayer != mLayer)
		return nullptr;
	if (mNode != nullptr)
	{
		mNode->setName("decoration");
		return mNode;
	}
	return mParticleNode;
}

// 0x411210
void Decoration::setPosition(const engine::IntPair& thePosition)
{
	engine::Node* aNode = mNode != nullptr ? (engine::Node*)mNode : (engine::Node*)mParticleNode;
	if (aNode != nullptr)
		aNode->setPosition((float)thePosition.mFirst, (float)thePosition.mSecond);
	mPosition = thePosition;
}
