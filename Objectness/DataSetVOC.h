/************************************************************************/
/* This source code is free for both academic and industry use.         */
/* Some important information for better using the source code could be */
/* found in the project page: http://mmcheng.net/bing					*/
/************************************************************************/

#pragma once

struct DataSetVOC
{
	DataSetVOC(CStr &wkDir);
	~DataSetVOC(void);

	// Organization structure data for the dataset
	string wkDir; // Root working directory, all other directories are relative to this one
	string resDir, localDir; // Directory for saving results and local data
	string imgPathW, annoPathW; // Image and annotation path

	// Information for training and testing
	int trainNum, testNum;
	vecS trainSet, testSet; // File names (NE) for training and testing images
	vecS classNames; // Object class names
	vector<vector<Vec4i>> gtTrainBoxes, gtTestBoxes; // Ground truth bounding boxes for training and testing images
	vector<vecI> gtTrainClsIdx, gtTestClsIdx; // Object class indexes  


	// Load annotations
	void loadAnnotations();

	static bool cvt2OpenCVYml(CStr &annoDir); // Needs to call yml.m in this solution before running this function.

	static bool importSaliencyBench(CStr &salDir = "./THUS10000/", CStr &vocDir = "./THUS10000/");

	static void importPaulData(CStr &inData = "Z:/datasets/toMing/", CStr &outData = "D:/WkDir/DetectionProposals/Paul/");

	static bool importImageNetBenchMark(CStr &orgDir = "D:/WkDir/ImageNet/", CStr &newDir = "D:/WkDir/DetectionProposals/ImgNet/");

	static inline double interUnio(const Vec4i &box1, const Vec4i &box2);

	// Get training and testing for demonstrating the generative of the objectness over classes
	void getTrainTest(); 

public: // Used for testing the ability of generic over classes
	void loadDataGenericOverCls();

private:
	void loadBox(FileNode &fn, vector<Vec4i> &boxes, vecI &clsIdx);
	bool loadBBoxes(CStr &nameNE, vector<Vec4i> &boxes, vecI &clsIdx);
	static void getXmlStrVOC(CStr &fName, string &buf);
	static inline string keepXmlChar(CStr &str);
	static bool cvt2OpenCVYml(CStr &yamlName, CStr &ymlName); // Needs to call yml.m in this solution before running this function.
};

string DataSetVOC::keepXmlChar(CStr &_str)
{
	string str = _str;
	int sz = (int)str.size(), count = 0;
	for (int i = 0; i < sz; i++){
		char c = str[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '.')
			str[count++] = str[i];
	}
	str.resize(count);
	return str;
}

double DataSetVOC::interUnio(const Vec4i &bb, const Vec4i &bbgt)
{
	int bi[4];
	bi[0] = max(bb[0], bbgt[0]);
	bi[1] = max(bb[1], bbgt[1]);
	bi[2] = min(bb[2], bbgt[2]);
	bi[3] = min(bb[3], bbgt[3]);	

	double iw = bi[2] - bi[0] + 1;
	double ih = bi[3] - bi[1] + 1;
	double ov = 0;
	if (iw>0 && ih>0){
		double ua = (bb[2]-bb[0]+1)*(bb[3]-bb[1]+1)+(bbgt[2]-bbgt[0]+1)*(bbgt[3]-bbgt[1]+1)-iw*ih;
		ov = iw*ih/ua;
	}	
	return ov;
}
