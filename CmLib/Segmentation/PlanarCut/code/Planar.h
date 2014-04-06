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

#ifndef __PLANAR_H
#define __PLANAR_H

#include "CutPlanarDefs.h"


class PlanarVertex; //forward declaration
class PlanarEdge; //forward declaration
class PlanarFace 
{};

class PlanarEdge 
{
    friend class PlanarVertex; //for efficiency vertex and edge are closely connected

    CapType cap;  //forward edge capacity 
    CapType rcap; //backward edge capacity

    PlanarVertex *tail;     //an edge points from the tail vertex
    PlanarVertex *head;     //to the head vertex.

    PlanarFace   *tailDual; //the face left of the edge wrt. the pointing direction
    PlanarFace   *headDual; //the face right of the edge wrt. the pointing direction

    //these are for fast access to edge IDs
    int tailEdgeID; //the ID of this edge with respect to the tail vertex
    int headEdgeID; //the ID of this edge with respect to the head vertex

    uchar  flags; //a maximum of 8 flags that can be used freely

    
public:

    PlanarEdge();
    CapType getCapacity() { return cap; }
    CapType getRevCapacity() { return rcap; }

    void   setCapacity(CapType cap) { this->cap = cap; }
    void   setRevCapacity(CapType rcap) { this->rcap = rcap; }

    void   setEdge(PlanarVertex *tail,     PlanarVertex *head,     
		   PlanarFace   *tailDual, PlanarFace   *headDual, 
		   CapType cap=1.0, CapType rcap=0.0);

    void  setFlags(uchar value) { flags = value; } 
    uchar getFlags() { return flags; }
  
    PlanarVertex *getHead() { return head; }
    PlanarVertex *getTail() { return tail; }
    PlanarFace   *getHeadDual() { return headDual; }
    PlanarFace   *getTailDual() { return tailDual; }
  
};

class PlanarVertex 
{
  friend class PlanarEdge; //for efficiency vertex and edge are closely connected

  int nEdges;             //number of adjacent edges
  PlanarEdge **edgesCCW;  //ccw list of adjacent edges

 public:
  PlanarVertex();
  ~PlanarVertex();
  
  int         getNumEdges() { return nEdges; };
  PlanarEdge *getEdge(int id) { id=id%nEdges; return (id<0)?edgesCCW[id+nEdges]:edgesCCW[id]; };
  inline void setEdgesCCW(PlanarEdge **ccw, int nEdges);
  inline int  getEdgeID(PlanarEdge *e);
};


/***************************************************
 *** PlanarVertex INLINE ***************************
 ***************************************************/
void PlanarVertex::setEdgesCCW(PlanarEdge **ccw, int nEdges) {

  if (nEdges != this->nEdges) {
    if (this->nEdges)
      delete [] edgesCCW;
    if (nEdges)
      edgesCCW = new PlanarEdge*[nEdges];
    this->nEdges = nEdges;
  }
  for (int i=0; i<nEdges; i++) {
    edgesCCW[i] = ccw[i];
    if (edgesCCW[i]->getTail() == this)
      edgesCCW[i]->tailEdgeID = i;
    else if (edgesCCW[i]->getHead() == this)
      edgesCCW[i]->headEdgeID = i;
  }

}


int PlanarVertex::getEdgeID(PlanarEdge *e) {
  //for efficiency edge IDs are stored in the edges themselves
  if (e->getTail() == this)
    return e->tailEdgeID;
  else if (e->getHead() == this)
    return e->headEdgeID;
  else
    return -1;
}

#endif
