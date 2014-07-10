// SaliencyICCV13.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int main(int argc, char* argv[])
{	
	CStr wkDir = "D:/Saliency/MSRA10K/";

	// Saliency detection method pretended in my ICCV 2013 paper http://mmcheng.net/effisalobj/.
	return CmSaliencyGC::Demo(wkDir + "Imgs/*.jpg", wkDir + "/Saliency/"); 

	// Saliency detection method presented in CVPR11 paper and its journal version http://mmcheng.net/salobj/.
	return CmSaliencyRC::Demo("D:/WkDir/Saliency/FT1000/");

	vecS des;
	des.push_back("_RC");
	des.push_back("_HC");
	CmEvaluation::Evaluate(wkDir + "Imgs/*.png", wkDir + "Saliency/", wkDir + "Results.m", des);
}
