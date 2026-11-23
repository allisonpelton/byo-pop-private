#ifndef __FEEDINGFRENZY2_FISHFACTORY_H__
#define __FEEDINGFRENZY2_FISHFACTORY_H__

// The library of the actors' kinds (FishFactory, 0x70 bytes, one for the program at 0x5ac614 (feedingfrenzy2), made
// on first use by 0x423a20 (feedingfrenzy2), its constructor 0x4a01e0 (feedingfrenzy2)), read by the loader from the
// actors' file (resource/actors/fishes.xml): the models, visuals and behaviours of ActorInfo.h, by name. Its handler
// (FishFactory::DocHandler, 0x5c bytes, its constructor 0x4903c0 (feedingfrenzy2)) reads the elements under
// <fishes>, each through a handler of its own.

#include "ActorInfo.h"
#include "MasterDocHandler.h"

#include <map>
#include <string>

class FishFactory
{
public:
	// 0x423a20 (feedingfrenzy2)
	static FishFactory* get();

	// 0x407f00 (feedingfrenzy2): the file read into the library; whether it parsed (the loader does not look)
	bool load(const std::string& thePath);
	// 0x4897f0 (feedingfrenzy2): the library emptied, each map in turn (the folder kept)
	void clear();

	// 0x4112a0 (feedingfrenzy2): null for none
	ModelInfo* findModel(const std::string& theName) const { return find(mModels, theName); }
	// 0x424d70 (feedingfrenzy2): null for none
	FishModelInfo* findFishModel(const std::string& theName) const { return find(mFishModels, theName); }
	// 0x517e6d (feedingfrenzy2): null for none
	BehaviorInfo* findBehavior(const std::string& theName) const { return find(mBehaviors, theName); }
	// 0x40c9d0 (feedingfrenzy2): null for none
	FishBehaviorInfo* findFishBehavior(const std::string& theName) const { return find(mFishBehaviors, theName); }
	// 0x4100d0 (feedingfrenzy2): the model of the name, else the fish model; null, and a message, for none
	ModelInfo* findAnyModel(const std::string& theName) const;
	// 0x40c550 (feedingfrenzy2): the visual of the name, else a new one of the model of the name, kept under it; null,
	// and a message, for neither
	VisualInfo* findVisual(const std::string& theName);
	// 0x40add0 (feedingfrenzy2): null, and a message, for none
	FishVisualInfo* findFishVisual(const std::string& theName) const;

	std::string mAnimBase;                                                 // +0x00: the animations' folder
	std::map<std::string, engine::Ref<ModelInfo>> mModels;                 // +0x1c
	std::map<std::string, engine::Ref<VisualInfo>> mVisuals;               // +0x28
	// +0x34: a map of the same shape, filled by nothing the actors' file reaches (not yet translated)
	std::map<std::string, engine::Ref<BehaviorInfo>> mBehaviors;           // +0x40
	std::map<std::string, engine::Ref<FishModelInfo>> mFishModels;         // +0x4c
	std::map<std::string, engine::Ref<FishVisualInfo>> mFishVisuals;       // +0x58
	std::map<std::string, engine::Ref<FishBehaviorInfo>> mFishBehaviors;   // +0x64

private:
	template <class T>
	static T* find(const std::map<std::string, engine::Ref<T>>& theMap, const std::string& theName)
	{
		typename std::map<std::string, engine::Ref<T>>::const_iterator anIt = theMap.find(theName);
		return anIt == theMap.end() ? nullptr : anIt->second.get();
	}
};

// FishFactory::DocHandler
class FishFactoryDocHandler : public engine::SubDocHandler
{
public:
	explicit FishFactoryDocHandler(FishFactory* theFactory) : mFactory(theFactory) {}

	// 0x04 of its own table (0x40b3e0 (feedingfrenzy2)): the animations' folder
	void begin(engine::Params& theAttributes) override;
	// 0x00 (0x45e490 (feedingfrenzy2)): a kind of actor, read by a handler for its element, or a behaviour at once
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x5014fa (feedingfrenzy2)): nothing
	void endElement(const std::string& theName) override {}

private:
	FishFactory* mFactory;  // +0x50
};

#endif
