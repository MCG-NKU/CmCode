/************************************************************************/
/* This source code is free for both academic and industry use.         */
/* Some important information for better using the source code could be */
/* found in the project page: http://mmcheng.net/bing					*/
/************************************************************************/

#pragma once
#include "DataSetVOC.h"
#include "ValStructVec.h"
#include "FilterBING.h"


class Objectness
{
public:
	// base for window size quantization, feature window size (W, W), and non-maximal suppress size NSS
	Objectness(DataSetVOC &voc, double base = 2, double intUionThr = 0.5, int W = 8, int NSS = 2);
	~Objectness(void);

	// Load trained model. 
	int loadTrainedModel(string modelName = ""); // Return -1, 0, or 1 if partial, none, or all loaded

	// Get potential bounding boxes, each of which is represented by a Vec4i for (minX, minY, maxX, maxY).
	// The trained model should be prepared before calling this function: loadTrainedModel() or trainStageI() + trainStageII().
	// Use numDet to control the final number of proposed bounding boxes, and number of per size (scale and aspect ratio)
	void getObjBndBoxes(CMat &img3u, ValStructVec<float, Vec4i> &valBoxes, int numDetPerSize = 120);

	// Training and testing on the dataset
	void trainObjectness(int numDetPerSize = 100);
	void getObjBndBoxesForTests(vector<vector<Vec4i>> &boxesTests, int numDetPerSize = 100); 
	void getObjBndBoxesForTestsFast(vector<vector<Vec4i>> &boxesTests, int numDetPerSize = 100); 
	void getRandomBoxes(vector<vector<Vec4i>> &boxesTests, int numD = 10000);
	void evaluatePerClassRecall(vector<vector<Vec4i>> &boxesTests, CStr &saveName = "Plot.m", const int numDet = 1000);
	void evaluatePerImgRecall(const vector<vector<Vec4i>> &boxesTests, CStr &saveName, const int numDet = 1000);
	void setColorSpace(int clr = MAXBGR);
	
	// Training SVM with feature vector X and label Y. 
	// Each row of X is a feature vector, with corresponding label in Y.
	// Return a CV_32F weight Mat
	static Mat trainSVM(CMat &X1f, const vecI &Y, int sT, double C, double bias = -1, double eps = 0.01);

	// pX1f, nX1f are positive and negative training samples, each is a row vector 
	static Mat trainSVM(const vector<Mat> &pX1f, const vector<Mat> &nX1f, int sT, double C, double bias = -1, double eps = 0.01, int maxTrainNum = 100000);

	enum {MAXBGR, HSV, G};

public: // For illustration etc.

	// Write matrix to binary file
	static bool matWrite(CStr& filename, CMat& M);

	// Read matrix from binary file
	static bool matRead( const string& filename, Mat& M);

	void illuTestReults(const vector<vector<Vec4i>> &boxesTests);
	void evaluatePAMI12(CStr &saveName = "PlotMAMI12.m");
	void evaluateIJCV13(CStr &saveName = "IJCV13.m");

	static void meanStdDev(CMat &data1f, Mat &mean1f, Mat &stdDev1f);	
	void illustrate();
	inline static float LoG(float x, float y, float delta) {float d = -(x*x+y*y)/(2*delta*delta);  return -1.0f/((float)(CV_PI)*pow(delta, 4)) * (1+d)*exp(d);} // Laplacian of Gaussian
	static Mat aFilter(float delta, int sz);
	static void illutrateLoG();

private: // Parameters
	const double _base, _logBase; // base for window size quantization
	const double _intUionThr; // Intersection Union threshold for considering as a successful detection. 
	const int _W; // As described in the paper: #Size, Size(_W, _H) of feature window. 
	const int _NSS; // Size for non-maximal suppress
	const int _maxT, _minT, _numT; // The minimal and maximal dimensions of the template
	
	int _Clr; //
	static const char* _clrName[3];
	
	DataSetVOC &_voc; // The dataset for training, testing
	string _modelName, _trainDirSI, _bbResDir;

	vecI _svmSzIdxs; // Indexes of active size. It's equal to _svmFilters.size() and _svmReW1f.rows
	Mat _svmFilter; // Filters learned at stage I, each is a _H by _W CV_32F matrix
	FilterBING _bingF; // BING filter
	Mat _svmReW1f; // Re-weight parameters learned at stage II. 	

private: // Help functions

	bool filtersLoaded() {int n = _svmSzIdxs.size(); return n > 0 && _svmReW1f.size() == Size(2, n) && _svmFilter.size() == Size(_W, _W);}
	
	int gtBndBoxSampling(const Vec4i &bbgt, vector<Vec4i> &samples, vecI &bbR);

	Mat getFeature(CMat &img3u, const Vec4i &bb); // Return region feature
	
	inline double maxIntUnion(const Vec4i &bb, const vector<Vec4i> &bbgts) {double maxV = 0; for(size_t i = 0; i < bbgts.size(); i++) maxV = max(maxV, DataSetVOC::interUnio(bb, bbgts[i])); return maxV; }
	
	// Convert VOC bounding box type to OpenCV Rect
	inline Rect pnt2Rect(const Vec4i &bb){int x = bb[0] - 1, y = bb[1] - 1; return Rect(x, y, bb[2] -  x, bb[3] - y);}

	// Template length at quantized scale t
	inline int tLen(int t){return cvRound(pow(_base, t));} 
	
	// Sub to quantization index
	inline int sz2idx(int w, int h) {w -= _minT; h -= _minT; CV_Assert(w >= 0 && h >= 0 && w < _numT && h < _numT); return h * _numT + w + 1; }
	inline string strVec4i(const Vec4i &v) const {return format("%d, %d, %d, %d", v[0], v[1], v[2], v[3]);}

	void generateTrianData();
	void trainStageI();
	void trainStateII(int numPerSz = 100);
	void predictBBoxSI(CMat &mag3u, ValStructVec<float, Vec4i> &valBoxes, vecI &sz, int NUM_WIN_PSZ = 100, bool fast = true);
	void predictBBoxSII(ValStructVec<float, Vec4i> &valBoxes, const vecI &sz);
	
	// Calculate the image gradient: center option as in VLFeat
	void gradientMag(CMat &imgBGR3u, Mat &mag1u);

	static void gradientRGB(CMat &bgr3u, Mat &mag1u);
	static void gradientGray(CMat &bgr3u, Mat &mag1u);
	static void gradientHSV(CMat &bgr3u, Mat &mag1u);
	static void gradientXY(CMat &x1i, CMat &y1i, Mat &mag1u);

	static inline int bgrMaxDist(const Vec3b &u, const Vec3b &v) {int b = abs(u[0]-v[0]), g = abs(u[1]-v[1]), r = abs(u[2]-v[2]); b = max(b,g);  return max(b,r);}
	static inline int vecDist3b(const Vec3b &u, const Vec3b &v) {return abs(u[0]-v[0]) + abs(u[1]-v[1]) + abs(u[2]-v[2]);}

	//Non-maximal suppress
	static void nonMaxSup(CMat &matchCost1f, ValStructVec<float, Point> &matchCost, int NSS = 1, int maxPoint = 50, bool fast = true);

	static void PrintVector(FILE *f, const vecD &v, CStr &name);

	vecD getVector(CMat &t1f);
};

