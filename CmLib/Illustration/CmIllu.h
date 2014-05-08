#pragma once

namespace CmIllu{
	/************************************************************************/
	/* Conversions between CV_8UC3 color and label type T.                  */
	/*   T can be any type if sizeof(T) < 8;								*/
	/************************************************************************/
	template<typename T> void rgb2Label(CMat &rgb3u, Mat_<T> &label);
	template<typename T> inline void rgb2Label(const Vec3b &rgb, T &label); 
	template<typename T> inline void label2Rgb(const T &label, Vec3b &rgb); 
	template<typename T> void label2Rgb(const Mat_<T> &label, Mat &rgb3u);
	template<typename T> Mat loadRgbLabel(CStr &fName); 
	template<typename T> void saveRgbLabel(CStr &fName, const Mat_<T> &label);


	void showColorCodes(CvecS &classNames, CStr title);
}



/************************************************************************/
/*                       Implementations                                */
/*----------------------------------------------------------------------*/
/* Conversions between CV_8UC3 color and label type T.                  */
/*   T can be byte, short, or int; singed or unsigned.					*/
/************************************************************************/

namespace CmIllu{
	template<typename T> inline void rgb2Label(const Vec3b &rgb, T &label){
		label = 0;
		for(int i = 0; i < 8; i++) 
			label = (label << 3) | (((rgb[2] >> i) & 1) << 0) | (((rgb[1] >> i) & 1) << 1) | (((rgb[0] >> i) & 1) << 2);
	}

	template<typename T> inline void label2Rgb(const T &label, Vec3b &rgb){
		T lab = label;
		rgb[0] = rgb[1] = rgb[2] = 0;
		for(int i = 0; lab > 0; i++, lab >>= 3){
			rgb[2] |= (unsigned char) (((lab >> 0) & 1) << (7 - i));
			rgb[1] |= (unsigned char) (((lab >> 1) & 1) << (7 - i));
			rgb[0] |= (unsigned char) (((lab >> 2) & 1) << (7 - i));
		}
	}

	template<typename T> void rgb2Label(CMat &rgb3u, Mat_<T> &label){
		label.create(rgb3u.size());
		for (int r = 0; r < label.rows; r++){
			T* idx = label.ptr<T>(r);
			const Vec3b *color = rgb3u.ptr<Vec3b>(r);
			for (int c = 0; c < label.cols; c++)
				if (c > 0 && color[c - 1] == color[c])
					idx[c] = idx[c-1];
				else
					rgb2Label(color[c], idx[c]);
		}
	}

	template<typename T> void label2Rgb(const Mat_<T> &label, Mat &color3u){
		color3u.create(label.size(), CV_8UC3);
		for (int r = 0; r < label.rows; r++){
			const T* idx = label.ptr<T>(r);
			Vec3b *color = color3u.ptr<Vec3b>(r);
			for (int c = 0; c < label.cols; c++)
				if (c > 0 && idx[c-1] == idx[c])
					color[c] = color[c-1];
				else
					label2Rgb(idx[c], color[c]);
		}

	}

	template<typename T> Mat loadRgbLabel(CStr &fName){
		Mat rgb3u = imread(fName);
		if (rgb3u.data == NULL)
			return rgb3u;

		Mat_<T> label;
		CV_Assert_(rgb3u.data != NULL, ("Load rgb label failed: %s\n", _S(fName)));
		rgb2Label<T>(rgb3u, label);
		return label;
	}

	template<typename T> void saveRgbLabel(CStr &fName, const Mat_<T> &label){
		Mat rgb3u;
		label2Rgb<T>(label, rgb3u);
		imwrite(fName, rgb3u);
	}
}
