#pragma once

class CmIllustr
{
public:
	struct ImgsOptions{
		string _inDir, _outDir, _texName;
		int _w, _H;
		vecS _exts; // Extensions of different image types,

		ImgsOptions(CStr &wkDir, vecS &exts) : _exts(exts){	Initial(wkDir); }
		void Initial(CStr &wkDir);
	};

	static Mat Retreival(CStr wkDir, vector<vecM> &subImgs, const char* nameW[], int H = 3600, int W = 200);

	static void Imgs(const ImgsOptions &opts, int maxImgNum = INT_MAX);

	static void Demo(CStr &wkDir, int height = 4000);

private:
	static Mat ArrangeImgs(vecM &imgs, vecD &len, int W, int H, bool toRow);

	// load format(imgW, i) and add information to the back of imgs and lens
	static void LoadImgs(CStr &imgW, vecM &imgs, vecD &lens, int W, int H);

	static void WriteFigure(const vecS &names, FILE *f, const vecI &sHeights, const ImgsOptions &opts);

	static const int space = 4;
};

