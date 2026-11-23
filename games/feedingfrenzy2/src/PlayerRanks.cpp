#include "PlayerRanks.h"

#include "Engine.h"
#include "Params.h"

using namespace engine;

void PlayerRanks::load(const std::string& thePath)
{
	mRanks.clear();
	MasterDocHandler aMaster;
	RankDocHandler* aHandler = new RankDocHandler(this);
	aHandler->attach(&aMaster, "playerRanks");
	getEngine()->parseXml(thePath, &aMaster);
}

void PlayerRanks::addRank(int thePoints, const std::string& theTitle)
{
	Rank aRank;
	aRank.mPoints = thePoints;
	aRank.mTitle = theTitle;
	for (std::vector<Rank>::iterator anIt = mRanks.begin(); anIt != mRanks.end(); ++anIt)
	{
		if (thePoints < anIt->mPoints)
		{
			mRanks.insert(anIt, aRank);
			return;
		}
	}
	mRanks.push_back(aRank);
}

void RankDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "rank")
	{
		int aPoints = theAttributes.getInt("points", 0);
		mRanks->addRank(aPoints, theAttributes.getString("title", "xxx"));
	}
}

std::string PlayerRanks::getTitle(int thePoints) const
{
	std::string aTitle;
	for (const Rank& aRank : mRanks)
	{
		if (thePoints < aRank.mPoints)
			return aTitle;
		aTitle = aRank.mTitle;
	}
	return aTitle;
}
