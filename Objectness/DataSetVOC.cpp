#include "StdAfx.h"
#include "DataSetVOC.h"


DataSetVOC::DataSetVOC(CStr &_wkDir)
{
	wkDir = _wkDir;
	resDir = wkDir + "Results/";
	localDir = wkDir + "Local/";
	imgPathW = wkDir + "JPEGImages/%s.jpg";
	annoPathW = wkDir + "Annotations/%s.yml";
	CmFile::MkDir(resDir);
	CmFile::MkDir(localDir);

	trainSet = CmFile::loadStrList(wkDir + "ImageSets/Main/TrainVal.txt");
	testSet = CmFile::loadStrList(wkDir + "ImageSets/Main/Test.txt");
	classNames = CmFile::loadStrList(wkDir + "ImageSets/Main/class.txt");

	// testSet.insert(testSet.end(), trainSet.begin(), trainSet.end());	
	// testSet.resize(min(1000, (int)testSet.size()));

	trainNum = trainSet.size();
	testNum = testSet.size();
}


Vec4i getMaskRange(CMat &mask1u, int ext = 0)
{
	int maxX = INT_MIN, maxY = INT_MIN, minX = INT_MAX, minY = INT_MAX, rows = mask1u.rows, cols = mask1u.cols;
	for (int r = 0; r < rows; r++)	{
		const byte* data = mask1u.ptr<byte>(r);
		for (int c = 0; c < cols; c++)
			if (data[c] > 10) {
				maxX = max(maxX, c);
				minX = min(minX, c);
				maxY = max(maxY, r);
				minY = min(minY, r);
			}
	}

	maxX = maxX + ext + 1 < cols ? maxX + ext + 1 : cols;
	maxY = maxY + ext + 1 < rows ? maxY + ext + 1 : rows;
	minX = minX - ext > 0 ? minX - ext : 0;
	minY = minY - ext > 0 ? minY - ext : 0;

	return Vec4i(minX + 1, minY + 1, maxX, maxY); // Rect(minX, minY, maxX - minX, maxY - minY);
}

bool DataSetVOC::importImageNetBenchMark(CStr &_orgDir, CStr &newDir)
{
	CStr orgDir = _orgDir + "ILSVRC2012_bbox_train_v2/";
	vecS subFolders;
	int numF = CmFile::GetSubFolders(orgDir, subFolders);
	CmFile::MkDir(newDir + "ImageSets/Main/");
	CmFile::writeStrList(newDir + "ImageSets/Main/SubFolders.txt", subFolders);
	for (int i = 0; i < numF; i++){
		printf("%d/%d: %s\n", i, numF, _S(subFolders[i]));
		cvt2OpenCVYml(orgDir + subFolders[i]);
	}

	return true;
}

bool DataSetVOC::importSaliencyBench(CStr &salDir, CStr &vocDir)
{
	string inDir, outImgDir = vocDir + "JPEGImages/", annoDir = vocDir + "Annotations/";
	CmFile::MkDir(outImgDir);
	CmFile::MkDir(annoDir);
	CmFile::MkDir(vocDir + "ImageSets/Main/");
	vecS namesNE;
	int imgNum = CmFile::GetNamesNE(salDir + "Src/*.jpg", namesNE, inDir);
	random_shuffle(namesNE.begin(), namesNE.end());
	for (int i = 0; i < imgNum; i++){
		CmFile::Copy(inDir + namesNE[i] + ".jpg", outImgDir + namesNE[i] + ".jpg");
		Mat mask1u = CmFile::LoadMask(inDir + namesNE[i] + ".png");
		Vec4i bb = getMaskRange(mask1u);
		FileStorage fs(annoDir + namesNE[i] + ".yml", FileStorage::WRITE);
		fs<<"annotation"<<"{"<<"object"<<"{"<<"bndbox"<<"{";
		fs<<"xmin"<<format("'%d'", bb[0])<<"ymin"<<format("'%d'", bb[1])<<"xmax"<<format("'%d'", bb[2])<<"ymax"<<format("'%d'", bb[3]);
		fs<<"}"<<"name"<<"Salient"<<"}"<<"}";
	}

	vecS trainSet(namesNE.begin(), namesNE.begin() + imgNum/2);
	vecS testSet(namesNE.begin() + imgNum/2, namesNE.end());
	CmFile::writeStrList(vocDir + "ImageSets/Main/TrainVal.txt", trainSet);
	CmFile::writeStrList(vocDir + "ImageSets/Main/Test.txt", testSet);
	vecS classNames;
	classNames.push_back("Salient");
	CmFile::writeStrList(vocDir + "ImageSets/Main/Class.txt", classNames);
	
	return true;	
}


void DataSetVOC::importPaulData(CStr &_inDir, CStr &vocDir)
{
	string inDir, outImgDir = vocDir + "JPEGImages/", annoDir = vocDir + "Annotations/";
	CmFile::MkDir(outImgDir);
	CmFile::MkDir(annoDir);
	CmFile::MkDir(vocDir + "ImageSets/Main/");
	vecS namesNE;
	FILE *f = fopen(_S(_inDir + "Possitive.txt"), "r");
	CV_Assert(f != NULL);
	char fName[1000];
	int l, x, y, w, h;
	while (fscanf(f, "%s %d %d %d %d %d\n", fName, &l, &x, &y, &w, &h) == 6){
		string nameNE = CmFile::GetNameNE(fName);
		namesNE.push_back(nameNE);
		Mat img = imread(_inDir + fName), imgN;
		double ratio = 500.0 / max(img.cols, img.rows);
		resize(img, imgN, Size(), ratio, ratio);
		imwrite(outImgDir + nameNE + ".jpg", imgN);
		
		FileStorage fs(annoDir + nameNE + ".yml", FileStorage::WRITE);
		fs<<"annotation"<<"{"<<"object"<<"{"<<"bndbox"<<"{";
		fs<<"xmin"<<format("'%d'", 1 + cvRound(x*ratio))<<"ymin"<<format("'%d'", 1 + cvRound(y*ratio));
		fs<<"xmax"<<format("'%d'", min(imgN.cols, cvRound((x+w)*ratio)))<<"ymax"<<format("'%d'", min(imgN.rows, cvRound((y+h)*ratio)));
		fs<<"}"<<"name"<<"Salient"<<"}"<<"}";
	}
	fclose(f);

	int imgNum = namesNE.size();
	random_shuffle(namesNE.begin(), namesNE.end());
	vecS trainSet(namesNE.begin(), namesNE.begin() + imgNum/2);
	vecS testSet(namesNE.begin() + imgNum/2, namesNE.end());
	CmFile::writeStrList(vocDir + "ImageSets/Main/TrainVal.txt", trainSet);
	CmFile::writeStrList(vocDir + "ImageSets/Main/Test.txt", testSet);
	
}

DataSetVOC::~DataSetVOC(void)
{
}

void DataSetVOC::loadAnnotations()
{
	gtTrainBoxes.resize(trainNum);
	gtTrainClsIdx.resize(trainNum);
	for (int i = 0; i < trainNum; i++)
		if (!loadBBoxes(trainSet[i], gtTrainBoxes[i], gtTrainClsIdx[i]))
			return;

	gtTestBoxes.resize(testNum);
	gtTestClsIdx.resize(testNum);
	for (int i = 0; i < testNum; i++)
		if(!loadBBoxes(testSet[i], gtTestBoxes[i], gtTestClsIdx[i]))
			return;
	printf("Load annotations finished\n");
}

void DataSetVOC::loadDataGenericOverCls()
{
	vecS allSet = trainSet;
	allSet.insert(allSet.end(), testSet.begin(), testSet.end());
	int imgN = (int)allSet.size();
	trainSet.clear(), testSet.clear();
	trainSet.reserve(imgN), testSet.reserve(imgN);
	vector<vector<Vec4i>> gtBoxes(imgN);
	vector<vecI> gtClsIdx(imgN);
	for (int i = 0; i < imgN; i++){
		if (!loadBBoxes(allSet[i], gtBoxes[i], gtClsIdx[i]))
			return;
		vector<Vec4i> trainBoxes, testBoxes;
		vecI trainIdx, testIdx;
		for (size_t j = 0; j < gtBoxes[i].size(); j++)
			if (gtClsIdx[i][j] < 6){
				trainBoxes.push_back(gtBoxes[i][j]);
				trainIdx.push_back(gtClsIdx[i][j]);
			}
			else{
				testBoxes.push_back(gtBoxes[i][j]);
				testIdx.push_back(gtClsIdx[i][j]);
			}
		if (trainBoxes.size()){
			trainSet.push_back(allSet[i]);
			gtTrainBoxes.push_back(trainBoxes);
			gtTrainClsIdx.push_back(trainIdx);
		}
		else{
			testSet.push_back(allSet[i]);
			gtTestBoxes.push_back(testBoxes);
			gtTestClsIdx.push_back(testIdx);
		}
	}
	trainNum = trainSet.size();
	testNum = testSet.size();
	printf("Load annotations (generic over classes) finished\n");
}

void DataSetVOC::loadBox(FileNode &fn, vector<Vec4i> &boxes, vecI &clsIdx){
	string isDifficult;
	fn["difficult"]>>isDifficult;
	if (isDifficult == "1")
		return; 

	string strXmin, strYmin, strXmax, strYmax;
	fn["bndbox"]["xmin"] >> strXmin;
	fn["bndbox"]["ymin"] >> strYmin;
	fn["bndbox"]["xmax"] >> strXmax;
	fn["bndbox"]["ymax"] >> strYmax;
	boxes.push_back(Vec4i(atoi(_S(strXmin)), atoi(_S(strYmin)), atoi(_S(strXmax)), atoi(_S(strYmax))));

	string clsName;
	fn["name"]>>clsName;
	clsIdx.push_back(findFromList(clsName, classNames));	
	CV_Assert_(clsIdx[clsIdx.size() - 1] >= 0, ("Invalidate class name\n"));
}

bool DataSetVOC::loadBBoxes(CStr &nameNE, vector<Vec4i> &boxes, vecI &clsIdx)
{
	string fName = format(_S(annoPathW), _S(nameNE));
	FileStorage fs(fName, FileStorage::READ);
	FileNode fn = fs["annotation"]["object"];
	boxes.clear();
	clsIdx.clear();
	if (fn.isSeq()){
		for (FileNodeIterator it = fn.begin(), it_end = fn.end(); it != it_end; it++)
			loadBox(*it, boxes, clsIdx);
	}
	else
		loadBox(fn, boxes, clsIdx);
	return true;
}

// Needs to call yml.m in this solution before running this function.
bool DataSetVOC::cvt2OpenCVYml(CStr &annoDir)
{
	vecS namesNE;
	int imgNum = CmFile::GetNamesNE(annoDir + "*.yaml", namesNE);
	printf("Converting annotations to OpenCV yml format:\n");
	for (int i = 0; i < imgNum; i++){
		printf("%d/%d %s.yaml\r", i, imgNum, _S(namesNE[i]));	
		string fPath = annoDir + namesNE[i];
		cvt2OpenCVYml(fPath + ".yaml", fPath + ".yml");
	}
	return true;
}

// Needs to call yml.m in this solution before running this function.
bool DataSetVOC::cvt2OpenCVYml(CStr &yamlName, CStr &ymlName)
{	
	ifstream f(yamlName);	
	FILE *fO = fopen(_S(ymlName), "w");
	if (!f.is_open() && fO == NULL)
		return false;
	fprintf(fO, "%s\n", "%YAML:1.0\n");
	string line;

	int addIdent = 0;
	while(getline(f, line)){
		if (line.substr(0, 12) == "  filename: ")
			line = "  filename: \"" + line.substr(12) + "\"";			
		int tmp = line.find_first_of('-');
		if (tmp != string::npos){
			bool allSpace = true;
			for (int k = 0; k < tmp; k++)
				if (line[k] != ' ')
					allSpace = false;
			if (allSpace)
				addIdent = tmp;
		}
		for (int k = 0; k < addIdent; k++)
			fprintf(fO, " ");
		fprintf(fO, "%s\n", _S(line));
	}
	fclose(fO);

	FileStorage fs(ymlName, FileStorage::READ);
	string tmp;
	fs["annotation"]["folder"]>>tmp;
	return true;
}


// Get training and testing for demonstrating the generative of the objectness over classes
void DataSetVOC::getTrainTest()
{
	const int TRAIN_CLS_NUM = 6;
	string trainCls[TRAIN_CLS_NUM] = {"bird", "car", "cat", "cow", "dog", "sheep"};

}

void DataSetVOC::getXmlStrVOC(CStr &fName, string &buf)
{
	ifstream fin(fName);
	string strLine;
	buf.clear();
	buf.reserve(100000);
	buf += "<?xml version=\"1.0\"?>\n<opencv_storage>\n";
	while (getline(fin, strLine) && strLine.size())	{
		int startP = strLine.find_first_of(">") + 1;
		int endP = strLine.find_last_of("<");
		if (endP > startP){
			string val = keepXmlChar(strLine.substr(startP, endP - startP));
			if (val.size() < endP - startP)
				strLine = strLine.substr(0, startP) + val + strLine.substr(endP);
		}
		buf += strLine + "\n";
	}
	buf += "</opencv_storage>\n";
	//FileStorage fs(buf, FileStorage::READ + FileStorage::MEMORY);
	ofstream fout("D:/t.xml");
	fout<< buf;
}