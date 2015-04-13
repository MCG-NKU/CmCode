#include "stdafx.h"
#include "CmGMM.h"


void CmGMM::Demo(CStr &wkDir)
{
	CStr inDir = wkDir + "In/", outDir = wkDir + "Out/";
	vecS namesNE;
	int imgNum = CmFile::GetNamesNE(inDir + "*.jpg", namesNE);
	CmFile::MkDir(outDir);

	CmTimer tm("Timer32");
//#pragma omp parallel for
	for (int i = 0; i < imgNum; i++){
		Mat idx1i, img = imread(inDir + namesNE[i] + ".jpg");
		CV_Assert(img.data != NULL);
		tm.Start();
		CmGMM gmm(8);
		img.convertTo(img, CV_32F, 1/255.0);
		gmm.BuildGMMs(img, idx1i);
		gmm.RefineGMMs(img, idx1i);

		gmm.Show(idx1i, outDir + namesNE[i] + "_C.jpg");
		gmm.View(outDir + namesNE[i] + "_GMM.jpg");

		//* Write components
		vecM probs;
		gmm.GetProbs(img, probs);
		CmFile::Copy(inDir + namesNE[i] + ".jpg", outDir + namesNE[i] + ".jpg");
		for (int k = 0; k < gmm.K(); k++)
			imwrite(outDir + namesNE[i] + format("_C%d.png", k), probs[k]*255); //*/


		tm.Stop();
	}
	tm.Report();
}


void CmGMM::View(CStr &title, bool decreaseShow)
{
	Mat_<Vec3f> color3f(1, _K);
	Mat_<double> weight1d(1, _K), cov1d(1, _K);
	for (int i = 0; i < _K; i++)	{
		color3f(0, i) = getMean(i);
		weight1d(0, i) = getWeight(i);
		cov1d(0, i) = sqrt(this->_Guassians[i].det);
	}
	CmShow::HistBins(color3f, weight1d, title, decreaseShow, cov1d);
}


double CmGMM::ViewFrgBkgProb(const CmGMM &fGMM, const CmGMM &bGMM, CStr &title)
{
	double Pf = fGMM.GetSumWeight(), Pb = bGMM.GetSumWeight(), sumN = Pf + Pb;
	Pf /= sumN, Pb /= sumN;
	printf("#ForeSample = %.2g%%, ", Pf);	
	int n = 6 * 6 * 6;
	Mat color3f(1, n, CV_64FC3);
	Vec3f* colors = (Vec3f*)(color3f.data);
	for (int i = 0; i < n; i++){
		colors[i][0] = ((float)(i / 36) + 0.5f)/6.f;
		int idx = i % 36;
		colors[i][1] = ((float)(idx / 6) + 0.5f)/6.f;
		colors[i][2] = ((float)(idx % 6) + 0.5f)/6.f;
	}

	Mat fCount1d(1, n, CV_64F), bCount1d(1, n, CV_64F), rCount1d;
	double *fCount = (double*)(fCount1d.data), *bCount = (double*)(bCount1d.data);
	for (int i = 0; i < n; i++)
		fCount[i] = fGMM.P(colors[i]), bCount[i] = bGMM.P(colors[i]);
	double fMax, fMin, bMax, bMin;
	minMaxLoc(fCount1d, &fMin, &fMax);
	minMaxLoc(bCount1d, &bMin, &bMax);
	printf("minF = %.2g, maxF = %.2g, minB = %.2g maxB = %.2g\n", fMin, fMax, bMin, bMax);
	double epsilon = min(fMax, bMax) * 1e-4;
	Mat res = (fCount1d*Pf + 0.5*epsilon)/(fCount1d*Pf + bCount1d*Pb + epsilon) - 0.5;
	CmShow::HistBins(color3f, res, title, true);
	return epsilon;
}

void CmGMM::GetGMMs(CStr &smplW, CStr &annoExt, CmGMM &fGMM, CmGMM &bGMM)
{
	vector<Vec3b> frg3b, bkg3b;
	frg3b.reserve(10000000);
	bkg3b.reserve(10000000);
	vecS namesNE;
	string dir, ext;
	int num = CmFile::GetNamesNE(smplW, namesNE, dir, ext);
	int foreBinN = 0, backBinN = 0;
	for (int i = 0; i < num; i++)	{
		Mat mask1u = imread(dir + namesNE[i] + annoExt, CV_LOAD_IMAGE_GRAYSCALE);
		Mat img = imread(dir + namesNE[i] + ext);
		CV_Assert(mask1u.data != NULL && img.data != NULL);
		for (int r = 0; r < mask1u.rows; r++)	{
			byte *mVal = mask1u.ptr<byte>(r);
			Vec3b *imgVal = img.ptr<Vec3b>(r);
			for (int c = 0; c < mask1u.cols; c++)
				if (mVal[c] > 200)
					frg3b.push_back(imgVal[c]);
				else if (mVal[c] < 100)
					bkg3b.push_back(imgVal[c]);
		}
	}

	Mat fore3f, bg3f, mat3b = Mat(1, (int)frg3b.size(), CV_8UC3, &frg3b[0]);
	mat3b.convertTo(fore3f, CV_64FC3, 1.0/255);
	frg3b.clear();
	mat3b = Mat(1, (int)bkg3b.size(), CV_8UC3, &bkg3b[0]);
	mat3b.convertTo(bg3f, CV_64FC3, 1.0/255);
	bkg3b.clear();

	Mat fComp1i, bComp1i; // foreground and background components
	fGMM.BuildGMMs(fore3f, fComp1i);
	fGMM.RefineGMMs(fore3f, fComp1i);
	fore3f.release();
	fComp1i.release();
	bGMM.BuildGMMs(bg3f, bComp1i);
	bGMM.RefineGMMs(bg3f, bComp1i);
}


// show GMM images
void CmGMM::Show(CMat& components1i, CStr& title)
{
	Mat pShow(components1i.size(), CV_32FC3);
	for (int y = 0; y < components1i.rows; y++)	{
		float* show = pShow.ptr<float>(y);
		const int* comp = components1i.ptr<int>(y);
		for (int x = 0; x < components1i.cols; x++) {
			CV_Assert(comp[x] >= 0 && comp[x] < _K);
			show[0] = (float)_Guassians[comp[x]].mean[0];
			show[1] = (float)_Guassians[comp[x]].mean[1];
			show[2] = (float)_Guassians[comp[x]].mean[2];
			show += 3;
		}
	}
	CmShow::SaveShow(pShow, title);
}


void CmGMM::reWeights(vecD &mulWs)
{
	double sumW = 0;
	vecD newW(_K);
	for (int i = 0; i < _K; i++){
		newW[i] = _Guassians[i].w * mulWs[i];
		sumW += newW[i];
	}
	for (int i = 0; i < _K; i++)
		_Guassians[i].w = newW[i] / sumW;

}