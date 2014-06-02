#pragma once

/************************************************************************/
/* For educational and research use only; commercial use are forbidden.	*/
/* Download more source code from: http://mmcheng.net/					*/
/* If you use any part of the source code, please cite related papers:	*/
/* [1] SalientShape: Group Saliency in Image Collections. M.M. Cheng,	*/
/*	 N.J. Mitra, X. Huang, S.M. Hu. The Visual Computer, 2013.			*/
/* [2] Salient Object Detection and Segmentation. M.M. Cheng, N.J.		*/
/*   Mitra, X. Huang, P.H.S. Torr, S.M. Hu. Submitted to IEEE TPAMI		*/
/*	 (TPAMI-2011-10-0753), 2011.										*/
/* [3] Global Contrast based Salient Region Detection, Cheng et. al.,	*/
/*	   CVPR 2011.														*/
/************************************************************************/

struct CmColorQua
{
	enum {S_BGR, S_HSV, S_LAB, D_BGR};
	typedef int (*S_QUANTIZE_FUNC)(const Vec3f &c);  
	typedef void (*S_RECOVER_FUNC)(int idx, Vec3f &c);
	static const int S_Q_NUM = 3, Q_NUM = 4;
	static const char* descr[Q_NUM];
	static const int binNum[Q_NUM];
	static const S_QUANTIZE_FUNC sqFuns[S_Q_NUM];
	static const S_RECOVER_FUNC srFuns[S_Q_NUM];

	static void TestColorQuantize(CStr &inImgs, CStr &outDir);

	// Static quantization and recover without prior color statistics, img3f: BGR image
	static void S_Quantize(CMat& img3f, Mat &idx1i, int method = S_BGR);
	static void S_Recover(CMat& idx1i, Mat& img3f, int method = 1, CMat &src3f = Mat()); // img3f and src3f are BGR.

	static int D_Quantize(CMat& img3f, Mat &idx1i, Mat &_color3f, Mat &_colorNum1i, double ratio = 0.95, const int colorNums[3] = DefaultNums);
	//static int D_QuantizeSet(int colorNums[3]);
	static void D_Recover(CMat& idx1i, Mat &img3f, CMat &color3f);

	// src3f are BGR, color3f are 1xBinDim matrix represent color fore each histogram bin
	static int S_BinInf(CMat& idx1i, Mat &color3f, vecI &colorNum, int method = 1, CMat &src3f = Mat()); 

	static void Show(CMat &counts1d, CStr title, Mat &show3f, int method = 1);
	static void Show(CMat &counts1d, CStr title, int method = 1){Mat show3f; Show(counts1d, title, show3f, method);}

	// Static quantization and recover without prior color statistics
	static int SQ_BGR(const Vec3f &c);		// B,G,R[0,1] --> [0, 215]
	static void SR_BGR(int idx, Vec3f &c);	// [0,215] --> B,G,R[0,1]
	static int SQ_HSV(const Vec3f &c);		// H[0,360], S,V[0,1] ---> [0, 255]
	static void SR_HSV(int idx, Vec3f &c); // [0,255] ---> H[0,360], S,V[0,1]
	static int SQ_Lab(const Vec3f &c);		// L[0, 100], a, b [-127, 127] ---> [0, 244]
	static void SR_Lab(int idx, Vec3f &c); // [0, 244] ---> L[0, 100], a, b [-127, 127]

private:
	static const int DefaultNums[3];
	//static int clrNums[3];
	//static float clrTmp[3];
	//static int w[3];
};

