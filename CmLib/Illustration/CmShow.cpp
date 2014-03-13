#include "StdAfx.h"
#include "CmShow.h"

const Vec3b CmShow::gColors[CmShow::COLOR_NUM] = 
{
	Vec3b(0, 0, 255),	  Vec3b(0, 255, 0),		Vec3b(255, 0, 0),     Vec3b(153, 0, 48),	Vec3b(0, 183, 239),  
	Vec3b(255, 255, 0),   Vec3b(255, 126, 0),   Vec3b(255, 194, 14),  Vec3b(168, 230, 29),
	Vec3b(237, 28, 36),   Vec3b(77, 109, 243),  Vec3b(47, 54, 153),   Vec3b(111, 49, 152),  Vec3b(156, 90, 60),
	Vec3b(255, 163, 177), Vec3b(229, 170, 122), Vec3b(245, 228, 156), Vec3b(255, 249, 189), Vec3b(211, 249, 188),
	Vec3b(157, 187, 97),  Vec3b(153, 217, 234), Vec3b(112, 154, 209), Vec3b(84, 109, 142),  Vec3b(181, 165, 213),
	Vec3b(40, 40, 40),	  Vec3b(70, 70, 70),	Vec3b(120, 120, 120), Vec3b(180, 180, 180), Vec3b(220, 220, 220)
};


const Vec3b CmShow::Black(0, 0, 0);
const Vec3b CmShow::Blue(255, 0, 0);
const Vec3b CmShow::Brown(42, 42, 165);
const Vec3b CmShow::Gray(128, 128, 128);
const Vec3b CmShow::Green(0, 255, 0);
const Vec3b CmShow::Orange(0, 165, 255);
const Vec3b CmShow::Pink(203, 192, 255); 
const Vec3b CmShow::Purple(128, 0, 128); 
const Vec3b CmShow::Red(0, 0, 255);
const Vec3b CmShow::White(255, 255, 255); 
const Vec3b CmShow::Yellow(0, 255, 255);

const Scalar CmShow::BLACK = Black;
const Scalar CmShow::BLUE = Blue;
const Scalar CmShow::BROWN = Brown;
const Scalar CmShow::GRAY = Gray;
const Scalar CmShow::GREEN = Green;
const Scalar CmShow::ORANGE = Orange;
const Scalar CmShow::PINK = Pink;
const Scalar CmShow::PURPLE = Purple;
const Scalar CmShow::RED = Red;
const Scalar CmShow::WHITE = White;
const Scalar CmShow::YELLOW = Yellow;


const char* CmShow::NAMES[NAME_NUM] = {"Black", "Blue", "Brown", "Gray", "Green", "Orange", "Pink", "Purple", "Red", "White", "Yellow"};
const Vec3b CmShow::COLORS[NAME_NUM] = {Black, Blue, Brown, Gray, Green, Orange, Pink, Purple, Red, White, Yellow};


/************************************************************************/
/* Show an complex array: Mag, Angle, FFT shifted log mag               */
/************************************************************************/
Mat CmShow::Complex(CMat& _cmplx, CStr& title, float minMagShowAng, int flag)
{
	CV_Assert(_cmplx.channels() == 2 && _cmplx.data != NULL);
	Mat ang(_cmplx.size(), CV_32FC1), mag(_cmplx.size(), CV_32FC1), cmplx;
	_cmplx.convertTo(cmplx, CV_32F);

	for (int y = 0; y < cmplx.rows; y++){
		float* cpV = cmplx.ptr<float>(y);
		float* angV = ang.ptr<float>(y);
		float* magV = mag.ptr<float>(y);
		for (int x = 0; x < cmplx.cols; x++, cpV+=2){
			magV[x] = sqrt(cpV[0] * cpV[0] + cpV[1] * cpV[1]);
			angV[x] = cvFastArctan(cpV[1], cpV[0]);
		}
	}
	return Complex(ang, mag, title, minMagShowAng, flag);
}

Mat CmShow::ComplexD(CMat& dx, CMat& dy, CStr& tile, float minMagShowAng, int flag)
{
	vector<Mat> chanals(2);
	chanals[0] = dx, chanals[1] = dy;
	Mat cmplx;
	merge(chanals, cmplx);
	return Complex(cmplx, tile, minMagShowAng, flag);
}


/************************************************************************/
/* Show an complex array: Mag, Angle, FFT shifted log mag               */
/*            ang(in degree) and mag should be 64F or 32F				*/
/************************************************************************/
Mat CmShow::Complex(CMat& _ang, CMat& _mag, CStr& title, float minMagShowAng, int flag)
{
	CV_Assert(_ang.size() == _mag.size() && _ang.channels() == 1 && _mag.channels() == 1);
	Mat ang, mag;
	_ang.convertTo(ang, CV_32F);
	_mag.convertTo(mag, CV_32F);
	if (flag & SHOW_MAG_LOG){
		cv::log(mag + 1, mag);
		CmCv::FFTShift(mag);
	}
	normalize(mag, mag, 0, 255, NORM_MINMAX, CV_8U);
	minMagShowAng *= 255;
	int rows = ang.rows, cols = ang.cols;
	Mat img8U3C(rows, flag & MAG_AS_SAT ? cols : cols * 2, CV_8UC3);
	if (!(flag & MAG_AS_SAT))
		cvtColor(mag, img8U3C(Rect(cols, 0, cols, rows)), CV_GRAY2BGR);

	Mat showAng = img8U3C(Rect(0, 0, cols, rows));
	Ornt2HueFunc ornt2HueFunc = flag & ORNT2HUE_SYM4 ? Ornt2HueSym4 : Ornt2Hue;
	for (int y = 0; y < rows; y++)	{
		float* angV = ang.ptr<float>(y);
		byte* magV = mag.ptr<byte>(y);
		byte* angShow = showAng.ptr<byte>(y);
		for (int x = 0; x < cols; x++, angShow += 3){
			if (magV[x] < minMagShowAng)
				continue;
			angShow[0] = ornt2HueFunc(angV[x]); 
			angShow[1] = flag & MAG_AS_SAT ? magV[x] : 255;
			angShow[2] = 255;
		}
	}

	cvtColor(showAng, showAng, CV_HSV2BGR);
	SaveShow(img8U3C, title);
	return img8U3C;
}

/************************************************************************/
/* Show color circle                                                    */
/************************************************************************/
void CmShow::ColorCircle(CStr &tile, int radius, int flag)
{
	int diameter = radius * 2;
	float maxDist = (float)(radius * radius);
	Mat showImg = Mat::zeros(diameter, diameter, CV_8UC3);
	Ornt2HueFunc ornt2HueFunc = flag & ORNT2HUE_SYM4 ? Ornt2HueSym4 : Ornt2Hue;
	for (int r = 0; r < diameter; r++)	{
		byte* showV = showImg.ptr<byte>(r);
		for (int c = 0; c < diameter; c++, showV += 3){
			float x = (float)(c-radius), y = (float)(r-radius);
			if (x*x + y*y > maxDist)
				continue;
			showV[0] = ornt2HueFunc(cvFastArctan(y, x));
			showV[1] = showV[2] = 255;
		}
	}
	cvtColor(showImg, showImg, CV_HSV2BGR);
	SaveShow(showImg, tile);
}

void CmShow::SaveColorSamples(CStr &dir)
{
	CmFile::MkDir(dir);
	const int NUM = 11;
	const char* names[NUM] = {"Black", "Blue", "Brown", "Gray", "Green", "Orange", "Pink", "Purple", "Red", "White", "Yellow"};
	const Scalar colors[NUM] = {BLACK, BLUE, BROWN, GRAY, GREEN, ORANGE, PINK, PURPLE, RED, WHITE, YELLOW};
	Mat sample(50, 100, CV_8UC3);
	for (int i = 0; i < NUM; i++){
		sample = colors[i];
		imwrite(dir + names[i] + ".jpg", sample);
	}
}

// Show a label map. labelNum: how many number of random colors used for show, use default colors if is -1
Mat CmShow::Label(CMat& label1i, CStr& title, int labelNum, bool showIdx)
{
	bool useRandom = labelNum > 0;
	labelNum = useRandom ? labelNum : COLOR_NU_NO_GRAY;
	vector<Vec3b> colors(labelNum);
	if (useRandom)
		for (size_t i = 0; i < colors.size(); i++)
			colors[i] = RandomColor();
	else
		for (size_t i = 0; i < colors.size(); i++)
			colors[i] = gColors[i];

	Mat showImg = Mat::zeros(label1i.size(), CV_8UC3);
	for (int y = 0; y < label1i.rows; y++)	{
		Vec3b* showD = showImg.ptr<Vec3b>(y);
		const int* label = label1i.ptr<int>(y);
		for (int x = 0; x < label1i.cols; x++)
			if (label[x] >= 0){
				showD[x] = colors[label[x] % labelNum];
				if (showIdx)
					showD[x][2] = (byte)(label[x]); 
			}
	}
	SaveShow(showImg, title);
	return showImg;
}

int CmShow::GetColorIdx(CStr &colorName)
{
	for (int i = 0; i < CmShow::NAME_NUM; i++)
		if (colorName == CmShow::NAMES[i])
			return i;
	printf("Can't find color %s\n", _S(colorName));
	return -1;
}

Mat CmShow::HistBins(CMat& color3f, CMat& val, CStr& title, bool descendShow, CMat &with)
{
	// Prepare data
	int H = 300, spaceH = 6, barH = 10, n = color3f.cols;
	CV_Assert(color3f.size() == val.size() && color3f.rows == 1);
	Mat binVal1i, binColor3b, width1i;
	if (with.size() == val.size())
		with.convertTo(width1i, CV_32S, 400/sum(with).val[0]); // Default shown width
	else
		width1i = Mat(1, n, CV_32S, Scalar(600.0/(val.cols*val.rows))); // Default bin width = 10
	int W = cvRound(sum(width1i).val[0]);
	color3f.convertTo(binColor3b, CV_8UC3, 255);
	double maxVal, minVal;
	minMaxLoc(val, &minVal, &maxVal);
	val.convertTo(binVal1i, CV_32S, H/max(maxVal, -minVal));
	Size szShow(W, H + spaceH + barH);
	szShow.height += minVal < 0 && !descendShow ? H + spaceH : 0;
	Mat showImg3b(szShow, CV_8UC3, WHITE);
	int* binH = (int*)(binVal1i.data);
	Vec3b* binColor = (Vec3b*)(binColor3b.data);
	int* binW = (int*)(width1i.data);
	vector<CostiIdx> costIdx(n);
	if (descendShow){
		for (int i = 0; i < n; i++)
			costIdx[i] = make_pair(binH[i], i);
		sort(costIdx.begin(), costIdx.end(), std::greater<CostiIdx>());
	}

	// Show image
	for (int i = 0, x = 0; i < n; i++){
		int idx = descendShow ? costIdx[i].second : i;
		int h = descendShow ? abs(binH[idx]) : binH[idx];
		Scalar color(binColor[idx]);
		Rect reg(x, H + spaceH, binW[idx], barH);
		showImg3b(reg) = color; // Draw bar
		rectangle(showImg3b, reg, BLACK);

		reg.height = abs(h);
		reg.y = h >= 0 ? H - h : H + 2 * spaceH + barH;
		showImg3b(reg) = color;
		rectangle(showImg3b, reg, BLACK);

		x += binW[idx];
	}
	putText(showImg3b, format("Min = %g, Max = %g", minVal, maxVal), Point(5, 20), CV_FONT_HERSHEY_PLAIN, 1, CV_RGB(255,0,0));
	SaveShow(showImg3b, title);
	return showImg3b;
}

void CmShow::Pseudocolor(CMat& matfd1, CStr& title)
{
	Mat hsvMat[3], hsvM;
	matfd1.convertTo(hsvMat[0], CV_32FC1, -240, 240);
	hsvMat[1] = hsvMat[2] = Mat::ones(matfd1.size(), CV_32F);
	merge(hsvMat, 3, hsvM);
	cvtColor(hsvM, hsvM, CV_HSV2BGR);
	SaveShow(hsvM, title);
}

Mat CmShow::PntList(const PointSeti &pntList, Mat &showMat, CStr &title, int viewStep, int waite)
{
	Vec3b color(50, 50, 255);
	for (size_t i = 0; i < pntList.size(); i++)	{
		color[2] = (color[2] + 1)%256;
		if (color[2] == 0 && i != 0){
			color[0] = rand()%255;
			color[1] = rand()%255;
		}
		showMat.at<Vec3b>(pntList[i]) = color;
		if (i % viewStep == viewStep - 1)	{
			SaveShow(showMat, title);
			if (waite >=0)
				waitKey(waite);
		}
	}
	SaveShow(showMat, title);
	return showMat;
}


void CmShow::showTinyMat(CStr &title, CMat &m)
{
	int scale = 50, sz = m.rows * m.cols;
	while (sz > 200){
		scale /= 2;
		sz /= 4;
	}

	Mat img;
	resize(m, img, Size(), scale, scale, CV_INTER_NN);
	if (img.channels() == 3)
		cvtColor(img, img, CV_RGB2BGR);
	imshow(title, img);
	waitKey(1);
}

void CmShow::showTinySparseMat(CStr &title, const SparseMat &A1d)
{
	int N = A1d.nzcount();
	if (N > 1000)
		return;

	struct NodeSM{
		int r, c;
		double v;
		bool operator < (const NodeSM &n){
			if (r < n.r)
				return true;
			else if (r > n.r)
				return false;
			else 
				return c < n.c;
		}

		void print(){if (abs(v) > 1e-8) printf("(%d, %d) %-12g\t", r, c, v); }
	};

	vector<NodeSM> nodes(N);
	SparseMatConstIterator it = A1d.begin();

	for (int i = 0; i < N; i++, ++it){
		const int* idx = it.node()->idx;
		nodes[i].r = idx[0] + 1;
		nodes[i].c = idx[1] + 1;
		nodes[i].v = it.value<double>();
	}
	sort(nodes.begin(), nodes.end());
	for (int i = 0; i < N; i++)
		nodes[i].print();
	printf("\n");

	Mat m;
	A1d.convertTo(m, CV_64F);
	showTinyMat(title, m);
}


void CmShow::mulChannelMat(CMat &mulChaMatNf, CStr &title, int numShow)
{
	printf("An %dX%d mat with %d channels, with range:\n", mulChaMatNf.rows, mulChaMatNf.cols, mulChaMatNf.channels());
	vecM mats;
	split(mulChaMatNf, mats);
	numShow = min((int)mats.size(), numShow);
	for (int i = 0; i < numShow; i++){
		double minV, maxV;
		minMaxLoc(mats[i], &minV, &maxV);
		printf("\t%d[%g %g]\n", i, minV, maxV);
		normalize(mats[i], mats[i], 0, 1, NORM_MINMAX);
		SaveShow(mats[i], format(_S(title), i));
		waitKey(1);
	}
}