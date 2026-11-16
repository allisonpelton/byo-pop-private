#include "FishFactory.h"
#include "AngelFish.h"
#include "Barracuda.h"
#include "BasicJellyFish.h"
#include "BonusBubble2X.h"
#include "BonusBubbleFeedingFury.h"
#include "BonusBubbleFreeLife.h"
#include "BonusBubbleShield.h"
#include "BonusBubbleSpeed.h"
#include "BonusBubbleStar.h"
#include "BonusBubbleStun.h"
#include "BrineShrimp.h"
#include "ChaserFish.h"
#include "FlyByFish.h"
#include "GoldenMinnow.h"
#include "Mermaid.h"
#include "Mine.h"
#include "Oyster.h"
#include "PoisonMinnow.h"
#include "PufferFish.h"
#include "Shark.h"
#include "SoldierFish.h"
#include "SuicideFish.h"
#include <map>

namespace feedingfrenzy
{

// The names of the classes a fish file may give, which are also the type names the fish are given
const char* const BRINE_SHRIMP_CLASS = "BrineShrimp";
const char* const ANGEL_FISH_CLASS = "AngelFish";
const char* const SOLDIER_FISH_CLASS = "SoldierFish";
const char* const CHASER_FISH_CLASS = "ChaserFish";
const char* const BONUS_BUBBLE_STAR_CLASS = "BonusBubbleStar";
const char* const BONUS_BUBBLE_FREE_LIFE_CLASS = "BonusBubbleFreeLife";
const char* const OYSTER_CLASS = "Oyster";
const char* const FLY_BY_FISH_CLASS = "FlyByFish";
const char* const GOLDEN_MINNOW_CLASS = "GoldenMinnow";
const char* const MERMAID_CLASS = "Mermaid";
const char* const PUFFER_FISH_CLASS = "PufferFish";
const char* const MINE_CLASS = "Mine";
const char* const BASIC_JELLY_FISH_CLASS = "BasicJellyFish";
const char* const BONUS_BUBBLE_2X_CLASS = "BonusBubble2X";
const char* const BONUS_BUBBLE_SPEED_CLASS = "BonusBubbleSpeed";
const char* const BONUS_BUBBLE_FEEDING_FURY_CLASS = "BonusBubbleFeedingFury";
const char* const BONUS_BUBBLE_STUN_CLASS = "BonusBubbleStun";
const char* const BARRACUDA_CLASS = "Barracuda";
const char* const POISON_MINNOW_CLASS = "PoisonMinnow";
const char* const SHARK_CLASS = "Shark";
const char* const BONUS_BUBBLE_SHIELD_CLASS = "BonusBubbleShield";
const char* const SUICIDE_FISH_CLASS = "SuicideFish";

typedef FishBase* (*FishCreator)();

// 0x41e9e0
static FishBase* createBrineShrimp()
{
	FishBase* aFish = new BrineShrimp();
	aFish->setTypeName(BRINE_SHRIMP_CLASS);
	return aFish;
}

// 0x41ea70
static FishBase* createAngelFish()
{
	FishBase* aFish = new AngelFish();
	aFish->setTypeName(ANGEL_FISH_CLASS);
	return aFish;
}

// 0x41eb00
static FishBase* createSoldierFish()
{
	FishBase* aFish = new SoldierFish();
	aFish->setTypeName(SOLDIER_FISH_CLASS);
	return aFish;
}

// 0x41eb90
static FishBase* createChaserFish()
{
	FishBase* aFish = new ChaserFish();
	aFish->setTypeName(CHASER_FISH_CLASS);
	return aFish;
}

// 0x41ec20
static FishBase* createBonusBubbleStar()
{
	FishBase* aFish = new BonusBubbleStar();
	aFish->setTypeName(BONUS_BUBBLE_STAR_CLASS);
	return aFish;
}

// 0x41ecb0
static FishBase* createBonusBubbleFreeLife()
{
	FishBase* aFish = new BonusBubbleFreeLife();
	aFish->setTypeName(BONUS_BUBBLE_FREE_LIFE_CLASS);
	return aFish;
}

// 0x41ed40
static FishBase* createOyster()
{
	FishBase* aFish = new Oyster();
	aFish->setTypeName(OYSTER_CLASS);
	return aFish;
}

// 0x41edd0
static FishBase* createFlyByFish()
{
	FishBase* aFish = new FlyByFish();
	aFish->setTypeName(FLY_BY_FISH_CLASS);
	return aFish;
}

// 0x41ee60
static FishBase* createGoldenMinnow()
{
	FishBase* aFish = new GoldenMinnow();
	aFish->setTypeName(GOLDEN_MINNOW_CLASS);
	return aFish;
}

// 0x41eef0
static FishBase* createMermaid()
{
	FishBase* aFish = new Mermaid();
	aFish->setTypeName(MERMAID_CLASS);
	return aFish;
}

// 0x41ef80
static FishBase* createPufferFish()
{
	FishBase* aFish = new PufferFish();
	aFish->setTypeName(PUFFER_FISH_CLASS);
	return aFish;
}

// 0x41f010
static FishBase* createMine()
{
	FishBase* aFish = new Mine();
	aFish->setTypeName(MINE_CLASS);
	return aFish;
}

// 0x41f0a0
static FishBase* createBasicJellyFish()
{
	FishBase* aFish = new BasicJellyFish();
	aFish->setTypeName(BASIC_JELLY_FISH_CLASS);
	return aFish;
}

// 0x41f130
static FishBase* createBonusBubble2X()
{
	FishBase* aFish = new BonusBubble2X();
	aFish->setTypeName(BONUS_BUBBLE_2X_CLASS);
	return aFish;
}

// 0x41f1c0
static FishBase* createBonusBubbleSpeed()
{
	FishBase* aFish = new BonusBubbleSpeed();
	aFish->setTypeName(BONUS_BUBBLE_SPEED_CLASS);
	return aFish;
}

// 0x41f250
static FishBase* createBonusBubbleFeedingFury()
{
	FishBase* aFish = new BonusBubbleFeedingFury();
	aFish->setTypeName(BONUS_BUBBLE_FEEDING_FURY_CLASS);
	return aFish;
}

// 0x41f2e0
static FishBase* createBonusBubbleStun()
{
	FishBase* aFish = new BonusBubbleStun();
	aFish->setTypeName(BONUS_BUBBLE_STUN_CLASS);
	return aFish;
}

// 0x41f370
static FishBase* createBarracuda()
{
	FishBase* aFish = new Barracuda();
	aFish->setTypeName(BARRACUDA_CLASS);
	return aFish;
}

// 0x41f400
static FishBase* createPoisonMinnow()
{
	FishBase* aFish = new PoisonMinnow();
	aFish->setTypeName(POISON_MINNOW_CLASS);
	return aFish;
}

// 0x41f490
static FishBase* createShark()
{
	FishBase* aFish = new Shark();
	aFish->setTypeName(SHARK_CLASS);
	return aFish;
}

// 0x41f520
static FishBase* createBonusBubbleShield()
{
	FishBase* aFish = new BonusBubbleShield();
	aFish->setTypeName(BONUS_BUBBLE_SHIELD_CLASS);
	return aFish;
}

// 0x41f5b0
static FishBase* createSuicideFish()
{
	FishBase* aFish = new SuicideFish();
	aFish->setTypeName(SUICIDE_FISH_CLASS);
	return aFish;
}

// 0x420f50: the registry of the classes, each name with the function that makes a fish of it
static std::map<std::string, FishCreator>& getRegistry()
{
	static std::map<std::string, FishCreator> sRegistry;
	if (sRegistry.empty())
	{
		sRegistry[BRINE_SHRIMP_CLASS] = createBrineShrimp;
		sRegistry[ANGEL_FISH_CLASS] = createAngelFish;
		sRegistry[SOLDIER_FISH_CLASS] = createSoldierFish;
		sRegistry[CHASER_FISH_CLASS] = createChaserFish;
		sRegistry[BONUS_BUBBLE_STAR_CLASS] = createBonusBubbleStar;
		sRegistry[BONUS_BUBBLE_FREE_LIFE_CLASS] = createBonusBubbleFreeLife;
		sRegistry[OYSTER_CLASS] = createOyster;
		sRegistry[FLY_BY_FISH_CLASS] = createFlyByFish;
		sRegistry[GOLDEN_MINNOW_CLASS] = createGoldenMinnow;
		sRegistry[MERMAID_CLASS] = createMermaid;
		sRegistry[PUFFER_FISH_CLASS] = createPufferFish;
		sRegistry[MINE_CLASS] = createMine;
		sRegistry[BASIC_JELLY_FISH_CLASS] = createBasicJellyFish;
		sRegistry[BONUS_BUBBLE_2X_CLASS] = createBonusBubble2X;
		sRegistry[BONUS_BUBBLE_SPEED_CLASS] = createBonusBubbleSpeed;
		sRegistry[BONUS_BUBBLE_FEEDING_FURY_CLASS] = createBonusBubbleFeedingFury;
		sRegistry[BONUS_BUBBLE_STUN_CLASS] = createBonusBubbleStun;
		sRegistry[BARRACUDA_CLASS] = createBarracuda;
		sRegistry[POISON_MINNOW_CLASS] = createPoisonMinnow;
		sRegistry[SHARK_CLASS] = createShark;
		sRegistry[BONUS_BUBBLE_SHIELD_CLASS] = createBonusBubbleShield;
		sRegistry[SUICIDE_FISH_CLASS] = createSuicideFish;
	}
	return sRegistry;
}

// 0x4208b6 (the lookup that FishSpawner::createFish does inline)
FishBase* createFishOfClass(const std::string& theClassName)
{
	const std::map<std::string, FishCreator>& aRegistry = getRegistry();
	std::map<std::string, FishCreator>::const_iterator aClass = aRegistry.find(theClassName);
	if (aClass == aRegistry.end())
		return nullptr;
	return aClass->second();
}

} // namespace feedingfrenzy
