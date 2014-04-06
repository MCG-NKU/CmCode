/*****************************************************************************
*    PlanarCut - software to compute MinCut / MaxFlow in a planar graph      *
*                                Version 1.0                                 *
*                                                                            *
*    Copyright 2011 Eno Töppe <toeppe@in.tum.de>                             *
*                   Frank R. Schmidt <fschmidt@uwo.ca>                       *
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

#include "CutShape.h"
#include <iostream>

using namespace std;

//number of shape descriptor points
#define N 10

int _tmain(int argc, _TCHAR* argv[])
{

  double shape1[N], shape2[N];
  double dissim[N*N];
  CutShape algorithm;

  int offset=3;
  for (int i=0; i<N; i++) shape1[i] = i;
  for (int i=0; i<N; i++) shape2[i] = (i+offset)%N;

  for (int i=0; i<N; i++) {
    for (int j=0; j<N; j++) {
      dissim[i*N+j] = shape1[i]-shape2[j];
      dissim[i*N+j] *= dissim[i*N+j];
    }
  }

  algorithm.setDissimilarityMatrix(N, dissim, CutShape::VERTEX_BASED);
  cout << "Matching Score: " << algorithm.getMatchingScore() << "\n";

  vector< pair<int,int> > path = algorithm.getMatching();
  cout << "Matching: ";

  for (unsigned i=0; i<path.size(); i++)
    cout << "(" << path[i].first << "," << path[i].second << ")  ";
  cout << endl;
  
}

