#ifndef __HAMMERHEADS_VALUELIST_H__
#define __HAMMERHEADS_VALUELIST_H__

// A short list of numbers that is kept as text, "1.000000;0.500000": what a creature adds to a saved game (a gnome's stage,
// its timers), written by SaveExtra and read back by LoadExtra. The original's 0x41deb0 group (0x41e340 writes, 0x41e090
// reads); every number goes through "%f" in the English locale, so a saved game has the same text on every machine.

#include <string>
#include <vector>

namespace Sexy
{

class ValueList
{
public:
	std::vector<double>	mValues;
	int					mPos;				// the next value Next... returns

public:
	ValueList();
	explicit ValueList(const std::string& theText);		// 0x41e090

	void			Add(double theValue);				// 0x41e050
	void			AddInt(int theValue);				// 0x41e030
	void			AddBool(bool theValue);				// 0x41e060
	double			NextDouble();						// 0x41def0
	int				NextInt();							// 0x41ded0
	bool			NextBool();							// 0x41df00
	std::string		ToString() const;					// 0x41e340
};

}

#endif
