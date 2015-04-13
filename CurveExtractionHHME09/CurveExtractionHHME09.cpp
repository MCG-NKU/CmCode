// CurveExtractionHHME09.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

/************************************************************************/
/*  This software is developed by Ming-Ming Cheng.				        */
/*       Url: http://mmcheng.net/						                */
/*  This software is free fro non-commercial use. In order to use this	*/
/*  software for academic use, you must cite the corresponding paper:	*/
/*      Ming-Ming Cheng, Curve Structure Extraction for Cartoon Images, */
/*		The 5th Joint Conference on Harmonious Human Machine Environment*/
/*		(HHME), 13-20.													*/
/************************************************************************/

int main(int argc, char* argv[])
{
	Mat cImg = imread(".\\data\\Test.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	Mat nImg = imread(".\\data\\Nature.png", CV_LOAD_IMAGE_GRAYSCALE);
	CV_Assert(cImg.data != NULL && nImg.data != NULL);

	CmCurveEx::Demo(cImg, true); 
	//CmCurveEx::Demo(nImg, false); 
	return 0;
}
