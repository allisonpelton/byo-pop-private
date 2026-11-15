// framework/TextScanner.h against std::istringstream, on every short string of a small alphabet and on random longer ones:
// the same reads must give the same values, the same successes and failures, the same positions and the same lines.
//   clang++ -std=c++17 -O1 -I framework tools/textscannertest.cpp -o textscannertest && ./textscannertest
#include "TextScanner.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

using Sexy::TextScanner;

static int sFailures = 0;

static void Check(bool ok, const std::string& theText, const char* theWhat)
{
	if (ok)
		return;
	if (++sFailures <= 20)
		printf("MISMATCH %s on \"%s\"\n", theWhat, theText.c_str());
}

// the sequences of reads the framework makes: a command line (count, word, rest), a point, a word then a point after a
// rewind, a click's button and count, a wheel's two, and a file's lines
static void RunOne(const std::string& theText)
{
	{
		std::istringstream s(theText);
		TextScanner t(theText);
		int a = 7, b = 7, c = 7, d = 7;
		bool sa = !!(s >> a), ta = !!(t >> c);
		Check(sa == ta && a == c, theText, "int");
		std::string w1 = "x", w2 = "x";
		bool sw = !!(s >> w1), tw = !!(t >> w2);
		Check(sw == tw && (!sw || w1 == w2), theText, "word after int");
		std::string r1, r2;
		std::getline(s, r1);
		t.GetLine(r2);
		Check(r1 == r2, theText, "rest of line");
		(void) b;
		(void) d;
	}
	{
		std::istringstream s(theText);
		TextScanner t(theText);
		int x1 = 5, y1 = 5, x2 = 5, y2 = 5;
		bool a = !!(s >> x1 >> y1), b = !!(t >> x2 >> y2);
		Check(a == b && x1 == x2 && y1 == y2, theText, "two ints");
		int z1 = 9, z2 = 9;
		bool c = !!(s >> z1), d = !!(t >> z2);
		Check(c == d && z1 == z2, theText, "a third int");
	}
	{
		std::istringstream s(theText);
		TextScanner t(theText);
		std::streampos start = s.tellg();
		size_t tstart = t.Tell();
		std::string w1, w2;
		bool a = !!(s >> w1), b = !!(t >> w2);
		Check(a == b && (!a || w1 == w2), theText, "word");
		if (!(a && w1 == "game"))
		{
			s.clear();
			s.seekg(start);
			t.SeekTo(tstart);
		}
		int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
		bool c = !!(s >> x1 >> y1), d = !!(t >> x2 >> y2);
		Check(c == d && x1 == x2 && y1 == y2, theText, "point after rewind");
		int k1 = 1, k2 = 1;
		bool e = !!(s >> k1), f = !!(t >> k2);
		Check(e == f && k1 == k2, theText, "click count");
	}
	for (char delim : {'\n', ','})
	{
		std::istringstream s(theText);
		TextScanner t(theText);
		std::string l1, l2;
		for (int n = 0; n < 40; n++)
		{
			bool a = !!std::getline(s, l1, delim), b = t.GetLine(l2, delim);
			Check(a == b && (!a || l1 == l2), theText, "lines");
			if (!a)
				break;
		}
	}
}

int main()
{
	const char alphabet[] = {' ', '\t', '\n', ',', '+', '-', '0', '1', '9', 'a', 'g', '.', '\r'};
	const int n = sizeof(alphabet);
	std::vector<std::string> texts = {"", "game 3 4", "left", "-2147483648", "2147483647", "2147483648", "-2147483649",
		"99999999999999999999", "  12  34  ", "12abc", "+5", "+-5", "-", "+", "0012", "1\n2\n", "a,b,,c,", "x y"};
	for (int len = 1; len <= 5; len++)
	{
		std::string s(len, ' ');
		std::vector<int> idx(len, 0);
		for (;;)
		{
			for (int i = 0; i < len; i++)
				s[i] = alphabet[idx[i]];
			texts.push_back(s);
			int p = len - 1;
			while (p >= 0 && ++idx[p] == n)
				idx[p--] = 0;
			if (p < 0)
				break;
		}
	}
	srand(12345);
	for (int i = 0; i < 200000; i++)
	{
		std::string s;
		int len = rand() % 30;
		for (int j = 0; j < len; j++)
			s += alphabet[rand() % n];
		texts.push_back(s);
	}
	for (const std::string& t : texts)
		RunOne(t);
	printf("%zu strings, %d mismatches\n", texts.size(), sFailures);
	return sFailures ? 1 : 0;
}
