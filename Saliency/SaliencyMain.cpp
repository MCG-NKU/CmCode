#include "stdafx.h"

int main(int argc, char* argv[])
{	
	CStr wkDir = "D:/WkDir/Saliency/MSRA10K/";
	CStr inDir = wkDir + "Imgs/", outDir = wkDir + "Saliency/";
	CmFile::Copy2Dir(inDir + "*.jpg", outDir);

	CStr srcDir = "D:/Downloads/MSRA10K_Maps_Cuts/SaliencyCut/";
	vecS namesNE;
	int imgNum = CmFile::GetNamesNE(srcDir + "*_RCC.png", namesNE);
	for (int i = 0; i < imgNum; i++)
		CmFile::Copy(srcDir + namesNE[i] + ".png", outDir + namesNE[i] + "O.png");


	// Saliency detection method pretended in my ICCV 2013 paper http://mmcheng.net/effisalobj/.
	//CmSaliencyGC::Demo(inDir + "*.jpg", outDir); 

	// Saliency detection method presented in PAMI 2014 (CVPR 2011) paper http://mmcheng.net/salobj/.
	CmSalCut::Demo(inDir + "83262*.jpg", inDir + "*.png", outDir);

	//CmSaliencyRC::Get(inDir + "*.jpg", outDir);

	vecS des, desCut;
	des.push_back("RC");  des.push_back("GC");
	desCut.push_back("RCC"); desCut.push_back("RCCO");
	//CmEvaluation::Evaluate(inDir + "*.png", outDir, wkDir + "Results.m", des);
	CmEvaluation::EvalueMask(inDir + "*.png", outDir, desCut, wkDir + "CutRes.m");

	return 0;
}
