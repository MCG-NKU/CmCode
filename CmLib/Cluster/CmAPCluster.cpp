#include "StdAfx.h"
#include "CmAPCluster.h"

CmAPCluster::CmAPCluster(void) : dlh(NULL)
{
	if (!(dlh=LoadLibrary("apclusterwin.dll"))) // Download the corresponding dll from (Notice: only 32bit dll is public available at the moment): http://www.psi.toronto.edu/index.php?q=affinity+propagation
		printf("LoadLibrary() failed: %d. %s:%d\n", GetLastError(), __FILE__, __LINE__); 

	apoptions.cbSize = sizeof(APOPTIONS);
	apoptions.lambda = 0.9;
	apoptions.minimum_iterations = 1;
	apoptions.converge_iterations = 200;
	apoptions.maximum_iterations = 2000;
	apoptions.nonoise = 0;
	apoptions.progress = NULL; //callback; 
	apoptions.progressf = NULL;

	apFun = (apcluster32)GetProcAddress(dlh, "apcluster32");
	kcFun = (kcenters32)GetProcAddress(dlh, "kcenters32");
	if (kcFun == NULL || apFun == NULL)
		printf("GetProcAddress() failed: %d\n", GetLastError());
}

CmAPCluster::~CmAPCluster(void)
{
	FreeLibrary(dlh);
}

int CmAPCluster::callback(double *a, double *r, int N, int *idx, int I, double netsim, double dpsim, double expref, int iter)
{
	static double netsimOld = 0;
	if (netsimOld == netsim)
		printf(".");
	else
		printf(" %g ", netsim), netsimOld = netsim;

	return(0); /* 0=continue apcluster */
}

int CmAPCluster::ReMapIdx(vecI &mapIdx)
{
	int N = (int)mapIdx.size(), newCount = 0;
	map<int, int> idxCount, oldNewIdx;
	vecI newIdx(N);
	for (int i = 0; i < N; i++){
		if (idxCount.find(mapIdx[i]) == idxCount.end())
			oldNewIdx[mapIdx[i]] = newCount++, idxCount[mapIdx[i]]++;
		mapIdx[i] = oldNewIdx[mapIdx[i]];
	}
	return (int)idxCount.size();
}
