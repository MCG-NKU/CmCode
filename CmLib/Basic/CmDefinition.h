#pragma once

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

extern Point const DIRECTION4[5];
extern Point const DIRECTION8[9];
extern Point const DIRECTION16[17];
extern float const DRT_ANGLE[8];
extern float const PI_FLOAT;
extern float const PI2;
extern double const Cm2PI;
extern float const PI_HALF;
extern double const SQRT2;
extern double const CmEpsilon;

typedef vector<Point2d> PointSetd;
typedef vector<Point2i> PointSeti;
typedef const vector<Point2d> CPointSetd;
typedef const vector<Point2i> CPointSeti;
typedef vector<string> vecS;
typedef vector<int> vecI;
typedef vector<byte> vecB;
typedef vector<float> vecF;
typedef vector<double> vecD;
typedef vector<Mat> vecM;
typedef pair<double, int> CostIdx;
typedef pair<float, int> CostfIdx;
typedef pair<int, int> CostiIdx;
typedef vector<CostIdx> CostIdxV;
typedef vector<CostfIdx> CostfIdxV;
typedef vector<CostiIdx> CostiIdxV;
typedef complex<double> complexD;
typedef complex<float> complexF;

typedef const string CStr;
typedef const Mat CMat;
typedef const SparseMat CSMat;
typedef const vecI CvecI;
typedef const vecB CvecB;
typedef const vecF CvecF;
typedef const vecD CvecD;
typedef const vecM CvecM;
typedef const vecS CvecS;

extern mt19937 randEng;
extern uniform_int<int> randInt;

extern double dummyD;
extern int dummyI;

/************************************************************************/
/* Define Macros & Global functions                                     */
/************************************************************************/
const double EPS = 1e-200;		// Epsilon (zero value)
const double INF = 1e200;
#define _S(str) ((str).c_str())
#define _QS(str) (QString(_S(str)))

#define CHECK_IND(c, r) (c >= 0 && c < _w && r >= 0 && r < _h)
#define CHK_IND(p) ((p).x >= 0 && (p).x < _w && (p).y >= 0 && (p).y < _h)
#define CHECK_INDEX(p, s) ((p).x >= 0 && (p).x < (s.width) && (p).y >= 0 && (p).y < (s.height))
#define CLAMP(v, l, h)	v = ((v)<(l) ? (l) : (v) > (h) ? (h) : v)

#define ForPointsD(pnt, pntS, pntE, d) 	for (Point pnt = (pntS); pnt.y != (pntE).y; pnt.y += (d)) for (pnt.x = (pntS).x; pnt.x != (pntE).x; pnt.x += (d))
#define ForPointsD_(pnt, pntS, pntE, d) for (Point pnt = (pntS); pnt.x != (pntE).x; pnt.x += (d)) for (pnt.y = (pntS).y; pnt.y != (pntE).y; pnt.y += (d))
#define ForPoints(pnt, pntS, pntE) 		for (Point pnt = (pntS); pnt.y != (pntE).y; pnt.y++) for (pnt.x = (pntS).x; pnt.x != (pntE).x; pnt.x++)
#define ForPoints2(pnt, xS, yS, xE, yE)	for (Point pnt(0, (yS)); pnt.y != (yE); pnt.y++) for (pnt.x = (xS); pnt.x != (xE); pnt.x++)

/************************************************************************/
/* Useful template                                                      */
/************************************************************************/

template<typename T> inline T sqr(T x) { return x * x; } // out of range risk for T = byte, ...
template<typename T> inline int CmSgn(T number) {if(abs(number) < EPS) return 0; return number > 0 ? 1 : -1; }
template<class T> inline T pntSqrDist(const Point_<T> &p1, const Point_<T> &p2) {return sqr(p1.x - p2.x) + sqr(p1.y - p2.y);} // out of range risk for T = byte, ...
template<class T> inline double pntDist(const Point_<T> &p1, const Point_<T> &p2) {return sqrt((double)pntSqrDist(p1, p2));} // out of range risk for T = byte, ...
//template<class T> inline T vecSqrDist(const Vec<T, 3> &v1, const Vec<T, 3> &v2) {return sqr(v1[0] - v2[0])+sqr(v1[1] - v2[1])+sqr(v1[2] - v2[2]);} // out of range risk for T = byte, ...
//template<class T> inline T vecDist(const Vec<T, 3> &v1, const Vec<T, 3> &v2) {return sqrt(vecSqrDist(v1, v2));} 
template<class T, int D> inline T vecSqrDist(const Vec<T, D> &v1, const Vec<T, D> &v2) {T s = 0; for (int i=0; i<D; i++) s += sqr(v1[i] - v2[i]); return s;} // out of range risk for T = byte, ...
template<class T, int D> inline T    vecDist(const Vec<T, D> &v1, const Vec<T, D> &v2) { return sqrt(vecSqrDist(v1, v2)); } // out of range risk for T = byte, ...

//template<class T1, class T2> inline void operator /= (Vec<T1, 3> &v1, const T2 v2) { v1[0] = (T1)(v1[0]/v2); v1[1] = (T1)(v1[1]/v2); v1[2] = (T1)(v1[2]/v2); } // out of range risk for T = byte, ...

inline Point round(const Point2d &p) { return Point(cvRound(p.x), cvRound(p.y));}


// Return -1 if not in the list
template<typename T>
static inline int findFromList(const T &word, const vector<T> &strList) {size_t idx = find(strList.begin(), strList.end(), word) - strList.begin(); return idx < strList.size() ? idx : -1;};


template<typename T> vector<T> operator +(const vector<T>& v1, const vector<T> &v2)
{
	vector<T> result(v1);
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] + v2[i];
	return result;
}

template<typename T> vector<T> operator -(const vector<T>& v1, const vector<T> &v2)
{
	vector<T> result(v1);
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] - v2[i];
	return result;
}

template<typename T> vector<T> operator *(const vector<T>& v1, const vector<T> &v2)
{
	vector<T> result(v1);
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] * v2[i];
	return result;
}

template<typename T> vector<T> operator /(const vector<T>& v1, const vector<T> &v2)
{
	vector<T> result(v1.size());
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] / v2[i];
	return result;
}

template<typename T> void operator /=(vector<T>& v, T s)
{
	int vSize = (int)v.size();
	for (int i = 0; i < vSize; i++)
		v[i] /= s;
}

template<typename T> vector<T> operator /(const vector<T>& v1, T s)
{
	vector<T> result(v1.size());
	for (size_t i = 0, iEnd = v1.size(); i != iEnd; i++)
		result[i] = v1[i] / s;
	return result;
}

template<class T> void maxSize(const vector<Point_<T>> &pnts, T &minS, T &maxS)
{
	CV_Assert(pnts.size() > 0);
	minS = maxS = pnts[0].x;
	for (int i = 0, iEnd = (int)pnts.size(); i < iEnd; i++)
	{
		minS = min(minS, pnts[i].x);
		minS = min(minS, pnts[i].y);
		maxS = max(maxS, pnts[i].x);
		maxS = max(maxS, pnts[i].y);
	}
}

template<typename T> inline bool lessThan(const Vec<T, 2> &v1, const Vec<T, 2> &v2) 
{
	if (v1.val[0] < v2.val[0]) 
		return true; 
	else if (v1.val[0] > v2.val[0])
		return false;
	else
		return v1.val[1] < v2.val[1];
} 

#define CV_Assert_(expr, args) \
{\
	if(!(expr)) {\
	string msg = cv::format args; \
	printf("%s in %s:%d\n", msg.c_str(), __FILE__, __LINE__); \
	cv::error(cv::Exception(CV_StsAssert, msg, __FUNCTION__, __FILE__, __LINE__) ); }\
}

void splitStr(CStr& str, CStr& delimiters , vector<string>& tokens);
wstring s2ws(const std::string& s);
inline double MatMin(CMat &m) {double minVal; minMaxLoc(m, &minVal, NULL); return minVal; }
inline double MatMax(CMat &m) {double maxVal; minMaxLoc(m, NULL, &maxVal); return maxVal; }
template<class T> inline Point_<T> operator / (const Point_<T> &p, double s) { return Point_<T>((T)(p.x /s), (T)(p.y/s));}
template<class T> inline void operator /= (Point_<T> &p, double s) {p.x /= s, p.y /= s;}
template<class T> inline Vec<T, 3> operator / (const Vec<T, 3> &v, double s) { return Vec<T, 3>((T)(v[0]/s), (T)(v[1]/s), (T)(v[2]/s));}


#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif
