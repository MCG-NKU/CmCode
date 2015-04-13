#pragma once
/************************************************************************/
/* For educational and research use only; commercial use are forbidden.	*/
/* Download more source code from: http://mmcheng.net/					*/
/* If you use any part of the source code, please cite related papers:	*/
/* 1. SalientShape: Group Saliency in Image Collections. M.M. Cheng,	*/
/*	 N.J. Mitra, X. Huang, S.M. Hu. The Visual Computer, 2013.			*/
/* 2. Efficient Salient Region Detection with Soft Image Abstraction.	*/
/*	 M.M. Cheng, J. Warrell, W.Y. Lin, S. Zheng, V. Vineet, N. Crook.	*/
/*	 IEEE ICCV, 2013.													*/
/* 3. Salient Object Detection and Segmentation. M.M. Cheng, N.J. Mitra,*/
/*   X. Huang, P.H.S. Torr, S.M. Hu. Submitted to IEEE TPAMI			*/
/*	 (TPAMI-2011-10-0753), 2011.										*/
/* 4. Global Contrast based Salient Region Detection, Cheng et. al.,	*/
/*	   CVPR 2011.														*/
/************************************************************************/

template <int D> struct CmGaussian 
{
	double mean[D];			// mean value
	double covar[D][D];		// covariance matrix of the Gaussian
	double det;				// determinant of the covariance matrix
	double inv[D][D];			// inverse of the covariance matrix
	double w;					// weighting of this Gaussian in the GMM.

	// These are only needed during Orchard and Bouman clustering.
	double eValues[D];		// eigenvalues of covariance matrix
	double eVectors[D][D];	// eigenvectors
};

// Gaussian mixture models
template <int D> class CmGMM_
{
public:
	typedef Vec<float, D> Sample;

	// Initialize GMM with the number of Gaussian desired, default thrV for stop dividing
	CmGMM_(int K, double thrV = 0.01);
	~CmGMM_(void);

	int K() const {return _K; }
	int maxK() const { return _MaxK; }
	const CmGaussian<D>* GetGaussians() const {return _Guassians;}

	// Returns the probability density of color c in this GMM
	inline float P(const float c[D]) const;
	inline float P(const Sample &c) const {return P(c.val);}

	// Returns the probability density of color c in just Gaussian k
	inline double P(int i, const float c[D]) const;
	inline double P(int i, const Sample &c) const {return P(i, c.val);}

	//return the mean color of component k
	Sample getMean(int k) const;

	//return the weight of component k
	double getWeight(int k) const;

	// Build the initial GMMs using the Orchard and Bouman color clustering algorithm
	// w1f: CV32FC1 to indicate weights
	void BuildGMMs(CMat& sampleDf, Mat& component1i, CMat& w1f = Mat()); 
	int RefineGMMs(CMat& sampleDf, Mat& components1i, CMat& w1f = Mat(), bool needReAssign = true); // Iteratively refine GMM

	bool Save(CStr &name) const;
	bool Load(CStr &name);
	double GetSumWeight() const {return _sumW;}

	void GetProbs(CMat sampleDf, vector<Mat> &pci1f) const; // Get Probabilities of each Channel i
	void GetProbsWN(CMat sampleDf, vector<Mat> &pci1f) const; // Get Probabilities of each Channel i, without normalize

	void iluProbs(CMat sampleDf, CStr &nameNE) const; // Get Probabilities of each Channel i, and illustrate it, without normalize
	void iluProbsWN(CMat sampleDf, CStr &nameNE) const; // Get Probabilities of each Channel i, and illustrate it, without normalize

protected:
	int _K, _MaxK; // Number of Gaussian
	double _sumW; // Sum of sample weight. For typical weights it's the number of pixels
	double _ThrV; // The lowest variations of Gaussian
	CmGaussian<D>* _Guassians; // An array of K Gaussian

	void AssignEachPixel(CMat& sampleDf, Mat &component1i);
};

class CmGMM : public CmGMM_<3>{
public:
	CmGMM(int K, double thrV = 0.01):CmGMM_<3>(K, thrV) {}
	
	void View(CStr &title, bool decreaseShow = true);

	// Show foreground probabilities represented by the GMMs
	static double ViewFrgBkgProb(const CmGMM &fGMM, const CmGMM &bGMM, CStr &title);

	static void GetGMMs(CStr &smplW, CStr &annoExt, CmGMM &fGMM, CmGMM &bGMM);

	static void Demo(CStr &wkDir);

	// Show GMM images
	void Show(CMat& components1i, CStr& title);

	void reWeights(vecD &mulWs);
};


/************************************************************************/
/*  Helper class that fits a single Gaussian to color samples           */
/************************************************************************/

template <int D> class CmGaussianFitter
{
public:
	CmGaussianFitter() {Reset();}

	// Add a color sample
	template<typename T> __forceinline void Add(const T* _c);

	template<typename T> inline void Add(const T* _c, T _weight);

	void Reset() {memset(this, 0, sizeof(CmGaussianFitter));}

	// Build the Gaussian out of all the added color samples
	void BuildGuassian(CmGaussian<D>& g, double totalCount, bool computeEigens = false) const;

	inline double Count(){return count;}

private:
	double s[D];		// sum of r, g, and b
	double p[D][D] ;	// matrix of products (i.e. r*r, r*g, r*b), some values are duplicated.
	double count;	// count of color samples added to the Gaussian
};

/************************************************************************/
/*                            CmGaussian                                */
/************************************************************************/


// Add a color sample
template <int D> template<typename T> void CmGaussianFitter<D>::Add(const T* _c)
{
	double c[D];
	for (int i = 0;  i < D; i++)
		c[i] = _c[i];

	for (int i = 0; i < D; i++)	{
		s[i] += c[i];
		for (int j = 0; j < D; j++)
			p[i][j] += c[i] * c[j];
	}
	count++;
}

template <int D> template<typename T> void CmGaussianFitter<D>::Add(const T* _c, T _weight)
{
	double c[D];
	for (int i = 0;  i < D; i++)
		c[i] = _c[i];
	double weight = _weight;

	for (int i = 0; i < D; i++)	{
		s[i] += c[i] * weight;
		for (int j = 0; j < D; j++)
			p[i][j] += c[i] * c[j] * weight;
	}
	count += weight;
}

// Build the Gaussian out of all the added color samples
template <int D> void CmGaussianFitter<D>::BuildGuassian(CmGaussian<D>& g, double totalCount, bool computeEigens) const
{
	// Running into a singular covariance matrix is problematic. So we'll add a small epsilon
	// value to the diagonal elements to ensure a positive definite covariance matrix.
	const double Epsilon = 1e-7/(D*D);

	if (count < Epsilon)
		g.w = 0;
	else {
		// Compute mean of Gaussian and covariance matrix
		for (int i = 0; i < D; i++)
			g.mean[i] = s[i]/count;

		for (int i = 0; i < D; i++)	{
			for (int j = 0; j < D; j++)
				g.covar[i][j] = p[i][j]/count - g.mean[i] * g.mean[j];
			g.covar[i][i] += Epsilon;
		}

		// Compute determinant and inverse of covariance matrix
		Mat covar(D, D, CV_64FC1, g.covar);
		Mat inv(D, D, CV_64FC1, g.inv);
		invert(covar, inv, CV_LU); // Compute determinant and inverse of covariance matrix
		g.det = determinant(covar);
		g.w = count/totalCount; // Weight is percentage of this Gaussian

		if (computeEigens) 	{
			Mat eVals(D, 1, CV_64FC1, g.eValues);
			Mat eVecs(D, D, CV_64FC1, g.eVectors);
			Matx<double, D, D> tmp;		
			SVD::compute(covar, eVals, eVecs, tmp);
		}
	}
}

/************************************************************************/
/* Gaussian mixture models                                              */
/************************************************************************/
template <int D> CmGMM_<D>::CmGMM_(int K, double thrV) 
	: _K(K), _ThrV(thrV), _MaxK(K)
{
	_Guassians = new CmGaussian<D>[_K];
}

template <int D> CmGMM_<D>::~CmGMM_(void)
{
	if (_Guassians)
		delete []_Guassians;
}

template <int D> float CmGMM_<D>::P(const float c[D]) const
{
	double r = 0;
	if (_Guassians)
		for (int i = 0; i < _K; i++)
			r += _Guassians[i].w * P(i, c);
	return (float)r;
}

template <int D> double CmGMM_<D>::P(int i, const float c[D]) const
{
	double result = 0;
	CmGaussian<D>& guassian = _Guassians[i];
	if (guassian.w > 0) {
		double v[D];
		for (int t = 0; t < D; t++)
			v[t] = c[t] - guassian.mean[t];

		if (guassian.det > 0)	{
			double (&inv)[D][D] = guassian.inv;
			double d = 0;
			for(int i = 0; i < D; i++)
				for (int j = 0; j < D; j++)
					d += v[i] * inv[i][j] * v[j];
			result = (double)(0.0635 / sqrt(guassian.det) * exp(-0.5f * d));   // 1/(2*pi)^1.5 = 0.0635
		} 
		else {
			if (guassian.w < 1e-3)
				return 0;
			else
				printf("Zero det value of %dth GMMs with weight %g in %d:%s\n", i, guassian.w, __LINE__, __FILE__);
		}
	}
	return result;
}

template <int D> void CmGMM_<D>::BuildGMMs(CMat& sampleDf, Mat& component1i, CMat& w1f)
{
	bool weighted = w1f.data != NULL;
	int rows = sampleDf.rows, cols = sampleDf.cols; 
	component1i = Mat::zeros(sampleDf.size(), CV_32S);{ 
		CV_Assert(sampleDf.data != NULL && sampleDf.type() == CV_MAKETYPE(CV_32F,D));
		CV_Assert(!weighted || w1f.type() == CV_32FC1 && w1f.size == sampleDf.size);
		if (sampleDf.isContinuous() && component1i.isContinuous() && (!weighted || w1f.isContinuous()))
			cols *= sampleDf.rows, rows = 1;
		_sumW = weighted ? sum(w1f).val[0] : rows * cols; // Finding sum weight
	}

	// Initial first clusters
	CmGaussianFitter<D>* fitters = new CmGaussianFitter<D>[_K];
	for (int y = 0; y < rows; y++)	{
		int* components = component1i.ptr<int>(y);
		const float* img = sampleDf.ptr<float>(y);
		const float* w = weighted ? w1f.ptr<float>(y) : NULL;
		if (weighted){
			for (int x = 0; x < cols; x++, img += D) 
				fitters[0].Add(img, w[x]);
		}else{
			for (int x = 0; x < cols; x++, img += D)
				fitters[0].Add(img);
		}
	}
	fitters[0].BuildGuassian(_Guassians[0], _sumW, true);

	// Compute clusters
	int nSplit = 0; // Which cluster will be split
	for (int i = 1; i < _K; i++) {
		// Stop splitting for small eigenvalue
		if (_Guassians[nSplit].eValues[0] < _ThrV){
			_K = i;
			delete []fitters;
			return;
		}

		// Reset the filters for the splitting clusters
		fitters[nSplit] = CmGaussianFitter<D>();

		// For brevity, get reference to splitting Gaussian
		CmGaussian<D>& sG = _Guassians[nSplit];

		// Compute splitting point
		double split = 0; // sG.eVectors[0][0] * sG.mean[0] + sG.eVectors[1][0] * sG.mean[1] + sG.eVectors[2][0] * sG.mean[2];
		for (int t = 0; t < D; t++)
			split += sG.eVectors[t][0] * sG.mean[t];

		// Split clusters nSplit, place split portion into cluster i
		for (int y = 0; y < rows; y++)	{
			int* components = component1i.ptr<int>(y);
			const float* img = sampleDf.ptr<float>(y);
			if (weighted){
				const float* w = w1f.ptr<float>(y);
				for (int x = 0; x < cols; x++, img += D) {// for each pixel
					if (components[x] != nSplit)
						continue;
					double tmp = 0;
					for (int t = 0; t < D; t++)
						tmp += sG.eVectors[t][0] * img[t];
					if (tmp > split)	
						components[x] = i, fitters[i].Add(img, w[x]);
					else
						fitters[nSplit].Add(img, w[x]);
				}
			}else{
				for (int x = 0; x < cols; x++, img += D) {// for each pixel
					if (components[x] != nSplit)
						continue;					
					double tmp = 0;
					for (int t = 0; t < D; t++)
						tmp += sG.eVectors[t][0] * img[t];
					if (tmp > split)	
						components[x] = i, fitters[i].Add(img);
					else
						fitters[nSplit].Add(img);					
				}
			}
		}

		// Compute new split Gaussian
		fitters[nSplit].BuildGuassian(_Guassians[nSplit], _sumW, true);
		fitters[i].BuildGuassian(_Guassians[i], _sumW, true);

		// Find clusters with highest eigenvalue
		nSplit = 0;
		for (int j = 0; j <= i; j++)
			if (_Guassians[j].eValues[0] > _Guassians[nSplit].eValues[0])
				nSplit = j;
		//for (int j = 0; j <= i; j++) printf("G%d = %g ", j, _Guassians[j].eValues[0]); printf("\nnSplit = %d\n", nSplit);
	}
	//for (int j = 0; j < _K; j++) printf("G%d = %g ", j, _Guassians[j].eValues[0]); printf("\n");
	delete []fitters;
}

template <int D> int CmGMM_<D>::RefineGMMs(CMat& sampleDf, Mat& components1i, CMat& w1f, bool needReAssign)
{
	bool weighted = w1f.data != NULL;
	int rows = sampleDf.rows, cols = sampleDf.cols; { 
		CV_Assert(sampleDf.data != NULL && sampleDf.type() == CV_MAKETYPE(CV_32F,D));
		CV_Assert(!weighted || w1f.type() == CV_32FC1 && w1f.size == sampleDf.size);
		if (sampleDf.isContinuous() && components1i.isContinuous() && (!weighted || w1f.isContinuous()))
			cols *= sampleDf.rows, rows = 1;
	}
	
	if (needReAssign)
		AssignEachPixel(sampleDf, components1i);

	// Relearn GMM from new component assignments
	CmGaussianFitter<D>* fitters = new CmGaussianFitter<D>[_K];
	for (int y = 0; y < rows; y++)	{
		const float* pixel = sampleDf.ptr<float>(y); 
		const int* component = components1i.ptr<int>(y); 
		if (weighted){
			const float* w = w1f.ptr<float>(y);
			for (int x = 0; x < cols; x++, pixel += D)
				fitters[component[x]].Add(pixel, w[x]);
		}
		else
			for (int x = 0; x < cols; x++, pixel += D)
				fitters[component[x]].Add(pixel);
	}


	int newK = 0;
	for (int i = 0; i < _K; i++)
		if (fitters[i].Count() > 0)
			fitters[i].BuildGuassian(_Guassians[newK++], _sumW, false);
	delete []fitters;
	_K = newK;

	// Assign each pixel 
	AssignEachPixel(sampleDf, components1i);

	return _K;
}

template <int D> void CmGMM_<D>::AssignEachPixel(CMat& sampleDf, Mat &component1i)
{
	int rows = sampleDf.rows, cols = sampleDf.cols;
	if (sampleDf.isContinuous() && component1i.isContinuous())
		cols *= sampleDf.rows, rows = 1;

	for (int y = 0; y < rows; y++)	{
		const float* pixel = sampleDf.ptr<float>(y); 
		int* component = component1i.ptr<int>(y); 
		for (int x = 0; x < cols; x++, pixel += D)	{
			int k = 0;
			double maxP = 0;
			for (int i = 0; i < _K; i++) {
				double posb = P(i, pixel);
				if (posb > maxP)
					k = i, maxP = posb;
			}
			component[x] = k;
		}
	}
}

template <int D> Vec<float, D> CmGMM_<D>::getMean(int k) const {
	CV_Assert(k >= 0 && k <= _K);
	Vec3f meanColor;
	meanColor[0] = (float)_Guassians[k].mean[0];
	meanColor[1] = (float)_Guassians[k].mean[1];
	meanColor[2] = (float)_Guassians[k].mean[2];
	return meanColor;
}

template <int D> double CmGMM_<D>::getWeight(int k) const{
	if (k < 0 || k >= _K) 
		return 0.0;
	return _Guassians[k].w;
}

template <int D> bool CmGMM_<D>::Save(CStr &name) const
{
	FILE* file = fopen(_S(name), "wb");
	if (file == NULL)
		return false;
	int pre = fwrite("CmGMM", sizeof(char), 5, file);
	int dataLen = sizeof(int) * 1 + sizeof(double) * 3;
	CV_Assert(pre == 5 && fwrite(this, 1, dataLen, file) == dataLen);
	CV_Assert(fwrite(_Guassians, sizeof(CmGaussian), _K, file) == _K);
	fclose(file);
	return true;
}

template <int D> bool CmGMM_<D>::Load(CStr &name)
{
	FILE* file = fopen(_S(name), "rb");
	if(file == NULL)
		return false;

	char buf[8];
	int pre = fread(buf,sizeof(char), 5, file);
	CV_Assert(pre == 5);
	if (strncmp(buf, "CmGMM", 5) != 0)	{
		printf("Invalidate CmGMM data file %s\n", _S(name));
		return false;
	}
	int oldK = _K;
	int L1 = sizeof(CmGMM_), L2 = sizeof(CmGaussian*);
	int dataLen = sizeof(int) * 1 + sizeof(double) * 3;
	CV_Assert(fread(this, 1, dataLen, file) == dataLen);
	int g = fread(_Guassians, sizeof(CmGaussian), _K, file);
	CV_Assert(g == _K && oldK >= _K);
	fclose(file);
	return true;
}

template <int D> void CmGMM_<D>::GetProbs(CMat sampleDf, vector<Mat> &pci) const
{
	pci.resize(_K);
	Mat pI = Mat::zeros(sampleDf.size(), CV_32F);
	for (int c = 0; c < _K; c++) {// for each component c
		pci[c].create(sampleDf.size(), CV_32F);
		for (int y = 0; y < sampleDf.rows; y++){
			float* prob = pci[c].ptr<float>(y); 
			const float* sp = sampleDf.ptr<float>(y);
			for (int x = 0; x < sampleDf.cols; x++, sp += D)
				prob[x] = (float)(P(c, sp) *_Guassians[c].w);
		}
		add(pci[c], pI, pI); 
	}

	for (int c = 0; c < _K; c++)
		divide(pci[c], pI, pci[c]); 
}

template <int D> void CmGMM_<D>::GetProbsWN(CMat sampleDf, vector<Mat> &pci) const
{
	pci.resize(_K);
	for (int c = 0; c < _K; c++) {// for each component c
		pci[c].create(sampleDf.size(), CV_32F);
		for (int y = 0; y < sampleDf.rows; y++){
			float* prob = pci[c].ptr<float>(y);
			const float* sp = sampleDf.ptr<float>(y);
			for (int x = 0; x < sampleDf.cols; x++, sp += D)
				prob[x] = (float)(P(c, sp) *_Guassians[c].w);
		}
	}
}

template <int D> void CmGMM_<D>::iluProbs(CMat sampleDf, CStr &nameNE) const
{
	vector<Mat> pci;
	GetProbs(sampleDf, pci);
	Mat tmpShow;
	for (int i = 0; i < _K; i++){
		normalize(pci[i], tmpShow, 0, 255, NORM_MINMAX, CV_8U);
		CmShow::SaveShow(tmpShow, nameNE + format("%d.png", i));
	}
	for (int i = _K; i < _MaxK; i++)
		CmFile::WriteNullFile(nameNE + format("%d.nul", i));
}

template <int D> void CmGMM_<D>::iluProbsWN(CMat sampleDf, CStr &nameNE) const
{
	vector<Mat> pci;
	GetProbsWN(sampleDf, pci);
	Mat tmpShow;
	for (int i = 0; i < _K; i++){
		double minVal, maxVal;
		cv::minMaxLoc(pci[i], &minVal, &maxVal);
		pci[i].convertTo(tmpShow, CV_8U, 255 / (maxVal - minVal + EPS), -minVal);
		cvtColor(tmpShow, tmpShow, CV_GRAY2BGR);
		putText(tmpShow, format("Min = %g, ", minVal), Point(5, 20), CV_FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 0, 0));
		putText(tmpShow, format("Max = %g", maxVal), Point(5, 40), CV_FONT_HERSHEY_PLAIN, 1, CV_RGB(255, 0, 0));
		CmShow::SaveShow(tmpShow, nameNE + format("%d.png", i));
	}
	for (int i = _K; i < _MaxK; i++)
		CmFile::WriteNullFile(nameNE + format("%d.nul", i));
}