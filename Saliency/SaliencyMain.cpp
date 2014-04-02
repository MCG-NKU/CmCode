// SaliencyICCV13.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int main(int argc, char* argv[])
{
	CStr wkDir = "D:/WkDir/Saliency/FT1000/";

	// Saliency detection method pretended in my ICCV 2013 paper http://mmcheng.net/effisalobj/.
	return CmSaliencyGC::Demo(wkDir + "Imgs/*.jpg", wkDir + "/Saliency/"); 

	// Saliency detection method presented in CVPR11 paper and its journal version http://mmcheng.net/salobj/.
	//return CmSaliencyRC::Demo("D:/WkDir/Saliency/FT1000/");
}
