#pragma once

struct CmSaliencyRC
{
	typedef Mat (*GET_SAL_FUNC)(CMat &);

	// Get saliency values of a group of images.
	// Input image names and directory name for saving saliency maps.
	static void Get(CStr &imgNameW, CStr &salDir);

	// Evaluate saliency detection methods. Input ground truth file names and saliency map directory
	static void Evaluate(CStr gtW, CStr &salDir, CStr &resName);

	// Frequency Tuned [1].
	static Mat GetFT(CMat &img3f);

	// Histogram Contrast of [3]
	static Mat GetHC(CMat &img3f);

	// Region Contrast 
	static Mat GetRC(CMat &img3f);
	static Mat GetRC(CMat &img3f, CMat &idx1i, int regNum, double sigmaDist = 0.4);
	static Mat GetRC(CMat &img3f, double sigmaDist, double segK, int segMinSize, double segSigma);

	// Luminance Contrast [2]
	static Mat GetLC(CMat &img3f);

	// Spectral Residual [4]
	static Mat GetSR(CMat &img3f);


	static Mat Get(CMat &img3f, GET_SAL_FUNC fun, int wkSize);
	static void SmoothByHist(CMat &img3f, Mat &sal1f, float delta);
	static void SmoothByRegion(Mat &sal1f, CMat &idx1i, int regNum, bool bNormalize = true);
	static void SmoothByGMMs(CMat &img3f, Mat &sal1f, int fNum = 5, int bNum = 5, int wkSize = 0);

	static int Demo(CStr wkDir);

private:
	static const int SAL_TYPE_NUM = 5; 
	static const char* SAL_TYPE_DES[SAL_TYPE_NUM];
	static const GET_SAL_FUNC gFuns[SAL_TYPE_NUM];

	// Histogram based Contrast
	static void GetHC(CMat &binColor3f, CMat &colorNums1i, Mat &colorSaliency);

	static void SmoothSaliency(Mat &sal1f, float delta, const vector<vector<CostfIdx>> &similar);
	static void SmoothSaliency(CMat &colorNum1i, Mat &sal1f, float delta, const vector<vector<CostfIdx>> &similar);

	struct Region{
		Region() { pixNum = 0; ad2c = Point2d(0, 0);}
		int pixNum;  // Number of pixels
		vector<CostfIdx> freIdx;  // Frequency of each color and its index
		Point2d centroid;
		Point2d ad2c; // Average distance to image center
	};
	static void BuildRegions(CMat& regIdx1i, vector<Region> &regs, CMat &colorIdx1i, int colorNum);
	static void RegionContrast(const vector<Region> &regs, CMat &color3fv, Mat& regSal1d, double sigmaDist);

	static int Quantize(CMat& img3f, Mat &idx1i, Mat &_color3f, Mat &_colorNum, double ratio = 0.95, const int colorNums[3] = DefaultNums);
	static const int DefaultNums[3];

	// Get border regions, which typically corresponds to background region
	static Mat GetBorderReg(CMat &idx1i, int regNum, double ratio = 0.02, double thr = 0.3);  

	// AbsAngle: Calculate magnitude and angle of vectors.
	static void AbsAngle(CMat& cmplx32FC2, Mat& mag32FC1, Mat& ang32FC1);

	// GetCmplx: Get a complex value image from it's magnitude and angle.
	static void GetCmplx(CMat& mag32F, CMat& ang32F, Mat& cmplx32FC2);
};

/************************************************************************/
/*[1]R. Achanta, S. Hemami, F. Estrada and S. Susstrunk, Frequency-tuned*/
/*   Salient Region Detection, IEEE CVPR, 2009.							*/
/*[2]Y. Zhai and M. Shah. Visual attention detection in video sequences */
/*   using spatiotemporal cues. In ACM Multimedia 2006.					*/
/*[3]M.-M. Cheng, N. J. Mitra, X. Huang, P.H.S. Torr S.-M. Hu. Global	*/
/*   Contrast based Salient Region Detection. IEEE PAMI, 2014.			*/
/*[4]X. Hou and L. Zhang. Saliency detection: A spectral residual		*/
/*   approach. In IEEE CVPR 2007, 2007.									*/
/************************************************************************/
