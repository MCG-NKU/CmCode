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

#include "CGraph.h"

using namespace std;



/********************************************************************
     CGraph
********************************************************************/

CGraph::CGraph(uint numMaxNodes)
{
  nodeBlock = new BlockAllocatorStatic<CGNode, NODE_BLOCK_SIZE>;
  edgeBlock = new BlockAllocatorStatic<CGEdge, NODE_BLOCK_SIZE>;

  this->numMaxNodes = numMaxNodes;
}

CGraph::~CGraph()
{
  delete nodeBlock;
  delete edgeBlock;
}

void CGraph::clear() {
  delete nodeBlock;
  delete edgeBlock;
  nodeBlock = new BlockAllocatorStatic<CGNode, NODE_BLOCK_SIZE>;
  edgeBlock = new BlockAllocatorStatic<CGEdge, NODE_BLOCK_SIZE>;
}

CGNode * CGraph::addNode()
{
  CGNode * n = nodeBlock->alloc();

  n->heapId = NULL;
  n->dijkWeight= 0;
  n->dijkPrev = NULL;
  n->type = 0;

  n->first = NULL;

  return n;
}

CGNode * CGraph::addNode(int tag)
{
  CGNode * n = nodeBlock->alloc();

  n->tag = tag;
  n->heapId = NULL;
  n->dijkWeight= 0;
  n->dijkPrev = NULL;
  n->type = 0;

  n->first = NULL;

  return n;
}

void CGraph::addEdge(CGNode * from, CGNode * to, CapType weight) {
  CGEdge *e;

  e = edgeBlock->alloc();

  e->sister = NULL;
  e->next = ((CGNode *)from)->first;
  ((CGNode *)from)->first = e;
  e->head = (CGNode *)to;
  e->weight = weight;
}


CGNode **CGraph::getShortestPath(CGNode *dest, int *length) {

  CGNode *n = dest;
  int num = 0;

  do {
    n = n->dijkPrev;
    num++;
  } while(n);

  CGNode * *path = (CGNode **)new CGNode *[num+1];
  n = dest;
  int i = num-1;

  do {
    path[i] = n;
    i--;
    n = n->dijkPrev;
  } while(n);

  path[num] = NULL; //null terminated list

  if (length)
    (*length) = num;

  return path;
}

//outputs the computed shortest path from the start node to node "dest"
void CGraph::printShortestPath(CGNode *dest) {
  CGNode *n = dest;

  do {
    cout << n->tag << " ";
    n = n->dijkPrev;
  } while(n);

  cout << endl;
}

void CGraph::runDijkstra(CGNode * start) {

  CGNode *n = nodeBlock->getFirst();

  DijkHeap dh(numMaxNodes);

  if (!n)
    return;

  do {
    
    n->dijkWeight = CAP_INF;

  } while ((n = nodeBlock->getNext()));

  start->heapId = NULL;
  start->dijkPrev = NULL;
  start->dijkWeight = 0;
  
  CDijkNode dijk;
  dijk.dijkWeight = start->dijkWeight;
  dijk.node = start;
  start->heapId = dh.insert(dijk);

  //compute shortest path
  CDijkNode min;

  while(dh.deleteMin(min)) {

    CGNode *n = min.node, *d;
    CGEdge *e = n->first;

    n->heapId = 0;	//n has already been removed from the heap
			 
    //update all neighboring nodes
    while (e) {
      d = e->head;
      
      if (d->heapId) {	//does the target node already know the shortest path
	//no:
	
	//can we improve on the shortest path to the target node?
	if (d->dijkWeight > (n->dijkWeight + e->weight)) {
	  d->dijkWeight = n->dijkWeight + e->weight;
	  d->dijkPrev = n;
	  
	  /*	  CDijkNode dijk;
	  dijk.dijkWeight = d->dijkWeight;
	  dijk.node = d;*/
	  //	  fh.decreaseKey(d->fid, dijk);

	  dh.decrease(d->heapId, d->dijkWeight);
	}

      } else if (d->dijkWeight == CAP_INF) {

	d->dijkWeight = n->dijkWeight + e->weight;
	d->dijkPrev = n;
	  
	CDijkNode dijk;
	dijk.dijkWeight = d->dijkWeight;
	dijk.node = d;
	//	fh.insert(dijk, d->fid);
	d->heapId = dh.insert(dijk);
	
      }

      e = e->next;
    } //while(e)
  }

}



/********************************************************************
     DijkHeap 
********************************************************************/


DijkHeap::DijkHeap(uint maxHeapSize) : maxIdx(0), maxHeapSize(maxHeapSize) {

  //  this->maxHeapSize = maxHeapSize;

  heap = new CDijkNode*[maxHeapSize];

}



DijkHeap::~DijkHeap() {

  if (heap)
    delete [] heap;

}



void DijkHeap::ascend(int heapId) {

  int cIdx = heapId;
  int pIdx = (heapId-1)/2;

  CDijkNode *d;

  //let the element bubble up as far as possible
  while (heap[pIdx]->dijkWeight > heap[cIdx]->dijkWeight) {

    d = heap[pIdx];
    heap[pIdx] = heap[cIdx];
    heap[cIdx] = d;

    heap[pIdx]->heapId = pIdx;
    heap[cIdx]->heapId = cIdx;

    cIdx = pIdx;
    pIdx = (cIdx-1)/2;

  }

}



void DijkHeap::descend(int heapId) {

  int cIdx = heapId;
  //  int lIdx = 2*heapId + 1;
  //  int rIdx = 2*heapId + 2;
  int lIdx = (heapId << 1) | 1;
  int rIdx = lIdx + 1;

  CapType cW = heap[heapId]->dijkWeight;
  CapType lW = (lIdx >= maxIdx) ? CAP_INF : heap[lIdx]->dijkWeight;
  CapType rW = (rIdx >= maxIdx) ? CAP_INF : heap[rIdx]->dijkWeight;

  CapType pW;
  int pIdx;

  if (lW < rW) {
    pW = lW;
    pIdx = lIdx;
  } else {
    pW = rW;
    pIdx = rIdx;
  }
  
  CDijkNode *d;

  while (cW > pW) {

    d = heap[pIdx];
    heap[pIdx] = heap[cIdx];
    heap[cIdx] = d;
    
    heap[pIdx]->heapId = pIdx;
    heap[cIdx]->heapId = cIdx;
    
    cIdx = pIdx;

    //  lIdx = 2*cIdx + 1;
    //  rIdx = 2*cIdx + 2;
    int lIdx = (cIdx << 1) | 1;
    int rIdx = lIdx + 1;
    
    cW = heap[cIdx]->dijkWeight;
    lW = (lIdx >= maxIdx) ? CAP_INF : heap[lIdx]->dijkWeight;
    rW = (rIdx >= maxIdx) ? CAP_INF : heap[rIdx]->dijkWeight;

    if (lW < rW) {
      pW = lW;
      pIdx = lIdx;
    } else {
      pW = rW;
      pIdx = rIdx;
    }
    
  }

}



HeapId DijkHeap::insert(CDijkNode &node) {

  /*  if (maxIdx >= MAXHEAPSIZE-1)
      return 0;*/

  CDijkNode *d = dijkNodes.alloc();

  node.heapId = -1;
  node.heapNr = -1;
  *d = node;
  
  heap[maxIdx] = d;
  d->heapId = maxIdx;
  maxIdx++;
  ascend(maxIdx-1);

  return d;

}


void DijkHeap::decrease(HeapId node, CapType neww) {

  CDijkNode *d = node;
  d->dijkWeight = neww;

  ascend(d->heapId);

}



bool DijkHeap::deleteMin(CDijkNode &node) {

  if (maxIdx == 0) //heap empty?
    return false;

  CDijkNode *d = heap[0];
  node = *d;
  dijkNodes.dealloc(d);
  
  d = heap[--maxIdx];

  if (maxIdx == 0) //heap empty now?
    return true;

  heap[0] = d;
  d->heapId = 0;

  descend(0);

  return true;

}


bool DijkHeap::getMin(CDijkNode &node) {

  if (maxIdx == 0)
    return false;

  node = *heap[0];

  return true;

}



double DijkHeap::getMin() {
  /*
  if (maxIdx == 0)
    return -1.0;
  */
  return heap[0]->dijkWeight;

}



bool DijkHeap::isempty() {

  return (maxIdx == 0);

}



HeapId DijkHeap::deleteLast() {

  if (maxIdx == 0)
    return 0;

  return heap[--maxIdx];

}



void DijkHeap::insert(HeapId node) {

  if (maxIdx >= maxHeapSize-1)
    return;

  CDijkNode *d = node;
  
  heap[maxIdx] = d;
  d->heapId = maxIdx;
  maxIdx++;
  ascend(maxIdx-1);

  return;

}
