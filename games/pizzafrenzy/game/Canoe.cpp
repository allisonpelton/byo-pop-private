#include "Canoe.h"

using namespace pizzafrenzy;

// 0x431ad0
Canoe::Canoe(const std::string& theTypeName) : Van(theTypeName)
{
}

// 0x431c10
void Canoe::goTo(CustomerTile* theTile)
{
	mTarget = theTile;
	engine::IntPair aCell = theTile->getRoadCell();
	if (mPath.empty() && !isAtCell(aCell))
	{
		startPath(aCell);
		if (!mPath.empty())
		{
			mPhase = PHASE_DRIVING;
			// the driving state's entry
			if (mEmitter != nullptr)
				mEmitter->start();
		}
	}
}

// 0x431e40
void Canoe::face(const engine::IntPair& theCell)
{
	faceAlong(theCell);
}
