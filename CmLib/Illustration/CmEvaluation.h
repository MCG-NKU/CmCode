#pragma once

/************************************************************************/
/* For educational and research use only; commercial use are forbidden.	*/
/* Download more source code from: http://mmcheng.net/					*/
/* If you use any part of the source code, please cite related papers:	*/
/* [1] SalientShape: Group Saliency in Image Collections. M.M. Cheng,	*/
/*	 N.J. Mitra, X. Huang, S.M. Hu. The Visual Computer, 2013.			*/
/* [2] Efficient Salient Region Detection with Soft Image Abstraction.	*/
/*	 M.M. Cheng, J. Warrell, W.Y. Lin, S. Zheng, V. Vineet, N. Crook.	*/
/*	 IEEE ICCV, 2013.													*/
/* [3] Salient Object Detection and Segmentation. M.M. Cheng, N.J.		*/
/*   Mitra, X. Huang, P.H.S. Torr, S.M. Hu. Submitted to IEEE TPAMI		*/
/*	 (TPAMI-2011-10-0753), 2011.										*/
/* [4] Global Contrast based Salient Region Detection, Cheng et. al.,	*/
/*	   CVPR 2011.														*/
/************************************************************************/

struct CmEvaluation
{
	// Save the precision recall curve, ROC curve, AUC, MaxFMeasure, MeanFMeasure and MAE to a Matlab file for direct ploting
	static void Evaluate(CStr gtW, CStr &salDir, CStr &resName, vecS &des); 
	static void Evaluate(CStr gtW, CStr &salDir, CStr &resName, CStr &des) {vecS descri(1); descri[0] = des; Evaluate(gtW, salDir, resName, descri);} 

	// Plot the FMeasure bar
	static void EvalueMask(CStr gtW, CStr &maskDir, vecS &des, CStr resFile, double betaSqr = 0.3, bool alertNul = false, CStr suffix = "", CStr title = "");
	static void EvalueMask(CStr gtW, CStr &maskDir, CStr &des, CStr resFile, double betaSqr = 0.3, bool alertNul = false, CStr suffix = "");

public: // Assistant functions

	static void MeanAbsoluteError(CStr inDir, CStr salDir, vecS des, bool zeroMapIfMissing = false);
	
	static double FMeasure(CMat &mask1u, CMat &gtMask1u); // The two mask should contain values of either 0 or 255.
	// Format change from OpenCV region (x, y, width, height) to VOC bounding box (minx, minY, maxX, maxY)
	static inline Vec4i reg2Box(const Rect &reg) {return Vec4i(reg.x, reg.y, reg.x + reg.width - 1, reg.y + reg.height - 1);}

	static double interUnionBBox(const Vec4i &box1, const Vec4i &box2); // each box minx, minY, maxX, maxY
	static double interUnionBBox(const Rect &reg1, const Rect &reg2) {return interUnionBBox(reg2Box(reg1), reg2Box(reg2));}
	
	static int STEP; // Evaluation threshold density
	static void PrintVector(FILE *f, const vecD &v, CStr &name);

protected:
	static const int COLOR_NUM = 255;  
	static const int NUM_THRESHOLD;  // Number of difference threshold

	// Return mean absolute error (MAE)
	static double Evaluate_(CStr &gtImgW, CStr &inDir, CStr& resExt, vecD &precision, vecD &recall, vecD &tpr, vecD &fpr);
};

