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


#include "Planar.h"

/***************************************************
 *** PlanarFace ************************************
 ***************************************************/

// empty class


/***************************************************
 *** PlanarEdge ************************************
 ***************************************************/

PlanarEdge::PlanarEdge() : cap(0.0), rcap(0.0),
			   tail(0), head(0), 
			   tailDual(0), headDual(0),
			   tailEdgeID(-1), headEdgeID(-1),
			   flags(0)
{
}


void PlanarEdge::setEdge(PlanarVertex *tail,     PlanarVertex *head,     \
			 PlanarFace   *tailDual, PlanarFace   *headDual, \
			 CapType cap, CapType rcap) {

    this->tail = tail;
    this->head = head;
  
    this->tailDual = tailDual;
    this->headDual = headDual;

    this->cap  = cap;
    this->rcap = rcap;

    // Depending on the order of the calls setEdgesCCW() and setEdge(), 
    // the respective Edge Ids of tail and head still have to be set
    if (tailEdgeID < 0 && tail)
      for (int i=0; i<tail->nEdges; i++)
	if (tail->edgesCCW[i] == this) { 
	  tailEdgeID = i;
	  break;
	}
	  
    if (headEdgeID < 0 && head)
      for (int i=0; i<head->nEdges; i++)
	if (head->edgesCCW[i] == this) { 
	  headEdgeID = i;
	  break;
	}

}




/***************************************************
 *** PlanarVertex **********************************
 ***************************************************/

PlanarVertex::PlanarVertex() : nEdges(0), edgesCCW(0) {
}


PlanarVertex::~PlanarVertex() {

    if (nEdges)
	delete [] edgesCCW;

}

