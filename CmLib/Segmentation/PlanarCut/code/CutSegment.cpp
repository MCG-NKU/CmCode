/*****************************************************************************
*    PlanarCut - software to compute MinCut / MaxFlow in a planar graph      *
*                              Version 1.0.2                                 *
*                                                                            *
*    Copyright 2011 - 2013 Eno Töppe <toeppe@in.tum.de>                      *
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

#include "CutSegment.h"
#include "PlanarException.h"

CapType CutSegment::edgeCost(int row, int col, EDir dir) {
  int i = row * width + col;
  int j = i + DirToOfs[dir];

  if ((imMask[i]==IDX_SINK) || (imMask[j]==IDX_SOURCE))
    return CAP_INF; 

  if (bFormatRGB) {    
    double col1[3] = { (double) imData[i*3], (double) imData[i*3+1], (double) imData[i*3+2] };
    double col2[3] = { (double) imData[j*3], (double) imData[j*3+1], (double) imData[j*3+2] };
    return gradient(col1, col2);
  } else {
    return gradient(imData[i], imData[j]);
  }
}


CutSegment::CutSegment(int width, int height) : CutGrid(height, width), 
						imData(0), imMask(0), 
						bFormatRGB(0)
{
  DirToOfs[DIR_EAST]  = 1;
  DirToOfs[DIR_NORTH] = -width;
  DirToOfs[DIR_WEST]  = -1;
  DirToOfs[DIR_SOUTH] = width;

  if ((width > 0) && (height > 0)) {
    this->width  = width;
    this->height = height;
  } else {
    throw ExceptionUnexpectedError();
  }

  imData = new uchar[width * height * 3];
  imMask = new EMask[width * height];
}


CutSegment::~CutSegment() {
  if (imData)
    delete [] imData;
  if (imMask)
    delete [] imMask;
}


void CutSegment::setImageData(const uchar *grey) {
  memcpy(imData, grey, width * height);
  bFormatRGB = false;
}

 
void CutSegment::setImageData(const uchar *r, const uchar *g, const uchar *b) {
  int i,j;
  for (i=0, j=0; i<width*height; i++) {
    imData[j++] = r[i];
    imData[j++] = g[i];
    imData[j++] = b[i];
  }
  bFormatRGB = true;
}


void CutSegment::setSourceSink(const uchar *stMask, uchar source, uchar sink) {
  int idx;
  int maxl   = width*height;
  int *queue = new int[maxl];
  int sp, rp;

  // create EMask array
  for (idx=0; idx<maxl; idx++)
    imMask[idx] = IDX_UNDETERMINED;

  //search for first node marked as source and define at as THE source
  for (idx=0; idx<width*height; idx++)
    if (stMask[idx] == source)
      break; 
  if (idx >= width*height)
    throw ExceptionSourceNotDefined();
  imMask[idx] = IDX_SOURCE;
  setSource(idx / width, idx % width);

  //mark all nodes lying in the same component as the source
  sp = rp = 0;
  queue[sp++] = idx;
  
  while (sp % maxl != rp % maxl) {    
    idx = queue[rp++ % maxl];
    if (((idx+1)%width>0) && (stMask[idx+1] == source) && (imMask[idx+1]==IDX_UNDETERMINED)){
      queue[sp++ % maxl] = idx+1;
      imMask[idx+1] = IDX_SOURCE;
    }
    if ((idx%width>0) && (stMask[idx-1] == source) && (imMask[idx-1]==IDX_UNDETERMINED)) {
      queue[sp++ % maxl] = idx-1;
      imMask[idx-1] = IDX_SOURCE;
    }
    if ((idx+width<maxl) && (stMask[idx+width] == source) && (imMask[idx+width]==IDX_UNDETERMINED)) {
      queue[sp++ % maxl] = idx+width;
      imMask[idx+width] = IDX_SOURCE;
    }
    if ((idx>=width) && (stMask[idx-width] == source) && (imMask[idx-width]==IDX_UNDETERMINED)) {
      queue[sp++ % maxl] = idx-width;
      imMask[idx-width] = IDX_SOURCE;
    }
  }

  //search for first node marked as sink and define at as THE sink
  for (idx=0; idx<width*height; idx++)
    if (stMask[idx] == sink)
      break; 
  if (idx >= width*height)
    throw ExceptionSinkNotDefined();
  imMask[idx] = IDX_SINK;
  setSink(idx / width, idx % width);

  //mark all nodes lying in the same component as the source
  sp = rp = 0;
  queue[sp++] = idx;
  
  while (sp % maxl != rp % maxl) {
    idx = queue[rp++ % maxl];    
    if (((idx+1)%width>0) && (stMask[idx+1] == sink) && (imMask[idx+1]==IDX_UNDETERMINED)) {
      queue[sp++ % maxl] = idx+1;
      imMask[idx+1] = IDX_SINK;
    }
    if ((idx%width>0) && (stMask[idx-1] == sink) && (imMask[idx-1]==IDX_UNDETERMINED)) {
      queue[sp++ % maxl] = idx-1;
      imMask[idx-1] = IDX_SINK;
    }
    if ((idx+width<maxl) && (stMask[idx+width] == sink) && (imMask[idx+width]==IDX_UNDETERMINED)) {
      queue[sp++ % maxl] = idx+width;
      imMask[idx+width] = IDX_SINK;
    }
    if ((idx>=width) && (stMask[idx-width] == sink) && (imMask[idx-width]==IDX_UNDETERMINED)) {
      queue[sp++ % maxl] = idx-width;
      imMask[idx-width] = IDX_SINK;
    }
  }
  delete [] queue;
  return;
}
  
double CutSegment::gradient(double color1, double color2) {
  double diff, weight;
  diff = (color1-color2)*(color1-color2);
  weight = exp(-sqrt(diff));// + 0.0035;
  if (weight < EPSILON)
    weight = EPSILON;
  return weight;
}


double CutSegment::gradient(double color1[3], double color2[3]) {
  double diff, weight;
  diff = 0.0;
  for (int i=0; i<3; i++)
    diff += (color1[i]-color2[i])*(color1[i]-color2[i]);
  weight = exp(-sqrt(diff));// + 0.0035;
  if (weight < EPSILON)
    weight = EPSILON;
  return weight;
}
  
double CutSegment::segment() {
  return getMaxFlow();
}

CutPlanar::ELabel CutSegment::getLabel(int x, int y) {
  return CutGrid::getLabel(y, x);
}

void CutSegment::getLabels(CutPlanar::ELabel *lmask) {
  int i, j, l;
  for (j=0, l=0; j<height; j++)
    for (i=0; i<width; i++, l++)
      lmask[l] = getLabel(i, j);
}
