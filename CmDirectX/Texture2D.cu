/*
 * Copyright 1993-2013 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.1415926536f

texture<float, 2, cudaReadModeElementType> texRef;
/*
 * Paint a 2D texture with a moving red/green hatch pattern on a
 * strobing blue background.  Note that this kernel reads to and
 * writes from the texture, hence why this texture was not mapped
 * as WriteDiscard.
 */
__global__ void cuda_kernel_texture_2d(unsigned char *surface, int width, int height, size_t pitch, float t)
{
    int x = blockIdx.x*blockDim.x + threadIdx.x;
    int y = blockIdx.y*blockDim.y + threadIdx.y;
    float *pixel;

    // in the case where, due to quantization into grids, we have
    // more threads than pixels, skip the threads which don't
    // correspond to valid pixels
    if (x >= width || y >= height) return;

    // get a pointer to the pixel at (x,y)
    pixel = (float *)(surface + y*pitch) + 4*x;

    // populate it
    float value_x = 0.5f + 0.5f*cos(t + 10.0f*((2.0f*x)/width  - 1.0f));
    float value_y = 0.5f + 0.5f*cos(t + 10.0f*((2.0f*y)/height - 1.0f));
    pixel[0] = 0.5*pixel[0] + 0.5*pow(value_x, 3.0f); // red
    pixel[1] = 0.5*pixel[1] + 0.5*pow(value_y, 3.0f); // green
    pixel[2] = 0.5f + 0.5f*cos(t); // blue
    pixel[3] = 1; // alpha
}

extern "C"
void cuda_texture_2d(void *surface, int width, int height, size_t pitch, float t)
{
    cudaError_t error = cudaSuccess;

    dim3 Db = dim3(16, 16);   // block dimensions are fixed to be 256 threads
    dim3 Dg = dim3((width+Db.x-1)/Db.x, (height+Db.y-1)/Db.y);

    cuda_kernel_texture_2d<<<Dg,Db>>>((unsigned char *)surface, width, height, pitch, t);

    error = cudaGetLastError();

    if (error != cudaSuccess)
    {
        printf("cuda_kernel_texture_2d() failed to launch error = %d\n", error);
    }
}

