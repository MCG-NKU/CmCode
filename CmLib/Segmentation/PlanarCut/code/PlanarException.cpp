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

#include "PlanarException.h"

const char* ExceptionCheckConnectivity::what() const throw() {
  return "The provided graph is not connected.";
}

const char* ExceptionCheckNonNegativeCost::what() const throw() {
  return "The provided graph possesses edges of negative capacity.";
}

const char* ExceptionCheckPlanarity::what() const throw() {
  return "The provided graph is not planar.";
}

const char* ExceptionSourceSinkIdentical::what() const throw() {
  return "A cut between source and sink cannot be computed if they are identical.";
}

const char* ExceptionSourceNotDefined::what() const throw() {
  return "A cut between source and sink cannot be computed if the source is not defined.";
}

const char* ExceptionSinkNotDefined::what() const throw() {
  return "A cut between source and sink cannot be computed if the sink is not defined.";
}

const char* ExceptionUnexpectedError::what() const throw() {
  return "A bug is detected. Please contact the authors.";
}

