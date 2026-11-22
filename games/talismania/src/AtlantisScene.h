#ifndef __TALISMANIA_ATLANTISSCENE_H__
#define __TALISMANIA_ATLANTISSCENE_H__

// The game's scene (AtlantisScene over engine::Scene, constructor 0x5e5230, made by AtlantisClassInstance<SceneClass>
// from the scene class named by the application options' sceneclass). Three containers nest in it, camera over shake
// over screen, and the scene class's layers are built under them (0x5e5c00). Translated so far: the three containers.
// Not yet: the layers (clip, darkening and region-based containers), the camera and the shake, and the flag 0x100 the
// constructor sets on the camera container.

#include "AtlantisData.h"
#include "Container.h"
#include "Scene.h"

class AtlantisScene : public engine::Scene
{
public:
	explicit AtlantisScene(const SceneClass& theClass);

	// 0x5e57b0 (the instance's slot): the class this scene was made from
	const SceneClass& getSceneClass() const { return mClass; }

	engine::Ref<engine::Container> mCameraContainer;  // +0x14 of the instance: "Scene Camera Container"
	engine::Ref<engine::Container> mShakeContainer;   // +0x18: "Scene Shake Container"
	engine::Ref<engine::Container> mScreenContainer;  // +0x1c: "Scene Sceen Container" (the original's spelling)

private:
	SceneClass mClass;
};

#endif
