#include "stdafx.h"

int main(int argc, char* argv[])
{	
	CStr wkDir = "D:/WkDir/Saliency/MSRA10K/";
	CStr inDir = wkDir + "Imgs/", outDir = wkDir + "Saliency/";
	CmFile::Copy2Dir(inDir + "*.jpg", outDir);
	
	// Saliency detection method pretended in my ICCV 2013 paper http://mmcheng.net/effisalobj/.
	//CmSaliencyGC::Demo(inDir + "*.jpg", outDir); 

	// Saliency detection method presented in PAMI 2014 (CVPR 2011) paper http://mmcheng.net/salobj/.
	//CmSalCut::Demo(inDir + "*.jpg", inDir + "*.png", outDir); //CmSaliencyRC::Get(inDir + "*.jpg", outDir);	

	vecS des;
	des.push_back("GC");  des.push_back("RC");
	//CmEvaluation::Evaluate(inDir + "*.png", outDir, wkDir + "Results.m", des);
	CmEvaluation::EvalueMask(inDir + "*.png", outDir, "RCC", wkDir + "CutRes.m");

	return 0;
}
