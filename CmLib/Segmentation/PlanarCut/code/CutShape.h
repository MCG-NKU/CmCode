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

#include <vector>
#include "Planar.h"

class CutShape
{

public:
  enum DTWMode {
    VERTEX_BASED,
    EDGE_BASED,
  };

  CutShape() : 
	shapeNum(0), isMatched(false),
	numV(0), numE(0), numF(0), verts(0), edges(0), faces(0) {};
  ~CutShape();

  void setDissimilarityMatrix(int n, double *dissimilarity, DTWMode mode=EDGE_BASED);
  inline double getMatchingScore();
  inline std::vector< std::pair<int,int> > getMatching(); //returns the matching interpretation of the cut


private:
  enum edgeDirection {
    HORIZONTAL,
    VERTICAL,
    DIAGONAL
  };

  void computeMatching();
  PlanarVertex* vertXY(int x, int y);
  PlanarFace*   faceXY(int x, int y);
  double getEdgeWeight(int x, int y, double *dissimilarity, edgeDirection d);

  DTWMode matchingMode;
  int shapeNum;
  bool isMatched;
  double matchingScore;
  std::vector< std::pair<int,int> > matching;
  // internal graph
  int numV, numE, numF;
  PlanarVertex* verts;
  PlanarEdge*   edges;
  PlanarFace*   faces;
  PlanarVertex *source, *sink;
  // ShapeCut specific graph related variables
  int vertsPerRow, facesPerRow, vertRows, faceRows;
  int hEdgesPerRow, vEdgesPerRow, dEdgesPerRow;
};

/***************************************************
 *** CutShape INLINE *******************************
 ***************************************************/
inline double CutShape::getMatchingScore() {
  if (!isMatched) computeMatching();
  return matchingScore;
}


inline std::vector< std::pair<int,int> > CutShape::getMatching() {
  if (!isMatched) computeMatching();
  return matching;
}

