// LoadGame: the loading screen (0x43f430..0x43fc10 in the original). The picture fades in; GameWidget then makes the
// game's pictures one group a step, each step waiting for the bar to be drawn once at its new length; the screen
// fades out and deletes itself.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

enum LoadGamePhase
{
	LOADGAME_FADING_IN = 0,
	LOADGAME_SHOWN = 1,
	LOADGAME_DONE = 3,
};

class LoadGame : public ObjBase
{
public:
	int mPhase;              // LoadGamePhase
	float mProgress;         // 0 to 1
	bool mProgressPending;   // the bar's new length is not drawn yet
	ObjBase* mUpperBarTopLeft;    // the corners of the two bars: points in the picture, as drawn
	ObjBase* mUpperBarBottomRight;
	ObjBase* mLowerBarTopLeft;
	ObjBase* mLowerBarBottomRight;

	LoadGame();
	~LoadGame();
	void Notify(ObjBase* theObj) override;
	void Draw(Graphics* g) override;

	void BuildTracks();
	void MakeBarCorners();
	void FillBar(Graphics* g, ObjBase* theTopLeft, ObjBase* theBottomRight);
	void SetProgress(int thePercent);
	bool IsReady();
	void Finish() { StartTrack(1); } // 0x43f790
};

} // namespace Sexy
