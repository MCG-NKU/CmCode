/*****************************************************************************
*    PlanarCut - software to compute MinCut / MaxFlow in a planar graph      *
*                              Version 1.0.1                                 *
*                                                                            *
*    Copyright 2011 - 2012 Eno Töppe <toeppe@in.tum.de>                      *
*                          Frank R. Schmidt <info@frank-r-schmidt.de>        *
******************************************************************************

  If you use this software for research purposes, YOU MUST CITE the following 
  paper in any resulting publication:

    [1] Efficient Planar Graph Cuts with Applications in Computer Vision.
        F. R. Schmidt, E. Töppe, D. Cremers, 
	    IEEE CVPR, Miami, Florida, June 2009		

******************************************************************************

  This software is released under the LGPL license. Details are explained
  in the files 'COPYING' and 'COPYING.LESSER'.
	
*****************************************************************************/

#include "stdafx.h"
#include "windows.h"

#include "CutSegment.h" //include PlanarCut

#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <stdlib.h>

using namespace std;


unsigned char *loadSimplePPM(int &w, int &h, const string &filename) {

  char line[1000];
  int depth = 0;
  unsigned char *rgb = 0;
  long lastpos;

  /*  streampos lastpos;
      ifstream ifs(filename.c_str(), ios_base::binary);*/

  FILE *fh = fopen(filename.c_str(), "rb");
  
  w = 0, h = 0;

  fgets(line, 1000, fh);

  if (strcmp(line, "P6\n")) {
    cerr << filename << " is no PPM-Datei\n";
    return false;
  }

  while (!feof(fh)) {

    lastpos = ftell(fh);
    
    fgets(line, 1000, fh);

    if (line[0] == '#') {
      //      cout << "Comment: " << line;
    } else if (!w) {
      if (sscanf(line, "%d %d", &w, &h) < 2) {
	cerr << "error while reading the file " << filename;
	cerr << " expected width and height of image\n";
	return 0;
      }
    } else if (!depth) {
      if (sscanf(line, "%d", &depth) < 1) {
	cerr << "error while reading the file " << filename;
	cerr << " expected color depth\n";
	return 0;
      }
    } else {
      rgb = new unsigned char[w*h*3];
      fseek(fh, lastpos, SEEK_SET);
      fread(rgb, 1, w*h*3, fh);
      break;
    }
  
  }

  fclose(fh);

  return rgb;

}


bool saveSimplePPM(unsigned char *rgb, int w, int h, const string &filename) {

  ofstream fos(filename.c_str(), ios_base::binary);
  ostringstream ost;
  string s;

  if (!fos)
    return false;

  fos << "P6" << endl;

  ost << w << " " << h << endl;

  fos << ost.str();
  fos << "255" << endl;

  fos.write((const char*)rgb, w*h*3);

  fos.close();
  
  return true;

}


unsigned char *RGBDataToGrey(unsigned char *rgb, int w, int h) {
  
  unsigned char *pic = new unsigned char[w*h];
  int i;

  for (i=0; i<w*h; i++)
    pic[i] = rgb[i*3];

  return pic;

}


unsigned char *GreyDataToRGB(unsigned char *pic, int w, int h) {

  unsigned char *rgb = new unsigned char[w*h*3];
  int i;

  for (i=0; i<w*h; i++)
    rgb[i*3] = rgb[i*3+1] = rgb[i*3+2] = pic[i];
  
  return rgb;

}


unsigned char *SegMaskAndGreyDataToRGB(CutPlanar::ELabel *mask,
				       unsigned char *pic,
				       int w, int h) {

  unsigned char *rgb = new unsigned char[w*h*3];
  int i;

  for (i=0; i<w*h; i++) {

    rgb[i*3] = (mask[i]==CutPlanar::LABEL_SINK) ? (unsigned char)(pic[i*3] / 255.f * 200.f) : 255;
    rgb[i*3+1] = (unsigned char)(pic[i*3+1] / 255.f * 200.f);
    rgb[i*3+2] = (mask[i]==CutPlanar::LABEL_SINK) ? 255 : (unsigned char)(pic[i*3+2] / 255.f * 200.f);

  }
  
  return rgb;

}


unsigned char *getSegmentationInfo(unsigned char *rgb, int w, int h) {

  unsigned char *seginfo = new unsigned char[w*h];
  unsigned char r, g, b;
  int i;

  for (i=0; i<w*h; i++) {

    r = rgb[i*3];
    g = rgb[i*3+1];
    b = rgb[i*3+2];

    if (r >= 240 &&
	g <= 10  &&
	b <= 10) {
      seginfo[i] = 1;
    } else if (r <= 10 &&
	     g <= 10 &&
	     b >= 240) {
      seginfo[i] = 2;
    } else {
      seginfo[i] = 0;
    };

  }
    
  return seginfo;

}



int _tmain(int argc, _TCHAR* argv[]) {

  CutSegment *sc;
  int w, h;
  uchar *seg, *rgb, *rgbNew, *grey;
  CutPlanar::ELabel *mask;

  if (argc < 2) {
    cerr << "calling convention: testCutGrid [image-filename]\n"
		 << "for example: 'testCutSegment ../../../pics/walk.ppm'\n\n";
    return -1;
  }


  size_t pos;
  string picname, segname, basename, suffix;
  char tmp[4096]; //just needed for windows character conversion
  
  WideCharToMultiByte(CP_ACP, 0, argv[1], -1, tmp, 4096, NULL, NULL);
  picname = tmp;
  pos = picname.rfind(".ppm");

  if (pos == string::npos) {
    cerr << "testCutGrid only accepts .ppm images\n";
    return -1;
  }

  suffix = picname.substr(pos);
  basename = picname.substr(0, pos);
 
  picname = basename + suffix;
  segname = basename + "seg" + suffix;

 
  //load the input images from file
  rgb = loadSimplePPM(w, h, segname);
  seg = getSegmentationInfo(rgb, w, h);
  delete [] rgb;

  rgb = loadSimplePPM(w, h, picname);
  grey = RGBDataToGrey(rgb, w, h);

  cout << "Image width: " << w << " and image height " << h << endl;

  //perform segmentation task
  sc = new CutSegment(w, h);
  sc->setImageData(grey);
  sc->setSourceSink(seg,1,2);
  cout << "Cut: " << sc->segment() << "\n";
  mask = new CutPlanar::ELabel[w*h];
  sc->getLabels(mask);
  delete sc;

  //read out segmentation result and save to disk
  rgbNew = SegMaskAndGreyDataToRGB(mask, rgb, w, h);
  saveSimplePPM(rgbNew, w, h, string("segresult.ppm"));
  cout << "\nSegmentation result written to 'segresult.ppm'\n\n";


  delete [] grey;
  delete [] rgb;
  delete [] rgbNew;
  delete [] seg;
  delete [] mask;

  return 0;

}
