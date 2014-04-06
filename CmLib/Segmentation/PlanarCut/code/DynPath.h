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

#ifndef __DYNPATH_H__
#define __DYNPATH_H__

#include "BlockAllocator.h"
#include <memory.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "CutPlanarDefs.h"

#define STACKSIZE 100
//#define DYNPATH_DEBUG


#define REV_MASK 1
#define TMP_MASK 2
#define MAP_MASK 4

//forward declarations
class DynNode;
class DynRoot;
class DynLeaf;

//returned by DynRoot::destroy()
struct ResultDestroy {
  DynRoot *leftPath;
  DynRoot *rightPath;
  CapType  cost;
  CapType  costR;
};

//returned by DynRoot::split() and DynRoot::divide()
struct ResultSplit {
  DynRoot *leftPath;
  DynRoot *rightPath;
  CapType  costBefore;
  CapType  costBeforeR;
  CapType  costAfter;
  CapType  costAfterR;
  bool     mappingBefore;
  bool     mappingAfter;
  void    *dataBefore;
  void    *dataAfter;
};



class DynNode {

  unsigned char reversed;
  
public: 
  //the temp flag is used by prepareRootPath to remember the
  //computed reverse states of the nodes on the path to the root
  bool getTemp() { return (reversed & TMP_MASK) != 0; };
  void setTemp(bool b) { if (getTemp()!=b) reversed ^= TMP_MASK; };

  //the mapping flag indicates wether the forward cost describe the
  //cost of the arc- or the anti-arc of the corresponding edge in the graph
  bool getMapping() { return (reversed & MAP_MASK) != 0; }; 
  void setMapping(bool b) { if (getMapping()!=b) reversed ^= MAP_MASK; };

  //reversed state access
  bool getReversed() { return (reversed & REV_MASK) != 0; };
  void setReversed(bool b) { if (getReversed()!=b) reversed ^= REV_MASK; };

  //sets pn as the left/right child - this will only set the structural relationship!
  //no cost fields are reset or updated!
  //rState is the reversed state of the parent node
  void setAsLChild(DynNode *pn, bool rState);
  void setAsRChild(DynNode *pn, bool rState);

  //balancing methods for the binary path tree
  void rotateLeft(CapType grossminU, CapType rgrossminU);
  void rotateRight(CapType grossminU, CapType rgrossminU);
  void doubleRotateLeft(CapType grossminU, CapType rgrossminU);
  void doubleRotateRight(CapType grossminU, CapType rgrossminU);


  //DynRoot-fields
  DynNode *bParent;   //strong parent in the DynRoot
  DynNode *bHead;     //pointer to first node in the DynRoot
  DynNode *bTail;     //pointer to last node
  DynNode *bLeft;     //left child
  DynNode *bRight;    //right child

  void *data;  //user defined data

  CapType netCost, netCostR; //net costs
  CapType netMin, netMinR;   //net minimal costs of an edge in the path

  int height;  //height of tree with root "this"

#ifdef DYNPATH_DEBUG
  CapType grossMin, grossMinR;
  CapType grossCost, grossCostR;
#endif

  DynNode();

  CapType getNetMin(bool rState=false) { return (rState ? netMinR : netMin); };
  CapType getNetCost(bool rState=false) { return (rState ? netCostR : netCost); };

  void setNetMin(CapType netMin, bool rState=false);
  void setNetCost(CapType netCost, bool rState=false);

  //returns pointer to the net (minimal)costs in forward and backward
  //direction while respecting the reverse state (rState: reversed state of parent)
  void getNetMinPtr(CapType **pNetMin, CapType **pNetMinR, bool rState=false); 
  void getNetCostPtr(CapType **pNetCost, CapType **pNetCostR, bool rState=false);

  //normalizes reverse state to zero while leaving the tree structure unchanged
  void normalizeReverseState();

  bool isLeaf() { return !bLeft; };
};









//There are two instantiations of DynNode: one which represents a root
//node (DynRoot) and one which represents a leaf node (DynLeaf).
//DynLeafs constitute actual nodes in the DynTREE and can therefore be
//connected across different DynRoots (as they can be weakly connected)

//A DynRoot is nothing but a binary tree of DynNodes. 
//A single DynRoot is represented by the root node of the corresponding tree.
//In a way a DynRoot can therefore be regarded as a DynNode and is directly 
//derived from it.
class DynRoot : private DynNode {

  friend class DynLeaf;  //authorize DynRoot to convert from DynNode to DynLeaf

  //used in order to avoid frequent allocation and deallocation of nodes on the heap
  static BlockAllocator<DynNode> blockAllocator;

  //creates a new root with "this" as left and rightPath as right child
  //NOTE: does no rebalancing of resulting tree!
  DynRoot *construct(DynRoot *rightPath, CapType cost, CapType costR, 
		     bool revMapping=false, void *data=0);

  //used by DynLeaf::expose(): detects the next weak connection on the
  //path to the DynTree root and converts it to a strong one 
  DynRoot *splice(); 

 public:

  DynRoot();

  static DynRoot *DynRootFromLeafChain(DynLeaf **leaves, int numLeaves);
  static void resetBlockAllocator() { blockAllocator.reset(); };

  unsigned int getHeight () { return height; };
  //  void setData(void *data);

  DynLeaf *getHead();
  DynLeaf *getTail();
  DynLeaf *getMinCostLeaf();      //get node closest to tail having minimal edge costs along path
  void     addCost(CapType cost); //increases weights of all edges within path
  void     reverse();             //NOTE: may be applied to the root only!
  DynRoot *concatenate(DynRoot *rightPath, 
		       CapType cost, CapType costR, 
		       bool revMapping=false, 
		       void *data=0); 
  void     destroy(ResultDestroy *dr);


#if defined DYNPATH_DEBUG
  void print(bool weights = false); //prints the id fields of the leafs in order
  bool checkCostIntegrity(); //checks on the cost field integrity of the DynRoot
  bool checkStructuralIntegrity(); //checks on the structural integrity of the DynRoot
#endif
 
};








//A DynLeaf is a DynNode that is part of a DynRoot binary tree and is a leaf 
//(has no children). It provides operations on the path that is represented 
//by the chain of leaf nodes of the binary DynRoot tree. The operations 
//always act on the path the calling leaf node is part of and with
//respect to the calling leaf node.
class DynLeaf : private DynNode {

  friend class DynRoot; //authorize DynLeaf to convert from DynNode to DynRoot

  //DynTree-fields
  DynLeaf *wParent; //weak parent in the DynTree
  CapType  wCost;   //cost of the weak connection in forward direction
  CapType  wCostR;  //cost of the weak connection in backward direction

  //static stack declarations - 
  //use a global stacks for path computations so they do not have to be 
  //allocated for each call separately

  //stack pointer
  static int idxRightSide;
  static int idxLeftSide;
  static int idxCostR;
  static int idxCostL;
  static int idxMappingR;
  static int idxMappingL;
  static int idxDataL;
  static int idxDataR;
  static int idxRPath;
  
  //stacks
  static DynRoot* stackRightSide[STACKSIZE]; //subtrees of resulting right path
  static DynRoot* stackLeftSide[STACKSIZE];  //subtrees of resulting left path
  static CapType  stackCostR[STACKSIZE];     //costs of the temporarily deleted edges right of the split
  static CapType  stackCostL[STACKSIZE];     //costs of the temporarily deleted edges left of the split
  static bool     stackMappingR[STACKSIZE];  //mapping of the costs to arc / anti-arc right of the split
  static bool     stackMappingL[STACKSIZE];  //mapping og the costs to arc / anti-arc left of the split
  static void*    stackDataR[STACKSIZE];     //data fields for temporarily deleted nodes right of split
  static void*    stackDataL[STACKSIZE];     //data fields for temporarily deleted nodes left of split
  static DynNode* stackRPath[STACKSIZE];     //path to the root (used by DynNode::prepareRootPath())

 protected:

  //computes for each node on the path from "this" to the root 
  //the reversed state and returns grossmin(this).
  //(used by getPrev, getNext, getEdgeCost(Dbl))
  CapType prepareRootPath();
  void prepareRootPathDbl(CapType &grossMin, CapType &grossMinR);

  //decomposes the path tree along the path from the calling node to the root
  //and sorts the ensuing subtrees according to wether they belong to the left 
  //or right subpath
  void disassemble();
  //recomposes the subpath from the subtrees on the global stacks 
  void reassemble(DynRoot*& pdpl, DynRoot*& pdpr); 
  //(used by split() and divide() for convencience)

 public:

#if defined DYNPATH_DEBUG
  int id; 
#endif

  DynLeaf();

  //access to weak link fields
  DynLeaf *getWeakParent() { return wParent; };
  CapType  getWeakCost() { return wCost; };
  CapType  getWeakRevCost() { return wCostR; };
  bool     getWeakMapping() { return DynNode::getMapping(); };
  void    *getWeakData() { return data; };

  void setWeakLink(DynLeaf *parent, 
		   CapType cap, CapType rcap, 
		   bool mapping,
		   void *linkData);

  DynRoot *getPath();  //returns the path containing the calling node
  DynLeaf *getNext();  //returns the next node on the DynTree-path
  DynLeaf *getPrevDyn(); //return the previous node if it is on the same DynPath
  DynLeaf *getNextDyn(); //return the next node if it is on the same DynPath
  CapType  getEdgeCost();  //returns the cost of the edge after the calling node
  bool     getEdgeCostDbl(CapType &cost, CapType &costR);
  //splits the path (of which the calling node is part of) in three
  //subpaths: the first path contains all nodes to the left of the calling
  //node, the second path is the calling node itself and the third part
  //consists of the remaining nodes
  void split(ResultSplit *psr); 
  //divides the path at the calling node, with the calling node 
  //ending up in the right part
  void divide(ResultSplit *psr);
  
  //makes sure the path from the calling node to the root of the
  //DynTREE has no weak connections (i.e. it is a single a DynRoot) 
  DynRoot *expose();

};

/***************************************************
 *** DynNode INLINE ********************************
 ***************************************************/
inline void DynNode::setAsLChild(DynNode *pn, bool rState) {
  DynNode *newHead;

  this->bLeft = pn;
  pn->bParent = this;
  
  newHead = pn->isLeaf()?pn:((pn->getReversed()==rState)?pn->bHead:pn->bTail);

  if (rState)
    this->bTail = newHead;
  else
    this->bHead = newHead;    
}


inline void DynNode::setAsRChild(DynNode *pn, bool rState) {
  DynNode *newTail;

  this->bRight = pn;
  pn->bParent  = this;

  newTail = pn->isLeaf()?pn:((pn->getReversed()==rState)?pn->bTail:pn->bHead);
    
  if (rState)
    this->bHead = newTail;
  else
    this->bTail = newTail;
}


inline void DynNode::setNetMin(CapType netMin, bool rState) {
  if (rState)
    this->netMinR = netMin;
  else
    this->netMin = netMin;
}


inline void DynNode::setNetCost(CapType netCost, bool rState) {
  if (rState)
    this->netCostR = netCost;
  else
    this->netCost = netCost;
}

  
inline void DynNode::getNetMinPtr(CapType **pNetMin, CapType **pNetMinR, bool rState) {
  rState ^= getReversed();

  if (rState) {
    *pNetMin  = &netMinR;
    *pNetMinR = &netMin;
  } else {
    *pNetMin  = &netMin;
    *pNetMinR = &netMinR;
  }
}


inline void DynNode::getNetCostPtr(CapType **pNetCost, CapType **pNetCostR, bool rState) {
  rState ^= getReversed();

  if (rState) {
    *pNetCost  = &netCostR;
    *pNetCostR = &netCost;
  } else {
    *pNetCost  = &netCost;
    *pNetCostR = &netCostR;
  }
}



inline void DynNode::normalizeReverseState() {
  if (!getReversed()) return; //is normalized already

  setReversed(false);
  setMapping(!getMapping());

  DynNode *pn = bLeft;
  bLeft = bRight;
  bRight = pn;

  pn = bHead;
  bHead = bTail;
  bTail = pn;

  CapType c = netMin;
  netMin = netMinR;
  netMinR = c;

  c = netCost;
  netCost = netCostR;
  netCostR = c;

  //keep reversed state for children
  if (bRight && !bRight->isLeaf())
    bRight->setReversed(bRight->getReversed() ^ 1);

  if (bLeft && !bLeft->isLeaf())
    bLeft->setReversed(bLeft->getReversed() ^ 1);
}

/***************************************************
 *** DynRoot INLINE ********************************
 ***************************************************/
inline DynLeaf *DynRoot::getHead() {
  if (isLeaf())
    return static_cast<DynLeaf*>(static_cast<DynNode*>(this));

  if (getReversed())
    return static_cast<DynLeaf*>(bTail);
  
  return static_cast<DynLeaf*>(bHead);
}


inline DynLeaf *DynRoot::getTail() {
  if (isLeaf())
    return static_cast<DynLeaf*>(static_cast<DynNode*>(this));

  if (getReversed())
    return static_cast<DynLeaf*>(bHead);

  return static_cast<DynLeaf*>(bTail);
}


inline void DynRoot::addCost(CapType cost) {
    if (this->isLeaf())
      return;

    CapType netMin = getNetMin(getReversed());
    setNetMin(netMin + cost, getReversed());

    netMin = getNetMin(!getReversed());
    setNetMin(netMin - cost, !getReversed());
}


//NOTE: may be applied to the root only!
inline void DynRoot::reverse() {
  if (!this->isLeaf())
    setReversed(!getReversed());
}

/***************************************************
 *** DynLeaf INLINE ********************************
 ***************************************************/

inline DynLeaf *DynLeaf::getNext() {
//std::cout  <<"  [<" << this << ">::getNext():] Entering\n";   
  
  DynLeaf *pl;

  pl = getNextDyn();
//std::cout  <<"  [<" << this << ">::getNext():] getNextDyn: "<<pl<<"\n";   

  if (!pl) //this leaf is tail of the DynPath it belongs to
    return wParent;

  return pl;

}

inline DynLeaf *DynLeaf::getPrevDyn() {

  DynNode *pn, *pnSib, *rChild = 0;
  DynLeaf *prevLeaf = 0;
  bool parentRState = false, lChildRState = false;
  bool found = false;

  prepareRootPath();

  //on the path to the root, search for the first node being the right child of its parent
  pn = this;

  while (!found && pn->bParent) {

    //is pn the right child of its parent?
    parentRState = pn->bParent->getTemp();

    if (parentRState)  //check reversed state
      rChild = pn->bParent->bLeft;
    else
      rChild = pn->bParent->bRight;

    if (rChild == pn)
      found = true;

    pn = pn->bParent;

  }

  if (!found) 
    return static_cast<DynLeaf*>(0); //"this" is already head of the path

  //determine sibling of pn
  if (parentRState)
    pnSib = pn->bRight;
  else
    pnSib = pn->bLeft;

  lChildRState = pnSib->getReversed() ^ parentRState;
  
  //the previous node on the path is the tail of the subpath of which 
  //the sibling is the root node
  if (lChildRState) 
    prevLeaf = static_cast<DynLeaf*>(pnSib->bHead);
  else
    prevLeaf = static_cast<DynLeaf*>(pnSib->bTail);

  if (!prevLeaf)
    return static_cast<DynLeaf*>(pnSib); 

  return prevLeaf;

}

inline DynLeaf *DynLeaf::getNextDyn() {

  DynNode *pn, *pnSib, *lChild = 0;
  DynLeaf *nextLeaf = 0;
  bool parentRState = false, lChildRState = false;
  bool found = false;

  prepareRootPath();
  pn = this;

  //search for first node on the path to the root being left child of its parent
  while (!found && pn->bParent) {

    //is pn the left child of its parent?
    parentRState = pn->bParent->getTemp();

    if (parentRState) //check reversed state
      lChild = pn->bParent->bRight;
    else
      lChild = pn->bParent->bLeft;

    if (lChild == pn)
      found = true;

    pn = pn->bParent;

  }

  if (!found) 
    return (DynLeaf*)0; //"this" is already tail of the path

  //determine sibling of pn
  if (parentRState)
    pnSib = pn->bLeft;
  else
    pnSib = pn->bRight;

  lChildRState = pnSib->getReversed() ^ parentRState;

  //the next node on the path is the head of the subpath of which 
  //the sibling is the root node
  if (lChildRState) 
    nextLeaf = static_cast<DynLeaf*>(pnSib->bTail);
  else
    nextLeaf = static_cast<DynLeaf*>(pnSib->bHead);

  if (!nextLeaf)
    return static_cast<DynLeaf*>(pnSib); 

  return nextLeaf;

}



inline CapType DynLeaf::getEdgeCost() {

  DynNode *pn = this, *ch, *lChild = 0;
  bool parentRState = false;
  bool found = false;

  //prepare path to the root node
  CapType grossMin = prepareRootPath();

  //on the path to the root, search for the first node being the left child of its parent
  while (!found && pn->bParent) {
    parentRState = pn->bParent->getTemp();
    if (parentRState) //reversed state == true?
      lChild = pn->bParent->bRight;
    else
      lChild = pn->bParent->bLeft;
    //is pn left child of its parent?
    if (lChild == pn)
      found = true;
    ch = pn;
    pn = pn->bParent;

    //compute grossMin for current node
    if (!pn->bParent) //root node
      grossMin = pn->getNetMin(pn->getTemp()); 
    else if (!ch->isLeaf()) //inner node
      grossMin = grossMin - ch->getNetMin(ch->getTemp()); 
    //NOTE: grossmin(bparent) = grossmin(child) - netmin(child);
  }
  if (!found) 
    return 0; //"this" is the last node of the path
  CapType cost = pn->getNetCost(pn->getTemp()) + grossMin;
  return cost;
}


inline bool DynLeaf::getEdgeCostDbl(CapType &cost, CapType &costR) {

  //prepare path to the root node
  CapType grossMin, grossMinR;
  prepareRootPathDbl(grossMin, grossMinR);

  DynNode *pn = this, *ch, *lChild = 0;
  bool parentRState = false;
  bool found = false;

  //on the path to the root, search for the first node being the left child of its parent
  while (!found && pn->bParent) {

    parentRState = pn->bParent->getTemp();

    if (parentRState) //reversed-state == true?
      lChild = pn->bParent->bRight;
    else
      lChild = pn->bParent->bLeft;

    if (lChild == pn)
      found = true;

    ch = pn;
    pn = pn->bParent;

    //compute grossMin for current node
    if (!pn->bParent) { //root node
      grossMin  = pn->getNetMin(pn->getTemp());
      grossMinR = pn->getNetMin(!pn->getTemp());
    }
    else if (!ch->isLeaf()) { //inner node
      grossMin  = grossMin  - ch->getNetMin(ch->getTemp());
      grossMinR = grossMinR - ch->getNetMin(!ch->getTemp());
      //NOTE: grossmin(bparent) = grossmin(child) - netmin(child);
    }

  }

  if (!found) 
    return 0; //this ist der letzte Knoten im Pfad

  cost  = pn->getNetCost(pn->getTemp()) + grossMin;
  costR = pn->getNetCost(!pn->getTemp()) + grossMinR;

  return (pn->getMapping() ^ pn->getTemp());

}

#endif
