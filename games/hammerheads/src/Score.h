#ifndef __HAMMERHEADS_SCORE_H__
#define __HAMMERHEADS_SCORE_H__

// The score display (0x44b070-0x44bb80): the points in the corner of the screen, with the signs of the power-ups that
// are running under it. Also where points are added, as numbers that float up or fly to the display.

#include "Object.h"

namespace Sexy
{

class Image;

// Where the numbers of the score fly to, and where the sign of a power-up goes
struct ScoreDisplayPos
{
	double			mX;
	double			mY;
};

class Score : public Object
{
public:
	Score();												// 0x44b070 (made by MakeScore)
	virtual ~Score();										// 0x44b070

	OBJECT_TYPE(Score)

	virtual void	Draw(Graphics* g) override;				// 0x44b200

	void			SetScore(int theScore);					// 0x44b170
	int				GetScore() const;						// 0x44b190

	// 0x44b9b0: gives the points. A floater of theType (1 flies to the display and adds the points when it gets there, 2
	// and the others add them at once; -1 shows nothing) shows them at the point. While the score counts five times the
	// points are multiplied by five and a second floater says "x5".
	void			AddPoints(int thePoints, int theX, int theY, int theType);
	void			AddQuiet(int thePoints);				// 0x44b120: adds to the score and to the score of the level, shows nothing
};

// 0x44b0f0: the score display of the level on screen
Score* GetScore();
// 0x44b8c0: makes the display and remembers it
Score* MakeScore();

ScoreDisplayPos GetScoreDisplayPos();						// 0x44b1c0: (225, 35)
ScoreDisplayPos GetBonusTrayPos();							// 0x44b1a0: (193, -13)

// Where the sign of each power-up is in the corner of the screen, where its picture flies when it is picked up
ScoreDisplayPos GetIceCreamSignPos();						// 0x4494f0: (298, 30)
ScoreDisplayPos GetCloverSignPos();							// 0x449510: (378, 45)
ScoreDisplayPos GetPowerUpSignPos();						// 0x449530: (415, 20)
ScoreDisplayPos GetStrawberrySignPos();						// 0x449550: (338, 45)

// 0x44b090: no floater is carrying the image to the display at the moment
bool IsNoFloaterCarrying(Image* theImage);

}

#endif
