#include "StdAfx.h"
#include "CmEvaluation.h"

int CmEvaluation::STEP = 1;
const int CmEvaluation::MI =  COLOR_NUM / STEP + 1;

void CmEvaluation::Evaluate(CStr gtW, CStr &salDir, CStr &resName, vecS &des)
{
	int TN = des.size(); // Type Number of different methods
	vector<vecD> precision(TN), recall(TN), tpr(TN), fpr(TN);
	static const int CN = 21; // Color Number 
	static const char* c[CN] = {"'k'", "'b'", "'g'", "'r'", "'c'", "'m'", "'y'",
		"':k'", "':b'", "':g'", "':r'", "':c'", "':m'", "':y'", 
		"'--k'", "'--b'", "'--g'", "'--r'", "'--c'", "'--m'", "'--y'"
	};
	FILE* f = fopen(_S(resName), "w");
	CV_Assert(f != NULL);
	fprintf(f, "clear;\nclose all;\nclc;\n\n\n%%%%\nfigure(1);\nhold on;\n");
	vecD thr(MI);
	for (int i = 0; i < MI; i++)
		thr[i] = i * STEP;
	PrintVector(f, thr, "Threshold");
	fprintf(f, "\n");

	for (int i = 0; i < TN; i++)
		Evaluate_(gtW, salDir, "_" + des[i] + ".png", precision[i], recall[i], tpr[i], fpr[i]); //Evaluate(salDir + "*" + des[i] + ".png", gtW, val[i], recall[i], t);

	string leglendStr("legend(");
	vecS strPre(TN), strRecall(TN), strTpr(TN), strFpr(TN);
	for (int i = 0; i < TN; i++){
		strPre[i] = format("Precision_%s", _S(des[i]));
		strRecall[i] = format("Recall_%s", _S(des[i]));
		strTpr[i] = format("TPR_%s", _S(des[i]));
		strFpr[i] = format("FPR_%s", _S(des[i]));
		PrintVector(f, recall[i], strRecall[i]);
		PrintVector(f, precision[i], strPre[i]);
		PrintVector(f, tpr[i], strTpr[i]);
		PrintVector(f, fpr[i], strFpr[i]);
		fprintf(f, "plot(%s, %s, %s, 'linewidth', 2);\n", _S(strRecall[i]), _S(strPre[i]), c[i % CN]);
		leglendStr += format("'%s', ",  _S(des[i]));
	}
	leglendStr.resize(leglendStr.size() - 2);
	leglendStr += ");";
	string xLabel = "label('Recall');\n";
	string yLabel = "label('Precision')\n";
	fprintf(f, "hold off;\nx%sy%s\n%s\ngrid on;\naxis([0 1 0 1]);\n", _S(xLabel), _S(yLabel), _S(leglendStr));


	fprintf(f, "\n\n\n%%%%\nfigure(2);\nhold on;\n");
	for (int i = 0; i < TN; i++)
		fprintf(f, "plot(%s, %s,  %s, 'linewidth', 2);\n", _S(strFpr[i]), _S(strTpr[i]), c[i % CN]);
	xLabel = "label('False positive rate');\n";
	yLabel = "label('True positive rate')\n";
	fprintf(f, "hold off;\nx%sy%s\n%s\ngrid on;\naxis([0 1 0 1]);\n\n\n%%%%\nfigure(3)\n", _S(xLabel), _S(yLabel), _S(leglendStr));

	vecD areaROC(TN);
	for (int i = 0; i < TN; i++){
		areaROC[i] = 0;
		CV_Assert(fpr[i].size() == tpr[i].size());
		for (size_t t = 1; t < fpr[i].size(); t++)
			areaROC[i] += (tpr[i][t] + tpr[i][t - 1]) * (fpr[i][t - 1] - fpr[i][t]) / 2.0;
		fprintf(f, "%%ROC_%s: %g\n", _S(des[i]), areaROC[i]);
	}
	PrintVector(f, areaROC, "AUC");
	fprintf(f, "\nbar(AUC)\n\n\n\n%%%\nfigure(4)\nbeta = 0.3;\n");

	string meanFMeasure = "MeanFMeasure = [", maxFMeasure = "MaxFMeasure = [";
	for (int i = 0; i < TN; i++){
		const char* dStr = _S(des[i]);
		string fMeasureStr = format("FMeasure_%s", dStr);
		fprintf(f, "%s = ((1+beta)*Precision_%s .* Recall_%s) ./ (beta * Precision_%s + Recall_%s);\n", _S(fMeasureStr), dStr, dStr, dStr, dStr);
		meanFMeasure += "mean(" + fMeasureStr + ") ";
		maxFMeasure += "max(" + fMeasureStr + ") ";
	}
	fprintf(f, "%s];\n%s];\nbar([MeanFMeasure; MaxFMeasure]');\nlegend('Mean F_\\beta', 'Max F_\\beta');\n", _S(meanFMeasure), _S(maxFMeasure));

	fclose(f);
	printf("%-70s\r", "");
}

void CmEvaluation::PrintVector(FILE *f, const vecD &v, CStr &name)
{
	fprintf(f, "%s = [", name.c_str());
	for (size_t i = 0; i < v.size(); i++)
		fprintf(f, "%g ", v[i]);
	fprintf(f, "];\n");
}

// Return the threshold when significant amount of recall reach 0
void CmEvaluation::Evaluate_(CStr &gtImgW, CStr &inDir, CStr& resExt, vecD &precision, vecD &recall, vecD &tpr, vecD &fpr)
{
	vecS names;
	string truthDir, gtExt;
	int imgNum = CmFile::GetNamesNE(gtImgW, names, truthDir, gtExt); 
	precision.resize(MI, 0);
	recall.resize(MI, 0);
	tpr.resize(MI, 0);
	fpr.resize(MI, 0);
	if (imgNum == 0){
		printf("Can't load ground truth images %s\n", _S(gtImgW));
		return;
	}
	else
		printf("Evaluating %d saliency maps ... \r", imgNum);

	for (int i = 0; i < imgNum; i++){
		printf("Evaluating %03d/%d %-40s\r", i, imgNum, _S(names[i] + resExt));
		Mat resS = imread(inDir + names[i] + resExt, CV_LOAD_IMAGE_GRAYSCALE);
		CV_Assert_(resS.data != NULL, ("Can't load saliency map: %s\n", _S(names[i]) + resExt));
		normalize(resS, resS, 0, 255, NORM_MINMAX);
		Mat gtFM = imread(truthDir + names[i] + gtExt, CV_LOAD_IMAGE_GRAYSCALE), gtBM;
		if (gtFM.data == NULL) 
			continue;
		CV_Assert_(resS.size() == gtFM.size(), ("Saliency map and ground truth image size mismatch: %s\n", _S(names[i])));
		compare(gtFM, 128, gtFM, CMP_GT);
		bitwise_not(gtFM, gtBM);
		double gtF = sum(gtFM).val[0];
		double gtB = resS.cols * resS.rows * 255 - gtF;


#pragma omp parallel for
		for (int thr = 0; thr < MI; thr++){
			Mat resM, tpM, fpM;
			compare(resS, thr * STEP, resM, CMP_GE);
			bitwise_and(resM, gtFM, tpM);
			bitwise_and(resM, gtBM, fpM);
			double tp = sum(tpM).val[0]; 
			double fp = sum(fpM).val[0];
			double fn = gtF - tp;
			double tn = gtB - fp;

			recall[thr] += tp/(gtF+EPS);
			double total = EPS + tp + fp;
			precision[thr] += (tp+EPS)/total;

			tpr[thr] += (tp + EPS) / (tp + fn + EPS);
			fpr[thr] += (fp + EPS) / (fp + tn + EPS);
		}
	}

	int thrS = 0, thrE = MI, thrD = 1;
	for (int thr = thrS; thr != thrE; thr += thrD){
		precision[thr] /= imgNum;
		recall[thr] /= imgNum;
		tpr[thr] /= imgNum;
		fpr[thr] /= imgNum;
	}
}

double CmEvaluation::interUnionBBox(const Vec4i &box1, const Vec4i &box2) // each box minx, minY, maxX, maxY
{
	int bi[4];
	bi[0] = max(box1[0], box2[0]);
	bi[1] = max(box1[1], box2[1]);
	bi[2] = min(box1[2], box2[2]);
	bi[3] = min(box1[3], box2[3]);	

	double iw = bi[2] - bi[0] + 1;
	double ih = bi[3] - bi[1] + 1;
	double ov = 0;
	if (iw>0 && ih>0){
		double ua = (box1[2]-box1[0]+1)*(box1[3]-box1[1]+1)+(box2[2]-box2[0]+1)*(box2[3]-box2[1]+1)-iw*ih;
		ov = iw*ih/ua;
	}	
	return ov;
}

void CmEvaluation::EvalueMask(CStr gtW, CStr &maskDir, CStr &gtExt, CStr &maskExt, bool alertNul)
{
	vecS names;
	string gtDir;
	int imgNum = CmFile::GetNames(gtW, names, gtDir);
	int count = 0, gtExtLen = (int)gtExt.size();
	double p = 0, r = 0, fn = 0;
	for (int i = 0; i < imgNum; i++){
		printf("Processing %-40s\r", _S(names[i]));
		Mat truM = imread(gtDir + names[i], CV_LOAD_IMAGE_GRAYSCALE), truInvM;
		names[i].resize(names[i].size() - gtExtLen);
		Mat res = imread(maskDir + names[i] + maskExt, CV_LOAD_IMAGE_GRAYSCALE);
		if (truM.data == NULL || res.data == NULL || truM.size != res.size){
			if (alertNul)
				printf("Truth(%d, %d), Res(%d, %d): %s\n", truM.cols, truM.rows, res.cols, res.rows, _S(names[i] + maskExt));
			continue;
		}
		compare(truM, 128, truM, CMP_GE);
		compare(res, 128, res, CMP_GE);
		Mat commMat;
		bitwise_and(truM, res, commMat);
		double commV = sum(commMat).val[0];
		p += commV/(sum(res).val[0] + EPS);
		r += commV/(sum(truM).val[0] + EPS);
		count++;
	}	
	p /= count, r /= count;
	double f = 1.3 * p * r / (0.3 * p + r);
	printf("%s: precision = %.3g, recall = %.3g, FMeasure = %.3g\n", _S(maskExt), p, r, f);
}

void CmEvaluation::EvalueMask(CStr gtW, CStr &maskDir, CStr &gtExt, vecS &des, CStr resFile, double betaSqr, bool alertNul)
{
	vecS namesNS; 
	string gtDir;
	int imgNum = CmFile::GetNamesNoSuffix(gtW, namesNS, gtExt, gtDir);
	int methodNum = (int)des.size();
	vecD pr(methodNum), rec(methodNum), count(methodNum), fm(methodNum);
	for (int i = 0; i < imgNum; i++){
		Mat truM = imread(gtDir + namesNS[i] + gtExt, CV_LOAD_IMAGE_GRAYSCALE);
		for (int m = 0; m < methodNum; m++)	{
			Mat res = imread(maskDir + namesNS[i] + "_" + des[m] + ".png", CV_LOAD_IMAGE_GRAYSCALE);
			if (truM.data == NULL || res.data == NULL || truM.size != res.size){
				if (alertNul)
					printf("Truth(%d, %d), Res(%d, %d): %s\n", truM.cols, truM.rows, res.cols, res.rows, _S(namesNS[i] + "_" + des[m] + ".png"));
				continue;
			}
			compare(truM, 128, truM, CMP_GE);
			compare(res, 128, res, CMP_GE);
			Mat commMat;
			bitwise_and(truM, res, commMat);
			double commV = sum(commMat).val[0];
			double p = commV/(sum(res).val[0] + EPS);
			double r = commV/(sum(truM).val[0] + EPS);
			pr[m] += p;
			rec[m] += r;
			count[m]++;
		}
	}

	for (int m = 0; m < methodNum; m++)
		pr[m] /= count[m], rec[m] /= count[m];

	FILE *f; 
	fopen_s(&f, _S(resFile), "w");
	CV_Assert(f != NULL);
	CmEvaluation::PrintVector(f, pr, "Precision");
	CmEvaluation::PrintVector(f, rec, "Recall");
	fprintf(f, "BetaSqr = %g;\nFMeasure = Precision .* Recall * (1 + BetaSqr) ./ (Precision * BetaSqr + Recall);\nbar(FMeasure);\n", betaSqr);
	fclose(f);
}

double CmEvaluation::FMeasure(CMat &res, CMat &truM)
{
	Mat commMat;
	bitwise_and(truM, res, commMat);
	double commV = sum(commMat).val[0];
	double p = commV/(sum(res).val[0] + EPS);
	double r = commV/(sum(truM).val[0] + EPS);
	return 1.3*p*r / (0.3 *p + r);
}

void CmEvaluation::MeanAbsoluteError(CStr &inDir, CStr &salDir, vecS &des, CStr resFileName)
{
	vecS namesNE;
	int imgNum = CmFile::GetNamesNE(inDir + "*.jpg", namesNE);
	vecD costs(des.size());
	for (int i = 0; i < imgNum; i++){
		Mat gt = imread(inDir + namesNE[i] + ".png", CV_LOAD_IMAGE_GRAYSCALE);
		//printf("%s.jpg ", _S(namesNE[i]));
		gt.convertTo(gt, CV_32F, 1.0/255);
		for (size_t j = 0; j < des.size(); j++){
			Mat res = imread(salDir + namesNE[i] + "_" + des[j] + ".png", CV_LOAD_IMAGE_GRAYSCALE);
			CV_Assert_(res.data != NULL, ("Can't load file %s\n", _S(namesNE[i] + "_" + des[j] + ".png")));
			if (res.size != gt.size){
				printf("size don't match %s\n", _S(namesNE[i] + "_" + des[j] + ".png"));
				resize(res, res, gt.size());
				imwrite(string("C:/WkDir/Saliency/DataFT/Out/") + namesNE[i] + "_" + des[j] + ".png", res);
			}
			res.convertTo(res, CV_32F, 1.0/255);
			cv::absdiff(gt, res, res);
			costs[j] += sum(res).val[0] / (gt.rows * gt.cols);
		}
	}

	for (size_t j = 0; j < des.size(); j++){
		costs[j] /= imgNum;
		printf("%s:%g	", _S(des[j]), costs[j]);
	}
	printf("\n");

	if (resFileName.size())	{
		FILE *f = fopen(_S(resFileName), "w");
		CV_Assert_(f != NULL, ("Can't open %s\n", _S(resFileName)));
		for (size_t j = 0; j < des.size(); j++)
			fprintf(f, "%%MAE_%s:%g\n", _S(des[j]), costs[j]);
		PrintVector(f, costs, "MAE");
		fprintf(f, "bar(MAE);\n");
		fclose(f);
	}
}
