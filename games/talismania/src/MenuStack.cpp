#include "MenuStack.h"

#include <algorithm>

MenuStack::MenuStack(AtlantisScene* theScene, const std::string& theLayer, const std::string& thePopupLayer,
					 const std::string& theDarkenLayer, const std::string& theDarkenPopupLayer)
	: mScene(theScene),
	  mLayer(dynamic_cast<engine::Container*>(theScene->getLayer(theLayer))),
	  mPopupLayer(dynamic_cast<engine::Container*>(theScene->getLayer(thePopupLayer))),
	  mDarken(makeDark(theDarkenLayer)),
	  mDarkenPopup(makeDark(theDarkenPopupLayer))
{
}

MenuStack::Dark MenuStack::makeDark(const std::string& theName) const
{
	Dark aDark;
	aDark.mLayer = mScene->getLayer(theName);
	std::map<std::string, SceneLayerClass>::const_iterator aClass = mScene->getSceneClass().layers.find(theName);
	if (aClass != mScene->getSceneClass().layers.end())
	{
		aDark.mTime = aClass->second.darkenTime;
		aDark.mOpacity = aClass->second.darkenOpacity;
	}
	return aDark;
}

void MenuStack::push(engine::GenericScreen* theScreen, bool thePopup)
{
	engine::Container* aLayer = thePopup ? mPopupLayer : mLayer;
	if (aLayer != nullptr && theScreen->getParent() != aLayer)
		aLayer->addChild(theScreen);
	std::list<engine::Ref<engine::GenericScreen>>& aList = thePopup ? mPopups : mScreens;
	aList.push_back(theScreen);
	theScreen->show();
}

void MenuStack::pop(engine::GenericScreen* theScreen)
{
	for (int i = 0; i < 2; i++)
	{
		std::list<engine::Ref<engine::GenericScreen>>& aList = i == 0 ? mScreens : mPopups;
		for (std::list<engine::Ref<engine::GenericScreen>>::iterator it = aList.begin(); it != aList.end(); ++it)
		{
			if (it->get() == theScreen)
			{
				theScreen->hide();
				aList.erase(it);
				return;
			}
		}
	}
}

bool MenuStack::contains(engine::GenericScreen* theScreen) const
{
	for (std::list<engine::Ref<engine::GenericScreen>>::const_iterator it = mScreens.begin(); it != mScreens.end(); ++it)
		if (it->get() == theScreen)
			return true;
	for (std::list<engine::Ref<engine::GenericScreen>>::const_iterator it = mPopups.begin(); it != mPopups.end(); ++it)
		if (it->get() == theScreen)
			return true;
	return false;
}

// A dark layer moves at its opacity over its time, seconds per full fade
void MenuStack::fade(Dark& theDark, bool theOn, double theElapsed)
{
	if (theDark.mLayer == nullptr)
		return;
	float aTarget = theOn ? theDark.mOpacity : 0.0f;
	float aCurrent = theDark.mLayer->getOpacity();
	float aStep = (theDark.mTime > 0.0f ? theDark.mOpacity / theDark.mTime : theDark.mOpacity) * (float)theElapsed;
	if (aCurrent < aTarget)
		aCurrent = std::min(aTarget, aCurrent + aStep);
	else
		aCurrent = std::max(aTarget, aCurrent - aStep);
	theDark.mLayer->setOpacity(aCurrent);
	if (theOn)
		fprintf(stderr, "DARK %f %f %f\n", aCurrent, aTarget, theElapsed);
}

void MenuStack::update(double theElapsed)
{
	fade(mDarken, !mScreens.empty(), theElapsed);
	fade(mDarkenPopup, !mPopups.empty(), theElapsed);
}
