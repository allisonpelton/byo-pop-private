// port: the x87's sine, cosine, arc tangent and log as the original's code uses them. The original keeps the result of
// an fsin, fcos, fpatan or fyl2x on the x87 stack and works the next operation on it there. These give the full 64 bits of
// the x87's significand whatever the precision control, and the next fadd, fsub, fmul or fdiv rounds to a double's 53
// (the precision the CRT sets at startup, 0x4fb065), once. The C library's sin, cos, atan2 and log round to a double first,
// so in C the next operation rounds twice, and a product can differ in its last bit: enough to move an enemy by a few
// units in the last place, which the tank's lightning, hitting only what it lands on exactly, turns into a hit an update
// later (docs/IDENTICAL_PLAY.md). An X87Value is such a 64-bit result, held exactly as a pair of doubles; the operators
// below give what the x87 gives, the same bits natively and in the web build (docs/GAME_NOTES.md, "The x87's sines").

#ifndef HEAVYWEAPON_X87_H
#define HEAVYWEAPON_X87_H

namespace Sexy
{

// a value on the x87 stack with 64 bits of significand: mHigh is the nearest double, mLow what is left, a multiple of
// the value's 64th bit
class X87Value
{
public:
	double mHigh;
	double mLow;

	X87Value() : mHigh(0.0), mLow(0.0) { }
	X87Value(double theHigh, double theLow) : mHigh(theHigh), mLow(theLow) { }

	// fst: stored to a double
	double Rounded() const { return mHigh + mLow; }
	// fchs
	X87Value operator-() const { return X87Value(-mHigh, -mLow); }
};

X87Value X87Sin(double theAngle);
X87Value X87Cos(double theAngle);
// fpatan: the angle of the point (theX, theY), from -pi to pi
X87Value X87Atan2(double theY, double theX);
// fldln2, then fyl2x: the natural log of theValue > 0
X87Value X87Log(double theValue);

// fmul, fadd, fsub and fdiv with a double: the exact result rounded to a double
double operator*(const X87Value& theValue, double theFactor);
inline double operator*(double theFactor, const X87Value& theValue) { return theValue * theFactor; }
double operator+(const X87Value& theValue, double theAddend);
inline double operator+(double theAddend, const X87Value& theValue) { return theValue + theAddend; }
inline double operator-(const X87Value& theValue, double theSubtrahend) { return theValue + -theSubtrahend; }
inline double operator-(double theMinuend, const X87Value& theValue) { return -theValue + theMinuend; }
double operator/(const X87Value& theValue, double theDivisor);
double operator/(double theDividend, const X87Value& theValue);
// fadd of two such values
double operator+(const X87Value& theValue, const X87Value& theAddend);
// fcom with a double: the exact values compared
bool operator<=(double theLeft, const X87Value& theRight);
bool operator<=(const X87Value& theLeft, double theRight);

}

#endif
