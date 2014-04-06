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

#ifndef __CUTSEGMENT_H__
#define __CUTSEGMENT_H__

#include "CutGrid.h"
#include "CutPlanarDefs.h"


class CutSegment;


class CutSegment : private CutGrid
{
 private:

  int DirToOfs[4];

  enum EMask { 
    IDX_SINK,
    IDX_SOURCE,
    IDX_UNDETERMINED,
  };

  uchar *imData;
  EMask *imMask;
  bool bFormatRGB;

  int width;
  int height;

  CapType edgeCost(int row, int col, EDir dir);

 public:
  CutSegment(int width, int height);
  virtual ~CutSegment();

  void setImageData(const uchar *grey);
  void setImageData(const uchar *r, const uchar *g, const uchar *b);

  void setSourceSink(const uchar *stMask, uchar source, uchar sink);
  
  virtual double gradient(double color1, double color2);       //greyscale information of adjacent pixels
  virtual double gradient(double color1[3], double color2[3]); //RGB-color information of adjacent pixels
  
  double segment();

  CutPlanar::ELabel getLabel(int x, int y);

  void getLabels(CutPlanar::ELabel *lmask);
};


#endif
