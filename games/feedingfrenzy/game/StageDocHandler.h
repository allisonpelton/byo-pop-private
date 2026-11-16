#ifndef __FEEDINGFRENZY_STAGEDOCHANDLER_H__
#define __FEEDINGFRENZY_STAGEDOCHANDLER_H__

// The reader of a stage's file (StageDocHandler, 0x4240f0)

#include "DocHandler.h"

namespace feedingfrenzy
{

class StageMgr;

class StageDocHandler : public engine::BaseDocHandler
{
public:
	StageDocHandler(const std::string& theName, engine::DocHandlerStack* theParent, StageMgr* theStage);
};

} // namespace feedingfrenzy

#endif
