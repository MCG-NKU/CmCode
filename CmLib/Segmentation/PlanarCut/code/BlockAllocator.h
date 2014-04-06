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

#ifndef __BLOCKALLOCATOR_H__
#define __BLOCKALLOCATOR_H__

#include <iostream>

//NOTE: size of class C must be bigger than the size of a pointer
template<class C, unsigned int BLOCKSIZE=1024>
class BlockAllocator 
{  
  struct AllocBlock {
    C *data;
    AllocBlock *nextBlock;
  };

  AllocBlock *firstBlock;
  AllocBlock *currentBlock; //for fast access

  C *firstFree; //points to the first free slot

  C** castCtoCPtr(C *c) { return reinterpret_cast<C**>(c); }; //inline cast

 public:
   BlockAllocator() : firstBlock(0), currentBlock(0), firstFree(0) {};

  ~BlockAllocator();

   inline C *alloc();         //returns a pointer to a free slot
   inline void dealloc(C *);  //frees a slot
   void reset(); //resets the BlockAllocator effectively freeing all elements
};






//NOTE: size of class C must be bigger than the size of a pointer
template<class C, unsigned int BLOCKSIZE=1024>
class BlockAllocatorStatic
{  
  struct AllocBlock {
    C *data;
    AllocBlock *nextBlock;
  };
  
  AllocBlock *firstBlock;
  AllocBlock *currentBlock; //for fast access
    
  C *firstFree; //points to the first free slot

  //auxiliary variables for blockwise enumeration
  AllocBlock *enumBlock;
  C *enumItem;
  
public:
   BlockAllocatorStatic() : firstBlock(0), currentBlock(0), firstFree(0) {};
  ~BlockAllocatorStatic();
  
  inline C *alloc();         //returns a pointer to a free slot
  
  //methods for blockwise entity enumeration 
  inline C *getFirst();
  inline C *getNext();
};












//********************************************************************
//       Implementation
//********************************************************************


//********************************************************************
//       BlockAllocator
//********************************************************************

//(for a template class, the implementation has to appear inside the header file)

template<class C, unsigned int BLOCKSIZE>
BlockAllocator<C,BLOCKSIZE>::~BlockAllocator() {
  reset();
}


template<class C,unsigned int BLOCKSIZE>
C *BlockAllocator<C,BLOCKSIZE>::alloc() {
  C *c;

  if (!firstFree) {
    AllocBlock *block = new AllocBlock();
    block->nextBlock  = block;

    if (!currentBlock) //have any blocks been allocated so far?
      firstBlock = currentBlock = block;

    block->data = new C[BLOCKSIZE];
          
    currentBlock->nextBlock = block;
    currentBlock = block;

    //each slot in the new block points to the next one
    for (unsigned int i=0; i<BLOCKSIZE-1; i++) 
      *castCtoCPtr(block->data + i) = block->data + i + 1;

    //except the last one
    *castCtoCPtr(block->data + BLOCKSIZE - 1) = 0;

    firstFree = block->data;
  } 

  c         = firstFree;
  firstFree = *castCtoCPtr(firstFree);

  return c;
}


template<class C, unsigned int BLOCKSIZE>
void BlockAllocator<C,BLOCKSIZE>::dealloc(C *c) {
  if (c) {
    *castCtoCPtr(c) = firstFree;
    firstFree = c;
  }
}


template<class C, unsigned int BLOCKSIZE>
  void BlockAllocator<C,BLOCKSIZE>::reset() {
  AllocBlock *block = firstBlock, *prevBlock;
  //free all the allocated blocks
  while (block) {
    delete [] block->data;
    prevBlock = block;
    if (block->nextBlock == block) 
      block = 0; //last block in the list
    else
      block = block->nextBlock;
    delete prevBlock;
  }
  firstBlock = 0;
  currentBlock = 0;
  firstFree = 0;
}



//********************************************************************
//       BlockAllocatorStatic
//********************************************************************

template<class C, unsigned int BLOCKSIZE>
BlockAllocatorStatic<C,BLOCKSIZE>::~BlockAllocatorStatic() {
  AllocBlock *block = firstBlock, *prevBlock;

  //free all the allocated blocks
  while (block) {
    delete [] block->data;
    prevBlock = block;
    if (block->nextBlock == block) 
      block = 0; //last block in the list
    else
      block = block->nextBlock;
    delete prevBlock;
  }
}


template<class C,unsigned int BLOCKSIZE>
C *BlockAllocatorStatic<C,BLOCKSIZE>::alloc() {
  C *c;

  if (!firstFree) {
    AllocBlock *block = new AllocBlock();
    block->nextBlock  = block;
    if (!currentBlock) //have any blocks been allocated so far?
      firstBlock = currentBlock = block;
    block->data = new C[BLOCKSIZE];
    currentBlock->nextBlock = block;
    currentBlock = block;
    firstFree = block->data;
  } 
  c         = firstFree;
  firstFree = firstFree + 1;
  if (!((unsigned int)(firstFree - currentBlock->data) < BLOCKSIZE))
    firstFree = 0;

  return c;
}


template<class C,unsigned int BLOCKSIZE>
inline C *BlockAllocatorStatic<C, BLOCKSIZE>::getFirst() {
  if (!firstBlock)
    return 0;
  enumBlock = firstBlock;
  enumItem = enumBlock->data;
  return enumItem;
}


template<class C,unsigned int BLOCKSIZE>
inline C *BlockAllocatorStatic<C, BLOCKSIZE>::getNext() {
  C *nextEnumItem = enumItem + 1;
  if (!((unsigned int)(nextEnumItem - enumBlock->data) < BLOCKSIZE)) {
    if (enumBlock->nextBlock == enumBlock)
      return 0;
    enumBlock = enumBlock->nextBlock;
    nextEnumItem  = enumBlock->data;
  } else if (nextEnumItem == firstFree) {
    return 0;
  }
  return enumItem = nextEnumItem;
};


#endif
