#include "AtlantisScene.h"

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
	// The original then sets a flag (0x100) on the camera container (0x5e5230), and connects the scene's own update
	// and draw (0x5e5c00 builds the layers): not yet translated
}
