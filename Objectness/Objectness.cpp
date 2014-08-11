#include "StdAfx.h"
#include "Objectness.h"
#include "CmShow.h"

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))
void print_null(const char *s) {}
const char* Objectness::_clrName[3] = {"MAXBGR", "HSV", "I"};
const int CN = 21; // Color Number 
const char* COLORs[CN] = {"'k'", "'b'", "'g'", "'r'", "'c'", "'m'", "'y'",
	"':k'", "':b'", "':g'", "':r'", "':c'", "':m'", "':y'", 
	"'--k'", "'--b'", "'--g'", "'--r'", "'--c'", "'--m'", "'--y'"
};

//#define USE_BINARY

// base for window size quantization, R orientation channels, and feature window size (_W, _W)
Objectness::Objectness(DataSetVOC &voc, double base, double intUionThr, int W, int NSS)
	: _voc(voc)
	, _base(base)
	, _intUionThr(intUionThr)
	, _W(W)
	, _NSS(NSS)
	, _logBase(log(_base))
	, _minT(cvCeil(log(10.)/_logBase))
	, _maxT(cvCeil(log(500.)/_logBase))
	, _numT(_maxT - _minT + 1)
	, _Clr(MAXBGR)
{
	setColorSpace(_Clr);
}

Objectness::~Objectness(void)
{
}

void Objectness::setColorSpace(int clr)
{
	_Clr = clr;
	_modelName = _voc.resDir + format("ObjNessB%gW%d%s", _base, _W, _clrName[_Clr]);
	_trainDirSI = _voc.localDir + format("TrainS1B%gW%d%s/", _base, _W, _clrName[_Clr]);
	_bbResDir = _voc.resDir + format("BBoxesB%gW%d%s/", _base, _W, _clrName[_Clr]);
}

int Objectness::loadTrainedModel(string modelName) // Return -1, 0, or 1 if partial, none, or all loaded
{
	if (modelName.size() == 0)
		modelName = _modelName;
	CStr s1 = modelName + ".wS1", s2 = modelName + ".wS2", sI = modelName + ".idx";
	Mat filters1f, reW1f, idx1i, show3u;
	if (!matRead(s1, filters1f) || !matRead(sI, idx1i)){
		printf("Can't load model: %s or %s\n", _S(s1), _S(sI));
		return 0;
	}

	//filters1f = aFilter(0.8f, 8);
	//normalize(filters1f, filters1f, p, 1, NORM_MINMAX);

	normalize(filters1f, show3u, 1, 255, NORM_MINMAX, CV_8U);
	CmShow::showTinyMat(_voc.resDir + "Filter.png", show3u);

#ifdef USE_BINARY
	_bingF.update(filters1f);
	//_tigF.reconstruct(filters1f);
#endif // _DEBUG

	_svmSzIdxs = idx1i;
	CV_Assert(_svmSzIdxs.size() > 1 && filters1f.size() == Size(_W, _W) && filters1f.type() == CV_32F);
	_svmFilter = filters1f;

	if (!matRead(s2, _svmReW1f) || _svmReW1f.size() != Size(2, _svmSzIdxs.size())){
		_svmReW1f = Mat();
		return -1;
	}
	return 1;
}

void Objectness::predictBBoxSI(CMat &img3u, ValStructVec<float, Vec4i> &valBoxes, vecI &sz, int NUM_WIN_PSZ, bool fast)
{
	const int numSz = _svmSzIdxs.size();
	const int imgW = img3u.cols, imgH = img3u.rows;
	valBoxes.reserve(10000);
	sz.clear(); sz.reserve(10000);
	for (int ir = numSz - 1; ir >= 0; ir--){
		int r = _svmSzIdxs[ir]; 
		int height = cvRound(pow(_base, r/_numT + _minT)), width = cvRound(pow(_base, r%_numT + _minT));
		if (height > imgH * _base || width > imgW * _base)
			continue;

		height = min(height, imgH), width = min(width, imgW);
		Mat im3u, matchCost1f, mag1u;
		resize(img3u, im3u, Size(cvRound(_W*imgW*1.0/width), cvRound(_W*imgH*1.0/height)));
		gradientMag(im3u, mag1u);
		
		//imwrite(_voc.localDir + format("%d.png", r), mag1u);	
#ifdef USE_BINARY
		matchCost1f = _bingF.matchTemplate(mag1u);
#else
		Mat mag1f; 
		mag1u.convertTo(mag1f, CV_32F);
		matchTemplate(mag1f, _svmFilter, matchCost1f, CV_TM_CCORR);
#endif // _DEBUG		

		ValStructVec<float, Point> matchCost;
		nonMaxSup(matchCost1f, matchCost, _NSS, NUM_WIN_PSZ, fast);

		// Find true locations and match values
		double ratioX = width/_W, ratioY = height/_W;
		int iMax = min(matchCost.size(), NUM_WIN_PSZ);
		for (int i = 0; i < iMax; i++){
			float mVal = matchCost(i);
			Point pnt = matchCost[i];
			Vec4i box(cvRound(pnt.x * ratioX), cvRound(pnt.y*ratioY));
			box[2] = cvRound(min(box[0] + width, imgW));
			box[3] = cvRound(min(box[1] + height, imgH));
			box[0] ++;
			box[1] ++;
			valBoxes.pushBack(mVal, box); 
			sz.push_back(ir);
		}
	}
	//exit(0);
}

void Objectness::predictBBoxSII(ValStructVec<float, Vec4i> &valBoxes, const vecI &sz)
{
	int numI = valBoxes.size();
	for (int i = 0; i < numI; i++){
		const float* svmIIw = _svmReW1f.ptr<float>(sz[i]);
		valBoxes(i) = valBoxes(i) * svmIIw[0] + svmIIw[1]; 
	}
	valBoxes.sort();
}

// Get potential bounding boxes, each of which is represented by a Vec4i for (minX, minY, maxX, maxY).
// The trained model should be prepared before calling this function: loadTrainedModel() or trainStageI() + trainStageII().
// Use numDet to control the final number of proposed bounding boxes, and number of per size (scale and aspect ratio)
void Objectness::getObjBndBoxes(CMat &img3u, ValStructVec<float, Vec4i> &valBoxes, int numDetPerSize)
{
	CV_Assert_(filtersLoaded() , ("SVM filters should be initialized before getting object proposals\n"));
	vecI sz;
	predictBBoxSI(img3u, valBoxes, sz, numDetPerSize, false);
	predictBBoxSII(valBoxes, sz);
	return;
}

void Objectness::nonMaxSup(CMat &matchCost1f, ValStructVec<float, Point> &matchCost, int NSS, int maxPoint, bool fast)
{
	const int _h = matchCost1f.rows, _w = matchCost1f.cols;
	Mat isMax1u = Mat::ones(_h, _w, CV_8U), costSmooth1f;
	ValStructVec<float, Point> valPnt;
	matchCost.reserve(_h * _w);
	valPnt.reserve(_h * _w);
	if (fast){
		blur(matchCost1f, costSmooth1f, Size(3, 3));
		for (int r = 0; r < _h; r++){
			const float* d = matchCost1f.ptr<float>(r);
			const float* ds = costSmooth1f.ptr<float>(r);
			for (int c = 0; c < _w; c++)
				if (d[c] >= ds[c])
					valPnt.pushBack(d[c], Point(c, r));
		}
	}
	else{
		for (int r = 0; r < _h; r++){
			const float* d = matchCost1f.ptr<float>(r);
			for (int c = 0; c < _w; c++)
				valPnt.pushBack(d[c], Point(c, r));
		}
	}

	valPnt.sort();
	for (int i = 0; i < valPnt.size(); i++){
		Point &pnt = valPnt[i];
		if (isMax1u.at<byte>(pnt)){
			matchCost.pushBack(valPnt(i), pnt);
			for (int dy = -NSS; dy <= NSS; dy++) for (int dx = -NSS; dx <= NSS; dx++){
				Point neighbor = pnt + Point(dx, dy);
				if (!CHK_IND(neighbor))
					continue;
				isMax1u.at<byte>(neighbor) = false;
			}
		}
		if (matchCost.size() >= maxPoint)
			return;
	}
}

void Objectness::gradientMag(CMat &imgBGR3u, Mat &mag1u)
{
	switch (_Clr){
	case MAXBGR:
		gradientRGB(imgBGR3u, mag1u); break;
	case G:		
		gradientGray(imgBGR3u, mag1u); break;
	case HSV:
		gradientHSV(imgBGR3u, mag1u); break;
	default:
		printf("Error: not recognized color space\n");
	}
}

void Objectness::gradientRGB(CMat &bgr3u, Mat &mag1u)
{
	const int H = bgr3u.rows, W = bgr3u.cols;
	Mat Ix(H, W, CV_32S), Iy(H, W, CV_32S);

	// Left/right most column Ix
	for (int y = 0; y < H; y++){
		Ix.at<int>(y, 0) = bgrMaxDist(bgr3u.at<Vec3b>(y, 1), bgr3u.at<Vec3b>(y, 0))*2;
		Ix.at<int>(y, W-1) = bgrMaxDist(bgr3u.at<Vec3b>(y, W-1), bgr3u.at<Vec3b>(y, W-2))*2;
	}

	// Top/bottom most column Iy
	for (int x = 0; x < W; x++)	{
		Iy.at<int>(0, x) = bgrMaxDist(bgr3u.at<Vec3b>(1, x), bgr3u.at<Vec3b>(0, x))*2;
		Iy.at<int>(H-1, x) = bgrMaxDist(bgr3u.at<Vec3b>(H-1, x), bgr3u.at<Vec3b>(H-2, x))*2; 
	}

	// Find the gradient for inner regions
	for (int y = 0; y < H; y++){
		const Vec3b *dataP = bgr3u.ptr<Vec3b>(y);
		for (int x = 2; x < W; x++)
			Ix.at<int>(y, x-1) = bgrMaxDist(dataP[x-2], dataP[x]); //  bgr3u.at<Vec3b>(y, x+1), bgr3u.at<Vec3b>(y, x-1));
	}
	for (int y = 1; y < H-1; y++){
		const Vec3b *tP = bgr3u.ptr<Vec3b>(y-1);
		const Vec3b *bP = bgr3u.ptr<Vec3b>(y+1);
		for (int x = 0; x < W; x++)
			Iy.at<int>(y, x) = bgrMaxDist(tP[x], bP[x]);
	}
	gradientXY(Ix, Iy, mag1u);
}

void Objectness::gradientGray(CMat &bgr3u, Mat &mag1u)
{
	Mat g1u;
	cvtColor(bgr3u, g1u, CV_BGR2GRAY); 
	const int H = g1u.rows, W = g1u.cols;
	Mat Ix(H, W, CV_32S), Iy(H, W, CV_32S);

	// Left/right most column Ix
	for (int y = 0; y < H; y++){
		Ix.at<int>(y, 0) = abs(g1u.at<byte>(y, 1) - g1u.at<byte>(y, 0)) * 2;
		Ix.at<int>(y, W-1) = abs(g1u.at<byte>(y, W-1) - g1u.at<byte>(y, W-2)) * 2;
	}

	// Top/bottom most column Iy
	for (int x = 0; x < W; x++)	{
		Iy.at<int>(0, x) = abs(g1u.at<byte>(1, x) - g1u.at<byte>(0, x)) * 2;
		Iy.at<int>(H-1, x) = abs(g1u.at<byte>(H-1, x) - g1u.at<byte>(H-2, x)) * 2; 
	}

	// Find the gradient for inner regions
	for (int y = 0; y < H; y++)
		for (int x = 1; x < W-1; x++)
			Ix.at<int>(y, x) = abs(g1u.at<byte>(y, x+1) - g1u.at<byte>(y, x-1));
	for (int y = 1; y < H-1; y++)
		for (int x = 0; x < W; x++)
			Iy.at<int>(y, x) = abs(g1u.at<byte>(y+1, x) - g1u.at<byte>(y-1, x));

	gradientXY(Ix, Iy, mag1u);
}


void Objectness::gradientHSV(CMat &bgr3u, Mat &mag1u)
{
	Mat hsv3u;
	cvtColor(bgr3u, hsv3u, CV_BGR2HSV);
	const int H = hsv3u.rows, W = hsv3u.cols;
	Mat Ix(H, W, CV_32S), Iy(H, W, CV_32S);

	// Left/right most column Ix
	for (int y = 0; y < H; y++){
		Ix.at<int>(y, 0) = vecDist3b(hsv3u.at<Vec3b>(y, 1), hsv3u.at<Vec3b>(y, 0));
		Ix.at<int>(y, W-1) = vecDist3b(hsv3u.at<Vec3b>(y, W-1), hsv3u.at<Vec3b>(y, W-2));
	}

	// Top/bottom most column Iy
	for (int x = 0; x < W; x++)	{
		Iy.at<int>(0, x) = vecDist3b(hsv3u.at<Vec3b>(1, x), hsv3u.at<Vec3b>(0, x));
		Iy.at<int>(H-1, x) = vecDist3b(hsv3u.at<Vec3b>(H-1, x), hsv3u.at<Vec3b>(H-2, x)); 
	}

	// Find the gradient for inner regions
	for (int y = 0; y < H; y++)
		for (int x = 1; x < W-1; x++)
			Ix.at<int>(y, x) = vecDist3b(hsv3u.at<Vec3b>(y, x+1), hsv3u.at<Vec3b>(y, x-1))/2;
	for (int y = 1; y < H-1; y++)
		for (int x = 0; x < W; x++)
			Iy.at<int>(y, x) = vecDist3b(hsv3u.at<Vec3b>(y+1, x), hsv3u.at<Vec3b>(y-1, x))/2;

	gradientXY(Ix, Iy, mag1u);
}

void Objectness::gradientXY(CMat &x1i, CMat &y1i, Mat &mag1u)
{
	const int H = x1i.rows, W = x1i.cols;
	mag1u.create(H, W, CV_8U);
	for (int r = 0; r < H; r++){
		const int *x = x1i.ptr<int>(r), *y = y1i.ptr<int>(r);
		byte* m = mag1u.ptr<byte>(r);
		for (int c = 0; c < W; c++)
			m[c] = min(x[c] + y[c], 255);   //((int)sqrt(sqr(x[c]) + sqr(y[c])), 255);
	}
}

void Objectness::trainObjectness(int numDetPerSize)
{
	CmTimer tm1("Train1"), tm2("Train 2");

	//* Learning stage I
	tm1.Start();
	generateTrianData();
	trainStageI();
	tm1.Stop();
	printf("Learning stage I takes %g seconds... \n", tm1.TimeInSeconds()); //*/

	//* Learning stage II
	tm2.Start();
	trainStateII(numDetPerSize);
	tm2.Stop();
	printf("Learning stage II takes %g seconds... \n", tm2.TimeInSeconds()); //*/
	return;
}

void Objectness::generateTrianData()
{
	const int NUM_TRAIN = _voc.trainNum;
	const int FILTER_SZ = _W*_W;
	vector<vector<Mat>> xTrainP(NUM_TRAIN), xTrainN(NUM_TRAIN);
	vector<vecI> szTrainP(NUM_TRAIN); // Corresponding size index. 
	const int NUM_NEG_BOX = 100; // Number of negative windows sampled from each image

#pragma omp parallel for
	for (int i = 0; i < NUM_TRAIN; i++)	{
		const int NUM_GT_BOX = (int)_voc.gtTrainBoxes[i].size();
		vector<Mat> &xP = xTrainP[i], &xN = xTrainN[i];
		vecI &szP = szTrainP[i];
		xP.reserve(NUM_GT_BOX*4), szP.reserve(NUM_GT_BOX*4), xN.reserve(NUM_NEG_BOX);
		Mat im3u = imread(format(_S(_voc.imgPathW), _S(_voc.trainSet[i])));

		// Get positive training data
		for (int k = 0; k < NUM_GT_BOX; k++){
			const Vec4i& bbgt =  _voc.gtTrainBoxes[i][k];
			vector<Vec4i> bbs; // bounding boxes;
			vecI bbR; // Bounding box ratios
			int nS = gtBndBoxSampling(bbgt, bbs, bbR);
			for (int j = 0; j < nS; j++){
				bbs[j][2] = min(bbs[j][2], im3u.cols);
				bbs[j][3] = min(bbs[j][3], im3u.rows);
				Mat mag1f = getFeature(im3u, bbs[j]), magF1f;
				flip(mag1f, magF1f, CV_FLIP_HORIZONTAL);
				xP.push_back(mag1f);
				xP.push_back(magF1f);
				szP.push_back(bbR[j]);
				szP.push_back(bbR[j]);
			}			
		}
		// Get negative training data
		for (int k = 0; k < NUM_NEG_BOX; k++){
			int x1 = rand() % im3u.cols + 1, x2 = rand() % im3u.cols + 1;
			int y1 = rand() % im3u.rows + 1, y2 = rand() % im3u.rows + 1;
			Vec4i bb(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2));
			if (maxIntUnion(bb, _voc.gtTrainBoxes[i]) < 0.5)
				xN.push_back(getFeature(im3u, bb));
		}
	}
	
	const int NUM_R = _numT * _numT + 1;
	vecI szCount(NUM_R); // Object counts of each size (combination of scale and aspect ratio) 
	int numP = 0, numN = 0, iP = 0, iN = 0;
	for (int i = 0; i < NUM_TRAIN; i++){
		numP += xTrainP[i].size();
		numN += xTrainN[i].size();
		const vecI &rP = szTrainP[i];
		for (size_t j = 0; j < rP.size(); j++)
			szCount[rP[j]]++;
	}
	vecI szActive; // Indexes of active size
	for (int r = 1; r < NUM_R; r++){
		if (szCount[r] > 50) // If only 50- positive samples at this size, ignore it.
			szActive.push_back(r-1);			
	}
	matWrite(_modelName + ".idx", Mat(szActive));

	Mat xP1f(numP, FILTER_SZ, CV_32F), xN1f(numN, FILTER_SZ, CV_32F);
	for (int i = 0; i < NUM_TRAIN; i++)	{
		vector<Mat> &xP = xTrainP[i], &xN = xTrainN[i];
		for (size_t j = 0; j < xP.size(); j++)
			memcpy(xP1f.ptr(iP++), xP[j].data, FILTER_SZ*sizeof(float));
		for (size_t j = 0; j < xN.size(); j++)
			memcpy(xN1f.ptr(iN++), xN[j].data, FILTER_SZ*sizeof(float));
	}
	CV_Assert(numP == iP && numN == iN);
	matWrite(_modelName + ".xP", xP1f);
	matWrite(_modelName + ".xN", xN1f);
}

Mat Objectness::getFeature(CMat &img3u, const Vec4i &bb)
{
	int x = bb[0] - 1, y = bb[1] - 1;
	Rect reg(x, y, bb[2] -  x, bb[3] - y);
	Mat subImg3u, mag1f, mag1u;
	resize(img3u(reg), subImg3u, Size(_W, _W));
	gradientMag(subImg3u, mag1u);
	mag1u.convertTo(mag1f, CV_32F);
	return mag1f;
}

int Objectness::gtBndBoxSampling(const Vec4i &bbgt, vector<Vec4i> &samples, vecI &bbR)
{
	double wVal = bbgt[2] - bbgt[0] + 1, hVal = (bbgt[3] - bbgt[1]) + 1;
	wVal = log(wVal)/_logBase, hVal = log(hVal)/_logBase;
	int wMin = max((int)(wVal - 0.5), _minT), wMax = min((int)(wVal + 1.5), _maxT);
	int hMin = max((int)(hVal - 0.5), _minT), hMax = min((int)(hVal + 1.5), _maxT);
	for (int h = hMin; h <= hMax; h++) for (int w = wMin; w <= wMax; w++){
		int wT = tLen(w) - 1, hT = tLen(h) - 1;
		Vec4i bb(bbgt[0], bbgt[1], bbgt[0] + wT, bbgt[1] + hT);
		if (DataSetVOC::interUnio(bb, bbgt) >= 0.5){
			samples.push_back(bb);
			bbR.push_back(sz2idx(w, h));
			//if (bbgt[3] > hT){
			//	bb = Vec4i(bbgt[0], bbgt[3] - hT, bbgt[0] + wT, bbgt[3]);
			//	CV_Assert(DataSetVOC::interUnio(bb, bbgt) >= 0.5);
			//	samples.push_back(bb);
			//	bbR.push_back(sz2idx(w, h));
			//}
			//if (bbgt[2] > wT){
			//	bb = Vec4i(bbgt[2] - wT, bbgt[1], bbgt[2], bbgt[1] + hT);
			//	CV_Assert(DataSetVOC::interUnio(bb, bbgt) >= 0.5);
			//	samples.push_back(bb);
			//	bbR.push_back(sz2idx(w, h));
			//}
			//if (bbgt[2] > wT && bbgt[3] > hT){
			//	bb = Vec4i(bbgt[2] - wT, bbgt[3] - hT, bbgt[2], bbgt[3]);
			//	CV_Assert(DataSetVOC::interUnio(bb, bbgt) >= 0.5);
			//	samples.push_back(bb);
			//	bbR.push_back(sz2idx(w, h));
			//}
		}
	}
	return samples.size();
}

void Objectness::trainStateII(int numPerSz)
{
	loadTrainedModel();
	const int NUM_TRAIN = _voc.trainNum;
	vector<vecI> SZ(NUM_TRAIN), Y(NUM_TRAIN);
	vector<vecF> VAL(NUM_TRAIN);

#pragma omp parallel for
	for (int i = 0; i < _voc.trainNum; i++)	{
		const vector<Vec4i> &bbgts = _voc.gtTrainBoxes[i];
		ValStructVec<float, Vec4i> valBoxes;
		vecI &sz = SZ[i], &y = Y[i];
		vecF &val = VAL[i];
		CStr imgPath = format(_S(_voc.imgPathW), _S(_voc.trainSet[i]));
		predictBBoxSI(imread(imgPath), valBoxes, sz, numPerSz, false);
		const int num = valBoxes.size();
		CV_Assert(sz.size() == num);
		y.resize(num), val.resize(num);
		for (int j = 0; j < num; j++){
			Vec4i bb = valBoxes[j];
			val[j] = valBoxes(j);
			y[j] = maxIntUnion(bb, bbgts) >= 0.5 ? 1 : -1;
		}
	}

	const int NUM_SZ = _svmSzIdxs.size();
	const int maxTrainNum = 100000;
	vector<vecM> rXP(NUM_SZ), rXN(NUM_SZ);
	for (int r = 0; r < NUM_SZ; r++){
		rXP[r].reserve(maxTrainNum);
		rXN[r].reserve(1000000);
	}
	for (int i = 0; i < NUM_TRAIN; i++){
		const vecI &sz = SZ[i], &y = Y[i];
		vecF &val = VAL[i];
		int num = sz.size();
		for (int j = 0; j < num; j++){
			int r = sz[j];
			CV_Assert(r >= 0 && r < NUM_SZ);
			if (y[j] == 1)
				rXP[r].push_back(Mat(1, 1, CV_32F, &val[j]));
			else 
				rXN[r].push_back(Mat(1, 1, CV_32F, &val[j]));
		}
	}

	Mat wMat(NUM_SZ, 2, CV_32F);
	for (int i = 0; i < NUM_SZ; i++){
		const vecM &xP = rXP[i], &xN = rXN[i];
		if (xP.size() < 10 || xN.size() < 10)
			printf("Warning %s:%d not enough training sample for r[%d] = %d. P = %d, N = %d\n", __FILE__, __LINE__, i, _svmSzIdxs[i], xP.size(), xN.size());	
		for (size_t k = 0; k < xP.size(); k++)
			CV_Assert(xP[k].size() == Size(1, 1) && xP[k].type() == CV_32F);

		Mat wr = trainSVM(xP, xN, L1R_L2LOSS_SVC, 100, 1);
		CV_Assert(wr.size() == Size(2, 1));
		wr.copyTo(wMat.row(i));
	}
	matWrite(_modelName + ".wS2", wMat);
	_svmReW1f = wMat;
}

void Objectness::meanStdDev(CMat &data1f, Mat &mean1f, Mat &stdDev1f)
{
	const int DIM = data1f.cols, NUM = data1f.rows;
	mean1f = Mat::zeros(1, DIM, CV_32F), stdDev1f = Mat::zeros(1, DIM, CV_32F);
	for (int i = 0; i < NUM; i++)
		mean1f += data1f.row(i);
	mean1f /= NUM;
	for (int i = 0; i < NUM; i++){
		Mat tmp;
		pow(data1f.row(i) - mean1f, 2, tmp);
		stdDev1f += tmp;
	}
	pow(stdDev1f/NUM, 0.5, stdDev1f);
}

vecD Objectness::getVector(const Mat &_t1f)
{
	Mat t1f;
	_t1f.convertTo(t1f, CV_64F);
	return (vecD)(t1f.reshape(1, 1));
}

void Objectness::illustrate()
{
	Mat xP1f, xN1f;
	CV_Assert(matRead(_modelName + ".xP", xP1f) && matRead(_modelName + ".xN", xN1f));
	CV_Assert(xP1f.cols == xN1f.cols && xP1f.cols == _W*_W && xP1f.type() == CV_32F && xN1f.type() == CV_32F);
	Mat meanP,  meanN, stdDevP, stdDevN;
	meanStdDev(xP1f, meanP, stdDevP);
	meanStdDev(xN1f, meanN, stdDevN);
	Mat meanV(_W, _W*2, CV_32F), stdDev(_W, _W*2, CV_32F);
	meanP.reshape(1, _W).copyTo(meanV.colRange(0, _W));
	meanN.reshape(1, _W).copyTo(meanV.colRange(_W, _W*2));
	stdDevP.reshape(1, _W).copyTo(stdDev.colRange(0, _W));
	stdDevN.reshape(1, _W).copyTo(stdDev.colRange(_W, _W*2));
	normalize(meanV, meanV, 0, 255, NORM_MINMAX, CV_8U);
	CmShow::showTinyMat(_voc.resDir + "PosNeg.png", meanV);
	
	FILE* f = fopen(_S(_voc.resDir + "PosNeg.m"), "w"); 
	CV_Assert(f != NULL);
	fprintf(f, "figure(1);\n\n");
	PrintVector(f, getVector(meanP), "MeanP");
	PrintVector(f, getVector(meanN), "MeanN");
	PrintVector(f, getVector(stdDevP), "StdDevP");
	PrintVector(f, getVector(stdDevN), "StdDevN");
	PrintVector(f, getVector(_svmFilter), "Filter");
	fprintf(f, "hold on;\nerrorbar(MeanP, StdDevP, 'r');\nerrorbar(MeanN, StdDevN, 'g');\nhold off;");
	fclose(f);	
}

void Objectness::trainStageI()
{
	vecM pX, nX;
	pX.reserve(200000), nX.reserve(200000);
	Mat xP1f, xN1f;
	CV_Assert(matRead(_modelName + ".xP", xP1f) && matRead(_modelName + ".xN", xN1f));
	for (int r = 0; r < xP1f.rows; r++)
		pX.push_back(xP1f.row(r));
	for (int r = 0; r < xN1f.rows; r++)
		nX.push_back(xN1f.row(r));
	Mat crntW = trainSVM(pX, nX, L1R_L2LOSS_SVC, 10, 1);
	crntW = crntW.colRange(0, crntW.cols - 1).reshape(1, _W);
	CV_Assert(crntW.size() == Size(_W, _W));
	matWrite(_modelName + ".wS1", crntW);
}

// Training SVM with feature vector X and label Y. 
// Each row of X is a feature vector, with corresponding label in Y.
// Return a CV_32F weight Mat
Mat Objectness::trainSVM(CMat &X1f, const vecI &Y, int sT, double C, double bias, double eps)
{
	// Set SVM parameters
	parameter param; {
		param.solver_type = sT; // L2R_L2LOSS_SVC_DUAL;
		param.C = C;
		param.eps = eps; // see setting below
		param.p = 0.1;
		param.nr_weight = 0;
		param.weight_label = NULL;
		param.weight = NULL;
		set_print_string_function(print_null);
		CV_Assert(X1f.rows == Y.size() && X1f.type() == CV_32F);
	}

	// Initialize a problem
	feature_node *x_space = NULL;
	problem prob;{
		prob.l = X1f.rows;
		prob.bias = bias;
		prob.y = Malloc(double, prob.l);
		prob.x = Malloc(feature_node*, prob.l);
		const int DIM_FEA = X1f.cols;
		prob.n = DIM_FEA + (bias >= 0 ? 1 : 0);
		x_space = Malloc(feature_node, (prob.n + 1) * prob.l);
		int j = 0;
		for (int i = 0; i < prob.l; i++){
			prob.y[i] = Y[i];
			prob.x[i] = &x_space[j];
			const float* xData = X1f.ptr<float>(i);
			for (int k = 0; k < DIM_FEA; k++){
				x_space[j].index = k + 1;
				x_space[j++].value = xData[k];
			}
			if (bias >= 0){
				x_space[j].index = prob.n;
				x_space[j++].value = bias;
			}
			x_space[j++].index = -1;
		}
		CV_Assert(j == (prob.n + 1) * prob.l);
	}

	// Training SVM for current problem
	const char*  error_msg = check_parameter(&prob, &param);
	if(error_msg){
		fprintf(stderr,"ERROR: %s\n",error_msg);
		exit(1);
	}
	model *svmModel = train(&prob, &param);
	Mat wMat(1, prob.n, CV_64F, svmModel->w);
	wMat.convertTo(wMat, CV_32F);
	free_and_destroy_model(&svmModel);
	destroy_param(&param);
	free(prob.y);
	free(prob.x);
	free(x_space);
	return wMat;
}

// pX1f, nX1f are positive and negative training samples, each is a row vector 
Mat Objectness::trainSVM(const vector<Mat> &pX1f, const vector<Mat> &nX1f, int sT, double C, double bias, double eps, int maxTrainNum)
{
	vecI ind(nX1f.size());
	for (size_t i = 0; i < ind.size(); i++)
		ind[i] = i;
	int numP = pX1f.size(), feaDim = pX1f[0].cols;
	int totalSample = numP + nX1f.size();
	if (totalSample > maxTrainNum)
		random_shuffle(ind.begin(), ind.end());
	totalSample = min(totalSample, maxTrainNum);
	Mat X1f(totalSample, feaDim, CV_32F);
	vecI Y(totalSample);
	for(int i = 0; i < numP; i++){
		pX1f[i].copyTo(X1f.row(i));
		Y[i] = 1;
	}
	for (int i = numP; i < totalSample; i++){
		nX1f[ind[i - numP]].copyTo(X1f.row(i));
		Y[i] = -1;
	}
	return trainSVM(X1f, Y, sT, C, bias, eps);
}

// Get potential bounding boxes for all test images
void Objectness::getObjBndBoxesForTests(vector<vector<Vec4i>> &_boxesTests, int numDetPerSize)
{
	const int TestNum = _voc.testSet.size();
	vecM imgs3u(TestNum);
	vector<ValStructVec<float, Vec4i>> boxesTests;
	boxesTests.resize(TestNum);

#pragma omp parallel for
	for (int i = 0; i < TestNum; i++){
		imgs3u[i] = imread(format(_S(_voc.imgPathW), _S(_voc.testSet[i])));
		boxesTests[i].reserve(10000);
	}

	int scales[3] = {1, 3, 5};
	for (int clr = MAXBGR; clr <= G; clr++){
		setColorSpace(clr);
		trainObjectness(numDetPerSize);
		loadTrainedModel();
		CmTimer tm("Predict");
		tm.Start();

#pragma omp parallel for
		for (int i = 0; i < TestNum; i++){
			ValStructVec<float, Vec4i> boxes;
			getObjBndBoxes(imgs3u[i], boxes, numDetPerSize);
			boxesTests[i].append(boxes, scales[clr]);
		}

		tm.Stop();
		printf("Average time for predicting an image (%s) is %gs\n", _clrName[_Clr], tm.TimeInSeconds()/TestNum);
	}

	_boxesTests.resize(TestNum);
	CmFile::MkDir(_bbResDir);
#pragma omp parallel for
	for (int i = 0; i < TestNum; i++){
		CStr fName = _bbResDir + _voc.testSet[i];
		ValStructVec<float, Vec4i> &boxes = boxesTests[i];
		FILE *f = fopen(_S(fName + ".txt"), "w");
		fprintf(f, "%d\n", boxes.size());
		for (size_t k = 0; k < boxes.size(); k++)
			fprintf(f, "%g, %s\n", boxes(k), _S(strVec4i(boxes[k])));
		fclose(f);

		_boxesTests[i].resize(boxesTests[i].size());
		for (int j = 0; j < boxesTests[i].size(); j++)
			_boxesTests[i][j] = boxesTests[i][j];
	}

	evaluatePerImgRecall(_boxesTests, "PerImgAllNS.m", 5000);

#pragma omp parallel for
	for (int i = 0; i < TestNum; i++){
		boxesTests[i].sort(false);
		for (int j = 0; j < boxesTests[i].size(); j++)
			_boxesTests[i][j] = boxesTests[i][j];
	}
	evaluatePerImgRecall(_boxesTests, "PerImgAllS.m", 5000);
}


// Get potential bounding boxes for all test images
void Objectness::getObjBndBoxesForTestsFast(vector<vector<Vec4i>> &_boxesTests, int numDetPerSize)
{
	//setColorSpace(HSV);
	trainObjectness(numDetPerSize);
	loadTrainedModel();
	//illustrate();

	const int TestNum = _voc.testSet.size();
	vecM imgs3u(TestNum);
	vector<ValStructVec<float, Vec4i>> boxesTests;
	boxesTests.resize(TestNum);

#pragma omp parallel for
	for (int i = 0; i < TestNum; i++){
		imgs3u[i] = imread(format(_S(_voc.imgPathW), _S(_voc.testSet[i])));
		boxesTests[i].reserve(10000);
	}

	printf("Start predicting\n");
	CmTimer tm("Predict");
	tm.Start();

#pragma omp parallel for
	for (int i = 0; i < TestNum; i++)
		getObjBndBoxes(imgs3u[i], boxesTests[i], numDetPerSize);
	
	tm.Stop();
	printf("Average time for predicting an image (%s) is %gs\n", _clrName[_Clr], tm.TimeInSeconds()/TestNum);

	_boxesTests.resize(TestNum);
	CmFile::MkDir(_bbResDir);

#pragma omp parallel for
	for (int i = 0; i < TestNum; i++){
		CStr fName = _bbResDir + _voc.testSet[i];
		ValStructVec<float, Vec4i> &boxes = boxesTests[i];
		FILE *f = fopen(_S(fName + ".txt"), "w");
		fprintf(f, "%d\n", boxes.size());
		for (size_t k = 0; k < boxes.size(); k++)
			fprintf(f, "%g, %s\n", boxes(k), _S(strVec4i(boxes[k])));
		fclose(f);

		_boxesTests[i].resize(boxesTests[i].size());
		for (int j = 0; j < boxesTests[i].size(); j++)
			_boxesTests[i][j] = boxesTests[i][j];
	}

	evaluatePerImgRecall(_boxesTests, format("PerImgAllB%gT%d.m", _base, cvRound(100*_intUionThr)), 5000);
}


void Objectness::getRandomBoxes(vector<vector<Vec4i>> &boxesTests, int num)
{
	const int TestNum = _voc.testSet.size();
	boxesTests.resize(TestNum);
#pragma omp parallel for
	for (int i = 0; i < TestNum; i++){
		Mat imgs3u = imread(format(_S(_voc.imgPathW), _S(_voc.testSet[i])));
		int W = imgs3u.cols, H = imgs3u.rows;
		boxesTests[i].reserve(num);
		for (int k = 0; k < num; k++){
			int x1 = rand()%W + 1, x2 = rand()%W + 1;
			int y1 = rand()%H + 1, y2 = rand()%H + 1;
			boxesTests[i].push_back(Vec4i(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2)));
		}
	}
	evaluatePerImgRecall(boxesTests, "PerImgAll.m", num);
}

void Objectness::evaluatePerImgRecall(const vector<vector<Vec4i>> &boxesTests, CStr &saveName, const int NUM_WIN)
{
	vecD recalls(NUM_WIN);
	vecD avgScore(NUM_WIN);
	const int TEST_NUM = _voc.testSet.size();
	for (int i = 0; i < TEST_NUM; i++){
		const vector<Vec4i> &boxesGT = _voc.gtTestBoxes[i];
		const vector<Vec4i> &boxes = boxesTests[i];
		const int gtNumCrnt = boxesGT.size();
		vecI detected(gtNumCrnt);
		vecD score(gtNumCrnt);
		double sumDetected = 0, abo = 0;
		for (int j = 0; j < NUM_WIN; j++){
			if (j >= (int)boxes.size()){
				recalls[j] += sumDetected/gtNumCrnt;
				avgScore[j] += abo/gtNumCrnt;
				continue;
			}

			for (int k = 0; k < gtNumCrnt; k++)	{
				double s = DataSetVOC::interUnio(boxes[j], boxesGT[k]);
				score[k] = max(score[k], s);
				detected[k] = score[k] >= _intUionThr ? 1 : 0;
			}
			sumDetected = 0, abo = 0;
			for (int k = 0; k < gtNumCrnt; k++)	
				sumDetected += detected[k], abo += score[k];
			recalls[j] += sumDetected/gtNumCrnt;
			avgScore[j] += abo/gtNumCrnt;
		}
	}

	for (int i = 0; i < NUM_WIN; i++){
		recalls[i] /=  TEST_NUM;
		avgScore[i] /= TEST_NUM;
	}

	int idx[8] = {1, 10, 100, 1000, 2000, 3000, 4000, 5000};
	for (int i = 0; i < 8; i++){
		if (idx[i] > NUM_WIN)
			continue;
		printf("%d:%.3g,%.3g\t", idx[i], recalls[idx[i] - 1], avgScore[idx[i] - 1]);
	}
	printf("\n");
	
	FILE* f = fopen(_S(_voc.resDir + saveName), "w"); 
	CV_Assert(f != NULL);
	fprintf(f, "figure(1);\n\n");
	PrintVector(f, recalls, "DR");
	PrintVector(f, avgScore, "MABO");
	fprintf(f, "semilogx(1:%d, DR(1:%d));\nhold on;\nsemilogx(1:%d, DR(1:%d));\naxis([1, 5000, 0, 1]);\nhold off;\n", NUM_WIN, NUM_WIN, NUM_WIN, NUM_WIN);
	fclose(f);	
}

void Objectness::illuTestReults(const vector<vector<Vec4i>> &boxesTests)
{
	CStr resDir = _voc.localDir + "ResIlu/";
	CmFile::MkDir(resDir);
	const int TEST_NUM = _voc.testSet.size();
	for (int i = 0; i < TEST_NUM; i++){
		const vector<Vec4i> &boxesGT = _voc.gtTestBoxes[i];
		const vector<Vec4i> &boxes = boxesTests[i];
		const int gtNumCrnt = boxesGT.size();
		CStr imgPath = format(_S(_voc.imgPathW), _S(_voc.testSet[i]));
		CStr resNameNE = CmFile::GetNameNE(imgPath);
		Mat img = imread(imgPath);
		Mat bboxMatchImg = Mat::zeros(img.size(), CV_32F);

		vecD score(gtNumCrnt);
		vector<Vec4i> bboxMatch(gtNumCrnt);
		for (int j = 0; j < boxes.size(); j++){
			const Vec4i &bb = boxes[j];
			for (int k = 0; k < gtNumCrnt; k++)	{
				double mVal = DataSetVOC::interUnio(boxes[j], boxesGT[k]);
				if (mVal < score[k])
					continue;
				score[k] = mVal;
				bboxMatch[k] = boxes[j];
			}
		}

		for (int k = 0; k < gtNumCrnt; k++){
			const Vec4i &bb = bboxMatch[k];
			rectangle(img, Point(bb[0], bb[1]), Point(bb[2], bb[3]), Scalar(0), 3);
			rectangle(img, Point(bb[0], bb[1]), Point(bb[2], bb[3]), Scalar(255, 255, 255), 2);
			rectangle(img, Point(bb[0], bb[1]), Point(bb[2], bb[3]), Scalar(0, 0, 255), 1);
		}

		imwrite(resDir + resNameNE + "_Match.jpg", img);
	}
}

void Objectness::evaluatePerClassRecall(vector<vector<Vec4i>> &boxesTests, CStr &saveName, const int WIN_NUM) 
{
	const int TEST_NUM = _voc.testSet.size(), CLS_NUM = _voc.classNames.size();
	if (boxesTests.size() != TEST_NUM){
		boxesTests.resize(TEST_NUM);
		for (int i = 0; i < TEST_NUM; i++){
			Mat boxes;
			matRead(_voc.localDir + _voc.testSet[i] + ".dat", boxes);
			Vec4i* d = (Vec4i*)boxes.data;
			boxesTests[i].resize(boxes.rows, WIN_NUM);
			memcpy(&boxesTests[i][0], boxes.data, sizeof(Vec4i)*boxes.rows);
		}
	}

	for (int i = 0; i < TEST_NUM; i++)
		if ((int)boxesTests[i].size() < WIN_NUM){
			printf("%s.dat: %d, %d\n", _S(_voc.testSet[i]), boxesTests[i].size(), WIN_NUM);
			boxesTests[i].resize(WIN_NUM);
		}
	

	// #class by #win matrix for saving correct detection number and gt number
	Mat crNum1i = Mat::zeros(CLS_NUM, WIN_NUM, CV_32S);
	vecD gtNums(CLS_NUM); {
		for (int i = 0; i < TEST_NUM; i++){
			const vector<Vec4i> &boxes = boxesTests[i];
			const vector<Vec4i> &boxesGT = _voc.gtTestBoxes[i];
			const vecI &clsGT = _voc.gtTestClsIdx[i];
			CV_Assert((int)boxes.size() >= WIN_NUM);
			const int gtNumCrnt = boxesGT.size();
			for (int j = 0; j < gtNumCrnt; j++){
				gtNums[clsGT[j]]++;
				double maxIntUni = 0;
				int* crNum = crNum1i.ptr<int>(clsGT[j]);
				for (int k = 0; k < WIN_NUM; k++) {
					double val = DataSetVOC::interUnio(boxes[k], boxesGT[j]);
					maxIntUni = max(maxIntUni, val);
					crNum[k] += maxIntUni >= 0.5 ? 1 : 0;
				}
			}
		}
	}

	FILE* f = fopen(_S(_voc.resDir + saveName), "w"); {
		CV_Assert(f != NULL);
		fprintf(f, "figure(1);\nhold on;\n\n\n");
		vecD val(WIN_NUM), recallObjs(WIN_NUM), recallClss(WIN_NUM);
		for (int i = 0; i < WIN_NUM; i++)
			val[i] = i;
		PrintVector(f, gtNums, "GtNum");
		PrintVector(f, val, "WinNum");
		fprintf(f, "\n");
		string leglendStr("legend(");
		double sumObjs = 0;
		for (int c = 0; c < CLS_NUM; c++){
			sumObjs += gtNums[c];
			memset(&val[0], 0, sizeof(double)*WIN_NUM);
			int* crNum = crNum1i.ptr<int>(c);
			for (int i = 0; i < WIN_NUM; i++){
				val[i] = crNum[i]/(gtNums[c] + 1e-200);
				recallClss[i] += val[i];
				recallObjs[i] += crNum[i];
			}
			CStr className = _voc.classNames[c];
			PrintVector(f, val, className);
			fprintf(f, "plot(WinNum, %s, %s, 'linewidth', 2);\n", _S(className), COLORs[c % CN]);
			leglendStr += format("'%s', ", _S(className));
		}
		for (int i = 0; i < WIN_NUM; i++){
			recallClss[i] /= CLS_NUM;
			recallObjs[i] /= sumObjs;
		}
		PrintVector(f, recallClss, "class");
		fprintf(f, "plot(WinNum, %s, %s, 'linewidth', 2);\n", "class", COLORs[CLS_NUM % CN]);
		leglendStr += format("'%s', ", "class");
		PrintVector(f, recallObjs, "objects");
		fprintf(f, "plot(WinNum, %s, %s, 'linewidth', 2);\n", "objects", COLORs[(CLS_NUM+1) % CN]);
		leglendStr += format("'%s', ", "objects");
		leglendStr.resize(leglendStr.size() - 2);
		leglendStr += ");";		
		fprintf(f, "%s\nhold off;\nxlabel('#WIN');\nylabel('Recall');\ngrid on;\naxis([0 %d 0 1]);\n", _S(leglendStr), WIN_NUM);
		fprintf(f, "[class([1,10,100,1000]);objects([1,10,100,1000])]\ntitle('%s')\n", _S(saveName));
		fclose(f);
		printf("%-70s\r", "");
	}
	evaluatePerImgRecall(boxesTests, CmFile::GetNameNE(saveName) + "_PerI.m", WIN_NUM);
}

void Objectness::PrintVector(FILE *f, const vecD &v, CStr &name)
{
	fprintf(f, "%s = [", name.c_str());
	for (size_t i = 0; i < v.size(); i++)
		fprintf(f, "%g ", v[i]);
	fprintf(f, "];\n");
}

// Write matrix to binary file
bool Objectness::matWrite(CStr& filename, CMat& _M){
	Mat M;
	_M.copyTo(M);
	FILE* file = fopen(_S(filename), "wb");
	if (file == NULL || M.empty())
		return false;
	fwrite("CmMat", sizeof(char), 5, file);
	int headData[3] = {M.cols, M.rows, M.type()};
	fwrite(headData, sizeof(int), 3, file);
	fwrite(M.data, sizeof(char), M.step * M.rows, file);
	fclose(file);
	return true;
}

// Read matrix from binary file
bool Objectness::matRead(const string& filename, Mat& _M){
	FILE* f = fopen(_S(filename), "rb");
	if (f == NULL)
		return false;
	char buf[8];
	int pre = fread(buf,sizeof(char), 5, f);
	if (strncmp(buf, "CmMat", 5) != 0)	{
		printf("Invalidate CvMat data file %s\n", _S(filename));
		return false;
	}
	int headData[3]; // Width, height, type
	fread(headData, sizeof(int), 3, f);
	Mat M(headData[1], headData[0], headData[2]);
	fread(M.data, sizeof(char), M.step * M.rows, f);
	fclose(f);
	M.copyTo(_M);
	return true;
}

void Objectness::evaluatePAMI12(CStr &saveName)
{
	const int TEST_NUM = _voc.testSet.size();
	vector<vector<Vec4i>> boxesTests(TEST_NUM);
	CStr dir = _voc.wkDir + "PAMI12/";
	const int numDet = 1853;
	for (int i = 0; i < TEST_NUM; i++){
		FILE *f = fopen(_S(dir + _voc.testSet[i] + ".txt"), "r");
		double score;
		boxesTests[i].resize(numDet);
		for (int j = 0; j < numDet; j++){
			Vec4i &v = boxesTests[i][j];
			fscanf(f, "%d %d %d %d %g\n", &v[0], &v[1], &v[2], &v[3], &score);
		}
		fclose(f);
	}
	printf("Load data finished\r");
	//evaluatePerImgRecall(boxesTests, saveName, numDet);
	evaluatePerClassRecall(boxesTests, saveName, numDet);
}

void Objectness::evaluateIJCV13(CStr &saveName)
{
	const int TEST_NUM = _voc.testSet.size();
	vector<vector<Vec4i>> boxesTests(TEST_NUM);
	CStr dir = _voc.wkDir + "IJCV13/";
	const int numDet = 10000;
	for (int i = 0; i < TEST_NUM; i++){
		FILE *f = fopen(_S(dir + _voc.testSet[i] + ".txt"), "r");
		boxesTests[i].resize(numDet);
		for (int j = 0; j < numDet; j++){
			Vec4i &v = boxesTests[i][j];
			fscanf(f, "%d, %d, %d, %d\n", &v[1], &v[0], &v[3], &v[2]);
		}
		fclose(f);
	}
	printf("Load data finished\r");
	evaluatePerImgRecall(boxesTests, saveName, numDet);
	//evaluate(boxesTests, saveName, numDet);
}

inline float distG(float d, float delta) {return exp(-d*d/(2*delta*delta));}

Mat Objectness::aFilter(float delta, int sz)
{
	float dis = float(sz-1)/2.f;
	Mat mat(sz, sz, CV_32F);
	for (int r = 0; r < sz; r++)
		for (int c = 0; c < sz; c++)
			mat.at<float>(r, c) = distG(sqrt(sqr(r-dis)+sqr(c-dis)) - dis, delta);
	return mat;
}

void Objectness::illutrateLoG()
{
	for (float delta = 0.5f; delta < 1.1f; delta+=0.1f){
		Mat f = Objectness::aFilter(delta, 8);
		normalize(f, f, 0, 1, NORM_MINMAX);
		CmShow::showTinyMat(format("D=%g", delta), f);
	}
	waitKey(0);
}
