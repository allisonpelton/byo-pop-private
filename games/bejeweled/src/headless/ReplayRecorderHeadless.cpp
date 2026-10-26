// port(variant): the verifier's ReplayRecorder (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the
// original game. The board code calls its recorder only through Board::mRecorder, which is NULL on every board
// the verifier makes (a playback board, or a difftest board: neither records), so none of these can run. The
// recorder itself (games/bejeweled/src/ReplayRecorder.cpp) writes the user dir and reads the mods, which the
// verifier has not got; these definitions only let the board code link, and stop the verifier if one runs.

#include "ReplayRecorder.h"

#include <cstdio>
#include <cstdlib>

using namespace Sexy;

static void Unreachable(const char* theWhat)
{
	fprintf(stderr, "bejeweled_verify: ReplayRecorder::%s called: the verifier records nothing\n", theWhat);
	abort();
}

ReplayRecorder::ReplayRecorder(WinDMApp* theApp, Board* theBoard) { Unreachable("ReplayRecorder"); }
ReplayRecorder::~ReplayRecorder() { Unreachable("~ReplayRecorder"); }
void ReplayRecorder::SyncExternalRand() { Unreachable("SyncExternalRand"); }
void ReplayRecorder::AfterUpdate() { Unreachable("AfterUpdate"); }
void ReplayRecorder::OnSelect(Piece* thePiece) { Unreachable("OnSelect"); }
void ReplayRecorder::OnDeselect() { Unreachable("OnDeselect"); }
void ReplayRecorder::OnTarget(Piece* thePiece) { Unreachable("OnTarget"); }
void ReplayRecorder::OnTwist(int theCol, int theRow, int theDir) { Unreachable("OnTwist"); }
void ReplayRecorder::OnSlide(int theAxis, int theLine, int theStart, int theK, int theOffset) { Unreachable("OnSlide"); }
void ReplayRecorder::OnHint() { Unreachable("OnHint"); }
void ReplayRecorder::OnUndo() { Unreachable("OnUndo"); }
void ReplayRecorder::OnPause(bool theUserPause) { Unreachable("OnPause"); }
void ReplayRecorder::OnUnpause(bool theUserPause) { Unreachable("OnUnpause"); }
void ReplayRecorder::OnNewGame() { Unreachable("OnNewGame"); }
void ReplayRecorder::OnHover(int theX, int theY) { Unreachable("OnHover"); }
void ReplayRecorder::OnGameOver() { Unreachable("OnGameOver"); }
void ReplayRecorder::BeginSave() { Unreachable("BeginSave"); }
std::string ReplayRecorder::EndSave(const std::string& theSave) { Unreachable("EndSave"); return ""; }
void ReplayRecorder::OnLoad(const std::string& theSave, const Replay::File* theSidecar) { Unreachable("OnLoad"); }
void ReplayRecorder::OnBoardDestroyed() { Unreachable("OnBoardDestroyed"); }
std::string ReplayRecorder::SnapshotSidecar(const std::string& theSave) { Unreachable("SnapshotSidecar"); return ""; }
size_t ReplayRecorder::EventCount() const { Unreachable("EventCount"); return 0; }
bool ReplayRecorder::FirstSidecar() { Unreachable("FirstSidecar"); return false; }
void ReplayRecorder::KeepSidecarGame(const std::string& theSidecar, const std::string& theSave) { Unreachable("KeepSidecarGame"); }
