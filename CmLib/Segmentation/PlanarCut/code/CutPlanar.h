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

#ifndef __CUTPLANAR_H__
#define __CUTPLANAR_H__
#include "PlanarException.h"
#include "CutPlanarDefs.h"
#include "Planar.h"
#include "DynPath.h"
#include "CGraph.h"
#include <vector>


class CutPlanar 
{
public:
  static const int FIRST_VERT =  0;
  static const int LAST_VERT  = -1;

  enum ECheckFlags 
  {
    CHECK_NONE              = 0x00,
    CHECK_CONNECTIVITY      = 0x01,
    CHECK_NON_NEGATIVE_COST = 0x02,
    CHECK_PLANARITY         = 0x04,
    CHECK_ALL               = 0xFF, 
  };

  enum ELabel
  {
    LABEL_SINK   = 0,
    LABEL_SOURCE = 1,
  };

  //allocates memory for nodes, edges and faces
  CutPlanar();
  virtual ~CutPlanar();

  //define graph
  //class works in state, i.e., the arrays may be altered.
  void initialize(int numVerts, PlanarVertex *vertexList,
		  int numEdges, PlanarEdge   *edgeList,
		  int numFaces, PlanarFace   *faceList,
		  int idxSource          = FIRST_VERT,  //sets which node should be source
		  int idxSink            = LAST_VERT,   //sets which node should be sink
		  ECheckFlags checkInput = CHECK_ALL);  //enables advanced input validation

  void setSource(int idxSource);
  void setSink  (int idxSink);


  double getMaxFlow();
  ELabel      getLabel(int node);                // returns the label of a node
  std::vector<int> getLabels(ELabel label);      // returns all nodes of a specific label
  std::vector<int> getCutBoundary(ELabel label); // returns all cut-nodes in the source or the sink set
  std::vector<int> getCircularPath();

protected:
  virtual void preFlow();

  virtual void performChecks(ECheckFlags checks);


  
private:
  // planar encoding
  int nVerts;
  int nEdges;
  int nFaces;
  PlanarVertex *verts;
  PlanarFace   *faces;
  PlanarEdge   *edges;
  // source and sink
  int sourceID; // previously PlanarVertex *pvSource
  int sinkID;   // previously PlanarVertex *pvSink
  
  bool computedFlow; // stores whether the flow is already computed
                     // has to be maintained by 'maxflow' and 'initialize'
  double maxFlow;
  double capEps;     // zero capacity darts are replaced by this value

  PlanarFace *pfStartOfCut; //if computedFlow, retains the first 
                            //face of the cut loop in T*

  //primal spanning tree
  DynLeaf *primalTreeNodes; //nodes of the primal spanning tree
  DynLeaf *plSource;  //pointer on source in primal spanning tree
  DynLeaf *plSink;    //pointer to sink in primal spanning tree
  //dual spanning tree
  PlanarFace **dualTreeParent; // dual tree parent relationship
  PlanarEdge **dualTreeEdge;  // dual tree fast edge-access

  //labeling
  bool completelyLabeled;
  bool *isLabeled;
  ELabel *labels;
  
  //set during constructSpanningTrees() if Source is blocked
  bool isSourceBlocked; 

  //definition of planar input graph

  //auxiliary inline functions
  int getVertIndex(PlanarVertex *pv) {return (int)(pv - verts);}
  int getFaceIndex(PlanarFace *pf)   {return (int)(pf - faces);}
  int getDynNodeIndex(DynLeaf *pl)   {return (int)(pl - primalTreeNodes);}

  //constructs the primal and dual spanning trees used by maxflow()
  void constructSpanningTrees();
};


#endif
