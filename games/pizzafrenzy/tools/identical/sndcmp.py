"""sndcmp.py <origrun sound.log> <port -soundlog file> [--from N] [--to N] [--slack N]: Word Harmony's sound
comparison (games/wordharmony/tools/identical/sndcmp.py, which describes it) on Pizza Frenzy's clock: the original's
updates come 33.83 ms of origrun's virtual clock apart here, as its sound log's times show (edit2's press sounds land
on the port's frames at that rate over 1459 updates).
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'wordharmony', 'tools',
                                'identical'))
import sndcmp  # noqa: E402

sndcmp.UPDATE_MS = 33.83

if __name__ == '__main__':
    sndcmp.main()
