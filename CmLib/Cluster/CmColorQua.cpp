#include "StdAfx.h"
#include "CmColorQua.h"

const int CmColorQua::binNum[] = {6*6*6, 256, 5*7*7, 256};
const char* CmColorQua::descr[] = {"S_BGR", "S_HSV", "S_Lab", "D_BGR"};
const CmColorQua::S_QUANTIZE_FUNC CmColorQua::sqFuns[] = {SQ_BGR, SQ_HSV, SQ_Lab};
const CmColorQua::S_RECOVER_FUNC  CmColorQua::srFuns[] = {SR_BGR, SR_HSV, SR_Lab};

void CmColorQua::TestColorQuantize(CStr &inImgs, CStr &outDir)
{
	printf("Quantize images '%s' and  save results to '%s'\n", inImgs.c_str(), outDir.c_str());
	CmFile::MkDir(outDir);
	string inDir;
	vecS names;
	int imgNum = CmFile::GetNames(inImgs, names, inDir);

#pragma omp parallel for
	for (int i = 0; i < imgNum; i++){
		printf("Processing %-40s\r", names[i].c_str());
		string nameNE = CmFile::GetNameNE(names[i]);
		Mat img = imread(inDir + names[i]);
		imwrite(outDir + nameNE + ".jpg", img);
		img.convertTo(img, CV_32FC3, 1/255.f);
		Mat idx, rImg3f;

		for (int j = 0; j < S_Q_NUM; j++){
			S_Quantize(img, idx, j);
			S_Recover(idx, rImg3f, j);
			imwrite(outDir + nameNE + descr[j] + ".jpg", rImg3f * 255);

			S_Quantize(img, idx, j);
			S_Recover(idx, rImg3f, j, img);
			imwrite(outDir + nameNE + descr[j] + "2.jpg", rImg3f * 255);
		}

		//Mat avgColor, colorNum;
		//D_Quantize(img, idx, avgColor, colorNum, 256);
		//D_Recover(idx, rImg3f, avgColor);
		//imwrite(outDir + nameNE + "D_BGR.jpg", rImg3f*255);
	}
	printf("Test color quantization finished\t\t\t\n");

	/*
	Vec3f color;
	Mat org(100, 200, CV_32FC3), dst(100, 200, CV_32FC3), idx;
	while (scanf("%f %f %f", &color[0], &color[1], &color[2]))
	{
		printf("Org: %f, %f, %f\t", color[0], color[1], color[2]);
		org = Scalar(color);
		S_Quantize(org, idx);
		S_Recover(idx, dst);
		color = dst.at<Vec3f>(0, 0);
		printf("Dst: %f, %f, %f\n", color[0], color[1], color[2]);
		cvtColor(org, org, CV_Lab2BGR);
		cvtColor(dst, dst, CV_Lab2BGR);
		imshow("Org", org);
		imshow("Dst", dst);
		waitKey(1);
	}
	//*/
}

// img3f is the input BGR image
void CmColorQua::S_Quantize(CMat& _img3f, Mat &idx1i, int method)
{
	CV_Assert(method >= 0 && method < S_Q_NUM && _img3f.data != NULL && _img3f.type() == CV_32FC3);
	S_QUANTIZE_FUNC SQ_Function = sqFuns[method];

	Mat img;
	switch (method)	{
	case S_BGR: img = _img3f; break;
	case S_HSV: cvtColor(_img3f, img, CV_BGR2HSV); break;
	case S_LAB: cvtColor(_img3f, img, CV_BGR2Lab); break;
	}

	idx1i.create(img.size(), CV_32S);
	for (int r = 0; r < img.rows; r++)	{
		const Vec3f * imgD = img.ptr<Vec3f>(r);
		int *idx = idx1i.ptr<int>(r);
		for (int c = 0; c < img.cols; c++)
			idx[c] = (*SQ_Function)(imgD[c]);
	}
}

// src3f are BGR, color3f are 1xBinDim matrix represent color fore each histogram bin
int CmColorQua::S_BinInf(CMat& idx1i, Mat &color3f, vecI &colorNum, int method, CMat &src3f)
{
	int totalBinNum = 0;
	CV_Assert(idx1i.data != NULL && idx1i.type() == CV_32S && method >= 0 && method < S_Q_NUM);

	// Find colors for each bin
	color3f = Mat::zeros(1, binNum[method], CV_32FC3);
	Vec3f* color = (Vec3f*)(color3f.data);

	vector<Vec3d> colorD(color3f.cols, 0);
	colorNum.resize(color3f.cols, 0);
	if (src3f.size() != Size() && src3f.data != NULL)	{
		for (int r = 0; r < idx1i.rows; r++) {
			const int *idx = idx1i.ptr<int>(r);
			const Vec3f *src = src3f.ptr<Vec3f>(r);
			for (int c = 0; c < idx1i.cols; c++)	{
				colorD[idx[c]] += src[c];
				colorNum[idx[c]] ++;
			}
		}
	}
	S_RECOVER_FUNC SR_Function = srFuns[method];
	for (int i = 0; i < color3f.cols; i++)	{
		if (colorNum[i] == 0)
			(*SR_Function)(i, color[i]);
		else
			totalBinNum += colorNum[i];
	}
	if (method == 1)
		cvtColor(color3f, color3f, CV_HSV2BGR);
	else if (method == 2)
		cvtColor(color3f, color3f, CV_Lab2BGR);

	for (int i = 0; i < color3f.cols; i++)
		if (colorNum[i] > 0)
			color[i] = Vec3f((float)(colorD[i][0]/colorNum[i]), (float)(colorD[i][1]/colorNum[i]), (float)(colorD[i][2]/colorNum[i]));

	return totalBinNum;
}

// img3f and src3f are BGR 
void CmColorQua::S_Recover(CMat& idx1i, Mat& img3f, int method, CMat &src3f)
{
	Mat color3f;
	vecI colorNum;
	S_BinInf(idx1i, color3f, colorNum, method, src3f);
	Vec3f* color = (Vec3f*)(color3f.data);

	img3f.create(idx1i.size(), CV_32FC3);
	for (int r = 0; r < idx1i.rows; r++)	{
		const int *idx = idx1i.ptr<int>(r);
		Vec3f *img = img3f.ptr<Vec3f>(r);
		for (int c = 0; c < idx1i.cols; c++)
			img[c] = color[idx[c]]; 
	}
}

void CmColorQua::Show(CMat &counts1d, CStr title, Mat &show3f, int method)
{
	int n = binNum[method];
	CV_Assert(method >= 0 && method < S_Q_NUM && counts1d.size() == Size(n, 1));
	Mat color3f(1, n, CV_32FC3);
	Vec3f* colors = (Vec3f*)(color3f.data);
	for (int i = 0; i < n; i++)	{
		switch (method)	{
		case 0: SR_BGR(i, colors[i]); break;
		case 1: SR_HSV(i, colors[i]); break;
		case 2: SR_Lab(i, colors[i]); break;
		}
	}
	switch (method)	{
	case 1: cvtColor(color3f, color3f, CV_HSV2BGR); break;
	case 2: cvtColor(color3f, color3f, CV_Lab2BGR); break;
	}
	//CmShow::HistBins(color3f, counts1d, title);
}

int CmColorQua::SQ_BGR(const Vec3f &c)
{
	return (int)(c[0]*5.9999f) * 36 + (int)(c[1]*5.9999f) * 6 + (int)(c[2]*5.9999f);
}

void CmColorQua::SR_BGR(int idx, Vec3f &c)
{
	c[0] = ((float)(idx / 36) + 0.5f)/6.f;
	idx %= 36;
	c[1] = ((float)(idx / 6) + 0.5f)/6.f;
	c[2] = ((float)(idx % 6) + 0.5f)/6.f;
}

int CmColorQua::SQ_HSV(const Vec3f &c)
{
	const float S_MIN_HSV = 0.1f;
	float h(c[0]/360.001f), s(c[1]/1.001f), v(c[2]/1.001f);

	int result;
	if (s < S_MIN_HSV) // 240 ... 255
		result = 240 + (int)(v * 16.f); 
	else{
		int ih, is, iv;
		ih = (int)(h * 15.f); //0..14
		is = (int)((s - S_MIN_HSV)/(1 - S_MIN_HSV) * 4.f); //0..3
		iv = (int)(v * 4.f); //0..3
		result = ih * 16 + is * 4 + iv; // 0..239

		CV_Assert(ih < 15 && is < 4 && iv < 4);
	}

	return result;
}

void CmColorQua::SR_HSV(int idx, Vec3f &c)
{
	const float S_MIN_HSV = 0.1f;
	if (idx >= 240)	{
		c[0] = c[1] = 0;
		c[2] = (idx - 239.5f) / 16.f;
	}
	else{
		c[0] = (float(idx / 16) + 0.5f) / 15.f;
		idx %= 16;
		c[1] = (float(idx / 4) + 0.5f) * (1 - S_MIN_HSV) / 4.f + S_MIN_HSV;
		c[2] = (float(idx % 4) + 0.5f) / 4.f;
	}
	CV_Assert(c[0] < 1.f && c[1] < 1.f && c[2] < 1.f);
	c[0] *= 360.f;
}

int CmColorQua::SQ_Lab(const Vec3f &c)
{
	float L(c[0] / 100.0001f), a((c[1] + 127) / 254.0001f), b((c[2] + 127) / 254.0001f);
	int iL = (int)(L * 5), ia = (int)(a * 7), ib = (int)(b * 7);
	CV_Assert(iL >= 0 && ia >= 0 && ib >= 0 && iL < 5 && ia < 7 && ib < 7);
	return iL + ia * 5 + ib * 35;
}

void CmColorQua::SR_Lab(int idx, Vec3f &c)
{
	c[2] = (float(idx / 35) + 0.5f) * (254.f / 7.f) - 127.f;
	idx %= 35;
	c[1] = (float(idx / 5) + 0.5f) * (254.f / 7.f) - 127.f;
	c[0] = (float(idx % 5) + 0.5f) * (100.f / 5.f);

	CV_Assert(c[0] < 100.f && c[0] >= 0.f && c[1] < 127.f  && c[1] > -127.f && c[2] < 127.f && c[2] > -127.f);
}

const int CmColorQua::DefaultNums[3] = {12, 12, 12};

int CmColorQua::D_Quantize(CMat& img3f, Mat &idx1i, Mat &_color3f, Mat &_colorNum, double ratio, const int clrNums[3])
{
	float clrTmp[3] = {clrNums[0] - 0.0001f, clrNums[1] - 0.0001f, clrNums[2] - 0.0001f};
	int w[3] = {clrNums[1] * clrNums[2], clrNums[2], 1};

	CV_Assert(img3f.data != NULL);
	idx1i = Mat::zeros(img3f.size(), CV_32S);
	int rows = img3f.rows, cols = img3f.cols;
	if (img3f.isContinuous() && idx1i.isContinuous()){
		cols *= rows;
		rows = 1;
	}

	// Build color pallet
	for (int y = 0; y < rows; y++)	{
		const float* imgDataP = img3f.ptr<float>(y);
		int* idx = idx1i.ptr<int>(y);
#pragma omp parallel for
		for (int x = 0; x < cols; x++){
			const float* imgData = imgDataP + 3*x;
			idx[x] = (int)(imgData[0]*clrTmp[0])*w[0] + (int)(imgData[1]*clrTmp[1])*w[1] + (int)(imgData[2]*clrTmp[2]);
		}
	}
	map<int, int> pallet;
	for (int y = 0; y < rows; y++)	{
		int* idx = idx1i.ptr<int>(y);
		for (int x = 0; x < cols; x++)
			pallet[idx[x]] ++;
	}

	// Find significant colors
	int maxNum = 0; {
		int count = 0;
		vector<pair<int, int>> num; // (num, color) pairs in num
		num.reserve(pallet.size());
		for (map<int, int>::iterator it = pallet.begin(); it != pallet.end(); it++)
			num.push_back(pair<int, int>(it->second, it->first)); // (color, num) pairs in pallet
		sort(num.begin(), num.end(), std::greater<pair<int, int>>());

		maxNum = (int)num.size();
		int maxDropNum = cvRound(rows * cols * (1-ratio));
		for (int crnt = num[maxNum-1].first; crnt < maxDropNum && maxNum > 1; maxNum--)
			crnt += num[maxNum - 2].first;
		maxNum = min(maxNum, 256); // To avoid very rarely case
		if (maxNum <= 10)
			maxNum = min(10, (int)num.size());

		pallet.clear();
		for (int i = 0; i < maxNum; i++)
			pallet[num[i].second] = i; 

		int numSZ = num.size();
		vector<Vec3i> color3i(numSZ);
#pragma omp parallel for
		for (int i = 0; i < numSZ; i++) {
			color3i[i][0] = num[i].second / w[0];
			color3i[i][1] = num[i].second % w[0] / w[1];
			color3i[i][2] = num[i].second % w[1];
		}

		for (unsigned int i = maxNum; i < num.size(); i++)	{
			int simIdx = 0, simVal = INT_MAX;
#pragma omp parallel for
			for (int j = 0; j < maxNum; j++) {
				int d_ij = vecSqrDist(color3i[i], color3i[j]);
				if (d_ij < simVal)
					simVal = d_ij, simIdx = j;
			}
			pallet[num[i].second] = pallet[num[simIdx].second];
		}
	}

	_color3f = Mat::zeros(1, maxNum, CV_32FC3);
	_colorNum = Mat::zeros(_color3f.size(), CV_32S);

	Vec3f* color = (Vec3f*)(_color3f.data);
	int* colorNum = (int*)(_colorNum.data);
	for (int y = 0; y < rows; y++) {
		const Vec3f* imgData = img3f.ptr<Vec3f>(y);
		int* idx = idx1i.ptr<int>(y);
#pragma omp parallel for
		for (int x = 0; x < cols; x++)	
			idx[x] = pallet[idx[x]];
		for (int x = 0; x < cols; x++)	{
			color[idx[x]] += imgData[x];
			colorNum[idx[x]] ++;
		}
	}
	for (int i = 0; i < _color3f.cols; i++)
		color[i] /= colorNum[i];

	return _color3f.cols;
}

void CmColorQua::D_Recover(CMat& idx1i, Mat &img3f, CMat &color3f)
{
	CV_Assert(idx1i.data != NULL);
	img3f.create(idx1i.size(), CV_32FC3);
	
	Vec3f* color = (Vec3f*)(color3f.data);
	for (int y = 0; y < idx1i.rows; y++) {
		Vec3f* imgData = img3f.ptr<Vec3f>(y);
		const int* idx = idx1i.ptr<int>(y);
		for (int x = 0; x < idx1i.cols; x++) {
			imgData[x] = color[idx[x]];
			CV_Assert(idx[x] < color3f.cols);
		}
	}
}

//
//CmColorIdx::CmColorIdx(int maxSupportedNum)
//{
//	_numEachDim = (int)ceil(pow(maxSupportedNum, 1.0/3)); 
//	_step = 255.0 / (_numEachDim - 1); 
//	_numEachDimSqr = sqr(_numEachDimSqr);
//	_maxSupportedNum = maxSupportedNum;
//	_Colors.resize(maxSupportedNum);
//	for (int i = 0; i < maxSupportedNum; i++){
//		int idx = i;
//		int r = (int)((idx % _numEachDim) * _step);
//		idx /= _numEachDim;
//		int g = (int)((idx % _numEachDim) * _step);
//		int b = (int)((idx / _numEachDim) * _step);
//		_Colors[i] = Vec3b(b, g, r);
//	}
//}
//
//void CmColorIdx::idx2Color(CMat &idx1u, Mat &color3u)
//{
//	color3u.create(idx1u.size(), CV_8UC3);
//	for (int r = 0; r < idx1u.rows; r++){
//		const byte* idx = idx1u.ptr<byte>(r);
//		Vec3b *color = color3u.ptr<Vec3b>(r);
//		for (int c = 0; c < idx1u.cols; c++)
//			color[c] = (*this)[idx[c]];
//	}
//}
//
//void CmColorIdx::color2Idx(CMat &color3u, Mat &idx1u)
//{
//	idx1u.create(color3u.size(), CV_8UC1);
//	int coutR = 0, countNR = 0;
//	for (int r = 0; r < idx1u.rows; r++){
//		byte* idx = idx1u.ptr<byte>(r);
//		const Vec3b *color = color3u.ptr<Vec3b>(r);
//		for (int c = 0; c < idx1u.cols; c++){
//			if (c > 0 && color[c - 1] == color[c])
//				idx[c] = idx[c-1], coutR += 1;
//			else
//				idx[c] = (*this)(color[c]), countNR+=1;
//		}
//	}
//	//printf("Reduced %g%% percent\n", coutR*100.0 / (coutR+countNR));
//}
//
//void CmColorIdx::demo()
//{
//	CmColorIdx cIdx(64);
//	for (int i = 0; i < 64; i++){
//		printf("%d:%d(%03d, %03d, %03d)\t", i, cIdx(cIdx[i]), cIdx[i][0], cIdx[i][1], cIdx[i][2]);
//		Mat img(100, 200, CV_8UC3);
//		img = Scalar(cIdx[i]);
//		//imshow(format("%d", i), img);
//		//waitKey(0);
//	}
//	printf("\n");
//}