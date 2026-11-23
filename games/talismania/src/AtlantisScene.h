#ifndef __TALISMANIA_ATLANTISSCENE_H__
#define __TALISMANIA_ATLANTISSCENE_H__

// The game's scene (AtlantisScene over engine::Scene, constructor 0x5e5230, made by AtlantisClassInstance<SceneClass>
// from the scene class named by the application options' sceneclass). Three containers nest in it, camera over shake
// over screen, and the scene class's layers are built under them (0x5e5c00). Translated so far: the three containers and the
// layers' containers. Not yet: the layers' darkening fades, the lists of layers that follow the camera or show in full
// screen only, the regions' scrolling (0x5e6680), the camera and the shake, and the flag 0x100 the constructor sets on
// the camera container.

#include "AtlantisData.h"
#include "Container.h"
#include "Scene.h"
#include "ScenePieces.h"

class AtlantisScene : public engine::Scene
{
public:
	explicit AtlantisScene(const SceneClass& theClass);

	// 0x5e57b0 (the instance's slot): the class this scene was made from
	const SceneClass& getSceneClass() const { return mClass; }

	engine::Ref<engine::Container> mCameraContainer;  // +0x14 of the instance: "Scene Camera Container"
	engine::Ref<engine::Container> mShakeContainer;   // +0x18: "Scene Shake Container"
	engine::Ref<engine::Container> mScreenContainer;  // +0x1c: "Scene Sceen Container" (the original's spelling)

	// 0x5e5c00: a layer's container made and put in the screen container (a darkening layer a filled black rectangle
	// at no opacity, a layer with a clip rectangle a clipping container, any other a plain container)
	void addLayer(const SceneLayerClass& theLayer);
	// 0x5e65a0: the camera put at once where the region (a named offset, "sky" or "land") is shown; a region the
	// scene class does not have is logged
	void setRegion(const std::string& theName);
	// The container of a layer by its name, null for none
	engine::Component* getLayer(const std::string& theName) const;

private:
	SceneClass mClass;
	std::map<std::string, engine::Ref<engine::Component>> mLayers;
};

#endif
