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
	names.reserve(6000);
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
	int extS = GetExtention(fileW).size();
	for (int i = 0; i < fNum; i++)
		names[i].resize(names[i].size() - extS);
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

	//CmLog::LogLine("Run: %s %s\n", ShExecInfo.lpFile, ShExecInfo.lpParameters);

	if (waiteF)
		WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
}

void CmFile::SegOmpThrdNum(double ratio /* = 0.8 */)
{
	int thrNum = omp_get_max_threads();
	int usedNum = cvRound(thrNum * ratio);
	usedNum = max(usedNum, 1);
	//CmLog::LogLine("Number of CPU cores used is %d/%d\n", usedNum, thrNum);
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

bool CmFile::writeStrList(CStr &fName, const vecS &strs)
{
	FILE *f = fopen(_S(fName), "w");
	if (f == NULL)
		return false;
	for (size_t i = 0; i < strs.size(); i++)
		fprintf(f, "%s\n", _S(strs[i]));
	fclose(f);
	return true;
}