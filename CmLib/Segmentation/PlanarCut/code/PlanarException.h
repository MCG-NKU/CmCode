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

#ifndef __PLANAREXCEPTION_H__
#define __PLANAREXCEPTION_H__

#include <exception>

class ExceptionCheckConnectivity : std::exception 
{
  virtual const char* what() const throw();
};

class ExceptionCheckNonNegativeCost : std::exception 
{
  virtual const char* what() const throw();
};

class ExceptionCheckPlanarity : std::exception 
{
  virtual const char* what() const throw();
};

class ExceptionSourceSinkIdentical : std::exception 
{
  virtual const char* what() const throw();
};

class ExceptionSourceNotDefined : std::exception 
{
  virtual const char* what() const throw();
};

class ExceptionSinkNotDefined : std::exception 
{
  virtual const char* what() const throw();
};

class ExceptionUnexpectedError : std::exception 
{
  virtual const char* what() const throw();
};

#endif
