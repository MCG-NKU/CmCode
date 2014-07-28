/************************************************************************/
/* Notice: this project is used to support speech recognition of my		*/
/* ImageSpirit project. Please see the corresponding paper for more		*/
/* details. The CORE part of ImageSpirit system will be made public		*/
/* available soon. More resource: http://mmcheng.net/imagespirit/		*/
/* ImageSpirit: Verbal Guided Image Parsing. M.-M. Cheng, S. Zheng,		*/
/* W.-Y. Lin, V. Vineet, P. Sturgess, N. Crook, N. Mitra, P. Torr,		*/
/* ACM TOG, 2014.														*/
/************************************************************************/

#include "stdafx.h"
#include "SpRecoUI.h"


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	SpRecoUI w;
	w.show();
	return a.exec();
}
