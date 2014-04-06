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
#include <assert.h>

#pragma warning(disable:4244)

/***************************************************
 * Public Methods                                  *
 ***************************************************/
CutPlanar::CutPlanar() : nVerts(0), nEdges(0), nFaces(0),
			 verts(0), faces(0), edges(0),
			 sourceID(0), sinkID(0),
			 computedFlow(false),
			 maxFlow(0), capEps(0),
			 primalTreeNodes(0), plSource(0), plSink(0),
			 dualTreeParent(0), dualTreeEdge(0),
			 completelyLabeled(false),
			 isLabeled(0), labels(0),
			 isSourceBlocked(false)
{

  DynRoot::resetBlockAllocator();

}


CutPlanar::~CutPlanar() {

  //free primal spanning tree T
  if (primalTreeNodes) 
    delete [] primalTreeNodes;

  //free dual spanning tree T*
  if (dualTreeParent)
    delete [] dualTreeParent;

  if (dualTreeEdge)
    delete [] dualTreeEdge;

  //free label infrastructure
  if (isLabeled)
    delete [] isLabeled;
  if (labels)
    delete [] labels;

  DynRoot::resetBlockAllocator();

}


void CutPlanar::initialize(int numVerts, PlanarVertex *vertexList,
		           int numEdges, PlanarEdge   *edgeList,
		           int numFaces, PlanarFace   *faceList,
                           int idxSource, int idxSink, ECheckFlags checkInput) {
  
  nVerts = numVerts; verts = vertexList;
  nEdges = numEdges; edges = edgeList;
  nFaces = numFaces; faces = faceList;
  sourceID = idxSource<0?numVerts-1:idxSource;
  sinkID   = idxSink  <0?numVerts-1:idxSink;
  computedFlow = false;
  performChecks(checkInput);

  CapType capInf = 0, capMin = CAP_INF, cap, rcap;
  PlanarEdge *e;
  int i;

  //reset edge flags
  for (int i=0; i<nEdges; i++)
    edges[i].setFlags(0);

  //determine minimum weight that is considered = infinity...
  for (i=0, e=edges; i<numEdges; i++, e++) {
    cap = e->getCapacity();
    if (cap != CAP_INF)
      capInf += cap;

    cap = e->getRevCapacity();
    if (cap != CAP_INF)
      capInf += cap;
  }

  capInf += 1.;

  //...and set all infinity edges to this weight
  for (i=0, e=edges; i<numEdges; i++, e++) {
    if (e->getCapacity() == CAP_INF)
      e->setCapacity(capInf);

    if (e->getRevCapacity() == CAP_INF)
      e->setRevCapacity(capInf);
  }

  //virtually remove all edges with capacity zero
  capEps = 0;

  for (i=0, e=edges; i<numEdges; i++, e++) {
    
    cap  = e->getCapacity();
    rcap = e->getRevCapacity();

    if (!cap)
      capEps++;
    else if (cap < capMin) 
      capMin = cap;
      
    if (!rcap)
      capEps++;
    else if (rcap < capMin)
      capMin = rcap;

  }

  capEps = capMin / (capEps * 2);

  if (capEps == 0)   //the graph completely consists of zero edges
    capEps = 0.1;    

  for (i=0, e=edges; i<numEdges; i++, e++) {

    if (!e->getCapacity()) {
      e->setCapacity(capEps);
      e->setFlags(e->getFlags() | 2);
    }
    
    if (!e->getRevCapacity()) {
      e->setRevCapacity(capEps);
      e->setFlags(e->getFlags() | 4);
    }

  }

}


void CutPlanar::setSource(int idxSource) {
  computedFlow &= (idxSource == sourceID);
  sourceID = idxSource;
}


void CutPlanar::setSink(int idxSink) {
  computedFlow &= (idxSink == sinkID);
  sinkID = idxSink;
}



double CutPlanar::getMaxFlow() {

  DynRoot *pr, *prLeft, *prRight;
  DynLeaf *plTailD, *plHeadD, *plTailE, *plHeadE;

  int tailEIdx, headEIdx;

  PlanarEdge *peD, *peE;
  PlanarFace *pfLeft, *pfRight;

  CapType eArcCap, eAntiArcCap;

  bool bMapping;

  //basic checks
  if (sourceID == sinkID) throw ExceptionSourceSinkIdentical();

  if (computedFlow) 
    return maxFlow;

  if (!nVerts || !nFaces || !nEdges ||
      !verts  || !edges  || !faces)
    return 0;


  //allocate memory for primal and dual spanning tree T and T*
  if (primalTreeNodes)
    delete [] primalTreeNodes;
  primalTreeNodes = new DynLeaf[nVerts];

  if (dualTreeParent)
    delete [] dualTreeParent;
  dualTreeParent = new PlanarFace*[nVerts];
  memset(dualTreeParent, 0, sizeof(PlanarFace*)*nVerts);

  if (dualTreeEdge)
    delete [] dualTreeEdge;
  dualTreeEdge   = new PlanarEdge*[nVerts];
  memset(dualTreeEdge, 0, sizeof(PlanarEdge*)*nVerts);


  //perform all precomputations
  preFlow();
  constructSpanningTrees();

  //initialize
  maxFlow = 0;

  //  if (!isSourceBlocked) { //TODO (F) please check if ok

  //enter augmentation loop
  while (true) {

    pr = plSource->expose();
    plTailD = pr->getMinCostLeaf();

    //augmentation step
    CapType augCap = plTailD->getEdgeCost();
    pr->addCost(-augCap);
    maxFlow += augCap; 

    //the nodes between plHeadD and plSink lie on the 
    //same DynPath due to the call of expose()
    plHeadD = plTailD->getNextDyn(); 

    //find the edge that has is to be saturated
    ResultSplit sres;

    plHeadD->divide(&sres);
    plTailD->setWeakLink(0, 0, 0, false, 0); 

    peD = static_cast<PlanarEdge*>(sres.dataBefore);

    //update the capacity of peD in the graph as well
    if (!sres.mappingBefore) { 
      //the mapping-bit indicates, whether the forward capacity of the DynPath edge 
      //maps to the arc or the antiarc of the corresponding edge in the graph
      peD->setCapacity(sres.costBefore);
      peD->setRevCapacity(sres.costBeforeR);
      
      pfLeft  = peD->getTailDual();
      pfRight = peD->getHeadDual();
    } else {
      peD->setRevCapacity(sres.costBefore);
      peD->setCapacity(sres.costBeforeR);
      
      pfRight = peD->getTailDual();
      pfLeft  = peD->getHeadDual();
    }

    //get the edge that leads to the parent of peD's right face in T*
    peE = dualTreeEdge[getFaceIndex(pfRight)];


    //update dual spanning tree T*:
    //insert peD into T* ...
    int fIdx = getFaceIndex(pfRight);
    dualTreeParent[fIdx] = pfLeft;
    dualTreeEdge[fIdx]   = peD;

    // //... and obviate numerical issues
    // if (peD->getCapacity() < EPSILON)
    //   peD->setCapacity(0.0);
    // if (peD->getRevCapacity() < EPSILON)
    //   peD->setRevCapacity(0.0);

    //check if peE is properly defined or if pfRight is root of T*
    if (peE) {
      
      eArcCap     = peE->getCapacity();
      eAntiArcCap = peE->getRevCapacity();

      //identify arc with positive costs - 
      //the costs of the antiarc should be zero
      if (eArcCap) {
	tailEIdx = getVertIndex(peE->getTail());
	headEIdx = getVertIndex(peE->getHead());

	bMapping = false;
      } else {
	tailEIdx = getVertIndex(peE->getHead());
	headEIdx = getVertIndex(peE->getTail());

	bMapping = true;
      }

      plTailE = primalTreeNodes + tailEIdx;
      plHeadE = primalTreeNodes + headEIdx;

      pr = plTailE->expose();

    } else {

      //special case: pfRight is root => termination condition fulfilled
      break;

    } //consistency check peE

    
      //check the invariant that plTailE is successor of plTailD
    if (!pr || pr->getTail() != plTailD) {

      //invariant inactive: termination condition fulfilled
      break;

    }
    
    //replace the darts on the path between plTailD and plTailE by their antidarts
    if (plTailD != plTailE) {

      pr->reverse();
      plTailE->setWeakLink(0, 0, 0, false, 0);

    }

    //insert peD into the primal spanning tree T
    prLeft  = pr;                 //path between plTailD and plTailE
    prRight = plHeadE->expose();  //path between plHeadE and plSink

    if (!bMapping)
      prLeft->concatenate(prRight, 
			  eArcCap, eAntiArcCap, 
			  bMapping, 
			  peE);
    else
      prLeft->concatenate(prRight, 
			  eAntiArcCap, eArcCap, 
			  bMapping, 
			  peE);


  } //while(true)

    //  } //if (!isSourceBlocked)

  computedFlow = true;

  //remember a starting point in the loop in T* representing the cut
  pfStartOfCut = pfRight;

  // reset the whole label infrastructure
  if (isLabeled)
    delete [] isLabeled;
  isLabeled = new bool[nVerts];
  memset(isLabeled, false, sizeof(bool)*nVerts);
  isLabeled[sourceID] = isLabeled[sinkID] = true;

  if (labels)
    delete [] labels;
  labels = new ELabel[nVerts];
  labels[sourceID] = LABEL_SOURCE;
  labels[sinkID]   = LABEL_SINK;

  completelyLabeled = false;
  // end label infrastructure

  //correct the value of maximum flow by the epsilon edges 
  PlanarFace *curFace = pfStartOfCut;
  int curFaceIdx;
  PlanarEdge *curEdge;

  do {

    curFaceIdx = curFace - faces;
    curEdge = dualTreeEdge[curFaceIdx];

    //if the edge has epsilon weight in the direction 
    //from source to sink reduce the actual flow
    if (!(curEdge->getCapacity()) && (curEdge->getFlags() & 2))
      maxFlow -= capEps;
    else if (!(curEdge->getRevCapacity()) && (curEdge->getFlags() & 4))
      maxFlow -= capEps;

    //proceed to next edge in cut
    curFace = dualTreeParent[curFaceIdx];

  } while(curFace != pfStartOfCut);

  //compensate for numerical issues
  if (maxFlow < EPSILON)
    maxFlow = 0;
  
  return maxFlow;
}


  CutPlanar::ELabel CutPlanar::getLabel(int node) {
    if (!computedFlow) getMaxFlow();
    if ((completelyLabeled) || (isLabeled[node])) return labels[node];

    DynLeaf *currLeaf;
    DynRoot *currRoot;
    std::vector<int> visitedID;

    currLeaf = primalTreeNodes + node;
    while (!isLabeled[node]) {
      currRoot = currLeaf->getPath();
      currLeaf = currRoot->getTail();
      node = currLeaf - primalTreeNodes;
      visitedID.push_back(node);
      currLeaf = currLeaf->getWeakParent();
      if ((isLabeled[node]) || (currLeaf == 0)) break;
      node = currLeaf - primalTreeNodes;
      visitedID.push_back(node);
    }

    ELabel ret = isLabeled[node]?labels[node]:LABEL_SOURCE;
    while (!visitedID.empty()) {
      node = visitedID.back();
      visitedID.pop_back();
      labels[node]    = ret;
      isLabeled[node] = true;
    }
    return ret;
  }


  std::vector<int> CutPlanar::getLabels(ELabel label) {
    DynRoot   *path;
    DynLeaf   *leaf;
    int      leafID;
    ELabel curLabel;
    std::vector<int> vertices;

    if (!computedFlow) getMaxFlow();

    if (!completelyLabeled) {
      // compute all labels in O(N)
      for (int i=0; i<nVerts; i++) {
	if (isLabeled[i]) continue;
	path     = primalTreeNodes[i].getPath();
	leaf     = path->getTail();
	leafID   = leaf - primalTreeNodes;
	if (isLabeled[leafID])
	  curLabel = labels[leafID];
	else {
	  leaf     = leaf->getWeakParent();
	  if (leaf==0)
	    curLabel = LABEL_SOURCE;
	  else {
	    leafID   = leaf - primalTreeNodes;
	    curLabel = isLabeled[leafID]?labels[leafID]:getLabel(leafID);
	  }
	}
	leaf     = path->getHead();
	while (leaf) {
	  leafID = leaf - primalTreeNodes;
	  isLabeled[leafID] = true;
	  labels[leafID] = curLabel;
	  leaf = leaf->getNextDyn();
	}
      }
      completelyLabeled = true;
      delete [] isLabeled;
      isLabeled = 0;
    }

    // extract all relevant labels in O(N)
    for (int i=0; i<nVerts; i++) {
      if (labels[i]==label) {
	vertices.push_back(i);
      }
    }
    return vertices;
  }


  std::vector<int> CutPlanar::getCutBoundary(ELabel label) {
    if (!computedFlow) getMaxFlow();

    int         cutFace  = getFaceIndex(pfStartOfCut);  
    int         currFace = cutFace;
    PlanarEdge *currEdge;
    int         currHead, currTail;
    DynRoot *currRoot;
    DynLeaf *currLeaf;
    int      currLeafID;
    std::vector<int> boundary;

    while (true) {
      currEdge  = dualTreeEdge[currFace];
      currHead  = currEdge->getHead() - verts;
      currTail  = currEdge->getTail() - verts;
      if (!completelyLabeled) {
	// retrieve labels of 'currHead' and 'currTail'
	if (isLabeled[currHead]) {
	  if (!isLabeled[currTail]) {
	    isLabeled[currTail] = true;
	    labels[currTail]    = labels[currHead]==LABEL_SOURCE?LABEL_SINK:LABEL_SOURCE;
	    currLeaf   = primalTreeNodes + currTail;
	    currRoot   = currLeaf->getPath();
	    currLeaf   = currRoot->getTail();
	    currLeafID = currLeaf - primalTreeNodes;
	    isLabeled[currLeafID] = true;
	    labels[currLeafID]    = labels[currTail];
	  }
	} else {
	  if (isLabeled[currTail]) {
	    isLabeled[currHead] = true;
	    labels[currHead]    = labels[currTail]==LABEL_SOURCE?LABEL_SINK:LABEL_SOURCE;
	    currLeaf   = primalTreeNodes + currHead;
	    currRoot   = currLeaf->getPath();
	    currLeaf   = currRoot->getTail();
	    currLeafID = currLeaf - primalTreeNodes;
	    isLabeled[currLeafID] = true;
	    labels[currLeafID]    = labels[currHead];
	  } else {
	    labels[currHead]    = getLabel(currHead);
	    isLabeled[currHead] = isLabeled[currTail] = true;
	    labels[currTail]    = labels[currHead]==LABEL_SOURCE?LABEL_SINK:LABEL_SOURCE;
	    currLeaf   = primalTreeNodes + currTail;
	    currRoot   = currLeaf->getPath();
	    currLeaf   = currRoot->getTail();
	    currLeafID = currLeaf - primalTreeNodes;
	    isLabeled[currLeafID] = true;
	    labels[currLeafID]    = labels[currTail];
	  }
	}
	// end of head/tail labeling
      }
      boundary.push_back(labels[currHead]==label?currHead:currTail);
      currFace = dualTreeParent[currFace] - faces;
      if (currFace == cutFace) break; 
    }  
    return boundary;
  }


  std::vector<int> CutPlanar::getCircularPath() {
    if (!computedFlow) getMaxFlow();

    int         cutFace  = getFaceIndex(pfStartOfCut);  
    int         currFace = cutFace;
    std::vector<int> circel;
    while (true) {
      circel.push_back(currFace);
      currFace = dualTreeParent[currFace] - faces;
      if (currFace == cutFace) break; 
    }  
    return circel;
  }


  /***************************************************
   * Protected Methods                               *
   ***************************************************/
  void CutPlanar::preFlow() {

    CGNode **cgNodes;
    CGraph graph(nFaces + 1);

    int srcFaceIdx, dstFaceIdx;
    PlanarEdge *infEdge = verts[sinkID].getEdge(0);
    int infFaceIdx = (infEdge->getTail()-verts==sinkID)?(infEdge->getHeadDual()-faces):(infEdge->getTailDual()-faces);
    int i;

    cgNodes = new CGNode*[nFaces];

    for (i=0; i<nFaces; i++)
      cgNodes[i] = graph.addNode(i);


    //insert dual darts of input graph with costs lower than infinity
    for (i=0; i<nEdges; i++) {

      srcFaceIdx = getFaceIndex(edges[i].getTailDual());
      dstFaceIdx = getFaceIndex(edges[i].getHeadDual());

      graph.addEdge(cgNodes[srcFaceIdx], 
		    cgNodes[dstFaceIdx], 
		    edges[i].getCapacity());

      graph.addEdge(cgNodes[dstFaceIdx], 
		    cgNodes[srcFaceIdx], 
		    edges[i].getRevCapacity());
    
    }

    graph.runDijkstra(cgNodes[infFaceIdx]);

    int faceTIdx, faceHIdx;
    double w, rw;
    CapType eta;

    for (i=0; i<nEdges; i++) {
    
      faceTIdx = getFaceIndex(edges[i].getTailDual());
      faceHIdx = getFaceIndex(edges[i].getHeadDual());

      w  = edges[i].getCapacity();
      rw = edges[i].getRevCapacity();

      eta = cgNodes[faceHIdx]->dijkWeight - cgNodes[faceTIdx]->dijkWeight;

      w  = w  - eta;
      rw = rw + eta;

      //During Dijkstra's algorithm floating point errors accumulate
      //so that in the end the shortest path to a face is not
      //necessarily equal to the weakest edge in the clockwise
      //circle. As a consequence, weakest edges are likely not set to
      //zero, leaving the graph with clockwise circles. A remedy used
      //here is to force edges < EPSILON to zero.  Note, however, that
      //EPSILON depends on the maximal accumulatable error and thus on
      //the structure and (mainly) on the size of the graph.
      //Alternatively one could uniquely identify the predecessor edge
      //on the shortest path to the face and set it to zero
      //"manually".
      if (w < EPSILON)
      	w = 0;

      if (rw < EPSILON)
      	rw = 0;

      edges[i].setCapacity(w);
      edges[i].setRevCapacity(rw);
   
    }

    delete [] cgNodes;

  }


  void CutPlanar::performChecks(ECheckFlags checks) {
    // check whether the graph is connected 
    if (checks & CHECK_CONNECTIVITY) {
      int v, vNumE, e;
      bool *unconnected = new bool[nVerts];
      for (v=0; v<nVerts; v++) unconnected[v]=true;
      std::vector<int> boundary(1,0);
      while (boundary.size()>0) {
	v = boundary.back();
	unconnected[v] = false;
	boundary.pop_back();
	vNumE = verts[v].getNumEdges();
	for (e=0; e<vNumE; e++) {
	  PlanarEdge *pe = verts[v].getEdge(e);
	  int u1, u2;
	  u1 = pe->getHead()-verts;
	  u2 = pe->getTail()-verts;
	  if (unconnected[u1]) boundary.push_back(u1);
	  if (unconnected[u2]) boundary.push_back(u2);
	}
      }
      bool connected = true;
      for (v=0; v<nVerts; v++) {
	if (unconnected[v]) {
	  connected = false;
	  break;
	}
      }
      delete[] unconnected;
      if (!connected) throw ExceptionCheckConnectivity();
    }

    // check whether all edges have non-negative capacities
    if (checks & CHECK_NON_NEGATIVE_COST) {
      for (int edge=0; edge<nEdges; edge++) {
	if ((edges[edge].getCapacity()<0) || (edges[edge].getRevCapacity()<0))
	  throw ExceptionCheckNonNegativeCost();
      }
    }

    // check whether the graph is planar (some sanity checks)
    if (checks & CHECK_PLANARITY) {
      // Euler characteristic
      if (nVerts-nEdges+nFaces!=2) throw ExceptionCheckPlanarity();
      // check ccw integrity
      for(int vID=0; vID<nVerts; vID++) {
	PlanarVertex *v = verts+vID;
	for(int eID=0; eID<v->getNumEdges(); eID++) {
	  PlanarEdge *pe1 = v->getEdge(eID);
	  PlanarEdge *pe2 = v->getEdge(eID+1);
	  PlanarFace *faceLeftOf_e1  = (pe1->getTail()==v)?pe1->getTailDual():pe1->getHeadDual();
	  PlanarFace *faceRightOf_e2 = (pe2->getTail()==v)?pe2->getHeadDual():pe2->getTailDual();
	  if (faceLeftOf_e1 != faceRightOf_e2) {
	    throw ExceptionCheckPlanarity();
	  }
	}
      }
      // save to every face one edge
      int e0,u,v,f0,f1;
      int *firstEdge = new int[nFaces];
      for (f0=0; f0<nFaces; f0++) firstEdge[f0] = -1; 
      for (e0=0; e0<nEdges; e0++) {
	f0 = edges[e0].getTailDual() - faces;
	f1 = edges[e0].getHeadDual() - faces;
	if (firstEdge[f0]<0) firstEdge[f0] = e0;
	if (firstEdge[f1]<0) firstEdge[f1] = e0;
      }
      // check if every face has at least one edges
      // check if the edges along every face form a closed cycle
      bool sanity = true;
      bool *isSelected = new bool[nVerts];
      for (u=0; u<nVerts; u++) isSelected[u]=false;      
      std::vector<int> vertCycle;
      bool orient;          
      for (f0=0; f0<nFaces; f0++) {
	if (firstEdge[f0] < 0) {
	  sanity = false;
	  break;
	} 
	e0     = firstEdge[f0];
	orient = (edges[e0].getTailDual() == faces+f0);
	u      = (orient?edges[e0].getTail():edges[e0].getHead()) - verts;
	v      = (orient?edges[e0].getHead():edges[e0].getTail()) - verts;
	while (true) {
	  if (isSelected[u]) {
	    sanity = false;
	    break;
	  }
	  isSelected[u] = true;
	  vertCycle.push_back(u);
	  e0 = verts[v].getEdge(verts[v].getEdgeID(edges+e0)-1) - edges;
	  if (e0 == firstEdge[f0]) break;
	  u = v;
	  v = ((edges[e0].getTail()==verts+v)?edges[e0].getHead():edges[e0].getTail()) - verts;
	}
	if (!sanity) break;
	while (vertCycle.size()>0) {
	  u = vertCycle.back();
	  isSelected[u] = false;
	  vertCycle.pop_back();
	}
      }
      delete[] firstEdge;
      delete[] isSelected;
      if (sanity == false) throw ExceptionCheckPlanarity();
    }
  }


  /***************************************************
   * Private Methods                                 *
   ***************************************************/
  void CutPlanar::constructSpanningTrees() {

    //pointers to entities in the graph
    PlanarVertex *pvCurVert, *pvSource, *pvSink;
    PlanarEdge   *peCurEdge;
    PlanarFace   *pfLeft, *pfRight;

    //pointer to current node in primal spanning tree T
    DynLeaf *plCurNode;

    //indices of current edge and vertex
    short *maxEdgeIdx, *curEdgeIdx;
    int curVertIdx;

    //capacities of current edge in graph
    CapType arcCap, antiArcCap;
    CapType *pInDartCap, *pOutDartCap;

    //new branch for insertion in primal spanning tree T
    DynRoot *curBranch;
    int curBranchLength;
    DynLeaf **curBranchLeaves;

    //data for weak link in primal spanning tree T
    DynLeaf *linkNode;
    CapType  linkCost, linkCostR;
    bool     linkMapping;
    void    *linkData;

    //state variables
    bool bAddedNewPrimEdge; //true if new edges are being added, false in case of a backtrack

    //initialize first bit of edge flag to zero - this indicates whether an edge has been added to T*
    for (int i=0; i<nEdges; i++)
      edges[i].setFlags(edges[i].getFlags() & 0xfe);

    //set the source and sink pointers of the primal spanning tree
    //the primal spanning nodes and the vertices of the planar graph 
    //share the same index scheme
    plSource = primalTreeNodes + sourceID;
    plSink   = primalTreeNodes + sinkID;

    //  plSink->id = nVerts - 1;

    pvSource = verts + sourceID;
    pvSink   = verts + sinkID;

    //initialize depth search
    pvCurVert = pvSink;   //begin search at the sink
    plCurNode = plSink;

    curEdgeIdx = new short[nVerts];
    maxEdgeIdx = new short[nVerts];
  
    for (int i=0; i<nVerts; i++) {
      curEdgeIdx[i] = -1;
      maxEdgeIdx[i] = 0;
    }
  
    curVertIdx = getVertIndex(pvCurVert); 
    maxEdgeIdx[curVertIdx] = pvSink->getNumEdges();

    isSourceBlocked = true;
    bAddedNewPrimEdge = false;

    linkNode = 0;
    linkCost = linkCostR = 0;
    linkMapping = false;
    linkData = 0;

    curBranch = 0;
    curBranchLength = 0;
    curBranchLeaves = new DynLeaf*[nVerts];

  
    while (!(pvCurVert == pvSink && curEdgeIdx[curVertIdx] >= maxEdgeIdx[curVertIdx])) {

      curEdgeIdx[curVertIdx]++;
      peCurEdge = pvCurVert->getEdge(curEdgeIdx[curVertIdx]);

      PlanarVertex *pvTail = peCurEdge->getTail();
      PlanarVertex *pvDartTail;

      arcCap     = peCurEdge->getCapacity(); 
      antiArcCap = peCurEdge->getRevCapacity();

      //get capacities of the two darts
      if (pvCurVert == pvTail) { //edge points away from current vertex
	pInDartCap  = &antiArcCap;
	pOutDartCap = &arcCap;
	pvDartTail  = peCurEdge->getHead();
      } else { //edge points to current vertex
	pInDartCap  = &arcCap;
	pOutDartCap = &antiArcCap;
	pvDartTail  = peCurEdge->getTail();
      }
    

      //add edges to the dual spanning tree T* as long as...
      while ( (curEdgeIdx[curVertIdx] != maxEdgeIdx[curVertIdx]) && 
	      //...not all edges of the vertex have been visited yet AND...
	      ((!*pInDartCap) ||                                     
	       //...either the dart pointing to the current vertex has capcity 0...
	       (curEdgeIdx[getVertIndex(pvDartTail)] != -1)) ) {     
	//...or the vertex at the end of the edge has been visited already (or both).

	//check if the edge has not yet been added to T*
	if (!(peCurEdge->getFlags() & 1)) {

	  if (arcCap && antiArcCap) 
	    throw ExceptionUnexpectedError(); //throw ExceptionCyclesDetected(); 
	
	  //identify the faces left and right of the dart with positive capacity 
	  if (arcCap) {
	    pfRight = peCurEdge->getHeadDual();
	    pfLeft = peCurEdge->getTailDual();
	  } else {
	    pfRight = peCurEdge->getTailDual();
	    pfLeft = peCurEdge->getHeadDual();
	  }
      
	  int fIdx = getFaceIndex(pfLeft);
	  dualTreeParent[fIdx] = pfRight;
	  dualTreeEdge[fIdx]   = peCurEdge;

	  peCurEdge->setFlags((peCurEdge->getFlags() & 0xfe) + 1);

	}

	curEdgeIdx[curVertIdx]++;
      
	peCurEdge = pvCurVert->getEdge(curEdgeIdx[curVertIdx]);

	pvTail = peCurEdge->getTail();

	arcCap     = peCurEdge->getCapacity();
	antiArcCap = peCurEdge->getRevCapacity();

	//get capacities of the two darts
	if (pvCurVert == pvTail) { //edge points away from current vertex
	  pInDartCap  = &antiArcCap;
	  pOutDartCap = &arcCap;
	  pvDartTail  = peCurEdge->getHead();
	} else { //edge points to current vertex
	  pInDartCap  = &arcCap;
	  pOutDartCap = &antiArcCap;
	  pvDartTail  = peCurEdge->getTail();
	}
      

      } //finish adding edges to T*


      //check if a backtrack has to be performed
      if (curEdgeIdx[curVertIdx] == maxEdgeIdx[curVertIdx]) {

	if (pvCurVert != pvSink) {  //no backtrack at the sink

	  //go back via the edge that lead to the currrent vertex (current edge)
	  if (pvCurVert == peCurEdge->getHead())
	    pvCurVert = peCurEdge->getTail();
	  else
	    pvCurVert = peCurEdge->getHead();

	  curVertIdx = getVertIndex(pvCurVert);

	  //check if there has been found a new primary spanning tree edge in the last step
	  if (bAddedNewPrimEdge && curBranchLength) {  
	  
	    curBranch = DynRoot::DynRootFromLeafChain(curBranchLeaves, curBranchLength);

	    curBranch->getTail()->setWeakLink(linkNode,
					      linkCost, linkCostR,
					      linkMapping,
					      linkData);

	    curBranch = 0;
	    curBranchLength = 0;
	  }

	  bAddedNewPrimEdge = false;

	  //perform backtrack in the primal spanning tree as well
  
	  plCurNode = plCurNode->getNext();

	}

      } else {  //found another edge for the primal spanning tree T

	//check if it was preceded by a backtrack
	if (!bAddedNewPrimEdge) { 
	  //begin new branch
	  linkNode    = plCurNode;  
	  linkCost    = *pInDartCap;
	  linkCostR   = *pOutDartCap;
	  linkMapping = (pInDartCap == &antiArcCap);
	  linkData    = (void *)peCurEdge;

	  curBranchLength = 0;
	}
      
	bAddedNewPrimEdge = true;

	if (pInDartCap == &arcCap)
	  pvCurVert = peCurEdge->getTail();
	else
	  pvCurVert = peCurEdge->getHead();

	if (pvCurVert == pvSource)
	  isSourceBlocked = false;

	curVertIdx = getVertIndex(pvCurVert);
      
	curEdgeIdx[curVertIdx] = pvCurVert->getEdgeID(peCurEdge);
	maxEdgeIdx[curVertIdx] = curEdgeIdx[curVertIdx] + pvCurVert->getNumEdges();

	plCurNode = primalTreeNodes + curVertIdx;

	// plCurNode->id = curVertIdx;

	//add the current node to the new branch
	curBranchLeaves[curBranchLength++] = plCurNode;

	plCurNode->setWeakLink(0,
			       *pInDartCap, *pOutDartCap,
			       pInDartCap == &antiArcCap,
			       peCurEdge);

      } //backtrack oder new edge in primal spanning tree T


    } //finish building the spanning trees


    delete [] curEdgeIdx;
    delete [] maxEdgeIdx;
    delete [] curBranchLeaves;

  }

  #pragma warning(default:4244)