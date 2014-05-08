#include "StdAfx.h"
#include "CmIllu.h"

namespace CmIllu{

	void showColorCodes(CvecS &classNames, CStr title){
		const int N = (int)classNames.size(), hC = 32, w = 150;
		vector<Vec3b> colors(N);
		Mat show3u(hC * N, w, CV_8UC3);
		for (int i = 0; i < N; i++){
			label2Rgb((byte)i, colors[i]);
			Scalar color = Scalar(colors[i]);
			show3u(Rect(0, hC*i, w, hC)) = color;
			Scalar textColor = color[0] + color[1] + color[2] - 128*3 > 0 ? Scalar() : CmShow::WHITE;
			putText(show3u, classNames[i], Point(3, hC*i + 29), FONT_HERSHEY_TRIPLEX, 1, textColor);
		}
		CmShow::SaveShow(show3u, title);
	}


}
