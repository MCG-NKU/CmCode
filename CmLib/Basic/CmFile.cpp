#include "StdAfx.h"

#include <shlobj.h>
#include <Commdlg.h>
#include <ShellAPI.h>


BOOL CmFile::MkDir(CStr&  _path)
{
	if(_path.size() == 0)
		return FALSE;

	static char buffer[1024];
	strcpy(buffer, _S(_path));
	for (int i = 0; buffer[i] != 0; i ++) {
		if (buffer[i] == '\\' || buffer[i] == '/') {
			buffer[i] = '\0';
			CreateDirectoryA(buffer, 0);
			buffer[i] = '/';
		}
	}
	return CreateDirectoryA(_S(_path), 0);
}


string CmFile::BrowseFolder()   
{
	static char Buffer[MAX_PATH];
	BROWSEINFOA bi;//Initial bi 	
	bi.hwndOwner = NULL; 
	bi.pidlRoot = NULL;
	bi.pszDisplayName = Buffer; // Dialog can't be shown if it's NULL
	bi.lpszTitle = "BrowseFolder";
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.iImage = NULL;


	LPITEMIDLIST pIDList = SHBrowseForFolderA(&bi); // Show dialog
	if(pIDList)	{	
		SHGetPathFromIDListA(pIDList, Buffer);
		if (Buffer[strlen(Buffer) - 1]  == '\\')
			Buffer[strlen(Buffer) - 1] = 0;

		return string(Buffer);
	}
	return string();   
}

string CmFile::BrowseFile(const char* strFilter, bool isOpen)
{
	static char Buffer[MAX_PATH];
	OPENFILENAMEA   ofn;  
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = Buffer;
	ofn.lpstrFile[0] = '\0';   
	ofn.nMaxFile = MAX_PATH;   
	ofn.lpstrFilter = strFilter;   
	ofn.nFilterIndex = 1;    
	ofn.Flags = OFN_PATHMUSTEXIST;   

	if (isOpen) {
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		GetOpenFileNameA(&ofn);
		return Buffer;
	}

	GetSaveFileNameA(&ofn);
	return string(Buffer);

}

int CmFile::Rename(CStr& _srcNames, CStr& _dstDir, const char *nameCommon, const char *nameExt)
{
	vecS names;
	string inDir;
	int fNum = GetNames(_srcNames, names, inDir);
	for (int i = 0; i < fNum; i++) {
		string dstName = format("%s\\%.4d%s.%s", _S(_dstDir), i, nameCommon, nameExt);
		string srcName = inDir + names[i];
		::CopyFileA(srcName.c_str(), dstName.c_str(), FALSE);
	}
	return fNum;
}

int CmFile::ChangeImgFormat(CStr &imgW, CStr dstW)
{
	vecS names;
	string inDir, ext = GetExtention(imgW);
	int iNum = GetNames(imgW, names, inDir);
#pragma omp parallel for
	for (int i = 0; i < iNum; i++) {
		Mat img = imread(inDir + names[i]);
		imwrite(format(_S(dstW), _S(GetNameNE(names[i]))), img);
	}
	return iNum;
}

void CmFile::RenameSuffix(CStr dir, CStr orgSuf, CStr dstSuf)
{
	vecS namesNS;
	int fNum = CmFile::GetNamesNoSuffix(dir + "*" + orgSuf, namesNS, orgSuf);
	for (int i = 0; i < fNum; i++)
		CmFile::Move(dir + namesNS[i] + orgSuf, dir + namesNS[i] + dstSuf);
}

void CmFile::RmFolder(CStr& dir)
{
	CleanFolder(dir);
	if (FolderExist(dir))
		RunProgram("Cmd.exe", format("/c rmdir /s /q \"%s\"", _S(dir)), true, false);
}

void CmFile::CleanFolder(CStr& dir, bool subFolder)
{
	vecS names;
	int fNum = CmFile::GetNames(dir + "/*.*", names);
	for (int i = 0; i < fNum; i++)
		RmFile(dir + "/" + names[i]);

	vecS subFolders;
	int subNum = GetSubFolders(dir, subFolders);
	if (subFolder)
		for (int i = 0; i < subNum; i++)
			CleanFolder(dir + "/" + subFolders[i], true);
}

int CmFile::GetSubFolders(CStr& folder, vecS& subFolders)
{
	subFolders.clear();
	WIN32_FIND_DATAA fileFindData;
	string nameWC = folder + "\\*";
	HANDLE hFind = ::FindFirstFileA(nameWC.c_str(), &fileFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	do {
		if (fileFindData.cFileName[0] == '.')
			continue; // filter the '..' and '.' in the path
		if (fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			subFolders.push_back(fileFindData.cFileName);
	} while (::FindNextFileA(hFind, &fileFindData));
	FindClose(hFind);
	return (int)subFolders.size();
}

// Get image names from a wildcard. Eg: GetNames("D:\\*.jpg", imgNames);
int CmFile::GetNames(CStr &nameW, vecS &names, string &dir)
{
	dir = GetFolder(nameW);
	names.clear();
	names.reserve(10000);
	WIN32_FIND_DATAA fileFindData;
	HANDLE hFind = ::FindFirstFileA(_S(nameW), &fileFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	do{
		if (fileFindData.cFileName[0] == '.')
			continue; // filter the '..' and '.' in the path
		if (fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue; // Ignore sub-folders
		names.push_back(fileFindData.cFileName);
	} while (::FindNextFileA(hFind, &fileFindData));
	FindClose(hFind);
	return (int)names.size();
}

int CmFile::GetNames(CStr& rootFolder, CStr &fileW, vecS &names)
{
	GetNames(rootFolder + fileW, names);
	vecS subFolders, tmpNames;
	int subNum = CmFile::GetSubFolders(rootFolder, subFolders);
	for (int i = 0; i < subNum; i++){
		subFolders[i] += "/";
		int subNum = GetNames(rootFolder + subFolders[i], fileW, tmpNames);
		for (int j = 0; j < subNum; j++)
			names.push_back(subFolders[i] + tmpNames[j]);
	}
	return (int)names.size();
}

int CmFile::GetNamesNE(CStr& nameWC, vecS &names, string &dir, string &ext)
{
	int fNum = GetNames(nameWC, names, dir);
	ext = GetExtention(nameWC);
	for (int i = 0; i < fNum; i++)
		names[i] = GetNameNE(names[i]);
	return fNum;
}

int CmFile::GetNamesNE(CStr& rootFolder, CStr &fileW, vecS &names)
{
	int fNum = GetNames(rootFolder, fileW, names);
	int extS = (int)GetExtention(fileW).size();
	for (int i = 0; i < fNum; i++)
		names[i].resize(names[i].size() - extS);
	return fNum;
}

int CmFile::GetNamesNoSuffix(CStr& nameWC, vecS &namesNS, CStr suffix, string &dir)
{
	int fNum = CmFile::GetNames(nameWC, namesNS, dir);
	for (int i = 0; i < fNum; i++)
		namesNS[i] = GetNameNoSuffix(namesNS[i], suffix);
	return fNum;
}

// Load mask image and threshold thus noisy by compression can be removed
Mat CmFile::LoadMask(CStr& fileName)
{
	Mat mask = imread(fileName, CV_LOAD_IMAGE_GRAYSCALE);
	CV_Assert_(mask.data != NULL, ("Can't find mask image: %s", _S(fileName)));
	compare(mask, 128, mask, CV_CMP_GT);
	return mask;
}

BOOL CmFile::Move2Dir(CStr &srcW, CStr dstDir)
{
	vecS names;
	string inDir;
	int fNum = CmFile::GetNames(srcW, names, inDir);
	BOOL r = TRUE;
	for (int i = 0; i < fNum; i++)	
		if (Move(inDir + names[i], dstDir + names[i]) == FALSE)
			r = FALSE;
	return r;
}

BOOL CmFile::Copy2Dir(CStr &srcW, CStr dstDir)
{
	vecS names;
	string inDir;
	int fNum = CmFile::GetNames(srcW, names, inDir);
	BOOL r = TRUE;
	for (int i = 0; i < fNum; i++)	
		if (Copy(inDir + names[i], dstDir + names[i]) == FALSE)
			r = FALSE;
	return r;
}

void CmFile::ChkImgs(CStr &imgW)
{
	vecS names;
	string inDir;
	int imgNum = GetNames(imgW, names, inDir);
	printf("Checking %d images: %s\n", imgNum, _S(imgW));
	for (int i = 0; i < imgNum; i++){
		Mat img = imread(inDir + names[i]);
		if (img.data == NULL)
			printf("Loading file %s failed\t\t\n", _S(names[i]));
		if (i % 200 == 0)
			printf("Processing %2.1f%%\r", (i*100.0)/imgNum);
	}
	printf("\t\t\t\t\r");
}


void CmFile::AppendStr(CStr fileName, CStr str)
{
	FILE *f = fopen(_S(fileName), "a");
	if (f == NULL){
		printf("File %s can't be opened\n", _S(fileName));
		return;
	}
	fprintf(f, "%s", _S(str));
	fclose(f);
}


void CmFile::RunProgram(CStr &fileName, CStr &parameters, bool waiteF, bool showW)
{
	string runExeFile = fileName;
#ifdef _DEBUG
	runExeFile.insert(0, "..\\Debug\\");
#else
	runExeFile.insert(0, "..\\Release\\");
#endif // _DEBUG
	if (!CmFile::FileExist(_S(runExeFile)))
		runExeFile = fileName;

	SHELLEXECUTEINFOA  ShExecInfo  =  {0};  
	ShExecInfo.cbSize  =  sizeof(SHELLEXECUTEINFO);  
	ShExecInfo.fMask  =  SEE_MASK_NOCLOSEPROCESS;  
	ShExecInfo.hwnd  =  NULL;  
	ShExecInfo.lpVerb  =  NULL;  
	ShExecInfo.lpFile  =  _S(runExeFile);
	ShExecInfo.lpParameters  =  _S(parameters);         
	ShExecInfo.lpDirectory  =  NULL;  
	ShExecInfo.nShow  =  showW ? SW_SHOW : SW_HIDE;  
	ShExecInfo.hInstApp  =  NULL;              
	ShellExecuteExA(&ShExecInfo);  

	//printf("Run: %s %s\n", ShExecInfo.lpFile, ShExecInfo.lpParameters);

	if (waiteF)
		WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
}

string CmFile::GetCompName() 
{
	char buf[1024];
	DWORD dwCompNameLen = 1024;
	GetComputerNameA(buf, &dwCompNameLen);
	return string(buf);
}

void CmFile::SegOmpThrdNum(double ratio /* = 0.8 */)
{
	int thrNum = omp_get_max_threads();
	int usedNum = cvRound(thrNum * ratio);
	usedNum = max(usedNum, 1);
	//printf("Number of CPU cores used is %d/%d\n", usedNum, thrNum);
	omp_set_num_threads(usedNum);
}


// Copy files and add suffix. e.g. copyAddSuffix("./*.jpg", "./Imgs/", "_Img.jpg")
void CmFile::copyAddSuffix(CStr &srcW, CStr &dstDir, CStr &dstSuffix)
{
	vecS namesNE;
	string srcDir, srcExt;
	int imgN = CmFile::GetNamesNE(srcW, namesNE, srcDir, srcExt);
	CmFile::MkDir(dstDir);
	for (int i = 0; i < imgN; i++)
		CmFile::Copy(srcDir + namesNE[i] + srcExt, dstDir + namesNE[i] + dstSuffix);
}

vecS CmFile::loadStrList(CStr &fName)
{
	ifstream fIn(fName);
	string line;
	vecS strs;
	while(getline(fIn, line) && line.size())
		strs.push_back(line);
	return strs;
}


// Write matrix to binary file
bool CmFile::matWrite(CStr& filename, CMat& _M){
	Mat M;
	_M.copyTo(M);
	FILE* file = fopen(_S(filename), "wb");
	if (file == NULL || M.empty())
		return false;
	fwrite("CmMat", sizeof(char), 5, file);
	int headData[3] = {M.cols, M.rows, M.type()};
	fwrite(headData, sizeof(int), 3, file);
	fwrite(M.data, sizeof(char), M.step * M.rows, file);
	fclose(file);
	return true;
}

/****************************************************************************/
// Read matrix from binary file
bool CmFile::matRead( const string& filename, Mat& _M){
	FILE* f = fopen(_S(filename), "rb");
	if (f == NULL)
		return false;
	char buf[8];
	int pre = (int)fread(buf,sizeof(char), 5, f);
	if (strncmp(buf, "CmMat", 5) != 0)	{
		printf("Invalidate CvMat data file %s\n", _S(filename));
		return false;
	}
	int headData[3]; // Width, height, type
	fread(headData, sizeof(int), 3, f);
	Mat M(headData[1], headData[0], headData[2]);
	fread(M.data, sizeof(char), M.step * M.rows, f);
	fclose(f);
	M.copyTo(_M);
	return true;
}

void CmFile::ZipFiles(CStr &filesW, CStr &zipFileName, int compressLevel)
{
	string param = format("u -tzip -mmt -mx%d \"%s\" \"%s\"", compressLevel, _S(zipFileName), _S(filesW));
	printf("Zip files: %s --> %s\n", _S(filesW), _S(zipFileName));
	RunProgram("7z.exe", param, true, false);
}


void CmFile::UnZipFiles(CStr &zipFileName, CStr &tgtDir, bool overwriteWarning/* = true*/)
{
	string param = format("e \"%s\" \"-o%s\" -r", _S(zipFileName), _S(tgtDir));
	if (!overwriteWarning)
		param += " -y";
	printf("UnZip files: %s --> %s\n", _S(zipFileName), _S(tgtDir));
	CmFile::RunProgram("7z.exe", param, true, overwriteWarning);
}
