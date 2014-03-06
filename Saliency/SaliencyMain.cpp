// SaliencyICCV13.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include "CmSaliencyGC.h"

int main(int argc, char* argv[])
{
	// Saliency detection method pretended in my ICCV 2013 paper http://mmcheng.net/effisalobj/.
	//return CmSaliencyGC::Demo("D:/WkDir/Saliency/FT1000/"); // (un)comment `#define _GET_CSD' in "CmSaliencyGC.cpp":Line 3 for trade off between speed and accuracy

	// Saliency detection method presented in CVPR11 paper and its journal version http://mmcheng.net/salobj/.
	return CmSaliencyRC::Demo("D:/WkDir/Saliency/FT1000/");
}
