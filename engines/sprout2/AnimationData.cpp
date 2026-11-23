#include "AnimationData.h"

#include "Engine.h"
#include "Params.h"
#include "Surface.h"
#include "TheoraReader.h"

#include <algorithm>
#include <set>

namespace engine
{

namespace
{

// CharLowerA, as the run names are compared
std::string lowerCase(const std::string& theString)
{
	std::string aLower = theString;
	for (size_t i = 0; i < aLower.size(); i++)
	{
		if (aLower[i] >= 'A' && aLower[i] <= 'Z')
			aLower[i] = (char)(aLower[i] - 'A' + 'a');
	}
	return aLower;
}

} // namespace

void AnimationData::load(const std::string& thePath)
{
	MasterDocHandler aMaster;
	AnimationDataDocHandler* aHandler = new AnimationDataDocHandler(this);
	aHandler->attach(&aMaster, "animations");
	getEngine()->parseXml(thePath, &aMaster);
}

void AnimationData::addFrame(Surface* theFrame)
{
	Ref<Surface> aFrame = theFrame;
	if (aFrame)
		aFrame->setPivot(PIVOT_CENTER);
	mFrames.push_back(aFrame);
}

void AnimationData::addAnimation(const std::string& theName, unsigned int theStart, unsigned int theEnd,
                                 float theFrameRate)
{
	AnimationRun aRun;
	aRun.mName = lowerCase(theName);
	aRun.mStart = theStart;
	aRun.mEnd = theEnd;
	aRun.mFrameTime = (float)(1.0 / (double)theFrameRate);
	int anIndex = (int)mAnimations.size();
	mAnimations.push_back(aRun);
	mAnimationIndex[aRun.mName] = anIndex;
}

void AnimationData::loadFrames(const std::string& thePath)
{
	std::set<unsigned int> aDropped;
	TheoraReader aReader;
	aReader.open(thePath);
	while (!aReader.atEnd())
	{
		unsigned int aFrameIndex = aReader.getFrameIndex();
		bool aTaken = false;
		for (size_t i = 0; i < mAnimations.size(); i++)
		{
			if (mAnimations[i].mStart <= aFrameIndex && aFrameIndex < mAnimations[i].mEnd)
			{
				aTaken = true;
				break;
			}
		}
		if (aTaken)
		{
			Ref<Surface> aFrame = aReader.readFrame();
			addFrame(aFrame);
		}
		else
		{
			aDropped.insert(aFrameIndex);
			aReader.skipFrame();
		}
	}
	// 0x410ce0 (feedingfrenzy2): each run moved down by the frames dropped before it
	for (size_t i = 0; i < mAnimations.size(); i++)
	{
		unsigned int aStart = mAnimations[i].mStart;
		unsigned int aMoved = (unsigned int)std::count_if(aDropped.begin(), aDropped.end(),
		                                                  [aStart](unsigned int theFrame) { return theFrame < aStart; });
		mAnimations[i].mStart -= aMoved;
		mAnimations[i].mEnd -= aMoved;
	}
}

int AnimationData::findAnimation(const std::string& theName) const
{
	std::map<std::string, int>::const_iterator anIt = mAnimationIndex.find(lowerCase(theName));
	if (anIt == mAnimationIndex.end())
		return -1;
	return anIt->second;
}

void AnimationDataDocHandler::begin(Params& theAttributes)
{
	mFileName = theAttributes.getString("fileName", "");
}

void AnimationDataDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName != "animation")
		return;
	std::string aName = lowerCase(theAttributes.getString("name", ""));
	int aStart = theAttributes.getInt("startFrame", 0);
	int anEnd = theAttributes.getInt("endFrame", 0);
	float aFrameRate = theAttributes.getFloat("frameRate", 0.0f);
	mData->addAnimation(aName, (unsigned int)aStart, (unsigned int)(anEnd + 1), aFrameRate);
}

void AnimationDataDocHandler::endDocument()
{
	if (!mFileName.empty())
		mData->loadFrames(mFileName);
}

} // namespace engine
