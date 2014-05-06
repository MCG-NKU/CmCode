#include "stdafx.h"
#include "../stdafx.h"

Point const DIRECTION4[5] = {
	Point(1,  0), //Direction 0: right
	Point(0,  1), //Direction 1: bottom
	Point(-1, 0), //Direction 2: left
	Point(0, -1), //Direction 3: up
	Point(0, 0),
};  //format: {dx, dy}

Point const DIRECTION8[9] = {
	Point(1,  0), //Direction 0
	Point(1,  1), //Direction 1 
	Point(0,  1), //Direction 2
	Point(-1, 1), //Direction 3
	Point(-1, 0), //Direction 4
	Point(-1,-1), //Direction 5
	Point(0, -1), //Direction 6
	Point(1, -1),  //Direction 7
	Point(0, 0),
};  //format: {dx, dy}

Point const DIRECTION16[17] = {
	Point(2,  0), //Direction 0
	Point(2,  1), //Direction 1 
	Point(2,  2), //Direction 2
	Point(1,  2), //Direction 3
	Point(0,  2), //Direction 4
	Point(-1, 2), //Direction 5
	Point(-2, 2), //Direction 6
	Point(-2, 1), //Direction 7
	Point(-2, 0), //Direction 8
	Point(-2,-1), //Direction 9 
	Point(-2,-2), //Direction 10
	Point(-1,-2), //Direction 11
	Point(0, -2), //Direction 12
	Point(1, -2), //Direction 13
	Point(2, -2), //Direction 14
	Point(2, -1),  //Direction 15
	Point(0, 0),
}; //format: {dx, dy}

float const DRT_ANGLE[8] = {
	0.000000f,
	0.785398f,
	1.570796f,
	2.356194f,
	3.141593f,
	3.926991f,
	4.712389f,
	5.497787f
};

float const PI_FLOAT = 3.1415926535897932384626433832795f;
float const PI2 = PI_FLOAT * 2.0f;
double const Cm2PI = CV_PI * 2.0;
float const PI_HALF = PI_FLOAT * 0.5f;
double const SQRT2 = sqrt(2.0);
bool dbgStop = false;

mt19937 randEng;
uniform_int<int> randInt(0, 0x7fffffff);

double dummyD;
int dummyI;

void splitStr(CStr& str, CStr& delimiters , vector<string>& tokens)
{	
	string::size_type lastPos = str.find_first_not_of(delimiters, 0); // Skip delimiters at beginning.
	string::size_type pos     = str.find_first_of(delimiters, lastPos); // Find first "non-delimiter".
	while (string::npos != pos || string::npos != lastPos) {
		tokens.push_back(str.substr(lastPos, pos - lastPos)); // Found a token, add it to the vector.
		lastPos = str.find_first_not_of(delimiters, pos); // Skip delimiters.  Note the "not_of"
		pos = str.find_first_of(delimiters, lastPos); // Find next "non-delimiter"
	}
}

wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}
