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

#ifndef __CGRAPH_H__
#define __CGRAPH_H__

#include "CutPlanarDefs.h"
#include "BlockAllocator.h"
#include <iostream>

#define NODE_BLOCK_SIZE 512
#define ARC_BLOCK_SIZE 512

//values for CGNode::type
#define TYPE_STARTNODE 1
#define TYPE_ENDNODE 2


struct CGNode;

class CDijkNode {

 public:
  CapType dijkWeight;
  CGNode *node;           
  int heapId;           //index into the heap array
  int heapNr;           //number of heap

};

typedef CDijkNode* HeapId;

struct CGEdge; //forward declaration

/* node structure */
struct CGNode
{

  CGEdge    *first;     //first outcoming edge
  int	   tag;	      //just a tag
  uchar    type;	      //type of node (start- or end-node)
  HeapId   heapId;     //corresponding entity in heap
  CapType  dijkWeight; //used by runDijkstra() to compute shortest path
  CGNode    *dijkPrev;  //points to previous node on the shortest path

};



/* arc structure */
struct CGEdge
{

  CGNode    *head;   //node the arc points to 
  CGEdge    *next;   //next arc with the same originating node
  CGEdge    *sister; //reverse arc
  CapType  weight; //capacity

};



/********************************************************************
     DijkHeap 
********************************************************************/

class DijkHeap {

  CDijkNode **heap;
  BlockAllocator<CDijkNode> dijkNodes;

  int maxIdx;
  int maxHeapSize;

  void ascend(int heapId);
  void descend(int heapId);

 public:

  DijkHeap(uint maxHeapSize);
  ~DijkHeap();

  HeapId insert(CDijkNode &node);
  void   decrease(HeapId node, CapType amount);
  bool   deleteMin(CDijkNode &node);
  bool   getMin(CDijkNode &node);
  double getMin();
  bool   isempty();

  //Methoden zum Einfuegen und Loeschen bereits allozierter Elemente
  HeapId deleteLast();              //Loescht das letzte Element im Heap ohne es zu zerstoeren
  void insert(HeapId node);         //fuegt ein bereits alloziertes Element ein
};







/********************************************************************
	CGraph
********************************************************************/
class CGraph {

  BlockAllocatorStatic<CGNode, NODE_BLOCK_SIZE> *nodeBlock;
  BlockAllocatorStatic<CGEdge, NODE_BLOCK_SIZE> *edgeBlock;

  uint numMaxNodes;

 public:
  CGraph(uint numMaxNodes);
  ~CGraph();

  //remove all nodes and edges
  void clear();

  //adds a node to the graph
  CGNode *addNode();
  CGNode *addNode(int tag); //like addNode() but sets the tag flag

  void addEdge(CGNode *from, CGNode *to, CapType weight);
  /* void addbiEdge(CGNode *from, CGNode *to, CapType weight); */

  // returns whether the edge was found
  bool setEdgeWeight(CGNode *from, CGNode *to, CapType cap);

  void runDijkstra(CGNode *start);
  CGNode *runDijkstraMulti(CGNode **start, int numStart, CGNode **end, int numEnd);
  //returns null-terminated list of shortest path nodes
  CGNode **getShortestPath(CGNode *dest, int *length = NULL); 
  void printShortestPath(CGNode *dest);

};







#endif //#ifndef __CGRAPH_H__

