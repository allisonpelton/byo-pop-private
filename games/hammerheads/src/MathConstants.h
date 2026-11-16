// port: the numbers the game's drawing and random code share (the exe holds each as a double constant; the float ones are pi
// and its multiples rounded to a float first, which the ease curves use).
#ifndef __HAMMERHEADS_MATHCONSTANTS_H__
#define __HAMMERHEADS_MATHCONSTANTS_H__

const double RAND_SCALE = 4.656612875245797e-10;		// 1 / 2^31: Rand() as a number from 0 to 1
const double PI = 3.141592653589793;
const double HALF_PI = 1.5707963267948966;
const double TWO_PI = 6.283185307179586;
const double DEGREES_TO_RADIANS = 0.017453292519943295;		// pi / 180
const double PI_FLOAT = 3.1415927410125732;			// pi as a float
const double HALF_PI_FLOAT = 1.5707963705062866;		// pi / 2 as a float, the exe's constant for most of the ease curves
const double TWO_PI_FLOAT = 6.2831854820251465;			// 2 pi as a float

#endif
