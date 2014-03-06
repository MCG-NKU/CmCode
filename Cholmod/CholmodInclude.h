#pragma once

#include "./CHOLMOD/Include/cholmod.h"
#pragma warning(default:4244)


#if _WIN64 
#pragma comment(lib,"mkl_core.lib")		
#pragma comment(lib,"mkl_solver_lp64_sequential.lib")  // Change to ilp64 for sizeof(int)=64
#pragma comment(lib,"mkl_intel_lp64.lib")  // Change to ilp64 for sizeof(int)=64
#pragma comment(lib,"mkl_sequential.lib ")
#else
#pragma comment(lib,"mkl_core.lib")		
#pragma comment(lib,"mkl_solver_sequential.lib")
#pragma comment(lib,"mkl_intel_c.lib")
#pragma comment(lib,"mkl_sequential.lib ")
#endif