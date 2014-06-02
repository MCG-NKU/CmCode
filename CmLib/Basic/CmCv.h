#pragma once

enum{CV_FLIP_BOTH = -1, CV_FLIP_VERTICAL = 0, CV_FLIP_HORIZONTAL = 1};

struct CmCv {
	// AbsAngle: Calculate magnitude and angle of vectors.
	static void AbsAngle(CMat& cmplx32FC2, Mat& mag32FC1, Mat& ang32FC1);

	// GetCmplx: Get a complex value image from it's magnitude and angle.
	static void GetCmplx(CMat& mag32F, CMat& ang32F, Mat& cmplx32FC2);

	// Mat2GrayLog: Convert and arbitrary mat to [0, 1] for display.
	// The result image is in 32FCn format and range [0, 1.0].
	// Mat2GrayLinear(log(img+1), newImg). In place operation is supported.
	static void Mat2GrayLog(CMat& img, Mat& newImg);

	// Low frequency part is always been move to the central part:
	//				 -------                          -------	
	//				| 1 | 2 |                        | 3 | 4 |	
	//				 -------            -->           -------	
	//				| 4 | 3 |                        | 2 | 1 |	
	//				 -------                          -------	
	static void FFTShift(Mat& img);

	// Swap the content of two Mat with same type and size
	static inline void Swap(Mat& a, Mat& b);

	// Normalize size/image to min(width, height) = shortLen and use width 
	// and height to be multiples of unitLen while keeping its aspect ratio 
	// as much as possible. unitLen must not be 0.
	static inline Size NormalizeSize(const Size& sz, int shortLen, int unitLen = 1);
	static inline void NormalizeImg(CMat&img, Mat& dstImg, int shortLen = 256, int unitLen = 8);
	static void NormalizeImg(CStr &inDir, CStr &outDir, int minLen = 300, bool subFolders = true);

	// Get image region by two corner point.
	static inline Rect GetImgRange(Point p1, Point p2, Size imgSz);

	// Check an image (with size imgSz) point and correct it if necessary
	static inline void CheckPoint(Point &p, Size imgSz);

	static inline Mat Merge(CMat &m3c, CMat &m1c); // Merge a 3 channel and 1 channel mat to 4 channel one
	static inline void Split(CMat &m4c, Mat &m3c, Mat &m1c);
	
	// Get mask region. 
	static Rect GetMaskRange(CMat &mask1u, int ext = 0, int thresh = 10);
	
	// Get continuous components for same label regions. Return region index mat,
	// index-counter pair (Number of pixels for each index), and label of each idx
	static int GetRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxCount, vecB &idxLabel, bool noZero = false);
	static int GetRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxCount, bool noZero = false) {vecB idxLabel; return GetRegions(label1u, regIdx1i, idxCount, idxLabel, noZero);}

	// Get continuous components for non-zero labels. Return region index mat (region index 
	// of each mat position) and sum of label values in each region
	static int GetNZRegions(const Mat_<byte> &label1u, Mat_<int> &regIdx1i, vecI &idxSum);

	// Get continuous None-Zero label Region with Largest Sum value
	static Mat GetNZRegionsLS(CMat &mask1u, double ignoreRatio = 0.02);
	
	// Get points in border regions
	static int GetBorderPnts(Size sz, double ratio, vector<Point> &bdPnts);

	// Get border regions, which typically corresponds to background region
	static Mat GetBorderReg(CMat &idx1i, int regNum, double ratio = 0.02, double thr = 0.4);  
	static Mat GetBorderRegC(CMat &img3u, Mat &idx1i, vecI &idxCount);

	static void fillPoly(Mat& img, const vector<PointSeti> _pnts, const Scalar& color, int lineType = 8, int shift = 0, Point offset = Point());

	// down sample without convolution, similar to cv::pyrDown
	template<class T> static void PyrDownSample(CMat &src, Mat &dst);
	template<class T> static void PyrUpSample(CMat &src, Mat &dst, Size dSz);

	static void _inline SaveImgRGB(CStr &fName, CMat &img);// Saving RGB image for QImage data

	static void Demo(const char* fileName = "H:\\Resize\\cd3.avi");

	//// Adding alpha value to img to show. img: 8U3C, alpha 8U1C
	static void AddAlpha(CMat &fg3u, CMat &alpha1u, Mat &res3u);
	static void AddAlpha(CMat &bg3u, CMat &fg3u, CMat &alpha1u, Mat &res3u);
	//static void AddAlpha(CvMat *img, CvMat *alpha, CvScalar bgColor);

	static inline Mat getContinouse(CMat &mat) {Mat tmp; mat.copyTo(tmp); return tmp; }


	// Average multi-channel float values within each region. 
	// Region index should be int values in range [0, regNum -1]
	static void avgPerRegion(CMat &regIdx1i, Mat &unaryNf, int regNum);

	template <typename T> static Mat addChannel(CMat &mat, int num = 1, double defaultVal = 0);

	static void CannySimpleRGB(CMat &img3u, Mat &edge1u, double thresh1, double thresh2, int apertureSize, bool L2gradient = false);
	static Mat getGrabMask(CMat &img3u, Rect rect);//, CStr sameNameNE, int ext = 4
	static void rubustifyBorderMask(Mat& mask1u); 
};

// Normalize size/image to min(width, height) = shortLen and use width 
// and height to be multiples of unitLen while keeping its aspect ratio 
// as much as possible. unitLen must not be 0.

Size CmCv::NormalizeSize(const Size& sz, int shortLen, int unitLen)
{
	double ratio = double(shortLen) / min(sz.width, sz.height);
	return Size(cvRound(sz.width * ratio / unitLen) * unitLen, cvRound(sz.height * ratio /unitLen) * unitLen);
}

void CmCv::NormalizeImg(CMat&img, Mat& dstImg, int shortLen, int unitLen)
{
	resize(img, dstImg, NormalizeSize(img.size(), shortLen, unitLen));
}

void CmCv::CheckPoint(Point &p, Size imgSz)
{
	p.x = max(0, p.x), p.y = max(0, p.y);
	p.x = min(imgSz.width - 1, p.x);
	p.y = min(imgSz.height - 1, p.y);
}

Rect CmCv::GetImgRange(Point p1, Point p2, Size imgSz)
{
	CheckPoint(p1, imgSz);
	CheckPoint(p2, imgSz); 
	return Rect(min(p1.x, p2.x), min(p1.y, p2.y), abs(p1.x - p2.x), abs(p1.y - p2.y));
}

Mat CmCv::Merge(CMat &m3c, CMat &m1c)
{
	Mat m4c;
	vecM chs;
	cv::split(m3c, chs);
	chs.push_back(m1c);
	cv::merge(chs, m4c);
	return m4c;
}

void CmCv::Split(CMat &m4c, Mat &m3c, Mat &m1c)
{
	vecM chs;
	cv::split(m4c, chs);
	m1c = chs[3];
	chs.resize(3);
	cv::merge(chs, m3c);
}

template<class T> void CmCv::PyrDownSample(CMat &src, Mat &dst)
{
	dst.create((src.rows+1)/2, (src.cols+1)/2, src.type());
	for (int r = 0; r < dst.rows; r++)	{
		const T *sP = src.ptr<T>(r * 2);
		T *dP = dst.ptr<T>(r);
		for (int c = 0; c < dst.cols; c++)
			dP[c] = sP[c*2];
	}
}

template<class T> void CmCv::PyrUpSample(CMat &src, Mat &dst, Size sz)
{
	dst.create(sz, src.type());
	for (int r = 0; r < dst.rows; r++)	{
		const T *sP = src.ptr<T>(r/2);
		T *dP = dst.ptr<T>(r);
		for (int c = 0; c < dst.cols; c++)
			dP[c] = sP[c/2];
	}
}

void CmCv::SaveImgRGB(CStr &fName, CMat &img)
{
	Mat saveImg;
	cvtColor(img, saveImg, CV_RGB2BGR);
	imwrite(fName, saveImg);
}

template <typename T> Mat CmCv::addChannel(CMat &mat, int num, double defaultVal)
{
	const int inCh = mat.channels(), outCh = mat.channels() + num;
	Mat resM(mat.size(), CV_MAKETYPE(mat.depth(), outCh));
	for (int r = 0; r < mat.rows; r++){
		const T* inVal = mat.ptr<T>(r);
		T* outVal = resM.ptr<T>(r);
		for (int c = 0; c < mat.cols; c++, inVal += inCh, outVal += outCh){
			memcpy(outVal, inVal, sizeof(T)*inCh);
			for (int i = inCh; i < outCh; i++)
				outVal[i] = (T)defaultVal;
		}
	}
	return resM;
}
