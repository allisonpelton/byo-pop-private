#include "Scene.h"

#include "Engine.h"

namespace engine
{

Scene::Scene()
{
}

void Scene::enable()
{
	Engine* anEngine = getEngine();
	anEngine->mUpdateSignal.connect(this, &Scene::updateSlot);
	anEngine->mDrawSignal.connect(this, &Scene::drawSlot);
	// The activate (0x495860) and mouse (0x40dcb0, 0x40e220, 0x542097, 0x54206a, 0x41eea0, 0x54203a (feedingfrenzy2))
	// slots: not yet translated
}

void Scene::disable()
{
	Container::disable();
	Engine* anEngine = getEngine();
	anEngine->mUpdateSignal.disconnect(this);
	anEngine->mDrawSignal.disconnect(this);
}

void Scene::update(UpdateContext& theContext)
{
	Container::update(theContext);
	computeBounds();
}

} // namespace engine
