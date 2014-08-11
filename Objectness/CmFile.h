#pragma once

struct CmFile
{
	static string BrowseFile(const char* strFilter = "Images (*.jpg;*.png)\0*.jpg;*.png\0All (*.*)\0*.*\0\0", bool isOpen = true);
	static string BrowseFolder(); 

	static inline string GetFolder(CStr& path);
	static inline string GetName(CStr& path);
	static inline string GetNameNE(CStr& path);
	static inline string GetPathNE(CStr& path);

	// Get file names from a wildcard. Eg: GetNames("D:\\*.jpg", imgNames);
	static int GetNames(CStr &nameW, vecS &names, string &dir = string());
	static int GetNames(CStr& rootFolder, CStr &fileW, vecS &names);
	static int GetNamesNE(CStr& nameWC, vecS &names, string &dir = string(), string &ext = string());
	static int GetNamesNE(CStr& rootFolder, CStr &fileW, vecS &names);
	static inline string GetExtention(CStr name);

	static inline bool FileExist(CStr& filePath);
	static inline bool FilesExist(CStr& fileW);
	static inline bool FolderExist(CStr& strPath);

	static inline string GetWkDir();

	static BOOL MkDir(CStr&  path);

	// Eg: RenameImages("D:/DogImages/*.jpg", "F:/Images", "dog", ".jpg");
	static int Rename(CStr& srcNames, CStr& dstDir, const char* nameCommon, const char* nameExt);

	static inline void RmFile(CStr& fileW);
	static void RmFolder(CStr& dir);
	static void CleanFolder(CStr& dir, bool subFolder = false);

	static int GetSubFolders(CStr& folder, vecS& subFolders);

	inline static BOOL Copy(CStr &src, CStr &dst, BOOL failIfExist = FALSE);
	inline static BOOL Move(CStr &src, CStr &dst, DWORD dwFlags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	static BOOL Move2Dir(CStr &srcW, CStr dstDir);
	static BOOL Copy2Dir(CStr &srcW, CStr dstDir);

	//Load mask image and threshold thus noisy by compression can be removed
	static Mat LoadMask(CStr& fileName);

	static void WriteNullFile(CStr& fileName) {FILE *f = fopen(_S(fileName), "w"); fclose(f);}

	static void ChkImgs(CStr &imgW);

	static void RunProgram(CStr &fileName, CStr &parameters = "", bool waiteF = false, bool showW = true);

	static void SegOmpThrdNum(double ratio = 0.8);

	// Copy files and add suffix. e.g. copyAddSuffix("./*.jpg", "./Imgs/", "_Img.jpg")
	static void copyAddSuffix(CStr &srcW, CStr &dstDir, CStr &dstSuffix);

	static vecS loadStrList(CStr &fName);
	static bool writeStrList(CStr &fName, const vecS &strs);
};

/************************************************************************/
/* Implementation of inline functions                                   */
/************************************************************************/
string CmFile::GetFolder(CStr& path)
{
	return path.substr(0, path.find_last_of("\\/")+1);
}

string CmFile::GetName(CStr& path)
{
	int start = path.find_last_of("\\/")+1;
	int end = path.find_last_not_of(' ')+1;
	return path.substr(start, end - start);
}

string CmFile::GetNameNE(CStr& path)
{
	int start = path.find_last_of("\\/")+1;
	int end = path.find_last_of('.');
	if (end >= 0)
		return path.substr(start, end - start);
	else
		return path.substr(start,  path.find_last_not_of(' ')+1 - start);
}

string CmFile::GetPathNE(CStr& path)
{
	int end = path.find_last_of('.');
	if (end >= 0)
		return path.substr(0, end);
	else
		return path.substr(0,  path.find_last_not_of(' ') + 1);
}

string CmFile::GetExtention(CStr name)
{
	return name.substr(name.find_last_of('.'));
}

BOOL CmFile::Copy(CStr &src, CStr &dst, BOOL failIfExist)
{
	return ::CopyFileA(src.c_str(), dst.c_str(), failIfExist);
}

BOOL CmFile::Move(CStr &src, CStr &dst, DWORD dwFlags)
{
	return MoveFileExA(src.c_str(), dst.c_str(), dwFlags);
}

void CmFile::RmFile(CStr& fileW)
{ 
	vecS names;
	string dir;
	int fNum = CmFile::GetNames(fileW, names, dir);
	for (int i = 0; i < fNum; i++)
		::DeleteFileA(_S(dir + names[i]));
}


// Test whether a file exist
bool CmFile::FileExist(CStr& filePath)
{
	if (filePath.size() == 0)
		return false;

	return  GetFileAttributesA(_S(filePath)) != INVALID_FILE_ATTRIBUTES; // ||  GetLastError() != ERROR_FILE_NOT_FOUND;
}

bool CmFile::FilesExist(CStr& fileW)
{
	vecS names;
	int fNum = GetNames(fileW, names);
	return fNum > 0;
}

string CmFile::GetWkDir()
{	
	string wd;
	wd.resize(1024);
	DWORD len = GetCurrentDirectoryA(1024, &wd[0]);
	wd.resize(len);
	return wd;
}

bool CmFile::FolderExist(CStr& strPath)
{
	int i = (int)strPath.size() - 1;
	for (; i >= 0 && (strPath[i] == '\\' || strPath[i] == '/'); i--)
		;
	string str = strPath.substr(0, i+1);

	WIN32_FIND_DATAA  wfd;
	HANDLE hFind = FindFirstFileA(_S(str), &wfd);
	bool rValue = (hFind != INVALID_HANDLE_VALUE) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);   
	FindClose(hFind);
	return rValue;
}

/************************************************************************/
/*                   Implementations                                    */
/************************************************************************/