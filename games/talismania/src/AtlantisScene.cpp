#include "AtlantisScene.h"

#include "Engine.h"

// 0x5e5230
AtlantisScene::AtlantisScene(const SceneClass& theClass) : mClass(theClass)
{
	mCameraContainer = new engine::Container();
	mCameraContainer->setName("Scene Camera Container");
	mShakeContainer = new engine::Container();
	mShakeContainer->setName("Scene Shake Container");
	mScreenContainer = new engine::Container();
	mScreenContainer->setName("Scene Sceen Container");
	mCameraContainer->addChild(mShakeContainer.get());
	mShakeContainer->addChild(mScreenContainer.get());
	// The camera container is the scene's only child (0x5e5230 calls 0x64c7f0 on it, whose work is not read: it is
	// taken to put the camera container in the scene)
	addChild(mCameraContainer.get());
	// The original then sets a flag (0x100) on the camera container (0x5e5230), and connects the scene's own update
	// and draw: not yet translated
	// The layers are added in the scene's layer order (the order the original adds them in is not confirmed; a layer
	// the order does not name is not added)
	for (size_t i = 0; i < mClass.layerOrder.size(); i++)
	{
		std::map<std::string, SceneLayerClass>::const_iterator aLayer = mClass.layers.find(mClass.layerOrder[i]);
		if (aLayer != mClass.layers.end())
			addLayer(aLayer->second);
	}
}

void AtlantisScene::addLayer(const SceneLayerClass& theLayer)
{
	engine::Ref<engine::Component> aLayer;
	if (theLayer.darkenLayer)
	{
		// A black rectangle over the whole 800x600 screen, clear until the layer darkens
		engine::RectangleItem* aRect = new engine::RectangleItem(engine::RectF(0, 0, 800, 600));
		aRect->setColor(engine::Color(0.75f, 0.75f, 1.0f, 1.0f));
		aRect->setName(theLayer.name);
		aRect->setColor(0, 0, 0);
		aRect->setOpacity(0);
		aLayer = aRect;
	}
	else if ((theLayer.clip.right - theLayer.clip.left) * (theLayer.clip.bottom - theLayer.clip.top) > 0)
	{
		engine::ClipPane* aPane = new engine::ClipPane();
		aPane->setClip(theLayer.clip);
		aPane->setName(theLayer.name);
		aLayer = aPane;
	}
	else
	{
		engine::Container* aContainer = new engine::Container();
		aContainer->setName(theLayer.name);
		aLayer = aContainer;
	}
	// Placed at the layer's offset and its region's
	engine::Vector2f aPosition = theLayer.baseOffset;
	std::map<std::string, engine::Vector2f>::const_iterator aRegion = mClass.regions.find(theLayer.baseRegion);
	if (aRegion != mClass.regions.end())
	{
		aPosition.x += aRegion->second.x;
		aPosition.y += aRegion->second.y;
	}
	aLayer->setPosition(aPosition);
	mScreenContainer->addChild(aLayer.get());
	mLayers[theLayer.name] = aLayer;
}

engine::Component* AtlantisScene::getLayer(const std::string& theName) const
{
	std::map<std::string, engine::Ref<engine::Component>>::const_iterator aLayer = mLayers.find(theName);
	return aLayer == mLayers.end() ? nullptr : aLayer->second.get();
}

void AtlantisScene::setRegion(const std::string& theName)
{
	std::map<std::string, engine::Vector2f>::const_iterator aRegion = mClass.regions.find(theName);
	if (aRegion == mClass.regions.end())
	{
		engine::getEngine()->log("NO UI REGION NAMED %s\n", theName.c_str());
		return;
	}
	mCameraContainer->setPosition(engine::Vector2f(-aRegion->second.x, -aRegion->second.y));
}
