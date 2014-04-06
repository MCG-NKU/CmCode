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

#include "CutPlanar.h"
#include "CutShape.h"
#include "PlanarException.h"

#define SQRT05 0.7071067811865475244008443

#pragma warning(disable:4244)

CutShape::~CutShape() {
  // internal graph
  if (verts) delete [] verts;
  if (edges) delete [] edges;
  if (faces) delete [] faces;
}


void CutShape::setDissimilarityMatrix(int n, double *dissimilarity, DTWMode mode) {
  matchingMode = mode;
  shapeNum     = n;
  isMatched    = false;
  // delete existing memory
  matching.clear();
  if (verts) delete [] verts;
  if (edges) delete [] edges;
  if (faces) delete [] faces;
  // reserve memory for the graph
  int numEh, numEv;
  PlanarEdge *edgesH, *edgesV, *edgesD;

  vertsPerRow = 4 * shapeNum;
  facesPerRow = 2 * shapeNum + 1;
  vertRows    = shapeNum;
  faceRows    = shapeNum + 1;

  numF  = facesPerRow * faceRows - (shapeNum+1); // cylinder connectivity
  numV  = vertsPerRow * vertRows + 2;            // source and sink
  numE  = numF + numV - 2;
  numEh = 2 * (shapeNum * shapeNum) + shapeNum;  // horizontal edges
  numEv = 2 * (shapeNum * shapeNum) + shapeNum;  // vertical edges

  faces  = new PlanarFace[numF];
  verts  = new PlanarVertex[numV];
  edges  = new PlanarEdge[numE];
  edgesH = edges;
  edgesV = edges + numEh;
  edgesD = edges + numEh + numEv;
  sink   = verts + (numV - 1);
  source = verts + (numV - 2);
  
  // define edges
  int edgeID;
  PlanarVertex *u, *v;
  double weight, weightR;
  // define horizontal edges
  edgeID = 0;
  for (int y=0; y<faceRows; y++) {
    for (int x=0; (y==faceRows-1) ? (x<shapeNum) : (x<facesPerRow-1); x++) {
      u       = vertXY(x*2+1, y);
      v       = vertXY(x*2,   y-1);
      weight  = getEdgeWeight(x, y, dissimilarity, HORIZONTAL);
      weightR = CAP_INF;
      if (u == source || v == sink) {
	if (weight < EPSILON) weight = EPSILON;
        weightR = 0;
      } 
      edgesH[edgeID++].setEdge(u, v, faceXY(x+1,y), faceXY(x,y), weight, weightR);
    }
  }
  // define vertical edges
  edgeID = 0;
  for (int y=0; y<faceRows-1; y++) {
    for (int x=0; x<facesPerRow; x++) {
      u       = vertXY(x*2-1, y);
      v       = vertXY(x*2  , y);
      weight  = getEdgeWeight(x, y, dissimilarity, VERTICAL);
      weightR = CAP_INF;
      if (u == source || v == sink) {
	if (weight < EPSILON) weight = EPSILON;
	weightR = 0;
      }
      edgesV[edgeID++].setEdge(u, v, faceXY(x,y+1), faceXY(x,y), weight, weightR);
    }
  }
  // define diagonal edges
  edgeID = 0;
  for (int y=0; y<faceRows-1; y++) {
    for (int x=0; x<facesPerRow-1; x++) {
      u       = vertXY(x*2,   y);
      v       = vertXY(x*2+1, y);
      weight  = getEdgeWeight(x, y, dissimilarity, DIAGONAL);
      weightR = CAP_INF;
      if (u == source || v == sink) {
	if (weight < EPSILON) weight = EPSILON;
	weightR = 0;
      }
      edgesD[edgeID++].setEdge(u, v, faceXY(x+1,y+1), faceXY(x,y), weight, weightR);
    }
  }
  //define ccw for every vertex (except source and sink)
  PlanarEdge **ccw;
  int dIdx, hIdx, vIdx; 
  hEdgesPerRow = facesPerRow - 1;
  vEdgesPerRow = facesPerRow;
  dEdgesPerRow = facesPerRow - 1;

  ccw = new PlanarEdge*[3];
  dIdx = hIdx = vIdx = 0;
  for (int y=0; y<vertRows; y++) {
    int x1, x2;
    for (x1=0, x2=1; x1<vertsPerRow; x1+=2, x2+=2) {
      ccw[0] = edgesD + dIdx;
      ccw[1] = edgesH + ((hIdx + hEdgesPerRow) % numEh); //cyclic
      ccw[2] = edgesV + vIdx;
      (vertXY(x1,y))->setEdgesCCW(ccw, 3);

      ccw[1] = edgesH + hIdx;
      ccw[2] = edgesV + (vIdx + 1);
      (vertXY(x2,y))->setEdgesCCW(ccw, 3);

      dIdx++; hIdx++; vIdx++;
    }
    vIdx++; //between two rows, the verts do not have one single common edge
  }
  delete [] ccw;
  //define source edges
  ccw = new PlanarEdge*[2*shapeNum];
  for (edgeID=0, vIdx=0; edgeID<shapeNum; edgeID++, vIdx+=vEdgesPerRow) 
    ccw[edgeID] = edgesV + vIdx;
  for (edgeID=shapeNum, hIdx=shapeNum*hEdgesPerRow; edgeID<shapeNum*2; edgeID++,hIdx++)
    ccw[edgeID] = edgesH + hIdx;
  source->setEdgesCCW(ccw, shapeNum*2);
  //define sink edges
  for (edgeID=0, vIdx=shapeNum*vEdgesPerRow-1; edgeID<shapeNum; edgeID++, vIdx -= vEdgesPerRow)
    ccw[edgeID] = edgesV + vIdx;
  for (edgeID=shapeNum, hIdx=hEdgesPerRow-1; edgeID<shapeNum*2; edgeID++, hIdx--)
    ccw[edgeID] = edgesH + hIdx;
  sink->setEdgesCCW(ccw, shapeNum*2);
  delete [] ccw;
}


void CutShape::computeMatching() {
  CutPlanar algorithm;
  std::vector<int> matchingPath;

  algorithm.initialize(
        numV, verts, numE, edges, numF, faces, 
        source-verts, sink-verts, CutPlanar::CHECK_ALL);
  matchingScore = algorithm.getMaxFlow();
  isMatched     = true;
  matchingPath  = algorithm.getCircularPath();

  int pShape1, pShape2;
  int num, faceID;
  std::pair<int,int> pt;
  matching.clear();

  num = (int)matchingPath.size();

  for (int entry=0; entry<num; entry++) {
    faceID    = matchingPath[entry];
    pShape1   = faceID % facesPerRow;
    pShape2   = faceID / facesPerRow;
    pt.first  = pShape1 % shapeNum;
    pt.second = pShape2 % shapeNum;
    matching.push_back(pt);
  }
}


PlanarVertex *CutShape::vertXY(int x, int y) {
  if (x==-1) return source;
  if (x >= vertsPerRow) return sink;
  if (y == -1) {
    //cyclic
    if ((x >= 0) && (x < vertsPerRow/2)) {
      x += vertsPerRow/2;
      y  = vertRows-1;
    } else if (x >= vertsPerRow/2) {
      return sink;
    }
  } else if (y == vertRows) {
    //cyclic
    if ((x >= vertsPerRow/2) && (x < vertsPerRow)) {
      x -= vertsPerRow/2;
      y  = 0;
    } else if (x < vertsPerRow/2) {
      return source;
    }
  }
  return verts + (y*vertsPerRow + x);
}


PlanarFace *CutShape::faceXY(int x, int y) {
  if (y == -1) {
    x += facesPerRow/2;
    y = faceRows - 1;
  }
  if ((y == faceRows-1) && (x >= facesPerRow/2)) {
    y = 0;
    x -= facesPerRow/2;
  }
  return faces + (y*facesPerRow + x);
}


double CutShape::getEdgeWeight(int x, int y, double* dissimilarity, edgeDirection d) {
  double featDiffU, featDiffV;
  double fac;
  featDiffU = dissimilarity[(y % shapeNum)*shapeNum + (x % shapeNum)];
  switch (d) {
    case HORIZONTAL:
      featDiffV = dissimilarity[(y % shapeNum)*shapeNum + ((x+1) % shapeNum)];
      fac       = 0.5;
    break;
    case VERTICAL:
      featDiffV = dissimilarity[((y+1) % shapeNum)*shapeNum + (x % shapeNum)];
      fac       = 0.5;
    break;
    case DIAGONAL:
      featDiffV = dissimilarity[((y+1) % shapeNum)*shapeNum + ((x+1) % shapeNum)];
      fac       = matchingMode==EDGE_BASED?SQRT05:0.5;
    break;
    default:
      throw ExceptionUnexpectedError();
    break;
  }
  return (featDiffU + featDiffV) * fac;
}

#pragma warning(default:4244)
