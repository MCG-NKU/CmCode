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

#include "CutGrid.h"


CapType CutGrid::edgeCostNull(int row, int col, EDir dir) {
  
  return 0.0;

}

CutGrid::CutGrid(int nRows, int nCols) : idxSource(0), idxSink(0) {

  //set standard edge cost function so edgeCostFunc is always non-null
  edgeCostFunc = &edgeCostNull;

  if (nCols > 0 && nRows > 0) {
    this->nCols = nCols;
    this->nRows = nRows;
  } else {
    this->nCols = nCols = 5;
    this->nRows = nRows = 5;
  }
 


  //compute metrics of the planar graph and reserve memory
  nFacesPerRow = nCols - 1;
  nFacesPerCol = nRows - 1;
  nFaces = nFacesPerRow * nFacesPerCol + 1; //inner faces + one outside face

  nHorzEdgesPerRow = nCols - 1;
  nVertEdgesPerRow = nCols;
  nHorzEdges = nHorzEdgesPerRow * nRows;
  nVertEdges = nVertEdgesPerRow * (nRows-1);
  nEdges = nHorzEdges + nVertEdges; //horizontal edges and vertical edges

  nVerts = nCols * nRows;

  verts = new PlanarVertex[nVerts];
  edges = new PlanarEdge[nEdges];
  faces = new PlanarFace[nFaces]; 




  //determine embedding for vertices
  PlanarEdge *edgesCCW[4];

  int i, j; //column and row counter
  int v, e; //vertex and edge counter

  for (j=0, v=0; j<nRows; j++) {
    for (i=0; i<nCols; i++, v++) {

      e = 0;

      if (i<nCols-1)
	edgesCCW[e++] = &edges[j*nHorzEdgesPerRow + i];
      
      if (j>0)
	edgesCCW[e++] = &edges[nHorzEdges + (j-1)*nVertEdgesPerRow + i];

      if (i>0)
	edgesCCW[e++] = &edges[j*nHorzEdgesPerRow + i - 1];

      if (j<nRows-1)
	edgesCCW[e++] = &edges[nHorzEdges + j*nVertEdgesPerRow + i];


      verts[v].setEdgesCCW(edgesCCW, e);
      
    }
  }

}

CutGrid::~CutGrid() {

  if (verts)
    delete [] verts;

  if (edges)
    delete [] edges;

  if (faces)
    delete [] faces;

}


void CutGrid::setSource(int row, int col) {

  if (row >= 0 && row < nRows &&
      col >= 0 && col < nCols)
    idxSource = row * nCols + col;
  
}

void CutGrid::setSink(int row, int col) {

  if (row >= 0 && row < nRows &&
      col >= 0 && col < nCols)
    idxSink = row * nCols + col;

}


void CutGrid::getSource(int &row, int &col) {

  row = idxSource / nCols;
  col = idxSource % nCols;

}

void CutGrid::getSink(int &row, int &col) {

  row = idxSink / nCols;
  col = idxSink % nCols;

}


void CutGrid::setEdgeCostFunction(EdgeCostFunc edgeCostFunc) {

  if (edgeCostFunc)
    this->edgeCostFunc = edgeCostFunc;
  else
    this->edgeCostFunc = edgeCostNull;

}


double CutGrid::edgeCost(int row, int col, EDir dir) {

  //this call is safe, since we made sure that edgeCostFunc is always non-null
  return edgeCostFunc(row, col, dir);

}

double CutGrid::getMaxFlow() {

  int i, j; //column and row counter
  int e; //vertex and edge counter

  //add horizontal edges
  for (j=0, e=0; j<nRows; j++)
    for (i=0; i<nHorzEdgesPerRow; i++, e++)
      edges[e].setEdge(&verts[j*nCols + i], 
		       &verts[j*nCols + i + 1], 
		       &faces[(e-nFacesPerRow<0) ? (nFaces-1) : e-nFacesPerRow],
		       &faces[(e>nFaces-1) ? (nFaces-1) : e],
		       edgeCost(j, i, DIR_EAST),
		       edgeCost(j, i+1, DIR_WEST)
		       );

  //add vertical edges
  for (j=0, e=nHorzEdges; j<nRows-1; j++)
    for (i=0; i<nVertEdgesPerRow; i++, e++)
      edges[e].setEdge(&verts[j*nCols + i], 
		       &verts[(j+1)*nCols + i], 
		       &faces[(i>=nFacesPerRow) ? (nFaces-1) : (j*nFacesPerRow + i)],
		       &faces[(i==0) ? (nFaces-1) : (j*nFacesPerRow + i-1)],
		       edgeCost(j, i, DIR_SOUTH),
		       edgeCost(j+1, i, DIR_NORTH)
		       );


  pc.initialize(nVerts, verts, nEdges, edges, nFaces, faces, 
		idxSource, idxSink, CutPlanar::CHECK_NONE);


  return pc.getMaxFlow();
  
}

CutPlanar::ELabel CutGrid::getLabel(int row, int col) {
  if ((row >= 0) && (row < nRows) &&
      (col >= 0) && (col < nCols))
    return pc.getLabel(row*nCols + col);
  throw ExceptionUnexpectedError();
}

void CutGrid::getLabels(CutPlanar::ELabel *lmask) {
  for (int row=0; row<nRows; row++) {
    for (int col=0; col<nCols; col++) {
      lmask[row*nCols + col] = pc.getLabel(row*nCols + col);
    }
  }
}


