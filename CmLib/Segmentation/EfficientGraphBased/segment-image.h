/*
Modified by Ming-Ming on Aug. 15th, 2010.

Copyright (C) 2006 Pedro Felzenszwalb

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#ifndef SEGMENT_IMAGE
#define SEGMENT_IMAGE


/*
* Segment an image
*
* Returns a color image representing the segmentation.
* 
* Input:
*	im: image to segment.
*	sigma: to smooth the image.
*	c: constant for threshold function.
*	min_size: minimum component size (enforced by post-processing stage).
*	nu_ccs: number of connected components in the segmentation.
* Output:
*	colors: colors assigned to each components
*	pImgInd: index of each components
*/

//"Default: k = 500, sigma = 1.0, min_size = 1000\n") or k = 200, sigma = 0.5, min_size = 50
int SegmentImage(CMat &_src3f, Mat &pImgInd, double sigma = 0.5, double k = 200, int min_size = 50);


void SegmentImageDemo(CStr& inImgW, CStr& outDir, double sigma, double k, int min_size);

#endif
