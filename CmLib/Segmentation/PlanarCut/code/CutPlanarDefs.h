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

#ifndef __PLANARCUTDEFS_H
#define __PLANARCUTDEFS_H

#include <limits>


#define CAP_INF std::numeric_limits<double>::max()

const double EPSILON = 1e-6;       //used for numerical issues

typedef double CapType;      /* data type for flow capacity */
typedef unsigned char uchar; /* for convenience             */
typedef unsigned int uint;

inline CapType mmin(CapType a, CapType b) {

  if (a < b)
    return a;
  
  return b;

}

inline CapType mmax(CapType a, CapType b) {

  if (a > b)
    return a;
  
  return b;

}

inline CapType mmin3(CapType a, CapType b, CapType c) {

    if (a < b) {
      if (a < c)
	return a;
      else 
	return c;
    } else {
      if (b < c)
	return b;
      else
	return c;
    }

}

inline CapType mmax3(CapType a, CapType b, CapType c) {

    if (a > b) {
      if (a > c)
	return a;
      else 
	return c;
    } else {
      if (b > c)
	return b;
      else
	return c;
    }

}

#endif
