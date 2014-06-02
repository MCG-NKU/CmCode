#pragma once

/************************************************************************/
/*  This software is developed by Ming-Ming Cheng.				        */
/*       Url: http://mmcheng.net/						                */
/*  This software is free fro non-commercial use. In order to use this	*/
/*  software for academic use, you must cite the corresponding paper:	*/
/*      Ming-Ming Cheng, Curve Structure Extraction for Cartoon Images, */
/*		The 5th Joint Conference on Harmonious Human Machine Environment*/
/*		(HHME), 1-8														*/
/************************************************************************/

class CmCurveEx
{
public:
	typedef struct CEdge{
		CEdge(int _index){index = _index; lenSal = avg = sal = 0; }
		~CEdge(void){}

		// Domains assigned during link();
		int index;    // Start from 0
		int pointNum; 
		float avg;    // Average edge magnitude
		Point start, end; 
		vector<Point> pnts; 

		// Domain assigned during GetRobustCurveEnergy()
		float sal, lenSal; // Edge saliency, a combined effect of length and average weights
	}CEdge;

	CmCurveEx(float maxOrntDif = 0.25f * CV_PI);
	
	// Input kernel size for calculating derivatives, kSize should be 1, 3, 5 or 7
	CMat& CalSecDer(CMat &img1f, int kSize = 5);
	CMat& CalSecDer_(CMat &img3f, int kSize = 5);
	CMat& CalFirDer(CMat &img1f, int kSize = 5);
	CMat& CalDx(CMat &img3f, int kSize = 5);

	const vector<CEdge>& Link(int shortRemoveBound = 3, float lkEnd = 0.05f, float lkStart = 0.2f);
	CMat& CalRobustCurveEnergy(double longEdg = 50, double lenMin = 0.1, double avgMin = 0.1);

	// Get data pointers
	CMat& GetDer(){ return _pDer1f; }
	CMat& GetLineIdx() { return _pLabel1i; } // Edge index start from 1
	CMat& GetNextMap() { return _pNext1i; }
	CMat& GetOrnt() { return _pOrnt1f; }
	const vector<CEdge>& GetEdges() {return _vEdge;}
	CMat& GetRobustCurveEnergy() {return _engy1f;}

	static const int IND_BG = 0xffffffff, IND_NMS = 0xfffffffe, IND_SR = 0xfffffffd; // Background, Non Maximal Suppress and Short Remove

	static void Demo(CMat &img1u, bool isCartoon);
	static void Demo2(CStr &wkDir, bool isCartoon = true);
	void SaveShowResult(CMat &img1f, CStr &title, Mat &show3u, double longEdg = 50);
	void SaveShowResult(CMat &img1f, CStr &title, double longEdg = 50){Mat show3u; SaveShowResult(img1f, title, show3u, longEdg);}

private:
	static void MergeResults(CMat der1f[3], CMat ornt1f[3], Mat &der, Mat &ornt);

private:
	Mat _pDer1f;   // First or secondary derivatives. 32FC1
	Mat _pOrnt1f;  // Line orientation. 32FC1
	Mat _pLabel1i;  // Line index, 32SC1.
	Mat _pNext1i;   // Next point 8-direction index, [0, 1, ...,  7], 32SC1

	// Will be used for link process
	typedef pair<float, Point> PntImp;
	vector<PntImp> _StartPnt;
	vector<CEdge> _vEdge;
	static bool linePointGreater (const PntImp& e1, const PntImp& e2 ) {return e1.first > e2.first;};

	int _h, _w; // Image size	
	float _maxAngDif; // maximal allowed angle difference in a curve

	Mat _engy1f; // Robust energy map for the input image

	void NoneMaximalSuppress(float lkEnd, float lkStart);
	void findEdge(Point seed, CEdge& crtEdge, bool isBackWard);
	bool goNext(Point &pnt, float& ornt, CEdge& crtEdge, int orntInd, bool isBackward);
	bool jumpNext(Point &pnt, float& ornt, CEdge& crtEdge, int orntInd, bool isBackward);

	/* Compute the eigenvalues and eigenvectors of the Hessian matrix given by
	dfdrr, dfdrc, and dfdcc, and sort them in descending order according to
	their absolute values. */
	static void compute_eigenvals(double dfdrr, double dfdrc, double dfdcc, double eigval[2], double eigvec[2][2]);
	
	inline void AllocSpace(const Size &imgSz);
	static inline float angle(float ornt1, float orn2);
	static inline void refreshOrnt(float& ornt, float& newOrnt);
};

typedef CmCurveEx::CEdge CmEdge;
typedef vector<CmEdge> CmEdges;

