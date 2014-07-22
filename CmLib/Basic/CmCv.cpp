#include "StdAfx.h"
#include "CmCv.h"


/************************************************************************/
/* AbsAngle: Calculate magnitude and angle of vectors.					*/
/************************************************************************/
void CmCv::AbsAngle(CMat& cmplx32FC2, Mat& mag32FC1, Mat& ang32FC1)
{
	CV_Assert(cmplx32FC2.type() == CV_32FC2);
	mag32FC1.create(cmplx32FC2.size(), CV_32FC1);
	ang32FC1.create(cmplx32FC2.size(), CV_32FC1);

	for (int y = 0; y < cmplx32FC2.rows; y++)	{
		const float* cmpD = cmplx32FC2.ptr<float>(y);
		float* dataA = ang32FC1.ptr<float>(y);
		float* dataM = mag32FC1.ptr<float>(y);
		for (int x = 0; x < cmplx32FC2.cols; x++, cmpD += 2)	{
			dataA[x] = atan2(cmpD[1], cmpD[0]);
			dataM[x] = sqrt(cmpD[0] * cmpD[0] + cmpD[1] * cmpD[1]);
		}
	}
}

/************************************************************************/
/* GetCmplx: Get a complex value image from it's magnitude and angle    */
/************************************************************************/
void CmCv::GetCmplx(CMat& mag32F, CMat& ang32F, Mat& cmplx32FC2)
{
	CV_Assert(mag32F.type() == CV_32FC1 && ang32F.type() == CV_32FC1 && mag32F.size() == ang32F.size());
	cmplx32FC2.create(mag32F.size(), CV_32FC2);
	for (int y = 0; y < mag32F.rows; y++){
		float* cmpD = cmplx32FC2.ptr<float>(y);
		const float* dataA = ang32F.ptr<float>(y);
		const float* dataM = mag32F.ptr<float>(y);
		for (int x = 0; x < mag32F.cols; x++, cmpD += 2) {
			cmpD[0] = dataM[x] * cos(dataA[x]);
			cmpD[1] = dataM[x] * sin(dataA[x]);
		}
	}
}

// Mat2GrayLog: Convert and arbitrary mat to [0, 1] for display.
// The result image is in 32FCn format and range [0, 1.0].
// Mat2GrayLinear(log(img+1), newImg). In place operation is supported.
void CmCv::Mat2GrayLog(CMat& img, Mat& newImg)
{
	img.convertTo(newImg, CV_32F);
	newImg += 1;
	cv::log(newImg, newImg);
	cv::normalize(newImg, newImg, 0, 1, NORM_MINMAX);
}

// Low frequency part is always been move to the central part:
//				 -------                          -------	
//				| 1 | 2 |                        | 3 | 4 |	
//				 -------            -->           -------	
//				| 4 | 3 |                        | 2 | 1 |	
//				 -------                          -------	
void CmCv::FFTShift(Mat& img)
{
	int w = img.cols / 2, h = img.rows / 2;
	int cx2 = img.cols - w, cy2 = img.rows - h;
	Swap(img(Rect(0, 0, w, h)), img(Rect(cx2, cy2, w, h)));  // swap 1, 3
	Swap(img(Rect(cx2, 0, w, h)), img(Rect(0, cy2, w, h)));  // swap 2, 4
}

/************************************************************************/
/* Swap the content of two Mat with same type and size                  */
/************************************************************************/
void CmCv::Swap(Mat& a, Mat& b)
{
	CV_Assert(a.type() == b.type() && a.size() == b.size());
	Mat t;
	a.copyTo(t);
	b.copyTo(a);
	t.copyTo(b);
}

Rect CmCv::GetMaskRange(CMat &mask1u, int ext, int thresh)
{
	int maxX = INT_MIN, maxY = INT_MIN, minX = INT_MAX, minY = INT_MAX, rows = mask1u.rows, cols = mask1u.cols;
	for (int r = 0; r < rows; r++)	{
		const byte* data = mask1u.ptr<byte>(r);
		for (int c = 0; c < cols; c++)
			if (data[c] > thresh) {
				maxX = max(maxX, c);
				minX = min(minX, c);
				maxY = max(maxY, r);
				minY = min(minY, r);
			}
	}

	maxX = maxX + ext + 1 < cols ? maxX + ext + 1 : cols;
	maxY = maxY + ext + 1 < rows ? maxY + ext + 1 : rows;
	minX = minX - ext > 0 ? minX - ext : 0;
	minY = minY - ext > 0 ? minY - ext : 0;

	return Rect(minX, minY, maxX - minX, maxY - minY);
}

// Get continuous components for same label regions. Return region index mat,
// index-counter pair (Number of pixels for each index), and label of each idx
int CmCv::GetRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxCount, vecB &idxLabel, bool noZero)
{
	vector<pair<int, int>> counterIdx;
	int _w = label1u.cols, _h = label1u.rows, maxIdx = -1;
	regIdx1i.create(label1u.size());
	regIdx1i = -1;
	vecB labels;

	for (int y = 0; y < _h; y++){		
		int *regIdx = regIdx1i.ptr<int>(y);
		for (int x = 0; x < _w; x++) {
			if (regIdx[x] != -1) // If already assigned to a region
				continue;
			if (noZero && label1u(y, x) == 0)
				continue;
			
			byte crntVal = label1u(y, x);
			pair<int, int> counterReg(0, ++maxIdx); // Number of pixels in region with index maxIdx
			Point pt(x, y);
			queue<Point, list<Point>> neighbs;
			regIdx[x] = maxIdx;
			neighbs.push(pt);

			// Repeatably add pixels to the queue to construct neighbor regions
			while(neighbs.size()){
				// Mark current pixel
				pt = neighbs.front();
				neighbs.pop();
				counterReg.first++;

				for (int i = 0; i < 8; i++)	{
					Point nPt = pt + DIRECTION8[i];
					if (CHK_IND(nPt) && regIdx1i(nPt) == -1 && label1u(nPt) == crntVal)
						regIdx1i(nPt) = maxIdx, neighbs.push(nPt);  
				}		
			}

			// Add current region to regions
			counterIdx.push_back(counterReg);
			labels.push_back(crntVal);
		}
	}
	sort(counterIdx.begin(), counterIdx.end(), greater<pair<int, int>>());
	int idxNum = (int)counterIdx.size();
	vector<int> newIdx(idxNum);
	idxCount.resize(idxNum);
	idxLabel.resize(idxNum);
	for (int i = 0; i < idxNum; i++){
		idxCount[i] = counterIdx[i].first;
		newIdx[counterIdx[i].second] = i;
		idxLabel[i] = labels[counterIdx[i].second];
	}
	
	for (int y = 0; y < _h; y++){
		int *regIdx = regIdx1i.ptr<int>(y);
		for (int x = 0; x < _w; x++)
			if (!noZero || label1u(y, x) != 0)
				regIdx[x] = newIdx[regIdx[x]];
	}

	/* Test GetRegions
	{
		Mat showImg = Mat::zeros(_h, _w * 2, CV_8UC3);
		Mat showReg = showImg(Rect(_w, 0, _w, _h));
		Mat showLabel = showImg(Rect(0, 0, _w, _h));
		cvtColor(label1u, showLabel, CV_GRAY2BGR);
		Mat mask1u;
		for (size_t i = 0; i < idxCount.size(); i++)
		{
			compare(regIdx1i, i, mask1u, CMP_EQ);
			showReg.setTo(Scalar(rand() % 128 + 128, rand()%128 + 128, i%255), mask1u);
			imshow("Regions", showImg);
			printf("%d pixels in region %d, label = %d\n", idxCount[i], i, (int)idxLabel[i]);
			waitKey(1);
		}
		waitKey(0);
	}
	//*/
	return idxNum;
}

// Get continuous components for non-zero labels. Return region index mat (region index 
// of each mat position) and sum of label values in each region
int CmCv::GetNZRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxSum)
{
	vector<pair<int, int>> counterIdx;
	int _w = label1u.cols, _h = label1u.rows, maxIdx = -1;
	regIdx1i.create(label1u.size());
	regIdx1i = -1;

	for (int y = 0; y < _h; y++){		
		int *regIdx = regIdx1i.ptr<int>(y);
		const byte *label = label1u.ptr<byte>(y);
		for (int x = 0; x < _w; x++) {
			if (regIdx[x] != -1 || label[x] == 0)
				continue;
			
			pair<int, int> counterReg(0, ++maxIdx); // Number of pixels in region with index maxIdx
			Point pt(x, y);
			queue<Point, list<Point>> neighbs;
			regIdx[x] = maxIdx;
			neighbs.push(pt);

			// Repeatably add pixels to the queue to construct neighbor regions
			while(neighbs.size()){
				// Mark current pixel
				pt = neighbs.front();
				neighbs.pop();
				counterReg.first += label1u(pt);

				// Mark its unmarked neighbor pixels if similar
				Point nPt(pt.x, pt.y - 1); //Upper 
				if (nPt.y >= 0 && regIdx1i(nPt) == -1 && label1u(nPt) > 0){
					regIdx1i(nPt) = maxIdx;
					neighbs.push(nPt);  
				}

				nPt.y = pt.y + 1; // lower
				if (nPt.y < _h && regIdx1i(nPt) == -1 && label1u(nPt) > 0){
					regIdx1i(nPt) = maxIdx;
					neighbs.push(nPt);  
				}

				nPt.y = pt.y, nPt.x = pt.x - 1; // Left
				if (nPt.x >= 0 && regIdx1i(nPt) == -1 && label1u(nPt) > 0){
					regIdx1i(nPt) = maxIdx;
					neighbs.push(nPt);  
				}

				nPt.x = pt.x + 1;  // Right
				if (nPt.x < _w && regIdx1i(nPt) == -1 && label1u(nPt) > 0)	{
					regIdx1i(nPt) = maxIdx;
					neighbs.push(nPt);  
				}				
			}

			// Add current region to regions
			counterIdx.push_back(counterReg);
		}
	}
	sort(counterIdx.begin(), counterIdx.end(), greater<pair<int, int>>());
	int idxNum = (int)counterIdx.size();
	vector<int> newIdx(idxNum);
	idxSum.resize(idxNum);
	for (int i = 0; i < idxNum; i++){
		idxSum[i] = counterIdx[i].first;
		newIdx[counterIdx[i].second] = i;
	}
	
	for (int y = 0; y < _h; y++){
		int *regIdx = regIdx1i.ptr<int>(y);
		for (int x = 0; x < _w; x++)
			if (regIdx[x] >= 0)
				regIdx[x] = newIdx[regIdx[x]];
	}
	return idxNum;
}

Mat CmCv::GetNZRegionsLS(CMat &mask1u, double ignoreRatio)
{
	CV_Assert(mask1u.type() == CV_8UC1 && mask1u.data != NULL);
	ignoreRatio *= mask1u.rows * mask1u.cols * 255;
	Mat_<int> regIdx1i;
	vecI idxSum;
	Mat resMask;
	CmCv::GetNZRegions(mask1u, regIdx1i, idxSum);
	if (idxSum.size() >= 1 && idxSum[0] > ignoreRatio)
		compare(regIdx1i, 0, resMask, CMP_EQ);
	return resMask;
}

int CmCv::GetBorderPnts(Size sz, double ratio, vector<Point> &bdPnts)
{
	int w = sz.width, h = sz.height;
	int wGap = cvRound(w * ratio), hGap = cvRound(h * ratio);
	int idx = 0, bdCount = 2 * (hGap * w + wGap * h - 2 * hGap * wGap);
	bdPnts.resize(bdCount);

	ForPoints2(pnt, 0, 0, w, hGap) // Top region
		bdPnts[idx++] = pnt;
	ForPoints2(pnt, 0, h - hGap, w, h) // Bottom region
		bdPnts[idx++] = pnt;
	ForPoints2(pnt, 0, hGap, wGap, h - hGap) // Left center region
		bdPnts[idx++] = pnt;
	ForPoints2(pnt, w - wGap, hGap, w, h-hGap)
		bdPnts[idx++] = pnt;
	return bdCount;
}

Mat CmCv::GetBorderRegC(CMat &img3u, Mat &idx1i, vecI &idxCount)
{
	Mat img, edgC;
	GaussianBlur(img3u, img, Size(3, 3), 0);
	Mat edg = Mat::zeros(img.size(), CV_8U);
	vecM imgs;
	split(img, imgs);
	for (int c = 0; c < 3; c++)	{
		Canny(imgs[c], edgC, 100, 400, 5, true);
		edg += edgC;
	}
	compare(edg, 150, edg, CMP_LE);

	int _h = img.rows, _w = img.cols;
	vecI xCount(_w, 0), yCount(_h, 0);
	for (int r = 0; r < _h; r++)	{
		const byte* edgP = edg.ptr<byte>(r);
		for (int c = 0; c < _w; c++)
			if (edgP[c] == 0)
				xCount[c]++, yCount[r]++;
	}
	bool b1 = true, b2 = true, b3 = true, b4 = true;
	for (int t = 0; t < 8; t++){  // Remove frame
		if (t >= 3)
			b1 = b2 = b3 = b4 = false;
		if (b1 && xCount[t] > 0.6 * _h || xCount[t] > 0.8 * _h)
			edg.col(t) = 255, b1 = false;
		if (b2 && xCount[_w - 1 - t] > 0.6 * _h || xCount[_w - 1 - t] > 0.8 * _h)
			edg.col(_w - 1 - t) = 255, b2 = false;
		if (b3 && yCount[t] > 0.6 * _w || yCount[t] > 0.8 * _w)
			edg.row(t) = 255, b3 = false;
		if (b4 && yCount[_h - 1 - t] > 0.6 * _w || yCount[_h - 1 - t] > 0.8 * _w)
			edg.row(_h - 1 - t) = 255, b4 = false;
	}
	for (int r = 0; r < _h; r++)	{ // Remove single isolated points
		byte* edgP = edg.ptr<byte>(r);
		for (int c = 0; c < _w; c++)
			if (edgP[c] == 0){
				int count = 0;
				for (int t = 0; t < 8; t++){
					Point p = Point(c, r) + DIRECTION8[t];
					if (CHK_IND(p) && edg.at<byte>(p) == 0)
						count ++; 
				}
				if (count == 0)
					edgP[c] = 255;
			}
	}

	erode(edg, edg, Mat(), Point(-1, -1), 2);
	Mat_<int> idx1iT;
	int regNum = CmCv::GetRegions(edg, idx1iT, idxCount, vecB(), true);
	while(regNum > 1 && idxCount[regNum - 1] < 300)
		regNum--;
	Mat bdCMask = CmCv::GetBorderReg(idx1iT, regNum), ignoreMask;
	dilate(bdCMask, bdCMask, Mat(), Point(-1, -1), 3);
	erode(bdCMask, bdCMask, Mat(), Point(-1, -1), 2);
	compare(idx1iT, regNum, ignoreMask, CMP_GE);
	idx1iT.setTo(-1, ignoreMask);
	idx1i = idx1iT;
	idxCount.resize(regNum);
	return bdCMask;
}

Mat CmCv::GetBorderReg(CMat &idx1i, int regNum, double ratio, double thr)
{
	// Variance of x and y
	vecD vX(regNum), vY(regNum);
	int w = idx1i.cols, h = idx1i.rows;{
		vecD mX(regNum), mY(regNum), n(regNum); // Mean value of x and y, pixel number of region
		for (int y = 0; y < idx1i.rows; y++){
			const int *idx = idx1i.ptr<int>(y);
			for (int x = 0; x < idx1i.cols; x++, idx++)
				if (*idx >= 0 && *idx < regNum)
					mX[*idx] += x, mY[*idx] += y, n[*idx]++;
		}
		for (int i = 0; i < regNum; i++)
			mX[i] /= n[i], mY[i] /= n[i];
		for (int y = 0; y < idx1i.rows; y++){
			const int *idx = idx1i.ptr<int>(y);
			for (int x = 0; x < idx1i.cols; x++, idx++)
				if (*idx >= 0 && *idx < regNum)
					vX[*idx] += abs(x - mX[*idx]), vY[*idx] += abs(y - mY[*idx]);
		}
		for (int i = 0; i < regNum; i++)
			vX[i] = vX[i]/n[i] + EPS, vY[i] = vY[i]/n[i] + EPS;
	}

	// Number of border pixels in x and y border region
	vecI xbNum(regNum, 0), ybNum(regNum, 0); 
	int wGap = cvRound(w * ratio), hGap = cvRound(h * ratio);
	{ //vector<Point> bPnts; 
		ForPoints2(pnt, 0, 0, w, hGap) {// Top region
			int idx = idx1i.at<int>(pnt);
			if (idx >= 0 && idx < regNum)
				ybNum[idx]++; // bPnts.push_back(pnt);
		}
		ForPoints2(pnt, 0, h - hGap, w, h){ // Bottom region
			int idx = idx1i.at<int>(pnt);
			if (idx >= 0 && idx < regNum)
			ybNum[idx]++; // bPnts.push_back(pnt);
		}
		ForPoints2(pnt, 0, 0, wGap, h) {// Left region
			int idx = idx1i.at<int>(pnt);
			if (idx >= 0 && idx < regNum)
				xbNum[idx]++; // bPnts.push_back(pnt);
		}
		ForPoints2(pnt, w - wGap, 0, w, h){
			int idx = idx1i.at<int>(pnt);
			if (idx >= 0 && idx < regNum)
				xbNum[idx]++; // bPnts.push_back(pnt);
		}
	}

	Mat bReg1u = Mat::zeros(idx1i.size(), CV_8U);{  // likelihood map of border region
		double xR = 1.0/(4*wGap), yR = 1.0/(4*hGap);
		vector<byte> regL(regNum); // likelihood of each region belongs to border background
		for (int i = 0; i < regNum; i++) {
			double lk = xbNum[i] * xR / vY[i] + ybNum[i] * yR / vX[i];
			if (xbNum[i] < 40 * wGap && ybNum[i] < 40 * hGap)
				lk /= 2;
			regL[i] = lk/thr > 1 ? 255 : 0; //saturate_cast<byte>(255 * lk / thr);
		}

		for (int r = 0; r < h; r++)	{
			const int *idx = idx1i.ptr<int>(r);
			byte* maskData = bReg1u.ptr<byte>(r);
			for (int c = 0; c < w; c++, idx++)
				if (*idx >= 0 && *idx < regNum)
					maskData[c] = regL[*idx];
		}
	}
	return bReg1u;
}

void CmCv::fillPoly(Mat& img, const vector<PointSeti> _pnts, const Scalar& color, int lineType, int shift, Point offset)
{
	const int NUM((int)_pnts.size());
	const Point **pnts = new const Point *[NUM];
	int *num = new int[NUM];
	for (int i = 0; i < NUM; i++){
		pnts[i] = &_pnts[i][0];
		num[i] = (int)_pnts[i].size();
	}
	cv::fillPoly(img, pnts, num, NUM, color, lineType, shift, offset);

	delete []num;
	delete []pnts;
}
 
void CmCv::NormalizeImg(CStr &inDir, CStr &outDir, int minLen, bool subFolders)
{
	CmFile::MkDir(outDir);
	vecS names, subFold;
	int imgNum = CmFile::GetNames(inDir + "/*.jpg", names);
	for (int i = 0; i < imgNum; i++)	{
		Mat img = imread(inDir + '/' + names[i]), dstImg;
		if (img.data == NULL){
			printf("Can't load file %s\n", _S(names[i]));
			continue;
		}
		NormalizeImg(img, dstImg, minLen, 4);
		imwrite(outDir + "/" + names[i], dstImg);
	}
	if (imgNum)
		printf("\t%d images processed in folder %s/\n", imgNum, _S(inDir));

	if (subFolders){
		int subNum = CmFile::GetSubFolders(inDir, subFold);
		for (int i = 0; i < subNum; i++)
			NormalizeImg(inDir + '/' + subFold[i], outDir + '/' + subFold[i], minLen, subFolders);
	}
}


void CmCv::Demo(const char* fileName/* = "H:\\Resize\\cd3.avi"*/)
{

}

void CmCv::AddAlpha(CMat &fg3u, CMat &alpha1u, Mat &res3u)
{
	CV_Assert(fg3u.size == alpha1u.size && fg3u.type() == CV_8UC3 && alpha1u.type() == CV_8UC1);
	res3u.create(fg3u.size(), CV_8UC3);
#pragma omp parallel for
	for (int r = 0; r < fg3u.rows; r++){
		const Vec3b *imgD = fg3u.ptr<Vec3b>(r);
		const byte *alpD = alpha1u.ptr<byte>(r);
		Vec3b *resD = res3u.ptr<Vec3b>(r);
		for (int c = 0; c < fg3u.cols; c++)
			resD[c] = imgD[c] * (alpD[c] / 255.0);
	}
}

void CmCv::AddAlpha(CMat &bg3u, CMat &fg3u, CMat &alpha1u, Mat &res3u)
{
	CV_Assert(fg3u.size == alpha1u.size && bg3u.size == fg3u.size && bg3u.type() == CV_8UC3 && fg3u.type() == CV_8UC3 && alpha1u.type() == CV_8UC1);
	res3u.create(fg3u.size(), CV_8UC3);
#pragma omp parallel for
	for (int r = 0; r < fg3u.rows; r++){
		const Vec3b *fgD = fg3u.ptr<Vec3b>(r);
		const Vec3b *bgD = bg3u.ptr<Vec3b>(r);		
		const byte *alpD = alpha1u.ptr<byte>(r);
		Vec3b *resD = res3u.ptr<Vec3b>(r);
		for (int c = 0; c < fg3u.cols; c++){
			double alpha = alpD[c]/255.0;
			resD[c] = fgD[c] * alpha + bgD[c] * (1 - alpha);
		}
	}
}


// Average multi-channel float values within each region. 
// Region index should be int values in range [0, regNum -1]
void CmCv::avgPerRegion(CMat &regIdx1i, Mat &unaryNf, int regNum)
{
	int n = unaryNf.channels(), h = regIdx1i.rows, w = regIdx1i.cols;
	CV_Assert(regIdx1i.size == unaryNf.size && regIdx1i.type() == CV_32S && unaryNf.type() == CV_32FC(n));
	if (regIdx1i.isContinuous() && unaryNf.isContinuous())
		w *= regIdx1i.rows, h = 1;
	vector<vecF> valuesF(regNum);
	vector<vecD> values(regNum);
	vecI counts(regNum);
	for (int i = 0; i < regNum; i++)
		values[i].resize(n), valuesF[i].resize(n);
	for (int r = 0; r < h; r++){
		const int* idx = regIdx1i.ptr<int>(r);
		float *vals = unaryNf.ptr<float>(r);
		for (int c = 0; c < w; c++, vals+=n){
			counts[idx[c]]++;
			for (int i = 0; i < n; i++)
				values[idx[c]][i] += vals[i];
		}
	}

	for (int i = 0; i < regNum; i++)
		for (int j = 0; j < n; j++)
			valuesF[i][j] = (float)(values[i][j] / counts[i]);

	for (int r = 0; r < h; r++){
		const int* idx = regIdx1i.ptr<int>(r);
		float* vals = unaryNf.ptr<float>(r);
		for (int c = 0; c < w; c++, vals+=n)
			memcpy(vals, &valuesF[idx[c]][0], sizeof(float)*n);
	}
}


void CmCv::CannySimpleRGB(CMat &img3u, Mat &edge1u, double thresh1, double thresh2, int apertureSize, bool L2gradient)
{
	Mat bgr[3], edgTmp;
	split(img3u, bgr);
	Canny(bgr[0], edge1u, thresh1, thresh2, apertureSize, L2gradient);
	Canny(bgr[1], edgTmp, thresh1, thresh2, apertureSize, L2gradient);
	bitwise_or(edge1u, edgTmp, edge1u);
	Canny(bgr[2], edgTmp, thresh1, thresh2, apertureSize, L2gradient);
	bitwise_or(edge1u, edgTmp, edge1u);
}


void CmCv::rubustifyBorderMask(Mat& mask1u)
{
	Mat_<int> regIdx1i;
	int regNum = CmCv::GetRegions(mask1u, regIdx1i, vecI(), vecB(), true);
	mask1u = CmCv::GetBorderReg(regIdx1i, regNum, 0.02, 0.5);
}

int CmCv::intMatMax(CMat idx1i)
{
	int maxV = -INT_MAX;
	for (int r = 0; r < idx1i.rows; r++){
		const int *idx = idx1i.ptr<int>(r);
		for (int c = 0; c < idx1i.cols; c++)
			maxV = max(idx[c], maxV);
	}
	return maxV;
}

Mat CmCv::getGrabMask(CMat &img3u, Rect rect)
{		
	// Initialize flood fill
	queue<Point> selectedPnts;
	const int _h = img3u.rows, _w = img3u.cols, BW = 5;
	{// If not connected to image border, expand selection border unless stopped by edges
		Point rowT(rect.x, rect.y), rowB(rect.x, rect.y + rect.height - 1);
		Point colL(rect.x, rect.y), colR(rect.x + rect.width - 1, rect.y);
		if (rect.x >= BW) // Expand left edge
			for (int y = 0; y < rect.height; y++, colL.y++) selectedPnts.push(colL);
		else
			rect.x = BW;
		if (rect.y >= BW) // Expand top edge
			for (int x = 0; x < rect.width; x++, rowT.x++)	selectedPnts.push(rowT);
		else
			rect.y = BW;		
		if (rect.x + rect.width + BW <= _w) // Expand right edge	
			for (int y = 0; y < rect.height; y++, colR.y++) selectedPnts.push(colR);
		else
			rect.width = _w - rect.x - BW;
		if (rect.y + rect.height + BW <= _h) // Expand bottom edge
			for (int x = 0; x < rect.width; x++, rowB.x++) selectedPnts.push(rowB);
		else
			rect.height = _h - rect.y - BW;
	}

	Mat mask1u(img3u.size(), CV_8U);
	memset(mask1u.data, 255, mask1u.step.p[0]*mask1u.rows);
	mask1u(rect) = 0;

	Mat edge1u;
	CmCv::CannySimpleRGB(img3u, edge1u, 120, 1200, 5);
	dilate(edge1u, edge1u, Mat(), Point(-1, -1), 3);	
	//rectangle(edge1u, rect, Scalar(128));
	//imwrite(sameNameNE + "_Selection.png", edge1u);

	// Flood fill
	while (!selectedPnts.empty()){
		Point crntPnt = selectedPnts.front();
		mask1u.at<byte>(crntPnt) = 255;
		selectedPnts.pop();
		for (int i = 0; i < 4; i++){
			Point nbrPnt = crntPnt + DIRECTION4[i];
			if (CHK_IND(nbrPnt) && mask1u.at<byte>(nbrPnt) == 0 && edge1u.at<byte>(nbrPnt) == 0)
				mask1u.at<byte>(nbrPnt) = 255, selectedPnts.push(nbrPnt);
		}
	}
	rubustifyBorderMask(mask1u(Rect(rect.x+1, rect.y+1, rect.width-2, rect.height-2)));
	return mask1u;
}

