#include "StdAfx.h"
#include "CmCurveEx.h"

float const static PI_QUARTER = PI_FLOAT * 0.25f; 
float const static PI_EIGHTH = PI_FLOAT * 0.125f;

CmCurveEx::CmCurveEx(float maxOrntDif)
: _maxAngDif(maxOrntDif)
{
	_engy1f = Mat();	
}

/* Compute the eigenvalues and eigenvectors of the Hessian matrix given by
dfdrr, dfdrc, and dfdcc, and sort them in descending order according to
their absolute values. */
void CmCurveEx::compute_eigenvals(double dfdrr, double dfdrc, double dfdcc, double eigval[2], double eigvec[2][2])
{
	double theta, t, c, s, e1, e2, n1, n2; /* , phi; */

	/* Compute the eigenvalues and eigenvectors of the Hessian matrix. */
	if (dfdrc != 0.0) {
		theta = 0.5*(dfdcc-dfdrr)/dfdrc;
		t = 1.0/(fabs(theta)+sqrt(theta*theta+1.0));
		if (theta < 0.0) t = -t;
		c = 1.0/sqrt(t*t+1.0);
		s = t*c;
		e1 = dfdrr-t*dfdrc;
		e2 = dfdcc+t*dfdrc;
	} else {
		c = 1.0;
		s = 0.0;
		e1 = dfdrr;
		e2 = dfdcc;
	}
	n1 = c;
	n2 = -s;

	/* If the absolute value of an eigenvalue is larger than the other, put that
	eigenvalue into first position.  If both are of equal absolute value, put
	the negative one first. */
	if (fabs(e1) > fabs(e2)) {
		eigval[0] = e1;
		eigval[1] = e2;
		eigvec[0][0] = n1;
		eigvec[0][1] = n2;
		eigvec[1][0] = -n2;
		eigvec[1][1] = n1;
	} else if (fabs(e1) < fabs(e2)) {
		eigval[0] = e2;
		eigval[1] = e1;
		eigvec[0][0] = -n2;
		eigvec[0][1] = n1;
		eigvec[1][0] = n1;
		eigvec[1][1] = n2;
	} else {
		if (e1 < e2) {
			eigval[0] = e1;
			eigval[1] = e2;
			eigvec[0][0] = n1;
			eigvec[0][1] = n2;
			eigvec[1][0] = -n2;
			eigvec[1][1] = n1;
		} else {
			eigval[0] = e2;
			eigval[1] = e1;
			eigvec[0][0] = -n2;
			eigvec[0][1] = n1;
			eigvec[1][0] = n1;
			eigvec[1][1] = n2;
		}
	}
}

CMat& CmCurveEx::CalSecDer(CMat &img1f, int kSize)
{
	AllocSpace(img1f.size());
	Mat dxx, dxy, dyy;
	Sobel(img1f, dxx, CV_32F, 2, 0, kSize);
	Sobel(img1f, dxy, CV_32F, 1, 1, kSize);
	Sobel(img1f, dyy, CV_32F, 0, 2, kSize);

	double eigval[2], eigvec[2][2];
	for (int y = 0; y < _h; y++){
		float *xx = dxx.ptr<float>(y);
		float *xy = dxy.ptr<float>(y);
		float *yy = dyy.ptr<float>(y);
		float *pOrnt = _pOrnt1f.ptr<float>(y);
		float *pDer = _pDer1f.ptr<float>(y);
		for (int x = 0; x < _w; x++){
			compute_eigenvals(yy[x], xy[x], xx[x], eigval, eigvec);
			pOrnt[x] = (float)atan2(-eigvec[0][1], eigvec[0][0]); //计算法线方向
			if (pOrnt[x] < 0.0f)
				pOrnt[x] += PI2;
			pDer[x] = float(eigval[0] > 0.0f ? eigval[0] : 0.0f);//计算二阶导数
		}
	}
	GaussianBlur(_pDer1f, _pDer1f, Size(3, 3), 0);
	return _pDer1f;
}

CMat& CmCurveEx::CalSecDer_(CMat &img3f, int kSize)
{
	Mat img1f[3], der1f[3], ornt1f[3];
	split(img3f, img1f);
	for (int i = 0; i < 3; i++)	{
		CalSecDer(img1f[i], kSize);
		_pDer1f.copyTo(der1f[i]);
		_pOrnt1f.copyTo(ornt1f[i]);
	}
	MergeResults(der1f, ornt1f, _pDer1f, _pOrnt1f);
	GaussianBlur(_pDer1f, _pDer1f, Size(3, 3), 0);
	return _pDer1f;
}


CMat& CmCurveEx::CalFirDer(CMat &img1f, int kSize)
{
	AllocSpace(img1f.size());
	Mat dxMat, dyMat;
	Sobel(img1f, dxMat, CV_32F, 1, 0, kSize);
	Sobel(img1f, dyMat, CV_32F, 0, 1, kSize);
	for (int y = 0; y < _h; y++){
		float *dx = dxMat.ptr<float>(y);
		float *dy = dyMat.ptr<float>(y);
		float *pOrnt = _pOrnt1f.ptr<float>(y);
		float *pDer = _pDer1f.ptr<float>(y);
		for (int x = 0; x < _w; x++){
			pOrnt[x] = (float)atan2f(dx[x], -dy[x]);
			if (pOrnt[x] < 0.0f)
				pOrnt[x] += PI2;

			pDer[x] = sqrt(dx[x]*dx[x] + dy[x]*dy[x]);
		}
	}
	GaussianBlur(_pDer1f, _pDer1f, Size(3, 3), 0);
	return _pDer1f;
}

CMat& CmCurveEx::CalDx(CMat &img3f, int kSize)
{
	AllocSpace(img3f.size());
	Mat bgrImg[3], bgrDer[3];
	split(img3f, bgrImg);
	for (int i = 0; i < 3; i++)
		Sobel(bgrImg[i], bgrDer[i], CV_32F, 1, 0, kSize);
	
	Size sz = img3f.size();
	for (int r = 0; r < sz.height; r++)	{
		float* res = _pDer1f.ptr<float>(r);
		const float *d0 = bgrDer[0].ptr<float>(r), *d1 = bgrDer[1].ptr<float>(r), *d2 = bgrDer[2].ptr<float>(r);
		for (int c = 0; c < sz.width; c++)
			res[c] = sqrt(sqr(d0[c]) + sqr(d1[c]) + sqr(d2[c]));
	}
	GaussianBlur(_pDer1f, _pDer1f, Size(3, 3), 0);
	_pOrnt1f = PI_HALF;
	return _pDer1f;
}

void CmCurveEx::MergeResults(CMat der1f[3], CMat ornt1f[3], Mat &_der, Mat &_ornt)
{
	Size sz = der1f[0].size();
	_der.create(sz, CV_32FC1);
	_ornt.create(sz, CV_32FC1);
	for (int r = 0; r < sz.height; r++)	{
		float *der = _der.ptr<float>(r);
		float *ornt = _ornt.ptr<float>(r);
		const float *d[3], *or[3];
		for (int i = 0; i < 3; i++)
			d[i] = der1f[i].ptr<float>(r), or[i] = ornt1f[i].ptr<float>(r);
		for (int c = 0; c < sz.width; c++){
			float maxD = d[0][c];
			int idx = 0;
			for (int i = 1; i < 3; i++)
				if (d[i][c] > maxD)
					maxD = d[i][c], idx = i;
			der[c] = maxD;
			ornt[c] = or[idx][c];
		}
	}
}


void CmCurveEx::NoneMaximalSuppress(float lkEnd, float lkStart)
{
	CV_Assert(_pDer1f.data != NULL && _pLabel1i.data != NULL);

	_StartPnt.clear();
	_StartPnt.reserve(int(0.08 * _h * _w));
	PntImp linePoint;

	_pLabel1i = IND_BG;
	for (int r = 1; r < _h-1; r++)	{
		float* pDer = _pDer1f.ptr<float>(r);
		float* pOrnt = _pOrnt1f.ptr<float>(r);
		int* pLineInd = _pLabel1i.ptr<int>(r);
		for (int c = 1; c < _w-1; c++)	{
			if (pDer[c] < lkEnd)
				continue;

			float cosN = sin(pOrnt[c]);
			float sinN = -cos(pOrnt[c]);
			int xSgn = CmSgn<float>(cosN);
			int ySgn = CmSgn<float>(sinN);
			cosN *= cosN;
			sinN *= sinN;

			if (pDer[c] >= (pDer[c + xSgn] * cosN + _pDer1f.at<float>(r + ySgn, c) * sinN) 
				&& pDer[c] >= (pDer[c - xSgn] * cosN + _pDer1f.at<float>(r - ySgn, c) * sinN)) {
					pLineInd[c] = IND_NMS;
					if (pDer[c] < lkStart)
						continue;

					//add to _vStartPoint
					linePoint.second = Point(c, r);
					linePoint.first = pDer[c];
					_StartPnt.push_back(linePoint);
			}
		}
	}
}

const CmEdges& CmCurveEx::Link(int shortRemoveBound, float lkEnd, float lkStart)
{
	normalize(_pDer1f, _pDer1f, 0, 1, NORM_MINMAX);
	NoneMaximalSuppress(lkEnd, lkStart);
	CV_Assert(_pDer1f.data != NULL && _pLabel1i.data != NULL);

	sort(_StartPnt.begin(), _StartPnt.end(), linePointGreater);
	
	_pNext1i = -1;
	_vEdge.clear();
	_vEdge.reserve(int(0.01 * _w * _h));
	CEdge crtEdge(0);//当前边
	for (vector<PntImp>::iterator it = _StartPnt.begin(); it != _StartPnt.end(); it++)	{
		Point pnt = it->second;
		if (_pLabel1i.at<int>(pnt) != IND_NMS)
			continue;

		findEdge(pnt, crtEdge, FALSE);
		findEdge(pnt, crtEdge, TRUE);
		if (crtEdge.pointNum <= shortRemoveBound) {
			Point point = crtEdge.start;
			int i, nextInd;
			for (i = 1; i < crtEdge.pointNum; i++) {
				_pLabel1i.at<int>(point) = IND_SR;
				nextInd = _pNext1i.at<int>(point);
				point += DIRECTION8[nextInd];
			}
			_pLabel1i.at<int>(point) = IND_SR;
		}
		else {
			_vEdge.push_back(crtEdge);
			crtEdge.index++;
		}
	}

	// Get edge information
	int edgNum = (int)_vEdge.size();
	for (int i = 0; i < edgNum; i++) {
		CEdge &edge = _vEdge[i];
		vector<Point> &pnts = edge.pnts;
		pnts.resize(edge.pointNum);
		pnts[0] = edge.start;
		for (int j = 1; j < edge.pointNum; j++) {
			pnts[j] = pnts[j-1] + DIRECTION8[_pNext1i.at<int>(pnts[j-1])];
			edge.avg += _pDer1f.at<float>(pnts[j]);
		}
		edge.avg /= (float)(edge.pointNum);
	}
	return _vEdge;
}

/************************************************************************/
/* 如果isForward为TRUE则沿着_pOrnt方向寻找crtEdge, 并将沿途点的_pNext */
/* 相应值置为沿途的方向值, 同时把_pLineInd的值置为当前线的编号,找不到  */
/* 下一点的时候把最后一个点的坐标置为crtEdge的End坐标.                  */
/* 如果isForward为FALSE则沿着_pOrnt反方向寻找crtEdge, 并将沿途点的     */
/* _pNext相应值置为沿途的方向反向值, 同时把_pLineInd的值置为当前线的  */
/* 编号.找不到下一点的时候如果pointNum太小则active置为false并推出.否则  */
/* 把最后一个点的坐标置为crtEdge的End坐标.                              */
/************************************************************************/
void CmCurveEx::findEdge(Point seed, CEdge &crtEdge, bool isBackWard)
{
	Point pnt = seed;

	float ornt = _pOrnt1f.at<float>(pnt);
	if (isBackWard){
		ornt += PI_FLOAT;
		if (ornt >= PI2)
			ornt -= PI2;
	}
	else{
		crtEdge.pointNum = 1;
		_pLabel1i.at<int>(pnt) = crtEdge.index;
	}

	int orntInd, nextInd1, nextInd2;
	while (true) { 
		/*************按照优先级寻找下一个点，方向差异较大不加入**************/
		//下一个点在DIRECTION16最佳方向上找
		orntInd = int(ornt/PI_EIGHTH + 0.5f) % 16;
		if (jumpNext(pnt, ornt, crtEdge, orntInd, isBackWard)) 
			continue;
		//下一个点在DIRECTION8最佳方向上找
		orntInd = int(ornt/PI_QUARTER + 0.5f) % 8;
		if (goNext(pnt, ornt, crtEdge, orntInd, isBackWard)) 
			continue;
		//下一个点在DIRECTION16次优方向上找
		orntInd = int(ornt/PI_EIGHTH + 0.5f) % 16;
		nextInd1 = (orntInd + 1) % 16;
		nextInd2 = (orntInd + 15) % 16;
		if (angle(DRT_ANGLE[nextInd1], ornt) < angle(DRT_ANGLE[nextInd2], ornt)) {
			if(jumpNext(pnt, ornt, crtEdge, nextInd1, isBackWard))
				continue;
			if(jumpNext(pnt, ornt, crtEdge, nextInd2, isBackWard))
				continue;
		}
		else{//下一个点在DIRECTION16另一个方向上找
			if(jumpNext(pnt, ornt, crtEdge, nextInd2, isBackWard))
				continue;
			if(jumpNext(pnt, ornt, crtEdge, nextInd1, isBackWard))
				continue;
		}
		//下一个点在DIRECTION8次优方向上找
		orntInd = int(ornt/PI_QUARTER + 0.5f) % 8;
		nextInd1 = (orntInd + 1) % 8;
		nextInd2 = (orntInd + 7) % 8;
		if (angle(DRT_ANGLE[nextInd1], ornt) < angle(DRT_ANGLE[nextInd2], ornt)) {
			if(goNext(pnt, ornt, crtEdge, nextInd1, isBackWard))
				continue;
			if(goNext(pnt, ornt, crtEdge, nextInd2, isBackWard))
				continue;
		}
		else{//下一个点在DIRECTION8另一个方向上找
			if(goNext(pnt, ornt, crtEdge, nextInd2, isBackWard))
				continue;
			if(goNext(pnt, ornt, crtEdge, nextInd1, isBackWard))
				continue;
		}


		/*************按照优先级寻找下一个点，方向差异较大也加入**************/
		//下一个点在DIRECTION16最佳方向上找
		orntInd = int(ornt/PI_EIGHTH + 0.5f) % 16;
		if (jumpNext(pnt, ornt, crtEdge, orntInd, isBackWard)) 
			continue;
		//下一个点在DIRECTION8最佳方向上找
		orntInd = int(ornt/PI_QUARTER + 0.5f) % 8;
		if (goNext(pnt, ornt, crtEdge, orntInd, isBackWard)) 
			continue;
		//下一个点在DIRECTION16次优方向上找
		orntInd = int(ornt/PI_EIGHTH + 0.5f) % 16;
		nextInd1 = (orntInd + 1) % 16;
		nextInd2 = (orntInd + 15) % 16;
		if (angle(DRT_ANGLE[nextInd1], ornt) < angle(DRT_ANGLE[nextInd2], ornt)) {
			if(jumpNext(pnt, ornt, crtEdge, nextInd1, isBackWard))
				continue;
			if(jumpNext(pnt, ornt, crtEdge, nextInd2, isBackWard))
				continue;
		}
		else{//下一个点在DIRECTION16另一个方向上找
			if(jumpNext(pnt, ornt, crtEdge, nextInd2, isBackWard))
				continue;
			if(jumpNext(pnt, ornt, crtEdge, nextInd1, isBackWard))
				continue;
		}
		//下一个点在DIRECTION8次优方向上找
		orntInd = int(ornt/PI_QUARTER + 0.5f) % 8;
		nextInd1 = (orntInd + 1) % 8;
		nextInd2 = (orntInd + 7) % 8;
		if (angle(DRT_ANGLE[nextInd1], ornt) < angle(DRT_ANGLE[nextInd2], ornt)) {
			if(goNext(pnt, ornt, crtEdge, nextInd1, isBackWard))
				continue;
			if(goNext(pnt, ornt, crtEdge, nextInd2, isBackWard))
				continue;
		}
		else{//下一个点在DIRECTION8另一个方向上找
			if(goNext(pnt, ornt, crtEdge, nextInd2, isBackWard))
				continue;
			if(goNext(pnt, ornt, crtEdge, nextInd1, isBackWard))
				continue;
		}

		break;//如果ornt附近的三个方向上都没有的话,结束寻找
	}

	if (isBackWard)
		crtEdge.start = pnt;
	else
		crtEdge.end = pnt;
}


float CmCurveEx::angle(float ornt1, float orn2)
{//两个ornt都必须在[0, 2*PI)之间, 返回值在[0, PI/2)之间
	float agl = ornt1 - orn2;
	if (agl < 0)
		agl += PI2;
	if (agl >= PI_FLOAT)
		agl -= PI_FLOAT;
	if (agl >= PI_HALF)
		agl -= PI_FLOAT;
	return fabs(agl);
}

void CmCurveEx::refreshOrnt(float& ornt, float& newOrnt)
{
	static const float weightOld = 0.0f;
	static const float weightNew = 1.0f - weightOld;

	static const float largeBound = PI_FLOAT + PI_HALF;
	static const float smallBound = PI_HALF;

	if (newOrnt >= ornt + largeBound){
		newOrnt -= PI2;
		ornt = ornt * weightOld + newOrnt * weightNew;
		if (ornt < 0.0f)
			ornt += PI2;
	}
	else if (newOrnt + largeBound <= ornt){
		newOrnt += PI2;
		ornt = ornt * weightOld + newOrnt * weightNew;
		if (ornt >= PI2)
			ornt -= PI2;
	}
	else if (newOrnt >= ornt + smallBound){
		newOrnt -= PI_FLOAT;
		ornt = ornt * weightOld + newOrnt * weightNew;
		if (ornt < 0.0f)
			ornt += PI2;   
	}
	else if(newOrnt + smallBound <= ornt){
		newOrnt += PI_FLOAT;
		ornt = ornt * weightOld + newOrnt * weightNew;
		if (ornt >= PI2)
			ornt -= PI2;
	}
	else
		ornt = ornt * weightOld + newOrnt * weightNew;
	newOrnt = ornt;
}

bool CmCurveEx::goNext(Point &pnt, float &ornt, CEdge &crtEdge, int orntInd, bool isBackward)
{
	Point pntN = pnt + DIRECTION8[orntInd];
	int &label = _pLabel1i.at<int>(pntN);

	//如果该点方向与当前线方向差别比较大则不加入/***********一个可变域值**********************/
	if (CHK_IND(pntN) && (label == IND_NMS || label == IND_SR)) {			
		if (angle(ornt, _pOrnt1f.at<float>(pntN)) > _maxAngDif)
			return FALSE;
		
		label = crtEdge.index;
		if (isBackward)
			_pNext1i.at<int>(pntN) = (orntInd + 4) % 8;
		else
			_pNext1i.at<int>(pnt) = orntInd;
		crtEdge.pointNum++;

		//更新切线方向
		refreshOrnt(ornt, _pOrnt1f.at<float>(pntN));
		pnt = pntN;
		return true;
	}
	return false;
}

bool CmCurveEx::jumpNext(Point &pnt, float &ornt, CEdge &crtEdge, int orntInd, bool isBackward)
{
	Point pnt2 = pnt + DIRECTION16[orntInd]; 
	if (CHK_IND(pnt2) && _pLabel1i.at<int>(pnt2) <= IND_NMS) {		
		if (angle(ornt, _pOrnt1f.at<float>(pnt2)) > _maxAngDif) //如果该点方向与当前线方向差别比较大则不加入
			return false;

		// DIRECTION16方向上的orntInd相当于DIRECTION8方向上两个orntInd1,orntInd2
		// 的叠加,满足orntInd = orntInd1 + orntInd2.此处优先选择使得组合上的点具
		// IND_NMS标记的方向组合。(orntInd1,orntInd2在floor(orntInd/2)和
		// ceil(orntInd/2)中选择
		int orntInd1 = orntInd >> 1, orntInd2;
		Point pnt1 = pnt + DIRECTION8[orntInd1]; 
		if (_pLabel1i.at<int>(pnt1) >= IND_BG && orntInd % 2) {
			orntInd1 = ((orntInd + 1) >> 1) % 8;
			pnt1 = pnt + DIRECTION8[orntInd1];
		}

		int &lineIdx1 = _pLabel1i.at<int>(pnt1);
		if (lineIdx1 != -1) //当前nPos1点为其它线上的点，不能归入当前线
			return false;

		orntInd2 = orntInd - orntInd1;
		orntInd2 %= 8;

		lineIdx1 = crtEdge.index;
		_pLabel1i.at<int>(pnt2) = crtEdge.index;
		if (isBackward) {
			_pNext1i.at<int>(pnt1) = (orntInd1 + 4) % 8;
			_pNext1i.at<int>(pnt2) = (orntInd2 + 4) % 8;
		}
		else{
			_pNext1i.at<int>(pnt) = orntInd1;
			_pNext1i.at<int>(pnt1) = orntInd2;
		}
		crtEdge.pointNum += 2;

		refreshOrnt(ornt, _pOrnt1f.at<float>(pnt1));
		refreshOrnt(ornt, _pOrnt1f.at<float>(pnt2));
		pnt = pnt2;
		return true;
	}
	return false;
}

CMat& CmCurveEx::CalRobustCurveEnergy(double longEdg, double lenMin, double avgMin)
{
	const int edgNum = (int)_vEdge.size();
	Mat_<float> len1f(1, edgNum), avg1f(1, edgNum), edgSal1f(1, edgNum);
	for (int i = 0; i < edgNum; i++) {
		len1f(0, i) = (float)_vEdge[i].pointNum;
		avg1f(0, i) = (float)_vEdge[i].avg;
	}
	threshold(len1f, len1f, longEdg, 0, THRESH_TRUNC);
	normalize(len1f, len1f, lenMin, 1, NORM_MINMAX);
	normalize(avg1f, avg1f, avgMin, 1, NORM_MINMAX);
	edgSal1f = len1f.mul(avg1f);
	normalize(edgSal1f, edgSal1f, 0, 1, NORM_MINMAX);

	// Get initial energy multiply factor and mask
	_engy1f = Mat::zeros(_h, _w, CV_32F);
	for (int i = 0; i < edgNum; i++) {
		vector<Point> &pnts = _vEdge[i].pnts; 
		for (size_t j = 1; j < pnts.size(); j++)
			line(_engy1f, pnts[j], pnts[j-1], edgSal1f(0, i), 2, CV_AA);
		_vEdge[i].sal = edgSal1f(0, i);
		_vEdge[i].lenSal = len1f(0, i);
	}
	blur(_engy1f, _engy1f, Size(3, 3));
	return _engy1f;
}

void CmCurveEx::SaveShowResult(CMat &img1f, CStr &title, Mat &show3u, double longEdg)
{
	Mat show1f = Mat::zeros(_h * 2, _w * 2, CV_32F);
	Mat_<float> showAvg1f = show1f(Rect(_w, _h, _w, _h));
	//Mat_<float> showLen1f = show1f(Rect(2 * _w, 0, _w, _h));
	//Mat_<float> showSal1f = show1f(Rect(2 * _w, _h, _w, _h));
	img1f.copyTo(show1f(Rect(0, 0, _w, _h)));  // Show source image
	_pDer1f.copyTo(show1f(Rect(_w, 0, _w, _h))); // Show derivative
	//_engy1f.copyTo(show1f(Rect(2 * _w, _h, _w, _h)));
	for (size_t i = 0; i < _vEdge.size(); i++) 	{
		const CEdge &edg = _vEdge[i];
		for (int j = 0; j < edg.pointNum; j++) 	{
			const Point &p = edg.pnts[j];
			//showLen1f(p) = edg.lenSal;
			showAvg1f(p) = edg.avg;
			//showSal1f(p) = edg.sal;
		}
	}
	normalize(showAvg1f, showAvg1f, 0, 1, NORM_MINMAX);
	show1f.convertTo(show1f, CV_8UC1, 255);
	cvtColor(show1f, show3u, CV_GRAY2BGR);
	Mat showLabel3u = show3u(Rect(0, _h, _w, _h));

	// Show Label
	const int numNG = CmShow::COLOR_NU_NO_GRAY;
	for (int y = 0; y < _h; y++) {
		Vec3b* showLabel = showLabel3u.ptr<Vec3b>(y);
		const int* label = _pLabel1i.ptr<int>(y);
		for (int x = 0; x < _w; x++) {
			if (label[x] >= 0)
				showLabel[x] = CmShow::gColors[label[x] % numNG];
			else if (label[x] == IND_SR)
				showLabel[x] = CmShow::gColors[numNG];
		}			
	}
	CmShow::SaveShow(show3u, title);
}

void CmCurveEx::AllocSpace(const Size &imgSz)
{
	_h = imgSz.height;
	_w = imgSz.width;
	_pDer1f.create(imgSz, CV_32FC1);
	_pOrnt1f.create(imgSz, CV_32FC1);
	_pLabel1i.create(imgSz, CV_32SC1);
	_pNext1i.create(imgSz, CV_32SC1);
}


void CmCurveEx::Demo(CMat &img1u, bool isCartoon)
{
	Mat srcImg1f, show3u = Mat::zeros(img1u.size(), CV_8UC3);
	img1u.convertTo(srcImg1f, CV_32FC1, 1.0/255);
	GaussianBlur(srcImg1f, srcImg1f, Size(7, 7), 0, 0);

	CmCurveEx dEdge;
	if (isCartoon)
		imshow("Edge Map 2", dEdge.CalSecDer(srcImg1f, 9));
	else
		imshow("Edge Map 1", dEdge.CalFirDer(srcImg1f));

	dEdge.Link(40);
	const vector<CEdge> &edges = dEdge.GetEdges();

	for (size_t i = 0; i < edges.size(); i++) {
		Vec3b color(rand() % 255, rand() % 255, rand() % 255);
		const vector<Point> &pnts = edges[i].pnts;
		for (size_t j = 0; j < pnts.size(); j++)
			show3u.at<Vec3b>(pnts[j]) = color;
	}
	imshow("Linked edge (same color for the same edge index)", show3u);
	waitKey(0);
}

void CmCurveEx::Demo2(CStr &wkDir, bool isCartoon)
{
	string inDir = wkDir + "/In/";
	string outDir = wkDir + "/Out/";
	CmFile::MkDir(outDir);
	vecS names;
	int imgNum = CmFile::GetNamesNE(inDir + "*.jpg", names);
	for (int i = 0; i < imgNum; i++) {
		Mat inImg = imread(inDir + names[i] + ".jpg"), img1f, tmp;
		CV_Assert(inImg.data != NULL);
		cvtColor(inImg, img1f, CV_BGR2GRAY);
		img1f.convertTo(img1f, CV_32F, 1/255.0f);
		GaussianBlur(img1f, img1f, Size(3, 3), 0);
		//bilateralFilter(tmp, img1f, 3, 1, 1);

		CmCurveEx curveExt;

		if (isCartoon)
			curveExt.CalSecDer(img1f);
		else
			curveExt.CalFirDer(img1f);
		curveExt.Link(3, .01f, .1f);
		curveExt.CalRobustCurveEnergy();
		curveExt.SaveShowResult(img1f, outDir + names[i] + ".png");
		//imwrite(outDir + names[i] + "_Ro.png", curveExt.GetRobustCurveEnergy() * 255);
		//imwrite(outDir + names[i] + "_Mg.png", curveExt.GetDer() * 255);
	}
}
