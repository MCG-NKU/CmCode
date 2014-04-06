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
#include "CutGrid.h"

using namespace std;


CapType edgeCost3x3(int row, int col, CutGrid::EDir dir) {
  if ((dir == CutGrid::DIR_SOUTH) || (dir == CutGrid::DIR_NORTH)) return 3;
  if ((dir == CutGrid::DIR_EAST)  && (col == 0)) return 6;
  if ((dir == CutGrid::DIR_WEST)  && (col == 1)) return 6;
  if (row == 0) return 10;
  if (row == 1) return 9;
  return 4;
}

int _tmain(int argc, _TCHAR* argv[]) {

  CutGrid grid_cut(3,3);

  grid_cut.setEdgeCostFunction(edgeCost3x3);
  grid_cut.setSource(1,0);
  grid_cut.setSink(0,2);

  grid_cut.getMaxFlow();

  CutPlanar::ELabel  label;
  CutPlanar::ELabel *labels;

  //read out selected labels
  label  = grid_cut.getLabel(1,2);
  cout << "label at grid point (1,2) = " 
       << (label==CutPlanar::LABEL_SOURCE?"SOURCE(=1)":"SINK(=0)") << endl;
 
  label  = grid_cut.getLabel(0,0);
  cout << "label at grid point (0,0) = " 
       << (label==CutPlanar::LABEL_SOURCE?"SOURCE(=1)":"SINK(=0)") << endl << endl;

  //get all the labels at once
  labels = new CutPlanar::ELabel[9];
  grid_cut.getLabels(labels);

  cout << "all grid labels:\n";
  for (int i=0; i<3; i++) {
    for (int j=0; j<3; j++)
      cout << labels[i*3+j] << " ";
    cout << endl;
  }
	
  cout << endl;

  return 0;

}
