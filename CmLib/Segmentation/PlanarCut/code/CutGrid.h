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

#ifndef __CUTGRID_H__
#define __CUTGRID_H__

#include "CutPlanar.h"
#include <stdio.h>



class CutGrid {
public:

	enum EDir { 
		DIR_EAST,
		DIR_NORTH,
		DIR_WEST,
		DIR_SOUTH,
	};

private:

	//dimensions of the grid
	int nCols;
	int nRows;

	//planar graph entities
	PlanarVertex *verts;
	PlanarEdge   *edges;  
	PlanarFace   *faces; 

	//metrics of the planar graph
	int nFaces;
	int nFacesPerRow;
	int nFacesPerCol;

	int nEdges;
	int nHorzEdgesPerRow;
	int nVertEdgesPerRow;
	int nHorzEdges;
	int nVertEdges;

	int nVerts;

	//planar cut related
	CutPlanar pc;

	int idxSource;
	int idxSink;

	typedef CapType (*EdgeCostFunc)(int row, int col, EDir dir);
	EdgeCostFunc edgeCostFunc;

	static CapType edgeCostNull(int row, int col, EDir dir);

public:
	CutGrid(int nRows, int nCols);
	virtual ~CutGrid();

	void setSource(int row, int col);
	void setSink(int row, int col);

	void getSource(int &row, int &col);
	void getSink(int &row, int &col);

	void setEdgeCostFunction(CapType (*edgeCostFunc)(int row, int col, EDir dir));

	virtual CapType edgeCost(int row, int col, EDir dir);

	double getMaxFlow();

	//returns the label of a the pixel at (x,y)
	CutPlanar::ELabel getLabel(int row, int col);

	void getLabels(CutPlanar::ELabel *lmask);
};

#endif
