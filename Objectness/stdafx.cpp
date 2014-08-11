// stdafx.cpp : source file that includes just the standard includes
// Objectness.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file



// BitCount64 returns the number of 1s in a uint64.
inline int BitCount64(UINT64 w){
	const UINT64 ff = 0xffffffffffffffffLL;
	const UINT64 mask1(ff/3), mask3(ff/5), maskf(ff/17);
	const UINT64 maskp = (maskf>>3) & maskf;
	w -= (w>> 1) & mask1;
	w = (w&mask3) + ((w>>2)&mask3);
	return int(((w+(w>>4))&maskf) * maskp >> 56);
}


// BitCount32 returns the number of 1s in a uint32.
inline int BitCount32(UINT w){
	// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	const UINT ff = 0xffffffff; 
	const UINT mask1(ff/3), mask3(ff/5), maskf(ff/17), maskp(ff/255);
	w -= (w >>1) & mask1;
	w = (w&mask3) + ((w>>2)&mask3);
	return ((w+(w>>4))&maskf) * maskp >> 24;
}

