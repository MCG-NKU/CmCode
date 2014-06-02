#pragma once
#pragma warning(disable:4996)
#include <stdio.h>
#include <windows.h>
#include <string>
#include <time.h>
#include <assert.h>

#include <CmCode/CmDx/CmDx.h>


#include <opencv2/opencv.hpp> 
#define CV_VERSION_ID CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#define cvLIB(name) lnkLIB("opencv_" name CV_VERSION_ID)

#pragma comment( lib, cvLIB("core"))
#pragma comment( lib, cvLIB("imgproc"))
#pragma comment( lib, cvLIB("highgui"))

#pragma comment(lib, "cudart_static")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "winmm")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "usp10")

#include <cuda_runtime_api.h>
#include <cuda_d3d11_interop.h>

// includes, project
//#include <rendercheck_d3d11.h>
#include <helper_cuda.h>
#include <helper_functions.h>    // includes cuda.h and cuda_runtime_api.h