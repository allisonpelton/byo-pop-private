#!/usr/bin/env python3
"""Venice Deluxe's real-input driver: the shared loop of tools/playbymove (the port's bot answers `move`, the moves are played as real
SDL events), with Venice's own end of play. The state's "over" is true on every summary screen and on a level that is won or lost,
screens that the bot goes on from (with `bot 0 continue` its moves click through them: the summary, the map, "CLICK TO PLAY AGAIN"),
so the play is over here when
  - the same level is lost LOSSES times (the middle layer enters its defeat stage: no Journey try left, a Flood or Trick Shot level
    sunk, the Survival board sunk): the bot does not win the later levels and Journey has no end but a won last building, so a game
    ends where the bot cannot go on (REALINPUT_LOSSES, 3 by default; the Survival mode is one game, so 1 there); or
  - the state says over and its screen, mode, level and score stay the same for PATIENCE states running (REALINPUT_PATIENCE, 120): a
    game that has ended and waits for the player (the honor roll, a summary with nothing to click); or
  - a level is in play and nothing changes in it (screen, level, score, hits, filled slots) for STALL states running
    (REALINPUT_STALL, 3000): the bot has no shot to make (no slot takes the ready jewel and it never throws one away).
The arguments are playbymove's; the run's summary line says which end it was.
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'playbymove'))
import playbymove  # noqa: E402

PATIENCE = int(os.environ.get('REALINPUT_PATIENCE', '120'))
LOSSES = int(os.environ.get('REALINPUT_LOSSES', '3'))
STALL = int(os.environ.get('REALINPUT_STALL', '3000'))


class PlayEnd:
    def __init__(self):
        self.last = None
        self.same = 0
        self.was_lost = False
        self.lost_level = None
        self.losses = 0
        self.reason = None
        self.progress = None
        self.stalled = 0

    def __call__(self, state):
        board = state.get('board') or {}
        lost = board.get('middle') == 'defeat'
        level = (state.get('mode'), state.get('level'))
        if lost and not self.was_lost:
            self.losses = self.losses + 1 if level == self.lost_level else 1
            self.lost_level = level
            if self.losses >= LOSSES:
                self.reason = 'level %s of mode %s lost %d times running' % (level[1], state.get('modename'), self.losses)
                print('REALINPUT end: ' + self.reason)
                return True
        elif level != self.lost_level and state.get('over') is not True:
            self.losses = 0   # another level is in play: the count of lost tries of the last one is over
        self.was_lost = lost
        progress = (state.get('screen'), state.get('level'), state.get('score'), board.get('hits'), board.get('filled'))
        self.stalled = self.stalled + 1 if progress == self.progress else 0
        self.progress = progress
        if self.stalled >= STALL and board:
            self.reason = 'nothing changed in level %s for %d states' % (state.get('level'), STALL)
            print('REALINPUT end: ' + self.reason)
            return True
        if state.get('over') is not True:
            self.same = 0
            return False
        key = (state.get('screen'), state.get('mode'), state.get('level'), state.get('score'))
        self.same = self.same + 1 if key == self.last else 0
        self.last = key
        if self.same >= PATIENCE:
            self.reason = 'nothing to go on from for %d states' % PATIENCE
            print('REALINPUT end: ' + self.reason)
            return True
        return False


if __name__ == '__main__':
    sys.exit(playbymove.play(playbymove.parse_args(), PlayEnd()))
