// Sexy::NoLoseBoard (bejeweled3, vftable 0x868ef4, constructor 0x6da7a0, 0x3248 bytes) over Sexy::QuestBoard: the Quest boards of the Challenge "No Lose". Its 266 slots
// are QuestBoard's own (no slot of the vftable differs from it): the rules, a game that cannot be lost, come from the goal (Gold Lines, Wall Blast: not made yet) and
// the settings. See docs/NOTES.md, "Quest".
#pragma once

#include "QuestBoard.h"

namespace Sexy {

class NoLoseBoard : public QuestBoard {
};

}  // namespace Sexy
