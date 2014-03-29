#include "stdafx.h"
#include "CmSaliencyGC.h"
#define _GET_CSD  // Only calculating Global uniqueness could be very fast, if commented, will also release dependence to CmAPCluster


CmSaliencyGC::CmSaliencyGC(CMat &img3f, CStr &outName)
	:_gmm(DEFAULT_GMM_NUM), _img3f(img3f), _nameNE(outName)
{
}

const int CmSaliencyGC::DEFAULT_GMM_NUM = 15; 

void CmSaliencyGC::HistgramGMMs()
{
	// Color quantization
	Mat _colorNums1f, _binBGR3f;
	CmColorQua::D_Quantize(_img3f, _HistBinIdx1i, _binBGR3f, _colorNums1f);
	//CmShow::HistBins(_binBGR3f, _colorNums1i, _nameNE + "_QT.jpg", true);
	_colorNums1f.convertTo(_colorNums1f, CV_32F);

	// Train GMMs
	Mat gmmIdx1i;
	_gmm.BuildGMMs(_binBGR3f, gmmIdx1i, _colorNums1f);
	_NUM = _gmm.RefineGMMs(_binBGR3f, gmmIdx1i, _colorNums1f);
	_PixelSalCi1f.resize(_NUM);
	_HistBinSalCi1f.resize(_NUM);
	_gmmClrs.resize(_NUM);
	_gmmW.resize(_NUM);
	_csd.resize(_NUM);
	_gu.resize(_NUM);
	_fSal.resize(_NUM);

	// Assign GMM color means and weights
	for (int i = 0; i < _NUM; i++)
		_gmmClrs[i] = _gmm.getMean(i), _gmmW[i] = _gmm.getWeight(i);

	// Assign GMMs for each components
	_gmm.GetProbs(_binBGR3f, _HistBinSalCi1f);
	for (int c = 0; c < _NUM; c++){
		_PixelSalCi1f[c].create(_img3f.size(), CV_32FC1);
		float *prob = (float*)(_HistBinSalCi1f[c].data);
		for (int y = 0; y < _HistBinIdx1i.rows; y++){
			const int *_idx = _HistBinIdx1i.ptr<int>(y);
			float* probCI = _PixelSalCi1f[c].ptr<float>(y);
			for (int x = 0; x < _HistBinIdx1i.cols; x++)
				probCI[x] = prob[_idx[x]];
		}
		blur(_PixelSalCi1f[c], _PixelSalCi1f[c], Size(3, 3));
	}
}


Mat CmSaliencyGC::GetSaliencyCues()
{
	vecD d;
#ifdef _GET_CSD
	GetCSD(_csd, d);
	Mat salMatCSD = GetSalFromGMMs(_csd);
#endif 
	
	GetGU(_gu, d, 0.4, 15);
	Mat salMatGU = GetSalFromGMMs(_gu);

#ifdef _GET_CSD
	return SpatialVar(salMatCSD) < SpatialVar(salMatGU) ? salMatCSD : salMatGU;
#else
	return salMatGU;
#endif
}

void CmSaliencyGC::MergeGMMs()
{
	Mat_<double> cor = Mat_<double>::zeros(_NUM, _NUM);
	for (int y = 0; y < _img3f.rows; y++) {
		for (int x = 0; x < _img3f.cols; x++){
			vector<CostfIdx> pIdx;
			pIdx.reserve(_NUM);
			for (int c = 0;c < _NUM; c++)
				pIdx.push_back(make_pair(_PixelSalCi1f[c].at<float>(y, x), c));
			sort(pIdx.begin(), pIdx.end(), std::greater<CostfIdx>());
			int i1 = pIdx[0].second, i2 = pIdx[1].second;
			float cost = min(pIdx[0].first, pIdx[1].first);
			cor(i1, i2) += cost;
			cor(i2, i1) += cost;
		}
	}

	double scale = _img3f.rows*_img3f.cols * 7 / 1e4; // Scale values for better illustration
	for (int i = 0; i < _NUM; i++)
		for (int j = 0; j < i; j++)
			cor(i, j) = cor(j, i) = log(1 + cor(i, j) / (scale * min(_gmmW[i], _gmmW[j]) + 1E-4)) - 1;
	//CmShow::Correlation(_gmmClrs, cor, _nameNE + "_MCOR.png", 20);
	double preference = 4*sum(cor).val[0]/(cor.cols*cor.rows);
	for (int i = 0; i < _NUM; i++)
		cor(i, i) = preference; // Preference for apcluster
	_ClusteredIdx.resize(_NUM);
	double netSim = 0;
	CmAPCluster apCluter;
	CmAPCluster::apcluster32 apFun = apCluter.GetApFun();
	unsigned int N = _NUM * _NUM;
	apFun(cor.ptr<double>(0), NULL, NULL, N, &_ClusteredIdx[0], &netSim, &apCluter.apoptions);
	_NUM_Merged = CmAPCluster::ReMapIdx(_ClusteredIdx);

	_pciM1f.resize(_NUM_Merged);
	for (int i = 0; i < _NUM_Merged; i++)
		_pciM1f[i] = Mat::zeros(_img3f.size(), CV_32FC1); 
	for (int i = 0; i < _NUM; i++)
		_pciM1f[_ClusteredIdx[i]] += _PixelSalCi1f[i];
}

// Return the center distance and spatial variance of X and Y direction. Pixel locations are normalized to [0, 1]
double CmSaliencyGC::SpatialVar(CMat& map1f, double &cD)
{
	// Find centroid of the map
	const int h = map1f.rows, w = map1f.cols;
	const double H = h, W = w;
	double xM = 0, yM = 0, sumP = EPS;
	for (int y = 0; y < h; y++)	{
		const float* prob = map1f.ptr<float>(y);
		for (int x = 0; x < w; x++)
			sumP += prob[x], xM += prob[x] * x, yM += prob[x] * y;
	}
	xM /= sumP, yM /= sumP;

	// Find variance of X and Y direction
	double vX = 0, vY = 0;
	cD = 0; // average center distance
	for (int y = 0; y < h; y++)	{
		const float* prob = map1f.ptr<float>(y);
		for (int x = 0; x < w; x++){
			double p = prob[x];
			vX += p * sqr(x - xM), vY += p * sqr(y - yM);
			cD += p * sqrt(sqr(x/W - 0.5) + sqr(y/H - 0.5));
		}
	}
	cD /= sumP;
	return vX/sumP + vY/sumP;
}

void CmSaliencyGC::GetCSD(vecD &_csd, vecD &_cD)
{
	MergeGMMs();
	vecD csd, cD;
	int num = (int)_pciM1f.size(); // Number of Gaussian
	Size imgSz = _pciM1f[0].size();
	double centerX = imgSz.width / 2, centerY = imgSz.height / 2;

	cD.resize(num);
	vecD V(num), D(num);
	for (int i = 0; i < num; i++)
		V[i] = SpatialVar(_pciM1f[i], cD[i]);
	normalize(V, V, 0, 1, CV_MINMAX);
	normalize(cD, D, 0, 1, CV_MINMAX);
	csd.resize(num);
	for (int i = 0; i < num; i++)
		csd[i] = (1-V[i])*(1-D[i]);
	normalize(csd, csd, 0, 1, CV_MINMAX);
	_csd.resize(_NUM);
	_cD.resize(_NUM);
	for (int i = 0; i < _NUM; i++)
		_csd[i] = csd[_ClusteredIdx[i]], _cD[i] = cD[_ClusteredIdx[i]];
}

void CmSaliencyGC::ViewValues(vecD &vals, CStr &ext)
{
	int K = _NUM;
	Mat_<Vec3f> color3f(1, K);
	Mat_<double> weight1d(1, K), cov1d(1, K);
	for (int i = 0; i < K; i++)	{
		color3f(0, i) = _gmm.getMean(i);
		weight1d(0, i) = vals[i];
		cov1d(0, i) = sqrt(_gmm.GetGaussians()[i].det);
	}
	//CmShow::HistBins(color3f, weight1d, _nameNE + ext, false, cov1d);
}

void CmSaliencyGC::GetGU(vecD& gc, vecD &d, double sigmaDist, double dominate)
{
	if (d.size() == 0){
		d.resize(_NUM);
		for (int i = 0; i < _NUM; i++)
			SpatialVar(_PixelSalCi1f[i], d[i]);
	}

	Mat_<double> clrDist;
	clrDist = Mat_<double>::zeros(_NUM, _NUM);
	vector<Vec3f> gmmClrs(_NUM);
	cvtColor(_gmmClrs, gmmClrs, CV_BGR2Lab);
	vecD maxDist(_NUM);
	for (int i = 0; i < _NUM; i++) for (int j = 0; j < i; j++){
		double dCrnt = vecDist(gmmClrs[i], gmmClrs[j]);
		clrDist(i, j) = clrDist(j, i) = dCrnt;
		maxDist[i] = max(maxDist[i], dCrnt);
		maxDist[j] = max(maxDist[j], dCrnt);
	}

	for (int i = 0; i < _NUM; i++) {
		gc[i] = 0;
		for (int j = 0; j < _NUM; j++)
			gc[i] += _gmmW[j] * clrDist(i, j) / maxDist[i]; //min(clrDist(i, j), dominate);
	}

	for (int i = 0; i < _NUM; i++)
		_gu[i] = _gu[i] * exp(-9.0 * sqr(d[i])); //
	normalize(_gu, _gu, 0, 1, CV_MINMAX);
}

void CmSaliencyGC::ReScale(vecD& salVals, double minRatio)
{
	while (1){
		double sumW = 0, maxCrnt = 0;
		for (int i = 0; i < _NUM; i++)
			if (salVals[i] > 0.95)
				sumW += _gmmW[i];
			else
				maxCrnt = max(maxCrnt, salVals[i]);
		if (sumW < minRatio && maxCrnt > EPS)
			for (int i = 0; i < _NUM; i++)
				salVals[i] = min(salVals[i]/maxCrnt, 1.0);
		else
			break;
	}
}

Mat CmSaliencyGC::GetSalFromGMMs(vecD &val, bool bNormlize)
{
	ReScale(val);
	Mat binSal = Mat::zeros(_HistBinSalCi1f[0].size(), CV_32F);
	int num = (int)_HistBinSalCi1f.size(), type = _HistBinSalCi1f[0].type();
	for (int c = 0; c < num; c++){
		Mat tmp;
		_HistBinSalCi1f[c].convertTo(tmp, type, val[c]);
		add(binSal, tmp, binSal);
	}
	normalize(binSal, binSal, -4, 4, NORM_MINMAX);
	float *bVal = binSal.ptr<float>(0);
	for(int i = 0; i < binSal.cols; i++)
		bVal[i] = 1 / (1 + exp(-bVal[i])); // Sigmoid
	if (bNormlize)
		normalize(binSal, binSal, 0, 1, NORM_MINMAX);
	Mat salMat(_HistBinIdx1i.size(), CV_32F);
	float* binSalVal = (float*)(binSal.data);
	for (int r = 0; r < _HistBinIdx1i.rows; r++){
		const int* idx = _HistBinIdx1i.ptr<int>(r);
		float* sal = salMat.ptr<float>(r);
		for (int c = 0; c < _HistBinIdx1i.cols; c++)
			sal[c] = binSalVal[idx[c]];
	}
	blur(salMat, salMat, Size(3, 3));
	normalize(salMat, salMat, 0, 1, NORM_MINMAX);
	return salMat;
}

// See my CVPR 11 paper http://mmcheng.net/SalObj/ for source code used to generate
// nice statistical plots, many comparison figures and latex (see supplemental material).
int CmSaliencyGC::Demo(CStr imgDir, CStr salDir)
{
	CmFile::MkDir(salDir);
	vecS namesNE, des;
	int imgNum = CmFile::GetNamesNE(imgDir + "*.jpg", namesNE);
	CmTimer tm("Maps");
	tm.Start();
	printf("%d images found in %s\n", imgNum, _S(imgDir + "*.jpg"));

#pragma omp parallel for
	for (int i = 0; i < imgNum; i++){
		Mat img = imread(imgDir + namesNE[i] + ".jpg");
		CStr outName = salDir + namesNE[i];
		img.convertTo(img, CV_32FC3, 1.0/255);
		CmSaliencyGC sGC(img, outName);
		sGC.HistgramGMMs();
		Mat sal1f = sGC.GetSaliencyCues();
		imwrite(outName + "_GC.png", sal1f*255);
	}
	tm.Stop();
	printf("Speed: %g seconds = %g fps\n", tm.TimeInSeconds()/imgNum, imgNum/tm.TimeInSeconds());

	des.push_back("_GC");
	CmEvaluation::MeanAbsoluteError(imgDir, salDir, des);
	//CmSaliency::Evaluate(inDir + "*.png", outDir, wkDir + "Eval.m", des);
	return 0;
}
