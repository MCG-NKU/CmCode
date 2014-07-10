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

#include "DynPath.h"
#include <iostream>
#include <math.h>
#include <assert.h>
#include <algorithm>

using namespace std;

//static member definitions
//stack pointer
int DynLeaf::idxRightSide;
int DynLeaf::idxLeftSide;
int DynLeaf::idxCostR;
int DynLeaf::idxCostL;
int DynLeaf::idxMappingR;
int DynLeaf::idxMappingL;
int DynLeaf::idxDataL;
int DynLeaf::idxDataR;
int DynLeaf::idxRPath;
  
//stacks
DynRoot* DynLeaf::stackRightSide[STACKSIZE]; 
DynRoot* DynLeaf::stackLeftSide[STACKSIZE];  
CapType  DynLeaf::stackCostR[STACKSIZE];     
CapType  DynLeaf::stackCostL[STACKSIZE];     
bool     DynLeaf::stackMappingR[STACKSIZE];  
bool     DynLeaf::stackMappingL[STACKSIZE];  
void*    DynLeaf::stackDataR[STACKSIZE];     
void*    DynLeaf::stackDataL[STACKSIZE];     
DynNode* DynLeaf::stackRPath[STACKSIZE];     


BlockAllocator<DynNode> DynRoot::blockAllocator;


/***************************************************
 *** DynNode *****************************************
 ***************************************************/
DynNode::DynNode() {
  reversed = 0;

  bParent = 0;
  bHead   = 0;
  bTail   = 0;
  bLeft   = 0;
  bRight  = 0;

  netCost  = CapType(0);
  netMin   = CapType(0);
  netCostR = CapType(0);
  netMinR  = CapType(0);

  data = 0;

  height  = 0;
}




void DynNode::rotateRight(CapType grossminU, CapType grossminUR) {
  DynNode *u, *v;
  CapType *pNetMin, *pNetMinR;
  bool rState;

  if (isLeaf()) return;
  normalizeReverseState();

  u = this;
  v = bLeft;

  if (v->isLeaf()) return;
  v->normalizeReverseState();
  
  //save original node data of u and v
  DynNode uold = *this;
  DynNode vold = *v;

  bool uMapping = uold.getMapping();
  bool vMapping = vold.getMapping();

  void *uData = uold.data;
  void *vData = vold.data;

  CapType minU  = grossminU;   //TODO: use grossminU/R directly?
  CapType minUR = grossminUR;

  CapType minVOld  = v->netMin  + minU; 
  CapType minVOldR = v->netMinR + minUR;

  CapType costU  = u->netCost  + minU;
  CapType costUR = u->netCostR + minUR;

  CapType costV  = v->netCost  + minVOld;
  CapType costVR = v->netCostR + minVOldR;

  CapType minVl  = CAP_INF;
  CapType minVlR = CAP_INF;
  
  CapType minVr  = CAP_INF;
  CapType minVrR = CAP_INF;

  CapType minUr  = CAP_INF;
  CapType minUrR = CAP_INF;

  if (!v->bLeft->isLeaf()) {
    v->bLeft->getNetMinPtr(&pNetMin, &pNetMinR);
    
    minVl  = *pNetMin  + minVOld;
    minVlR = *pNetMinR + minVOldR;
  }

  if (!v->bRight->isLeaf()) {
    v->bRight->getNetMinPtr(&pNetMin, &pNetMinR);
    
    minVr  = *pNetMin  + minVOld;
    minVrR = *pNetMinR + minVOldR;
  }

  if (!u->bRight->isLeaf()) {
    u->bRight->getNetMinPtr(&pNetMin, &pNetMinR);

    minUr  = *pNetMin  + minU;
    minUrR = *pNetMinR + minUR;
  }

  

  DynNode *vnew = u; //let the old DynNode for u be the v after rotation
  DynNode *unew = v; //let the old DynNode for v be the u after rotation

  //restructure tree with u being new root
  vnew->setAsLChild(vold.bLeft, false);
  unew->setAsLChild(vold.bRight, false);
  unew->setAsRChild(uold.bRight, false);
  vnew->setAsRChild(unew, false);

  //update netMin fields of unew, vnew and their respective children

  //vnew is the new root of a tree that contains the same nodes as before
  //=> grossMin(vnew) = grossMin(uold)
  CapType minVNew  = minU;
  CapType minVNewR = minUR;

  CapType minUNew  = mmin3(costU, minUr, minVr);
  CapType minUNewR = mmin3(costUR, minUrR, minVrR);

  unew->setNetMin(minUNew  - minVNew,  false);
  unew->setNetMin(minUNewR - minVNewR, true);

  if (!vnew->bLeft->isLeaf()) {
    rState = vnew->bLeft->getReversed();

    vnew->bLeft->setNetMin(minVl  - minVNew,   rState);
    vnew->bLeft->setNetMin(minVlR - minVNewR, !rState);
  }

  if (!unew->bLeft->isLeaf()) {
    rState = unew->bLeft->getReversed();

    unew->bLeft->setNetMin(minVr  - minUNew,   rState);
    unew->bLeft->setNetMin(minVrR - minUNewR, !rState);
  }

  if (!unew->bRight->isLeaf()) {
    rState = unew->bRight->getReversed();

    unew->bRight->setNetMin(minUr  - minUNew,   rState);
    unew->bRight->setNetMin(minUrR - minUNewR, !rState);
  }


  //update netCost fields of unew and vnew 
  //netCost fields of respective children stay the same, since
  //grossMin stays unchanged 
  //set the data and the mapping as well

  vnew->setNetCost(costV  - minVNew,  false);
  vnew->setNetCost(costVR - minVNewR, true);
  vnew->setMapping(vMapping);
  vnew->data = vData;

  unew->setNetCost(costU  - minUNew,  false);
  unew->setNetCost(costUR - minUNewR, true);
  unew->setMapping(uMapping);
  unew->data = uData;

  //fix height fields
  unew->height = std::max(unew->bLeft->height, unew->bRight->height) + 1;
  vnew->height = std::max(vnew->bLeft->height, vnew->bRight->height) + 1;
}

void DynNode::rotateLeft(CapType grossminU, CapType grossminUR) {
  
  DynNode *u, *v;
  CapType *pNetMin, *pNetMinR;
  bool rState;


  if (isLeaf()) return;
  normalizeReverseState();

  u = this;
  v = bRight; //u->bRight

  if (v->isLeaf()) return;
  v->normalizeReverseState();
  
  //save original node data of u and v
  DynNode uold = *(this);
  DynNode vold = *v;

  bool uMapping = uold.getMapping();
  bool vMapping = vold.getMapping();

  void *uData = uold.data;
  void *vData = vold.data;

  CapType minU  = grossminU; //TODO: use grossminU/R directly?
  CapType minUR = grossminUR;

  CapType minVOld  = v->netMin  + minU;
  CapType minVOldR = v->netMinR + minUR;

  CapType costU  = u->netCost  + minU;
  CapType costUR = u->netCostR + minUR;

  CapType costV  = v->netCost +  minVOld;
  CapType costVR = v->netCostR + minVOldR;
  
  CapType minVl  = CAP_INF;
  CapType minVlR = CAP_INF;

  CapType minVr  = CAP_INF;
  CapType minVrR = CAP_INF;

  CapType minUl  = CAP_INF;
  CapType minUlR = CAP_INF;

  if (!v->bLeft->isLeaf()) {
    v->bLeft->getNetMinPtr(&pNetMin, &pNetMinR);

    minVl  = *pNetMin  + minVOld;
    minVlR = *pNetMinR + minVOldR;
  }

  if (!v->bRight->isLeaf()) {
    v->bRight->getNetMinPtr(&pNetMin, &pNetMinR);

    minVr  = *pNetMin  + minVOld;
    minVrR = *pNetMinR + minVOldR;
  }

  if (!u->bLeft->isLeaf()) {
    u->bLeft->getNetMinPtr(&pNetMin, &pNetMinR);

    minUl  = *pNetMin  + minU;
    minUlR = *pNetMinR + minUR;
  }

  

  DynNode *vnew = u; //let the old DynNode for u be the v after rotation
  DynNode *unew = v; //let the old DynNode for v be the u after rotation

  //restructure tree with u being new root
  vnew->setAsRChild(vold.bRight, false);
  unew->setAsRChild(vold.bLeft, false);
  unew->setAsLChild(uold.bLeft, false);
  vnew->setAsLChild(unew, false);

  //update netMin fields of unew, vnew and their respective children

  //vnew is the new root of a tree that contains the same nodes as before
  //=> grossMin(vnew) = grossMin(uold)
  CapType minVNew  = minU;
  CapType minVNewR = minUR;

  CapType minUNew  = mmin3(costU, minUl, minVl);
  CapType minUNewR = mmin3(costUR, minUlR, minVlR);

  unew->setNetMin(minUNew  - minVNew,  false);
  unew->setNetMin(minUNewR - minVNewR, true);


  if (!vnew->bRight->isLeaf()) {
    rState = vnew->bRight->getReversed();

    vnew->bRight->setNetMin(minVr  - minVNew,   rState);
    vnew->bRight->setNetMin(minVrR - minVNewR, !rState);
  }

  if (!unew->bRight->isLeaf()) {
    rState = unew->bRight->getReversed();

    unew->bRight->setNetMin(minVl  - minUNew,   rState);
    unew->bRight->setNetMin(minVlR - minUNewR, !rState);
  }

  if (!unew->bLeft->isLeaf()) {
    rState = unew->bLeft->getReversed();

    unew->bLeft->setNetMin(minUl  - minUNew,   rState);
    unew->bLeft->setNetMin(minUlR - minUNewR, !rState);
  }

  //update netCost fields of unew and vnew 
  //netCost fields of respective children stay the same, since
  //grossMin stays unchanged 
  //set the data and the mapping as well

  vnew->setNetCost(costV  - minVNew,  false);
  vnew->setNetCost(costVR - minVNewR, true);
  vnew->setMapping(vMapping);
  vnew->data = vData;

  unew->setNetCost(costU  - minUNew,  false);
  unew->setNetCost(costUR - minUNewR, true);
  unew->setMapping(uMapping);
  unew->data = uData;

  //fix height fields
  unew->height = max(unew->bLeft->height, unew->bRight->height) + 1;
  vnew->height = max(vnew->bLeft->height, vnew->bRight->height) + 1;

}

void DynNode::doubleRotateRight(CapType grossminU, CapType grossminUR) {
  DynNode *u, *v, *w;
  bool rState;
  
  if (isLeaf()) return;
  normalizeReverseState();

  u = this;
  v = bLeft;

  if (v->isLeaf()) return;
  v->normalizeReverseState();

  w = v->bRight;

  if (w->isLeaf()) return;
  w->normalizeReverseState();

  DynNode uold = *(this);
  DynNode vold = *v;
  DynNode wold = *w;

  bool uMapping = uold.getMapping();
  bool vMapping = vold.getMapping();
  bool wMapping = wold.getMapping();

  void *uData = uold.data;
  void *vData = vold.data;
  void *wData = wold.data;

  CapType minU  = grossminU;
  CapType minUR = grossminUR;

  CapType minVOld  = v->netMin  + minU;
  CapType minVOldR = v->netMinR + minUR;

  CapType minWOld  = w->netMin + minVOld;
  CapType minWOldR = w->netMinR + minVOldR;

  CapType costU  = u->netCost  + minU;
  CapType costUR = u->netCostR + minUR;

  CapType costV  = v->netCost  + minVOld;
  CapType costVR = v->netCostR + minVOldR;

  CapType costW  = w->netCost  + minWOld;
  CapType costWR = w->netCostR + minWOldR;

  CapType minVl  = CAP_INF;
  CapType minVlR = CAP_INF;

  CapType minUr  = CAP_INF;
  CapType minUrR = CAP_INF;

  CapType minWl  = CAP_INF;
  CapType minWlR = CAP_INF;

  CapType minWr  = CAP_INF;
  CapType minWrR = CAP_INF;

  if (!v->bLeft->isLeaf()) {
    rState = v->bLeft->getReversed();

    minVl  = v->bLeft->getNetMin(rState)  + minVOld;
    minVlR = v->bLeft->getNetMin(!rState) + minVOldR;
  }

  if (!u->bRight->isLeaf()) {
    rState = u->bRight->getReversed();

    minUr  = u->bRight->getNetMin(rState)  + minU;
    minUrR = u->bRight->getNetMin(!rState) + minUR;
  }

  if (!w->bLeft->isLeaf()) {
    rState = w->bLeft->getReversed();

    minWl  = w->bLeft->getNetMin(rState)  + minWOld;
    minWlR = w->bLeft->getNetMin(!rState) + minWOldR;
  }

  if (!w->bRight->isLeaf()) {
    rState = w->bRight->getReversed();

    minWr  = w->bRight->getNetMin(rState)  + minWOld;
    minWrR = w->bRight->getNetMin(!rState) + minWOldR;
  }

  DynNode *wnew = u; //let the old DynNode for u be the w after rotation
  DynNode *unew = w; //let the old DynNode for w be the u after rotation
  DynNode *vnew = v; //let the old DynNode for v be still v after rotation

  //restructure tree with u being new root
  unew->setAsLChild(wold.bRight, false);
  unew->setAsRChild(uold.bRight, false);

  vnew->setAsRChild(wold.bLeft, false);
  wnew->setAsRChild(unew, false);

  
  //update netMin fields of unew, vnew and their respective children

  //wnew is the new root of a tree that contains the same nodes as before
  //=> grossMin(wnew) = grossMin(uold)
  CapType minWNew  = minU;
  CapType minWNewR = minUR;

  CapType minVNew  = mmin3(costV, minWl, minVl);
  CapType minVNewR = mmin3(costVR, minWlR, minVlR);
  
  CapType minUNew  = mmin3(costU, minUr, minWr);
  CapType minUNewR = mmin3(costUR, minUrR, minWrR);

  vnew->setNetMin(minVNew  - minWNew,  false);
  vnew->setNetMin(minVNewR - minWNewR, true);

  unew->setNetMin(minUNew  - minWNew,  false);
  unew->setNetMin(minUNewR - minWNewR, true);

  if (!vnew->bLeft->isLeaf()) {
    rState = vnew->bLeft->getReversed();

    vnew->bLeft->setNetMin(minVl  - minVNew,   rState);
    vnew->bLeft->setNetMin(minVlR - minVNewR, !rState);
  }
  
  if (!vnew->bRight->isLeaf()) {
    rState = vnew->bRight->getReversed();

    vnew->bRight->setNetMin(minWl  - minVNew,   rState);
    vnew->bRight->setNetMin(minWlR - minVNewR, !rState);
  }
  

  if (!unew->bLeft->isLeaf()) {
    rState = unew->bLeft->getReversed();

    unew->bLeft->setNetMin(minWr  - minUNew,   rState);
    unew->bLeft->setNetMin(minWrR - minUNewR, !rState);
  }

  if (!unew->bRight->isLeaf()) {
    rState = unew->bRight->getReversed();

    unew->bRight->setNetMin(minUr  - minUNew,   rState);
    unew->bRight->setNetMin(minUrR - minUNewR, !rState);
  }
  
  //update netCost fields of wnew, vnew and unew 
  //netCost fields of respective children stay the same, since
  //grossMin stays unchanged 
  //set the data and the mapping as well

  wnew->setNetCost(costW  - minWNew,  false);
  wnew->setNetCost(costWR - minWNewR, true);
  wnew->setMapping(wMapping);
  wnew->data = wData;

  vnew->setNetCost(costV  - minVNew,  false);
  vnew->setNetCost(costVR - minVNewR, true);
  vnew->setMapping(vMapping);
  vnew->data = vData;

  unew->setNetCost(costU  - minUNew,  false);  
  unew->setNetCost(costUR - minUNewR, true);
  unew->setMapping(uMapping);
  unew->data = uData;

  //fix height fields while minding the order!
  vnew->height = max(vnew->bLeft->height, vnew->bRight->height) + 1;
  unew->height = max(unew->bLeft->height, unew->bRight->height) + 1;
  wnew->height = max(wnew->bLeft->height, wnew->bRight->height) + 1;

}

void DynNode::doubleRotateLeft(CapType grossminU, CapType grossminUR) {

  DynNode *u, *v, *w;
  bool rState;
  

  if (isLeaf()) return;
  normalizeReverseState();

  u = this;
  v = bRight; //u->bRight

  if (v->isLeaf()) return;
  v->normalizeReverseState();

  w = v->bLeft;

  if (w->isLeaf()) return;
  w->normalizeReverseState();

  DynNode uold = *(this);
  DynNode vold = *v;
  DynNode wold = *w;

  bool uMapping = uold.getMapping();
  bool vMapping = vold.getMapping();
  bool wMapping = wold.getMapping();

  void *uData = uold.data;
  void *vData = vold.data;
  void *wData = wold.data;

  CapType minU  = grossminU;
  CapType minUR = grossminUR;

  CapType minVOld  = v->netMin  + minU;
  CapType minVOldR = v->netMinR + minUR;

  CapType minWOld  = w->netMin  + minVOld;
  CapType minWOldR = w->netMinR + minVOldR;

  CapType costU  = u->netCost  + minU;
  CapType costUR = u->netCostR + minUR;

  CapType costV  = v->netCost  + minVOld;
  CapType costVR = v->netCostR + minVOldR;

  CapType costW  = w->netCost  + minWOld;
  CapType costWR = w->netCostR + minWOldR;

  CapType minVr  = CAP_INF;
  CapType minVrR = CAP_INF;

  CapType minUl  = CAP_INF;
  CapType minUlR = CAP_INF;

  CapType minWr  = CAP_INF;
  CapType minWrR = CAP_INF;

  CapType minWl  = CAP_INF;
  CapType minWlR = CAP_INF;


  if (!v->bRight->isLeaf()) {
    rState = v->bRight->getReversed();

    minVr  = v->bRight->getNetMin(rState)  + minVOld;
    minVrR = v->bRight->getNetMin(!rState) + minVOldR;
  }

  if (!u->bLeft->isLeaf()) {
    rState = u->bLeft->getReversed();
    
    minUl  = u->bLeft->getNetMin(rState)   + minU;
    minUlR = u->bLeft->getNetMin(!rState)  + minUR;

  }

  if (!w->bRight->isLeaf()) {
    rState = w->bRight->getReversed();

    minWr  = w->bRight->getNetMin(rState)  + minWOld;
    minWrR = w->bRight->getNetMin(!rState) + minWOldR;
  }

  if (!w->bLeft->isLeaf()) {
    rState = w->bLeft->getReversed();

    minWl  = w->bLeft->getNetMin(rState)  + minWOld;
    minWlR = w->bLeft->getNetMin(!rState) + minWOldR;
  }

  
  DynNode *unew = w; //let the old DynNode for u be the w after rotation  
  DynNode *wnew = u; //let the old DynNode for w be the u after rotation  
  DynNode *vnew = v; //let the old DynNode for v be still v after rotation

  //restructure tree with u being new root
  unew->setAsRChild(wold.bLeft, false);
  unew->setAsLChild(uold.bLeft, false);

  vnew->setAsLChild(wold.bRight, false);
  wnew->setAsLChild(unew, false);

  
  //update netMin fields of unew, vnew and their respective children

  //wnew is the new root of a tree that contains the same nodes as before
  //=> grossMin(wnew) = grossMin(uold)
  CapType minWNew  = minU;
  CapType minWNewR = minUR;

  CapType minVNew  = mmin3(costV, minWr,  minVr);
  CapType minVNewR = mmin3(costVR, minWrR, minVrR);

  CapType minUNew  = mmin3(costU, minUl,  minWl);
  CapType minUNewR = mmin3(costUR, minUlR, minWlR);


  vnew->setNetMin(minVNew  - minWNew,  false);
  vnew->setNetMin(minVNewR - minWNewR, true);

  unew->setNetMin(minUNew  - minWNew,  false);
  unew->setNetMin(minUNewR - minWNewR, true);

  if (!vnew->bRight->isLeaf()) {
    rState = vnew->bRight->getReversed();

    vnew->bRight->setNetMin(minVr  - minVNew,   rState);
    vnew->bRight->setNetMin(minVrR - minVNewR, !rState);
  }

  if (!vnew->bLeft->isLeaf()) {
    rState = vnew->bLeft->getReversed();

    vnew->bLeft->setNetMin(minWr  - minVNew,   rState);
    vnew->bLeft->setNetMin(minWrR - minVNewR, !rState);
  }
  
  if (!unew->bRight->isLeaf()) {
    rState = unew->bRight->getReversed();

    unew->bRight->setNetMin(minWl  - minUNew,   rState);
    unew->bRight->setNetMin(minWlR - minUNewR, !rState);
  }

  if (!unew->bLeft->isLeaf()) {
    rState = unew->bLeft->getReversed();

    unew->bLeft->setNetMin(minUl  - minUNew,   rState);
    unew->bLeft->setNetMin(minUlR - minUNewR, !rState);
  }
  

  //update netCost fields of wnew, vnew and unew 
  //netCost fields of respective children stay the same, since
  //grossMin stays unchanged 
  //set the data and the mapping as well

  wnew->setNetCost(costW  - minWNew,  false);
  wnew->setNetCost(costWR - minWNewR, true);
  wnew->setMapping(wMapping);
  wnew->data = wData;
  
  vnew->setNetCost(costV  - minVNew,  false);
  vnew->setNetCost(costVR - minVNewR, true);
  vnew->setMapping(vMapping);
  vnew->data = vData;

  unew->setNetCost(costU  - minUNew,  false);
  unew->setNetCost(costUR - minUNewR, true);
  unew->setMapping(uMapping);
  unew->data = uData;

  
  //fix height fields while minding the order!
  vnew->height = max(vnew->bLeft->height, vnew->bRight->height) + 1;
  unew->height = max(unew->bLeft->height, unew->bRight->height) + 1;
  wnew->height = max(wnew->bLeft->height, wnew->bRight->height) + 1;
}




/***************************************************
 *** DynRoot ***************************************
 ***************************************************/
DynRoot::DynRoot() {
}

// DynRoot *DynRoot::DynRootFromLeafChain(DynLeaf **leaves, int numLeaves) {
// //std::cout  <<"  [DynRootFromLeafChain] leaves: <"<<leaves<<" - "<<leaves+(numLeaves-1)<<"> ("<<numLeaves<<" entries)\n";
//   int pot2h, lastRow;
//   DynNode **baseDN, **mergeDN, **tmpDN;
//   DynNode *pn;
//   DynLeaf *pll, *prl;
//   DynNode *pln, *prn;
//   bool swapped;
//   int leafIdx, nodeIdx, baseIdx, mergeIdx;

//   // trivial case
//   if (numLeaves==1) return static_cast<DynRoot*>(static_cast<DynNode*>(leaves[0]));

//   // pot2h := 2^h <= numLeaves < 2^(h+1)
//   for(pot2h=2; pot2h<numLeaves;pot2h<<=1);
//   if (pot2h>numLeaves) pot2h>>=1;
//   lastRow = numLeaves - pot2h;

//   // create base and merge array
//   baseDN  = new DynNode*[pot2h<<1];
//   mergeDN = baseDN + pot2h;
//   swapped = false;
//   // fill base array with pot2h nodes
//   leafIdx = 0;
//   for (nodeIdx=0;nodeIdx<lastRow;nodeIdx++) {
//     // generate tree relationship
//     pn = pnAllocator->alloc();
//     baseDN[nodeIdx] = pn;
//     pll  = leaves[leafIdx++];
//     prl  = leaves[leafIdx++];
//     pn->setAsLChild(pll,0);
//     pn->setAsRChild(prl,0);
//     pn->height = 1;
//     // generate path data
//     pn->setReversed(false);
//     pn->netMin   = pll->wCost;
//     pn->netMinR  = pll->wCostR;
//     pn->netCost  = 0;
//     pn->netCostR = 0;
//     // generate graph data
//     pn->setMapping(pll->getWeakMapping());
//     pn->data = pll->getWeakData();
//   }
//   // convert remaining leaves of lowest level into nodes
//   while (nodeIdx < pot2h) baseDN[nodeIdx++]=static_cast<DynNode*>(leaves[leafIdx++]);

//   //merging base nodes
//   while (pot2h>1) {
//     baseIdx = 0;
//     pot2h >>= 1;
//     for (mergeIdx=0;mergeIdx<pot2h;mergeIdx++) {
//       // generate tree relationship
//       mergeDN[mergeIdx] = pn = pnAllocator->alloc();
//       pln  = baseDN[baseIdx++];
//       prn  = baseDN[baseIdx++];
//       pn->setAsLChild(pln,0);
//       pn->setAsRChild(prn,0);
//       pn->height = pln->isLeaf()?1:(pln->height+1); 
//       // generate path data
//       pll = static_cast<DynLeaf*>(pln->isLeaf()?pln:pln->bTail);
//       pn->setReversed(false);
//       pn->netCost  = pn->netMin   = pll->wCost;  // netCost  will be changed
//       pn->netCostR = pn->netMinR  = pll->wCostR; // netCostR will be changed
//       if (!pln->isLeaf()) {
//         pn->netMin  = mmin(pn->netMin,  pln->netMin );
//         pn->netMinR = mmin(pn->netMinR, pln->netMinR);
//       }
//       if (!prn->isLeaf()) {
//         pn->netMin  = mmin(pn->netMin,  prn->netMin );
//         pn->netMinR = mmin(pn->netMinR, prn->netMinR);
//       }
//       pn->netCost  -= pn->netMin;
//       pn->netCostR -= pn->netMinR;
//       if (!pln->isLeaf()) {
//         pn->netMin  -= pn->netMin;
//         pn->netMinR -= pn->netMinR;
//       }
//       if (!prn->isLeaf()) {
//         pn->netMin  -= pn->netMin;
//         pn->netMinR -= pn->netMinR;
//       }
//       // generate graph data
//       pn->setMapping(pln->isLeaf()?pll->getWeakMapping():pll->getMapping());
//       pn->data = pll->getWeakData();
//     }  
//     tmpDN   = baseDN; 
//     baseDN  = mergeDN; 
//     mergeDN = tmpDN;
//     swapped = !swapped;
//   }
//   delete [] (swapped?mergeDN:baseDN);

//   return static_cast<DynRoot*>(pn);
// }


DynRoot *DynRoot::DynRootFromLeafChain(DynLeaf **leaves, int numLeaves) {

  //depending on the number of leaves in "leaves" the resulting tree may
  //have a lowest row which is "incomplete" (#nodes != 2^height) -
  //this last row is handled separately in the following

  //this is where inner nodes are stored during successive tree construction
  DynNode **nodes;
  DynNode *pn, *pnl, *pnr;
  DynLeaf *pl, *pll, *plr;

  int maxNodes = numLeaves;
  int plIdx   = maxNodes - 1;
  int pnIdx   = maxNodes - 1; //points to the currently treated node in "nodes"
  int saveIdx = maxNodes - 1; //used to save newly created nodes in "nodes"
  int nLowerPairs; //number of node pairs in the lowest row
  int nLastFullRow = 1; //number of nodes in last full row
  int h = (int)(floor(log((float)numLeaves)/log((float)2))); //number of full rows

  //helper
  CapType grmin_l, grmin_r, rgrmin_l, rgrmin_r; 
  CapType cost, costR;
  bool mapping;
  void *data;
  int i, j;

  //detect trivial cases
  if (numLeaves == 1)
    return static_cast<DynRoot*>(static_cast<DynNode*>(leaves[0]));

  //compute number of nodes in the last full row
  for (i=0; i<h; i++)
    nLastFullRow *= 2;

  if (h==0)
    nLastFullRow = 0;

  nodes = new DynNode*[maxNodes]; 

  nLowerPairs = numLeaves - nLastFullRow;

  //firstly construct the incomplete lowest row
  for (i=0; i<nLowerPairs; i++) {
    
    pll = leaves[plIdx--];
    plr = leaves[plIdx--];

    pn = blockAllocator.alloc();

    pn->setAsLChild(pll,0);
    pn->setAsRChild(plr,0);

    pn->netMin  = pll->wCost;
    pn->netMinR = pll->wCostR;

    pn->netCost  = 0;
    pn->netCostR = 0;

    pn->setMapping(pll->getWeakMapping());
    pn->data = pll->getWeakData();
    pn->height = 1;
    pn->setReversed(false);

    nodes[saveIdx--] = pn;

  }

  //push back remaining nodes
  //  if (nLowerPairs > 0)
  while(plIdx >= 0)
    nodes[saveIdx--] = leaves[plIdx--];

  //build up the remaining tree rows by iteratively merging pairs of nodes
  for (i=0; i<h; i++) {

    pnIdx   = maxNodes - 1;
    saveIdx = maxNodes - 1;

    nLastFullRow >>= 1; //nLastFullRow /= 2;

    for (j=0; j<nLastFullRow; j++) {

      pnl = nodes[pnIdx--];
      pnr = nodes[pnIdx--];

      pn = blockAllocator.alloc();

      pn->setAsLChild(pnl,0);
      pn->setAsRChild(pnr,0);

      if (pnl->isLeaf()) {
	pl = static_cast<DynLeaf*>(pnl);
	cost     = pl->getWeakCost();
	costR    = pl->getWeakRevCost();
	mapping  = pl->getWeakMapping();
	data     = pl->getWeakData();

	grmin_l  = CAP_INF;
	rgrmin_l = CAP_INF;
      } else {
	pl = static_cast<DynLeaf*>(pnl->bTail);
	cost     = pl->getWeakCost();
	costR    = pl->getWeakRevCost();
	mapping  = pl->getMapping();
	data     = pl->getWeakData();

	grmin_l  = pnl->netMin;
	rgrmin_l = pnl->netMinR;
      }

      if (pnr->isLeaf()) {
	grmin_r  = CAP_INF;
	rgrmin_r = CAP_INF;
      } else {
	grmin_r  = pnr->netMin;
	rgrmin_r = pnr->netMin;
      }

      pn->netMin  = mmin3(cost, grmin_l,  grmin_r);
      pn->netMinR = mmin3(costR, rgrmin_l, rgrmin_r);
    
      pn->netCost  = cost  - pn->netMin;
      pn->netCostR = costR - pn->netMinR;

      pn->setMapping(mapping);
      pn->setReversed(false);
      pn->data = data;
    
      if (!pnl->isLeaf()) {
	pnl->netMin  -= pn->netMin;
	pnl->netMinR -= pn->netMinR;
      }

      if (!pnr->isLeaf()) {
	pnr->netMin  -= pn->netMin;
	pnr->netMinR -= pn->netMinR;
      }

      pn->height = max(pnr->height, pnl->height) + 1;

      nodes[saveIdx--] = pn;
    
    } 
    
  }

  pn = nodes[maxNodes-1];
  delete [] nodes;
  
  return static_cast<DynRoot*>(pn);

}



DynLeaf *DynRoot::getMinCostLeaf() {
  bool rState;
  DynNode *pn, *rChild = 0, *lChild = 0;
  DynLeaf *minCostLeaf;
  CapType rChildNetMin;

  rState = 0;
  pn = this;
  while (!pn->isLeaf()) {
    rState ^= pn->getReversed();
    if (rState) {
      rChild = pn->bLeft;
      lChild = pn->bRight;
    } else {
      rChild = pn->bRight;
      lChild = pn->bLeft;
    }
    rChildNetMin = rChild->getNetMin(rState ^ rChild->getReversed());
    //node already found?
    if ( (pn->getNetCost(rState) == 0) &&
	 (rChild->isLeaf() || rChildNetMin > 0) )
      break;
    //no, descend further if possible
    if (!rChild->isLeaf() && rChildNetMin == 0)
      pn = rChild;
    else
      pn = lChild;
  }
  if (pn->isLeaf()) {
    assert(0);
    return static_cast<DynLeaf*>(0); //either path consists of a single node or inconsistency
  }
  //pn = minimal edge -- now return the node left of the edge
  if (lChild->isLeaf())
    minCostLeaf = static_cast<DynLeaf*>(lChild);
  else {
    rState ^= lChild->getReversed();
    if (rState)
      minCostLeaf = static_cast<DynLeaf*>(lChild->bHead);
    else
      minCostLeaf = static_cast<DynLeaf*>(lChild->bTail);
  }
  return minCostLeaf;
}


DynRoot *DynRoot::concatenate(DynRoot *rightPath, 
			      CapType cost, CapType costR, 
			      bool revMapping, 
			      void *data) 
{

  DynRoot *pdp;
  DynNode *u, *v;
  CapType minU, minUR;
  int revFac;

  if (!rightPath)
    return 0;
  
  //create a new root with left and right part as children
  pdp = construct(rightPath, cost, costR, revMapping, data);

  u = pdp;
  minU  = u->netMin;
  minUR = u->netMinR;

  
  //The constructed tree is in general unbalanced, the left subtree
  //being significantly higher or lower than its sibling. Therefore
  //rebalancing is done in the following way (which has logarithmic 
  //complexity):

  //Assuming the left tree is higher: beginning with the root, in each
  //step either a single or double right rotation is done on the
  //current node. This reduces the overall height by 1 and the
  //procedure is recursively applied to the right subtree. In case of
  //a double rotation it's important that afterwards the left subtree
  //is not higher than its sibling. Otherwise a right rotation on the
  //right subtree in the next step would again result in an unbalanced
  //tree, since the height of the right subtree is again reduced by 1.
  while (u->bLeft->height - u->bRight->height > 1) {

    v = u->bLeft;
    revFac = (v->getReversed() ? -1 : 1);

    if (revFac * (v->bLeft->height - v->bRight->height) >= 0) {
      
      u->rotateRight(minU, minUR);

    } else {

      u->doubleRotateRight(minU, minUR);

      //extra right rotation required? (see above)
      if (u->bLeft->height > u->bRight->height) {
	u->rotateRight(minU, minUR);
	u = u->bRight;

	minU  += u->netMin;  //successor of u has normalized reverse state due to rotation
	minUR += u->netMinR;
      }

    }
      
    u = u->bRight;

    minU  += u->netMin; //already normalized
    minUR += u->netMinR;
      
  }




  //do the same procedure as above for the case that the right subtree
  //is significantly higher than the left one
  while (u->bRight->height - u->bLeft->height > 1) {

    v = u->bRight;
    revFac = (v->getReversed() ? -1 : 1);

    if (revFac * (v->bRight->height - v->bLeft->height) >= 0) {
      
      u->rotateLeft(minU, minUR);
    } else {

      u->doubleRotateLeft(minU, minUR);

      //extra left rotation required? (see above)
      if (u->bRight->height > u->bLeft->height) {
	u->rotateLeft(minU, minUR);
	u = u->bLeft;
	
	minU  += u->netMin;
	minUR += u->netMinR;
      }

    }
      
    u = u->bLeft;

    minU  += u->netMin;
    minUR += u->netMinR;
    
  }




  //fix the height fields on the way to the root
  while (u->bParent) {
    
    u = u->bParent;
    u->height = max(u->bLeft->height, u->bRight->height) + 1;
    
  }

  return pdp;

}


void DynRoot::destroy(ResultDestroy *dr) {

  CapType *pNetMin, *pNetMinR;

  if (isLeaf())
    return;

  //propagate reversed state downward
  normalizeReverseState();

  if (dr) {
    dr->cost      = netCost  + netMin;
    dr->costR     = netCostR + netMinR;
    dr->leftPath  = static_cast<DynRoot*>(bLeft);
    dr->rightPath = static_cast<DynRoot*>(bRight);
  }

  if (!bLeft->isLeaf()) {
    
    bLeft->getNetMinPtr(&pNetMin, &pNetMinR);

    *pNetMin  += netMin;
    *pNetMinR += netMinR;

  }

  if (!bRight->isLeaf()) {

    bRight->getNetMinPtr(&pNetMin, &pNetMinR);

    *pNetMin  += netMin;
    *pNetMinR += netMinR;

  }

  bLeft->bParent  = 0;
  bRight->bParent = 0;

  bLeft  = 0;
  bRight = 0;

  blockAllocator.dealloc(this); 

}
 
DynRoot *DynRoot::construct(DynRoot *rightPath, 
			    CapType cost, CapType costR, 
			    bool revMapping, 
			    void *data) {

  DynNode *pn = blockAllocator.alloc();

  CapType infCap = CAP_INF;

  CapType *pLNetMin = &infCap, *pLNetMinR = &infCap;
  CapType *pRNetMin = &infCap, *pRNetMinR = &infCap;
  
  if (!isLeaf())
    getNetMinPtr(&pLNetMin, &pLNetMinR);

  if (!rightPath->isLeaf())
    rightPath->getNetMinPtr(&pRNetMin, &pRNetMinR);

  pn->netMin  = mmin3(cost, *pLNetMin,  *pRNetMin);
  pn->netMinR = mmin3(costR, *pLNetMinR, *pRNetMinR);

  pn->netCost  = cost  - pn->netMin;
  pn->netCostR = costR - pn->netMinR;

  pn->setAsLChild(this, 0);
  pn->setAsRChild(rightPath, 0);

  if (!pn->bRight->isLeaf()) {
    *pRNetMin  -= pn->netMin;
    *pRNetMinR -= pn->netMinR;
  }

  if (!pn->bLeft->isLeaf()) {
    *pLNetMin  -= pn->netMin;
    *pLNetMinR -= pn->netMinR;
  }

  pn->height = max(rightPath->height, this->height) + 1;
  pn->setMapping(revMapping);
  pn->data = data;

  //  return DynRoot::DynNodeToDynRoot(pn);
  return static_cast<DynRoot*>(pn);

}


DynRoot *DynRoot::splice() {

  ResultSplit sres;
  DynLeaf *pl;
  DynRoot *pdp;
  
  //get the "weak" parent node of the last path node within the DynTree 
  pl = this->getTail()->getWeakParent();

  if (!pl)
    return this;

  //split up the parent nodes path
  pl->divide(&sres);

  //and reconnect the left subpath weakly to the parent node
  if (sres.leftPath) {
    sres.leftPath->getTail()->setWeakLink(pl,
					  sres.costBefore,
					  sres.costBeforeR,
					  sres.mappingBefore,
					  sres.dataBefore);
  }
  
  //now convert the connection to the parent node to a "strong" one
  pdp = this->concatenate(sres.rightPath,
			  this->getTail()->getWeakCost(),
			  this->getTail()->getWeakRevCost(),
			  this->getTail()->getWeakMapping(),
			  this->getTail()->getWeakData());
  
  return pdp;

}


#if defined DYNPATH_DEBUG

void DynRoot::print(bool weights) {

  DynLeaf *pl;
  CapType cost, costR;

  pl = getHead();

  do {

    if (pl != getHead()) {
      cout << " - ";

      if (weights)
	cout << endl;
    }

    cout << pl->id;
    
    if (weights && getTail() != pl) {
      pl->getEdgeCostDbl(cost, costR);
      cout << " - " << cost << " / " << costR;
    }

  }  while ((pl = pl->getNextDyn()));
    
  cout << "\n\n";

}

#endif //DYNPATH_DEBUG


#ifdef DYNPATH_DEBUG

//normalizes the reverse state of all tree nodes in the process
bool DynRoot::checkCostIntegrity() { 

  if (this->isLeaf())
    return true;
  
  //left search the tree and precompute minCost and grossCost
  DynNode *pn = this;
  
  while(true) {

    while (!pn->isLeaf()) {
      pn->normalizeReverseState();
      if (pn->bParent) 
	pn->grossMin = pn->netMin + pn->bParent->grossMin;
      else
	pn->grossMin = pn->netMin;
      pn->grossCost = pn->grossMin + pn->netCost;

      pn = pn->bLeft;
    }

    while (pn->bParent->bRight == pn) {
      pn = pn->bParent;
      if (pn == this)
	goto precomputed;
    }

    pn = pn->bParent->bRight;

  }

 precomputed:

  DynLeaf *pl = getHead();
  pn = pl;
  bool fromRight;
  CapType minCost = 1e6;

  while (true) {
    
    //pn = pl is a leaf node

    do { // while(fromRight)

      //ascend to parent
      if (!pn->bParent)
	goto finished; //we arrived at the root

      if (pn->bParent->bRight == pn)
	fromRight = true;
      else if (pn->bParent->bLeft == pn)
	fromRight = false;
      
      pn = pn->bParent; //ascend

      //check grossmin integrity
      if (pn->bRight->isLeaf() && pn->bLeft->isLeaf()) {
	minCost = pn->grossMin;
	if (fabs(pn->grossMin - pn->grossCost) > 1e-4)
	  return false;
      } else if (pn->bRight->isLeaf() && !pn->bLeft->isLeaf()) {
	minCost = pn->bLeft->grossMin;
      } else if (!pn->bRight->isLeaf() && pn->bLeft->isLeaf()) {
	minCost = pn->bRight->grossMin;
      } else {
	minCost = pn->bRight->grossMin < pn->bLeft->grossMin ? pn->bRight->grossMin : pn->bLeft->grossMin;
      }
      
      if (fabs(pn->grossMin - pn->grossCost) > 1e-4 &&
	  fabs(minCost - pn->grossMin) > 1e-4)
	  return false;
      
    } while (fromRight); // while(fromRight)
    
    //we came from the left - ascend further right
    pn = pn->bRight;
    
    if (!pn->isLeaf())
      pn = pn->bHead;
    
    pl = static_cast<DynLeaf*>(pn);
    
  } // while (true)

 finished:

  return true;
  
}

#endif //DYNPATH_DEBUG

#if defined DYNPATH_DEBUG

bool DynRoot::checkStructuralIntegrity() {

  DynLeaf *pl = getHead();
  DynNode *pn = pl, *pnRoot = this;
  bool fromRight, relationShipError;
  
 while (true) {
    
    //pn = pl is a leaf node
    cout << pl->id << " - ";

    //consistency checks
    if (pn->bLeft || pn->bRight)
      return false; //should be a leaf

    if (pn->height != 0)
      return false; //should be a leaf


    do { // while(fromRight)

      //ascend to parent
      if (!pn->bParent) {

	if (pn != pnRoot)
	  return false;
	
	goto finished; //we arrived at the root

      }

      relationShipError = true;

      if (pn->bParent->bRight == pn) {
	fromRight = true;
	relationShipError = false;
      } else if (pn->bParent->bLeft == pn) {
	fromRight = false;
	relationShipError = false;
      }
      
      if (relationShipError)
	return false;
    
      pn = pn->bParent; //ascend
    
      //we came from the right
      if (fromRight) {

	int heightB, heightS;
      
	if (pn->bLeft->height > pn->bRight->height) {
	  heightB = pn->bLeft->height;
	  heightS = pn->bRight->height;
	} else {
	  heightB = pn->bRight->height;
	  heightS = pn->bLeft->height;
	}

	int ballance = heightB - heightS;

	//check balance
	if (ballance > 1)
	  return false;

	//check height
	if (pn->height != heightB + 1)
	  return false;
      
      }

    } while (fromRight); // while(fromRight)

    //we came from the left - ascend further right
    pn = pn->bRight;
    
    if (!pn->isLeaf())
      pn = pn->bHead;
    
    pl = static_cast<DynLeaf*>(pn);
    
  } // while (true)

 finished:

  cout << "END" << endl;

  return true;
  
}

#endif



/***************************************************
 *** DynLeaf ***************************************
 ***************************************************/
DynLeaf::DynLeaf() : wParent(0), wCost(0), wCostR(0) {

#if defined DYNPATH_DEBUG
  id = 0; 
#endif

}


void DynLeaf::setWeakLink(DynLeaf *parent, 
			  CapType cap, CapType rcap, 
			  bool mapping,
			  void *linkData) {

  wParent = parent;
  wCost   = cap;
  wCostR  = rcap;
  data    = linkData;

  DynNode::setMapping(mapping);

}


CapType DynLeaf::prepareRootPath() {

  int idxRPath = 0;

  DynNode *pn;
  CapType grossMin;

  //compute and save the path to the root node
  pn = this;
//std::cout  <<"  [<" << this << ">::prepareRootPath():] \n";   

  while (pn->bParent != 0) {
    stackRPath[idxRPath++] = pn;
    pn = pn->bParent;
  }

  //for each node on the path compute the reversed state
  //plus grossMin up to the current node
  bool rState = pn->getReversed();
  pn->setTemp(rState);
  grossMin = pn->getNetMin(rState);

  while (idxRPath != 0) {

    pn = stackRPath[--idxRPath];
    rState ^= pn->getReversed();

    if (!pn->isLeaf()) {
      pn->setTemp(rState);	  
      grossMin += pn->getNetMin(rState);
    }
      
  }

  return grossMin;

}


void DynLeaf::prepareRootPathDbl(CapType &grossMin, CapType &grossMinR) {

  int idxRPath = 0;

  DynNode *pn;

  //compute and save the path to the root node
  pn = this;

  while (pn->bParent != 0) {

    stackRPath[idxRPath++] = pn;
    pn = pn->bParent;

  }

  //for each node on the path compute the reversed state
  //plus grossMin up to the current node
  bool rState = pn->getReversed();
  pn->setTemp(rState);
  grossMin  = pn->getNetMin(rState);
  grossMinR = pn->getNetMin(!rState);

  while (idxRPath != 0) {

    pn = stackRPath[--idxRPath];
    rState ^= pn->getReversed();

    if (!pn->isLeaf()) {
      pn->setTemp(rState);	  
      grossMin  += pn->getNetMin(rState);
      grossMinR += pn->getNetMin(!rState);
    }
      
  }

}


void DynLeaf::disassemble() {
  DynNode *pn, *pnP, *pnC;       //node variables for parent and child
  DynRoot *pdp;

  CapType grossMin, grossMinR;   //current grossmin value
  CapType cost, costR;           //cost of recently deleted node
  bool mapping;               //arc / anti-arc association of costs
  void    *data;

  ResultDestroy dr;                 //receives result of destroy()       

  //empty stacks
  idxRPath = 0;
  idxRightSide = 0;
  idxLeftSide = 0;
  idxCostR = 0;
  idxCostL = 0;
  idxMappingR = 0;
  idxMappingL = 0;
  idxDataL = 0;
  idxDataR = 0;

  
  //compute and save path to root node
  pn = this;

  while (pn->bParent != 0) { 

    stackRPath[idxRPath++] = pn;
    pn = pn->bParent;

  }

  //disassemble tree from root to destination node
  pnP = pn; //begin with root
  grossMin  = 0;
  grossMinR = 0;

  while(pnP != this) {

    pnP->normalizeReverseState(); //make sure the reversed state is zero

    grossMin  = pnP->netMin; //during disassembly pnP is always a root => grossMin = netMin
    grossMinR = pnP->netMinR;

    cost  = grossMin  + pnP->netCost;
    costR = grossMinR + pnP->netCostR;

    pnC = stackRPath[--idxRPath]; //get next child on path to destination node

    bool toRPath = (pnC == pnP->bLeft); //true, if the path continues left

    mapping = pnP->getMapping();
    data    = pnP->data;
    
    pdp = static_cast<DynRoot*>(pnP);
    pdp->destroy(&dr);
    
    if (toRPath) { //cut subtree belongs to the right half of the splitted path
      stackRightSide[idxRightSide++] = dr.rightPath;
      stackCostR[idxCostR++]         = cost;
      stackCostR[idxCostR++]         = costR;
      stackMappingR[idxMappingR++]   = mapping;
      stackDataR[idxDataR++]         = data;
    } else { //cut subtree belongs to the left half of the splitted path
      stackLeftSide[idxLeftSide++]   = dr.leftPath;
      stackCostL[idxCostL++]         = cost;
      stackCostL[idxCostL++]         = costR;
      stackMappingL[idxMappingL++]   = mapping;
      stackDataL[idxDataL++]         = data;
    }

    pnP = pnC;

  }

  //the calling node is part of the right subpath
  stackRightSide[idxRightSide++] = static_cast<DynRoot*>(static_cast<DynNode*>(this));
}


void DynLeaf::reassemble(DynRoot*& pdpl, DynRoot*& pdpr) {
  CapType cost, costR;           //cost of recently deleted node
  bool  mapping;               //arc / anti-arc association of costs
  void    *data;

  //reassemble left subpath from inside to outside
  //otherwise no log-runtime is guaranteed
  if (idxLeftSide != 0)
    pdpl = stackLeftSide[--idxLeftSide];

  while (idxLeftSide != 0) {
    costR   = stackCostL[--idxCostL];
    cost    = stackCostL[--idxCostL];
    mapping = stackMappingL[--idxMappingL];
    data    = stackDataL[--idxDataL];
    pdpl    = stackLeftSide[--idxLeftSide]->concatenate(pdpl, cost, costR, mapping, data);
  }


  //reassemble right subpath from inside to outside
  //otherwise no log-runtime is guaranteed
  if (idxRightSide != 0)
    pdpr = stackRightSide[--idxRightSide];

  while (idxRightSide != 0) {
    costR   = stackCostR[--idxCostR];
    cost    = stackCostR[--idxCostR];
    mapping = stackMappingR[--idxMappingR];
    data    = stackDataR[--idxDataR];
    pdpr    = pdpr->concatenate(stackRightSide[--idxRightSide], 
				cost, costR, 
				mapping, data);
  }
}


DynRoot *DynLeaf::getPath() {
  DynNode *pn;

  if (!bParent) //this DynPath is only a leaf (= a single node)
    return static_cast<DynRoot*>(static_cast<DynNode*>(this)); 

  pn = bParent;

  while (pn->bParent != 0) {    
    pn = pn->bParent;
  }

  //now we have arrived at the root node
  //  return DynPath::DynNodeToDynRoot(pn); //return the corresponding DynPath
  return static_cast<DynRoot*>(pn); //return the corresponding DynPath
}


void DynLeaf::split(ResultSplit *psr) {
  
  DynRoot *pdpl = 0, *pdpr = 0;

  //decompose the path tree in order to reassemble left and right subpath separately
  disassemble();

  if (psr)
    memset(psr, 0, sizeof(ResultSplit));

  //save data of the two edges where the split happens and delete from the stack
  if (idxCostL != 0)
    if (psr) {
      psr->costBeforeR   = stackCostL[--idxCostL];
      psr->costBefore    = stackCostL[--idxCostL];
      psr->mappingBefore = stackMappingL[--idxMappingL];
      psr->dataBefore    = stackDataL[--idxDataL];
    }

  if (idxCostR != 0) 
    if (psr) { 
      psr->costAfterR   = stackCostR[--idxCostR];
      psr->costAfter    = stackCostR[--idxCostR];
      psr->mappingAfter = stackMappingR[--idxMappingR];
      psr->dataAfter    = stackDataR[--idxDataR];
    }

  idxRightSide--; //the calling node should not be contained in any of the two subpaths

  //reassemble the left and right subpath from the stack
  reassemble(pdpl, pdpr);

  //return both subpaths
  if (psr) {
    psr->leftPath  = pdpl;
    psr->rightPath = pdpr;
  }
  
}

void DynLeaf::divide(ResultSplit *psr) {
  
  DynRoot *pdpl = 0, *pdpr = 0;

  //decompose the path tree in order to reassemble left and right subpath separately
  disassemble();

  if (psr)
    memset(psr, 0, sizeof(ResultSplit));

  //save data of the edge where the divide happens and delete from the stack
  if (idxCostL != 0)
    if (psr) {
      psr->costBeforeR   = stackCostL[--idxCostL];
      psr->costBefore    = stackCostL[--idxCostL];
      psr->mappingBefore = stackMappingL[--idxMappingL];
      psr->dataBefore    = stackDataL[--idxDataL];
    }

  //reassemble the left and right subpath from the stack
  reassemble(pdpl, pdpr);

  //return both subpaths
  if (psr) {
    psr->leftPath  = pdpl;
    psr->rightPath = pdpr;
  }
  
}


DynRoot *DynLeaf::expose() {

  ResultSplit sres;
  DynRoot *pdp;

  //make "this" the first node in the path
  divide(&sres); 
  
  if (sres.leftPath) {

    DynLeaf *plT = sres.leftPath->getTail();
    plT->wParent = this;
    plT->wCost   = sres.costBefore;
    plT->wCostR  = sres.costBeforeR;
    plT->setMapping(sres.mappingBefore);
    plT->data    = sres.dataBefore;

  }

  pdp = sres.rightPath;

  //connect nodes on the root path to one path
  while (pdp->getTail()->wParent) {

    pdp = pdp->splice();

  }

  return pdp;

}






