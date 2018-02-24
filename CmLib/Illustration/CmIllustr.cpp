#include "StdAfx.h"
#include "CmIllustr.h"

void CmIllustr::ImgsOptions::Initial(CStr &wkDir)
{
	_w = 400, _H = 2800;
	_inDir = wkDir + "/Sal/";
	_outDir = wkDir + "/Supp/";
	_texName = wkDir + "SupFig.tex";
}

Mat CmIllustr::Retreival(CStr wkDir, vector<vecM> &subImgs, const char* nameW[], int H, int W)
{
	bool toRow = W > H;
	vecM finImgs;
	vecD finLens;
	int TYPE_NUM = (int)subImgs.size();
	vector<vecD> subLens(TYPE_NUM);
	for (int i = 0; i < TYPE_NUM; i++) {
		LoadImgs(wkDir + nameW[i], subImgs[i], subLens[i], W, H);
		finImgs.push_back(ArrangeImgs(subImgs[i], subLens[i], W, H, toRow));
		finLens.push_back(min(W, H));
	}

	if (toRow)
		H = H * TYPE_NUM + (TYPE_NUM - 1) * space;
	else
		W = W * TYPE_NUM + (TYPE_NUM - 1) * space;
	return ArrangeImgs(finImgs, finLens, W, H, !toRow);
}

// load format(imgW, i) and add information to the back of imgs and lens
void CmIllustr::LoadImgs(CStr &imgW, vecM &imgs, vecD &lens, int W, int H)
{
	bool toRow = W > H;
	double crnt = -space;
	if (imgs.size()){ // There exist a predefined image for sketch
		lens.push_back(toRow ? H*imgs[0].cols*1./imgs[0].rows : W*imgs[0].rows*1./imgs[0].cols);
		crnt += lens[0] + space;
	}

	for (int i = 0; i < 500; i++){
		string imgN = format(_S(imgW), i), inDir, maskN;
		vecS names;
		int subN = CmFile::GetNames(imgN, names, inDir);
		if (subN == 0)
			continue;
		Mat img = imread(inDir + names[0]);
		if (img.data == NULL){
			printf("Can't load image file %-70s\n", _S(names[0]));
			continue;
		}
		if (subN > 1){
			Mat mask1u = imread(inDir + names[1], CV_LOAD_IMAGE_GRAYSCALE), big1u;
			dilate(mask1u, big1u, Mat(), Point(-1, -1), 5);
			bitwise_xor(mask1u, big1u, mask1u);
			img.setTo(Scalar(0, 0, 255), mask1u);
		}

		lens.push_back(toRow ? H*img.cols*1./img.rows : W*img.rows*1./img.cols);
		imgs.push_back(img);
		crnt += lens[lens.size() - 1] + space;
		if (crnt >= max(H, W))
			break;
	}
	int num = imgs.size();
	if (num && abs(crnt - max(H,W)) > abs(crnt - lens[num - 1] - space - max(H,W)))
		imgs.resize(num - 1), lens.resize(num - 1);

	printf("%s: %d\n", _S(imgW), num);
	if (crnt < max(H, W))	{
		printf(_S(imgW + ": not enough images\n"));
		exit(0);
	}
}

Mat CmIllustr::ArrangeImgs(vecM &imgs, vecD &len, int W, int H, bool toRow)
{
	int imgN = (int)(imgs.size()), s = 0;
	CV_Assert(len.size() == imgN);
	double ratio, sumL = 0, err = 0;
	for (int i = 0; i < imgN; i++)
		sumL += len[i]; 

	ratio = ((toRow ? W : H) - (imgN - 1) * space) / sumL;
	Mat dstImg(H, W, CV_8UC3);
	dstImg = Scalar(255, 255, 255);
	for (int i = 0; i < imgN; i++)	{
		len[i] *= ratio;
		int l = cvRound(len[i] + err);
		Rect reg = toRow ? Rect(s, 0, l, H) : Rect(0, s, W, l);
		resize(imgs[i], dstImg(reg), reg.size());
		err = len[i] + err - l;
		s += l + space;
	}
	CV_Assert(s - space == (toRow ? dstImg.cols : dstImg.rows));
	return dstImg;
}

void CmIllustr::Imgs(const ImgsOptions &opts, int maxImgNum)
{
	vecS names;
	CmFile::MkDir(opts._outDir);
	int imgNum = CmFile::GetNamesNE(opts._inDir + "*" + opts._exts[0], names);
	FILE *f = fopen(_S(opts._texName), "w");
	CV_Assert(f != NULL);

	//* Sort image names in order
	vector<pair<int, string>> costIdx(imgNum);
	for (int i = 0; i < imgNum; i++)
		costIdx[i] = make_pair(atoi(_S(names[i])), names[i]);
	sort(costIdx.begin(), costIdx.end());
	for (int i = 0; i < imgNum; i++)
		names[i] = costIdx[i].second;
	//*/
	imgNum = min(imgNum, maxImgNum);

	vecI heights(imgNum);
	for (int i = 0; i < imgNum; i++){
		Mat img = imread(opts._inDir + names[i] + opts._exts[0]);
		heights[i] = (img.rows * opts._w + img.cols/2) / img.cols;
	}

	vecS subNames;
	vecI subHeights;
	int height = -space;
	for (int i = 0; i < imgNum; i++) {
		height += heights[i] + space;
		subNames.push_back(names[i]);
		subHeights.push_back(heights[i]);
		if (height > opts._H){
			height = 0;
			WriteFigure(subNames, f, subHeights, opts);
			subNames.clear();
			subHeights.clear();
		}
	}
	WriteFigure(subNames, f, subHeights, opts);
	fclose(f);
	printf("%70s\r", "");
}

void CmIllustr::WriteFigure(const vecS &names, FILE *f, const vecI &sHeights, const ImgsOptions &opts)
{
	static int idx = -1;
	printf("Output %dth big image\n", ++idx);
	{//* Produce a big figure
		Size sz(opts._w * opts._exts.size() + space * (opts._exts.size() - 1), 40 - space);
		for (size_t i = 0; i < names.size(); i++)
			sz.height += space + sHeights[i];
		Mat bImg(sz, CV_8UC3);
		memset(bImg.data, -1, bImg.step * sz.height);
		Rect reg = Rect(0, 0, opts._w, 0);
		for (size_t i = 0; i < names.size(); i++) {
			reg.x = 0;
			reg.height = sHeights[i];
			for (size_t j = 0; j < opts._exts.size(); j++) {
				Mat subImg = bImg(reg);
				Mat crntImg = imread(opts._inDir + names[i] + opts._exts[j]);
				if (crntImg.data != NULL)
					resize(crntImg, subImg, subImg.size(), INTER_AREA);
				reg.x += space + opts._w;
			}
			reg.y += space + sHeights[i];
		}
		imwrite(opts._outDir + format("%d.jpg", idx), bImg);
	}//*/

	fprintf(f, "\\begin{figure*}[t!]\n\t\\centering\n");
	fprintf(f, "\t\\begin{overpic}[width=\\textwidth]{%d.jpg} \n", idx); // \\footnotesize
	fprintf(f, "\t\\PutDes\n\t\\end{overpic}\n");  
	fprintf(f, "\t\\PutCap\n\\end{figure*}\n\\clearpage\n\n"); // \\clearpage
}

void CmIllustr::Demo(CStr &wkDir, int height)
{
	/*/
	CStr typeN = CmFile::GetNameNE(wkDir.substr(0, wkDir.size() - 1));
	//CStr srcNames = string("Src/%d") + typeN + "_*.jpg";
	CStr srcNames = string("Src/%.3d") + ".jpg";
	const int TYPE_NUM = 3;
	vector<vecM> subImgs(TYPE_NUM);
	subImgs[1].push_back(imread(wkDir + "Sketch.jpg"));
	const char* nameW[TYPE_NUM] = {_S(srcNames), "Sort1/%.3dSr*.*", "Sort1/%.3dSHOG*.jpg"};
	Mat showRes = CmIllustr::Retreival(wkDir, subImgs, nameW, 200, height); 
	imwrite(wkDir + typeN + ".jpg", showRes); //*/

	//*/
	const char* _exts[] = {".jpg", "_S0.png", "_S1DR.png", "_S2DR.png", "_AC2.png", "_G.png"};
	vecS exts = charPointers2StrVec(_exts);
	CmIllustr::ImgsOptions opts(wkDir, exts);
	CmIllustr::Imgs(opts, 200); //*/
}