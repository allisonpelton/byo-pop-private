// The score and the difficulty, which the plays of the game change.

#ifndef VENICE_SCORING_H
#define VENICE_SCORING_H

// 0x408970: adds the points, times the shooter's score multiplier, to the current profile's score and returns the text that
// shows them ("+500", or "+500 x2" with a multiplier). The text is kept until the next call.
const char* FormatScore(int thePoints);

// 0x408810: moves the current profile's difficulty by the change (times the tuning's change on a win or on a loss, which are
// the signs of the change) and keeps it between the tuning's least and greatest.
void AdjustDifficulty(float theChange);

#endif
