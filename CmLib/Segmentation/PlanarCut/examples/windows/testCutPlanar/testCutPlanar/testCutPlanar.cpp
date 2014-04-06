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
#include "CutPlanar.h"
#include <iostream>

using namespace std;

int _tmain(int argc, _TCHAR* argv[]) {

  PlanarVertex vertex[ 9];
  PlanarEdge     edge[13];  
  PlanarFace     face[ 6];
  edge[ 0].setEdge(vertex+0, vertex+1, face+5, face+0, 1, 1);  
  edge[ 1].setEdge(vertex+0, vertex+2, face+0, face+1, 1, 1);  
  edge[ 2].setEdge(vertex+0, vertex+3, face+1, face+5, 1, 1);  
  edge[ 3].setEdge(vertex+1, vertex+2, face+2, face+0, 1, 1);  
  edge[ 4].setEdge(vertex+2, vertex+3, face+3, face+1, 1, 1);  
  edge[ 5].setEdge(vertex+1, vertex+6, face+5, face+2, 1, 1);  
  edge[ 6].setEdge(vertex+2, vertex+4, face+2, face+3, 1, 1);  
  edge[ 7].setEdge(vertex+3, vertex+5, face+3, face+5, 1, 1);  
  edge[ 8].setEdge(vertex+4, vertex+5, face+4, face+3, 1, 1);  
  edge[ 9].setEdge(vertex+4, vertex+7, face+2, face+4, 1, 1);  
  edge[10].setEdge(vertex+5, vertex+8, face+4, face+5, 1, 1);  
  edge[11].setEdge(vertex+6, vertex+7, face+5, face+2, 1, 1);  
  edge[12].setEdge(vertex+7, vertex+8, face+5, face+4, 1, 1);

  PlanarEdge *edges_CCW[4];
  edges_CCW[0] = edge+ 0; edges_CCW[1] = edge+ 2; edges_CCW[2] = edge+ 1;
  vertex[0].setEdgesCCW(edges_CCW, 3);  
  edges_CCW[0] = edge+ 0; edges_CCW[1] = edge+ 3; edges_CCW[2] = edge+ 5;
  vertex[1].setEdgesCCW(edges_CCW, 3);  
  edges_CCW[0] = edge+ 1; edges_CCW[1] = edge+ 4; edges_CCW[2] = edge+ 6; edges_CCW[3] = edge+ 3;
  vertex[2].setEdgesCCW(edges_CCW, 4);  
  edges_CCW[0] = edge+ 2; edges_CCW[1] = edge+ 7; edges_CCW[2] = edge+ 4;
  vertex[3].setEdgesCCW(edges_CCW, 3);  
  edges_CCW[0] = edge+ 6; edges_CCW[1] = edge+ 8; edges_CCW[2] = edge+ 9;
  vertex[4].setEdgesCCW(edges_CCW, 3);  
  edges_CCW[0] = edge+ 7; edges_CCW[1] = edge+10; edges_CCW[2] = edge+ 8;
  vertex[5].setEdgesCCW(edges_CCW, 3);  
  edges_CCW[0] = edge+ 5; edges_CCW[1] = edge+11;
  vertex[6].setEdgesCCW(edges_CCW, 2);  
  edges_CCW[0] = edge+ 9; edges_CCW[1] = edge+12; edges_CCW[2] = edge+11;
  vertex[7].setEdgesCCW(edges_CCW, 3);  
  edges_CCW[0] = edge+10; edges_CCW[1] = edge+12;
  vertex[8].setEdgesCCW(edges_CCW, 2);  

  CutPlanar planar_cut;
  planar_cut.initialize(9,vertex, 13,edge, 6,face);
  planar_cut.setSource(2);
  planar_cut.setSink  (6);

  double flow;
  flow = planar_cut.getMaxFlow();

  cout << "Maximal Flow: " << flow << endl;

  // --- example:
  //
  // CutPlanar::ELabel label;
  // vector<int>       labels;
  // label  = planar_cut.getLabel(5);
  // label  = planar_cut.getLabel(6);       
  // labels = planar_cut.getLabels(CutPlanar::LABEL_SOURCE);
  // labels = planar_cut.getLabels(CutPlanar::LABEL_SINK);

  // vector<int> dual_path;
  // vector<int> boundary;

  // dual_path = planar_cut.getCircularPath();
  // boundary  = planar_cut.getCutBoundary(CutPlanar::LABEL_SOURCE);
  // boundary  = planar_cut.getCutBoundary(CutPlanar::LABEL_SINK);

}
